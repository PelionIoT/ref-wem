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


import requests


class BearerAuth(requests.auth.AuthBase):
    def __init__(self, api_key):
        super(BearerAuth, self).__init__()
        self.api_key = api_key

    def __call__(self, r):
        r.headers.update({'Authorization': "Bearer %s" % self.api_key})
        return r


class MBEDCloudSession(requests.Session):
    """MBED wrapper around requests.Session

    These are some common things you might want to do with the MBED cloud API.
    https://cloud.mbed.com/docs/v1.2/service-api-references/connect-api.html

    """

    def __init__(self, api_key, url="https://api.us-east-1.mbedcloud.com"):
        super(MBEDCloudSession, self).__init__()
        self.auth = BearerAuth(api_key)
        self.base_url = url

    def request(self, method, path, **kwargs):
        url = self.base_url + path
        return super(MBEDCloudSession, self).request(method, url, **kwargs)

    """https://cloud.mbed.com/docs/v1.2/service-api-references/connect-api.html#notifications"""
    def get_callback(self):
        r = self.get("/v2/notification/callback")
        try:
            r.raise_for_status()
        except requests.HTTPError, e:
            if e.response.status_code == 404:
                return None
            else:
                raise
        return r.json()

    def set_callback(self, url, headers=None):
        data = {'url': url}
        if headers:
            data['headers'] = headers
        r = self.put("/v2/notification/callback", json=data)
        r.raise_for_status()

    def delete_callback(self):
        r = self.delete("/v2/notification/callback")
        r.raise_for_status()

    """https://cloud.mbed.com/docs/v1.2/service-api-references/connect-api.html#subscriptions"""
    def get_presubscriptions(self):
        r = self.get("/v2/subscriptions")
        r.raise_for_status()
        return r.json()

    def set_presubscriptions(self, presubs):
        r = self.put("/v2/subscriptions", json=presubs)
        r.raise_for_status()

    def delete_presubscriptions(self):
        r = self.delete("/v2/subscriptions")
        r.raise_for_status()

    def set_endpoint_resource(self, endpoint_id, resource_path, payload):
        data = '%r' % payload
        if resource_path[0] == '/':
            resource_path = resource_path[1:]
        r = self.put("/v2/endpoints/" + endpoint_id + "/" + resource_path, data=data)
        r.raise_for_status()

    def pull_notifications(self):
        r = self.get("/v2/notification/pull", headers={'connection': 'keep-alive'})
        r.raise_for_status()
        return r.json()

    def delete_long_poll(self):
        r = self.delete("/v2/notification/pull")
        r.raise_for_status()
