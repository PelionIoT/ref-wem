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


from django.core.management.base import BaseCommand, CommandError

from livedevice.models import MBEDCloudAccount


class Command(BaseCommand):
    help = "Start long polling all MBED Cloud accounts that don't have webhook callbacks set"

    def handle(self, *args, **options):
        for account in MBEDCloudAccount.objects.all():
            if not account.is_webhook_callback_set():
                account.start_long_polling()
