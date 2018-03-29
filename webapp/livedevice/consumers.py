"""
mbed tools
Copyright (c) 2018 ARM Limited
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""


import json
import logging
import math
import random

import googlemaps
import requests
from channels import Group, Channel
from googlemaps.geolocation import geolocate
from django.conf import settings
from django.core.urlresolvers import reverse
from django.contrib.sites.models import Site

from .models import Sensor, WebHookAuth, MBEDCloudAccount, SiteScheme

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


def ws_connect(message):
    """Upon websocket connect add that channel to the live device group"""
    message.reply_channel.send({"accept": True})
    # send initial data
    for data in Sensor.objects.all_data():
        update_message = {"type": "update", "update": data}
        message.reply_channel.send({"text": json.dumps(update_message)})
    Group("livedevice").add(message.reply_channel)


def ws_disconnect(message):
    """Upon websocket disconnect remove that channel from the live device group"""
    Group("livedevice").discard(message.reply_channel)


def gis_locate(data):
    """
    Use data (value) in the message to perform geolocation.
    The expected format of the message is a string representation of a JSON
    array containing objects with at minimum a single key called 'macAddress'
    with a corresponding string value in 802.11 format 'xx:xx:xx:xx:xx:xx'
    e.g. message = "[{'macAddress':'AA:BB:CC:DD:EE:FF'}, {'macAddress': ... }]"

    optional keys for each object are 'signalStrength' and 'channel' with integer
    values
    e.g. {'macAddress': 'AA:BB:CC:DD:EE:FF', 'signalStrength': -92, 'channel': 11}

    The return from geolocate is a JSON object (not string)
    """
    webhook_auth = WebHookAuth.objects.get(id=data['webhook_auth_id'])
    account = webhook_auth.mbed_cloud_account
    if account is None:
        return
    session = account.get_session()
    message = data['message']
    client = googlemaps.Client(key=settings.GOOGLE_MAPS_API_KEY)

    # Pull out the content and message to send
    content = message
    update = content['update']
    value = update['value']
    board_id = update['board']

    # The value field contains the list of APs and any extra data to make
    # the geolocation request
    req = geolocate(client, wifi_access_points=json.loads(value))

    # NOTE: This should be all or nothing but there is possibly a problem here.
    # If the first resource update for latitude succeeds but longitude fails then
    # our position reported by the device will be wrong. A strategy for updating
    # all or nothing would fix this problem.
    session.set_endpoint_resource(
            endpoint_id=board_id,
            resource_path='/3336/1/5514',
            payload=req['location']['lat']
        )
    session.set_endpoint_resource(
            endpoint_id=board_id,
            resource_path='/3336/1/5515',
            payload=req['location']['lng']
        )
    session.set_endpoint_resource(
            endpoint_id=board_id,
            resource_path='/3336/1/5516',
            payload=req['accuracy']
        )

    # store our values
    Sensor.objects.set(webhook_auth, update['board'], '/3336/1/5514', req['location']['lat'])
    Sensor.objects.set(webhook_auth, update['board'], '/3336/1/5515', req['location']['lng'])
    Sensor.objects.set(webhook_auth, update['board'], '/3336/1/5516', req['accuracy'])


def mbedcloudaccount_connect(data):
    account = MBEDCloudAccount.objects.get(id=data['mbedcloudaccount_id'])
    webhook_api_key = account.webhookauth.api_key

    site = Site.objects.get_current()
    try:
        scheme = site.sitescheme.scheme
    except SiteScheme.DoesNotExist:
        scheme = SiteScheme._meta.get_field('scheme').get_default()
    webhook_path = reverse('mbed cloud webhook')
    webhook_url = "%s://%s%s" % (scheme, site.domain, webhook_path)
    logger.debug("webhook_url: %r", webhook_url)

    try:
        session = account.get_session()
        try:
            session.delete_callback()
        except requests.RequestException, e:
            if e.response.status_code == 404:
                logger.debug('No callback found when trying to delete')
            else:
                raise

        headers = {'Authorization': "Bearer %s" % webhook_api_key}
        session.set_callback(webhook_url, headers=headers)

        # Delete presubscriptions can only fail currently because the
        # endpoint or resource could not be found. If that's the case
        # we don't need to attempt to setup new subscriptions
        session.delete_presubscriptions()
        session.set_presubscriptions(settings.MBED_CLOUD_PRESUBSCRIPTIONS)
    except Exception as e:
        account.connected = False
        account.connected_details = unicode(e)
    else:
        account.connected = True
        account.connected_details = 'success'
    account.save()


def mbedcloudaccount_poll(data):
    from .cloud_data_handlers import handle_data
    try:
        account = MBEDCloudAccount.objects.get(id=data['mbedcloudaccount_id'])
    except MBEDCloudAccount.DoesNotExist:
        return
    logger.info("polling MBED cloud account %s", account)
    account.is_long_polling = True
    webhook_auth = account.webhookauth
    session = account.get_session()
    repoll_delay = 0
    try:
        data = session.pull_notifications()
    except ValueError:
        logger.debug("poll of %s timed out", account)
    except requests.HTTPError as e:
        logger.error("got long poll error: %s", e)
        # exponential backoff https://en.wikipedia.org/wiki/Exponential_backoff
        backoff_ceiling = 6
        account.long_poll_failures += 1
        n = math.pow(2, min([account.long_poll_failures, backoff_ceiling])) - 1
        k = random.random() * n
        repoll_delay = k * 1  # seconds
        logger.info("delaying %d seconds before repolling %s", repoll_delay, account)
    else:
        account.long_poll_failures = 0
        try:
            handle_data(webhook_auth, data)
        except ValueError:
            pass
    finally:
        if account.should_stop_polling:
            logger.debug("stopping MBED cloud account %s polling", account)
            account.is_long_polling = False
        else:
            account.is_long_polling = {'value': account.is_long_polling, 'timeout': 60 + repoll_delay}
            delayed_message = {
                'channel': 'mbedcloudaccount.poll',
                'content': {'mbedcloudaccount_id': account.id},
                'delay': repoll_delay * 1000,
            }
            Channel('asgi.delay').send(delayed_message, immediately=True)
