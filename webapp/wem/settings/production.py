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


import os

from defaults import *

DEBUG = False

DATABASES['default']['NAME'] = os.path.join(PROJECT_ROOT, "dev.db")
ALLOWED_HOSTS = ['*']
SITE_ID = int(os.environ.get("SITE_ID", 2))

TEMPLATES[0]["OPTIONS"]["debug"] = DEBUG

CHANNEL_LAYERS = {
    "default": {
        "BACKEND": "asgiref.inmemory.ChannelLayer",
        "BACKEND": "asgi_redis.RedisChannelLayer",
        "CONFIG": {
            "prefix": u"production",
        },
        "ROUTING": "wem.routing.channel_routing",
    },
}

CACHES = {
    "default": {
        "BACKEND": "django_redis.cache.RedisCache",
        "LOCATION": "redis://127.0.0.1:6379/1",
        "OPTIONS": {
            "CLIENT_CLASS": "django_redis.client.DefaultClient",
        }
    }
}
