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

from django.core.management.base import BaseCommand, CommandError

from livedevice.models import MBEDCloudAccount


class Command(BaseCommand):
    help = "Import MBED Cloud accounts"

    def add_arguments(self, parser):
        parser.add_argument('accounts_path', help='an accounts.json file')

    def handle(self, *args, **options):
        accounts_data = json.load(open(options['accounts_path'], 'rb'))
        for account_data in accounts_data:
            account = MBEDCloudAccount(
                api_key=account_data['key'],
                display_name=account_data['email'],
            )
            account.save()
