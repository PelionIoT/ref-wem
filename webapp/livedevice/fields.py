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


from django.core.cache import cache


class CacheField(object):
    """A Model field that stores stuff in cache instead of the db"""
    def __init__(self, default=None, timeout=None):
        self.default = default
        self.timeout = timeout

    def __get__(self, obj, objtype):
        val = cache.get(self.get_cache_key(obj))
        if val is None:
            val = self.default
        return val

    def __set__(self, obj, value):
        """Setting None deletes the value from the cache"""
        if value is None:
            cache.delete(self.get_cache_key(obj))
        else:
            try:
                # Setting a dict with 'timeout' and 'value' keys allows
                # the user to set the value with a particular timeout
                timeout = value['timeout']
                value = value['value']
            except:
                timeout = self.timeout
            cache.set(self.get_cache_key(obj), value, timeout)

    def contribute_to_class(self, cls, name):
        self.model = cls
        self.name = name
        setattr(cls, self.name, self)

    def get_cache_key(self, obj):
        return ','.join((self.model.__name__, str(obj.id), self.name))
