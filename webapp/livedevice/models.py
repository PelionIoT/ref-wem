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


from __future__ import unicode_literals
import logging
from uuid import uuid4

import requests
from django.db import models, transaction
from django.contrib.sites.models import Site
from django.core.urlresolvers import reverse
from django.conf import settings
from django.utils.functional import cached_property
from channels import Channel

from .mbedcloud import MBEDCloudSession
from .fields import CacheField


logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class BoardManager(models.Manager):

    def all_data(self):
        boards = self.all()
        data = {}
        for board in boards:
            data[board.device_id] = {}
            for sensor in board.sensor_set.all():
                data[board.device_id][sensor.path] = sensor.value
        return data


class Board(models.Model):
    device_id = models.CharField(max_length=32, unique=True)
    webhook_auth = models.ForeignKey("WebHookAuth")
    objects = BoardManager()

    def __unicode__(self):
        return self.device_id


class SensorManager(models.Manager):

    @transaction.atomic
    def set(self, webhook_auth, device_id, sensor_path, value):
        logger.debug("setting %s %s %s", device_id, sensor_path, value)
        try:
            sensor = self.get(board__device_id=device_id,
                                        path=sensor_path)
        except Sensor.DoesNotExist:
            board, created = Board.objects.get_or_create(
                    device_id=device_id,
                    defaults={'webhook_auth': webhook_auth}
            )
            if not created:
                # NOTE: Technically this is possible if the user deletes their
                # mBed cloud API key and regenerates a new one, then uploads
                # that to the wem. We log that change here and update
                # the auth corresponding to that board
                if board.webhook_auth != webhook_auth:
                    logger.debug("board %s has changed API keys!", board.device_id)
                    board.webhook_auth = webhook_auth
                    board.save()
            sensor = self.create(board=board, path=sensor_path)
        sensor.value = value

    @transaction.atomic
    def all_data(self):
        sensors = self.all().prefetch_related('board')
        logger.debug("sensors: %r", sensors)
        for sensor in sensors:
            logger.debug("sensor.value: %r", sensor.value)
            if sensor.value is None:
                sensor.delete()
                if not sensor.board.sensor_set.exists():
                    sensor.board.delete()
            else:
                yield {
                       'board': sensor.board.device_id,
                       'sensor': sensor.path,
                       'value': sensor.value,
                      }


class Sensor(models.Model):
    board = models.ForeignKey(Board)
    path = models.CharField(max_length=50)
    value = CacheField(timeout=60*60*24)
    objects = SensorManager()

    class Meta:
        unique_together = (('board', 'path'))

    def __unicode__(self):
        return u"%s %s" % (self.board, self.path)

    def delete(self, *args, **kwargs):
        # delete the cache value
        self.value = None
        super(Sensor, self).delete(*args, **kwargs)

    @property
    def cache_key(self):
        return ','.join((self.board.device_id, self.path))


class MBEDCloudAccount(models.Model):
    """This is what we need to talk to an MBED Cloud account

    The api_key is an MBED Cloud API Key, ex: ak_blahblahblah

    """
    url = models.URLField(default="https://api.us-east-1.mbedcloud.com", verbose_name="URL")
    api_key = models.CharField(max_length=150, unique=True, verbose_name="API key")
    display_name = models.CharField(max_length=32, unique=True, blank=True)

    should_stop_polling = CacheField(default=True)
    is_long_polling = CacheField(default=False, timeout=60)
    long_poll_failures = CacheField(default=0)

    class Meta:
        verbose_name = "MBED Cloud account"

    def __unicode__(self):
        return self.display_name

    def save(self, *args, **kwargs):
        super(MBEDCloudAccount, self).save(*args, **kwargs)
        try:
            webhook_api_key = self.webhookauth.api_key
        except WebHookAuth.DoesNotExist:
            webhook_auth = WebHookAuth(mbed_cloud_account=self)
            webhook_auth.save()
            webhook_api_key = webhook_auth.api_key
        if not self.display_name:
            self.display_name = self.api_key
        logger.debug("webhook_api_key: %r", webhook_api_key)

    def get_session(self):
        return MBEDCloudSession(self.api_key, url=self.url)

    def get_webhook_url(self):
        site = Site.objects.get_current()
        try:
            scheme = site.sitescheme.scheme
        except SiteScheme.DoesNotExist:
            scheme = SiteScheme._meta.get_field('scheme').get_default()
        webhook_path = reverse('mbed cloud webhook')
        webhook_url = "%s://%s%s" % (scheme, site.domain, webhook_path)
        return webhook_url

    @cached_property
    def webhook_callback_set(self):
        if self.api_key:
            try:
                return self.get_session().get_callback()
            except requests.exceptions.HTTPError:
                return None
        else:
            return None

    def is_webhook_callback_set(self):
        return bool(self.webhook_callback_set)
    is_webhook_callback_set.boolean = True

    def set_webhook_callback(self):
        webhook_url = self.get_webhook_url()
        logger.debug("webhook_url: %r", webhook_url)
        session = self.get_session()
        session.delete_long_poll()
        self.delete_webhook_callback()

        webhook_api_key = self.webhookauth.api_key
        headers = {'Authorization': "Bearer %s" % webhook_api_key}
        session.set_callback(webhook_url, headers=headers)
        self.set_presubscriptions()

    def delete_webhook_callback(self):
        session = self.get_session()
        try:
            session.delete_callback()
        except requests.RequestException, e:
            if e.response.status_code == 404:
                logger.debug('No callback found when trying to delete')
            else:
                raise

    def set_presubscriptions(self):
        # Delete presubscriptions can only fail currently because the
        # endpoint or resource could not be found. If that's the case
        # we don't need to attempt to setup new subscriptions
        session = self.get_session()
        session.delete_presubscriptions()
        session.set_presubscriptions(settings.MBED_CLOUD_PRESUBSCRIPTIONS)

    def start_long_polling(self):
        if self.is_long_polling:
            raise Exception("is already running")
        session = self.get_session()
        session.delete_long_poll()
        self.delete_webhook_callback()
        self.set_presubscriptions()
        self.is_long_polling = True
        self.should_stop_polling = False
        Channel('mbedcloudaccount.poll').send({'mbedcloudaccount_id': self.id})

    def stop_long_polling(self):
        self.should_stop_polling = True


class WebHookAuth(models.Model):
    """This is what an MBED Cloud account needs to connect to us

    When a MBED Cloud webhook is setup, you can tell MBED Cloud to
    send optional headers[1].  We include an API Key header that matches
    api_key below.

    1. https://cloud.mbed.com/docs/v1.2/service-api-references/connect-api.html#webhook

    """
    api_key = models.CharField(max_length=200, unique=True, blank=True)
    mbed_cloud_account = models.OneToOneField(MBEDCloudAccount, null=True, blank=True)

    def __unicode__(self):
        return self.api_key

    def save(self, *args, **kwargs):
        if not self.api_key:
            self.api_key = unicode(uuid4())
        super(WebHookAuth, self).save(*args, **kwargs)


class SiteScheme(models.Model):
    SCHEMES = (
        ('http', 'http'),
        ('https', 'https'),
    )
    site = models.OneToOneField(Site)
    scheme = models.CharField(max_length=10, choices=SCHEMES, default='https')
