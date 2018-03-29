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


import logging
import base64
import re
import json

from channels import Group, Channel

from .models import WebHookAuth, Sensor, Board

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

NOTIFICATION_SENDERS = {
    '/26242/0/1': Channel('gis.locate').send
}


def handle_data(webhook_auth, data):
    """
    Handle the data from webhook/poll
    """
    logger.debug("data: %r", data)
    for key, value in data.items():
        if key == 'notifications':
            handle_notifications(webhook_auth, value)
        elif key == 'registrations':
            pass
        elif key == 'registrations-expired':
            handle_de_registration(value)
        elif key == 'de-registrations':
            handle_de_registration(value)
        elif key == 'reg-updates':
            pass
        elif key == 'async-responses':
            pass
        else:
            raise ValueError("unhandled cloud data type: %s" % key)


def handle_notifications(webhook_auth, notifications):
    """
    Handle the 'notifications' section of the webhook/polled data.

    Args:
        notifications: content under 'notifications', a list of sensor
        value updates
    """
    for notification in notifications:
        path = notification['path']
        value = decode_payload(path, notification['payload'])
        message = {
            'type': 'update',
            'update': {
                'board': notification['ep'],
                'sensor': path,
                'value': value}}
        send = NOTIFICATION_SENDERS.get(path, send_group)
        send({'webhook_auth_id': webhook_auth.id, 'message': message})


def handle_de_registration(de_registrations):
    """
    Handle 'de-registrations' in the webhook/polled data.

    Args:
        de_registrations: content under 'de-registrations', a list of
        de-registered endpoints
    """
    for endpoint in de_registrations:
        try:
            Board.objects.get(device_id=endpoint).delete()
        except Board.DoesNotExist:
            pass
        message = {'type': 'remove-board', 'board': endpoint}
        Group("livedevice").send({'text': json.dumps(message)})


def decode_string(value_txt):
    logger.debug("decoding string: %r", value_txt)
    return value_txt


def decode_float(value_txt):
    logger.debug("decoding float: %r", value_txt)
    return float(re.search(r"([+-]?\d+(?:\.\d+)?)", value_txt).group(1))


PAYLOAD_DECODERS = {
    '/26241/0/1': decode_string,
    '/26242/0/1': decode_string,
    '/3336/0/5750': decode_string
}


def decode_payload(path, payload):
    decoder = PAYLOAD_DECODERS.get(path, decode_float);
    value_txt = base64.b64decode(payload)
    try:
        value = decoder(value_txt)
    except AttributeError:
        logger.debug("WARN: failed to decode: path=%s, value=%r",
                     path, value_txt)
        value = decode_string(value_txt)
    return value


def send_group(data):
    webhook_auth_id = data['webhook_auth_id']
    message = data['message']
    Sensor.objects.set(WebHookAuth.objects.get(id=webhook_auth_id),
              message['update']['board'],
              message['update']['sensor'],
              message['update']['value'])
    Group("livedevice").send({'text': json.dumps(message)})
