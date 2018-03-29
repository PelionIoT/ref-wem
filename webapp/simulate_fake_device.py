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


'''Simulate a fake device.

This module can be used to simulate a fake device for testing.
'''
#!/usr/bin/env python
import logging
import sys
import random
import json
from base64 import b64encode
from StringIO import StringIO

from twisted.python import log
from twisted.internet import reactor, defer
from twisted.internet.task import LoopingCall
from twisted.internet.error import ReactorNotRunning
from twisted.web.client import Agent, readBody
from twisted.web.http_headers import Headers
from twisted.web.client import FileBodyProducer

logger = logging.getLogger(__name__)
logger.setLevel(logging.WARN)

WEBHOOK_URL = "http://localhost:8000/live-device/mbed-cloud-webhook/"

# @NOTE: How this list came to be
# 1) Go to https://www.randomlists.com/random-world-cities to get a random list of cities
# 2) Go to https://wigle.net/ and search for the cities you've selected.
#    a) Filter on 'WiFi Net' and ensure the data range is within the most recent year (e.g. 2017-2018)
#    b) Zoom in to a spot with a lot of APs (preferably near something like a hotel or Starbucks)
#    c) Select atleast 3 MAC Address that are close to each other (Google Geolocation requires at least 3)
#    d) Add them to the list below
# 3) Run the script and verify that address shows up on the demo wem page.
# 4) Plug the coordinates into Google Maps and verify it's in the correct city
#    e.g. https://www.google.com/maps/place/<lat>,<long>
#
WIFI_APLIST = [
    {
        'city': 'Beijing, China',
        'aplist': [
            {'macAddress': '00:69:6c:bb:0b:97'},
            {'macAddress': '02:69:6c:bb:09:90'},
            {'macAddress': '04:95:e6:e8:64:40'},
        ]
    },
    {
        'city': 'Casablance, Morocco',
        'aplist': [
            {'macAddress': 'b4:75:0e:1e:16:4c'},
            {'macAddress': 'a4:7e:39:ad:99:50'},
            {'macAddress': '30:91:8f:a2:63:31'}
        ],
    },
    {
        'city': 'Johannesburg, South Africa',
        'aplist': [
            {'macAddress': '84:be:52:60:16:03'},
            {'macAddress': '9c:b2:b2:3d:fd:a4'},
            {'macAddress': '80:37:73:d5:61:05'}
        ],
    },
    {
        'city': 'London, UK',
        'aplist': [
            {'macAddress': 'a0:63:91:b2:d9:c0'},
            {'macAddress': '94:b4:0f:66:6e:51'},
            {'macAddress': '94:b4:0f:66:65:b1'}
        ],
    },
    {
        'city': 'New York City, USA',
        'aplist': [
            {'macAddress': '34:12:98:05:b1:5a'},
            {'macAddress': '24:b6:57:f8:bf:ac'},
            {'macAddress': '98:de:d0:8b:d6:72'}
        ],
    },
    {
        'city': 'Madrid, Spain',
        'aplist': [
            {'macAddress': '08:6a:0a:87:db:d9'},
            {'macAddress': 'd8:f8:5e:1b:31:0d'},
            {'macAddress': 'd8:b6:b7:1e:64:e4'}
        ],
    },
    {
        'city': 'Tokyo, Japan',
        'aplist': [
            {'macAddress': '54:3d:37:26:44:cc'},
            {'macAddress': '74:a3:4a:9b:e3:dd'},
            {'macAddress': '34:db:fd:60:fc:ca'}
        ],
    },
]


def send(data):
    agent = Agent(reactor)
    body = FileBodyProducer(StringIO(json.dumps(data)))
    headers = Headers({
        'Content-Type': ['application/json'],
        'Authorization': ['Bearer foobar']
    })
    deferred = agent.request('PUT', WEBHOOK_URL, headers, body)

    def body_read(body, response):
        if response.code >= 400:
            raise ValueError("send failed with %s: %s" % (response.code, body))
        elif response.code >= 500:
            raise Exception(
                "send failed on server with %s: %s" %
                (response.code, body))

    def response_received(response):
        if response.code >= 400:
            will_read_body = readBody(response)
            will_read_body.addCallback(body_read, response)
            return will_read_body
    deferred.addCallback(response_received)
    return deferred


class Sensor(object):
    CHANGE_PROB = 0
    CHANGE_INCREMENT = 0
    MIN = 0
    MAX = 10
    ID = 'invalid'

    def __init__(self, board, value=None):
        self.board = board
        self.value = value
        self.last_value = None

    def get_initial(self):
        return self.value

    def get_change(self):
        """Get randomized change from current value"""
        logger.debug("CHANGE_PROB: %r" % self.CHANGE_PROB)
        should_change = random.random()
        logger.debug("should_change: %r" % should_change)
        if should_change < self.CHANGE_PROB:
            logger.debug("CHANGE_INCREMENT: %r" % self.CHANGE_INCREMENT)
            which_direction = random.random()
            logger.debug("which_direction: %r" % which_direction)
            if which_direction < 0.5:
                return self.CHANGE_INCREMENT
            else:
                return -self.CHANGE_INCREMENT
        return 0

    def get_value(self):
        """Get the current value by see if there should be a randomized change,
        then checking to make sure that change doesn't put us out of bounds."""
        if self.value is None:
            self.value = self.get_initial()
            return self.value
        change = self.get_change()
        self.value = self.value + change
        if self.value < self.MIN or self.value > self.MAX:
            self.value = self.value - 2 * change
        return self.value

    def start_sending(self, interval=5.0, stopAfter=None):
        """Start sending data"""
        will_start = self.send_value()
        reactor.callLater(interval, self.start_loop, interval)
        if stopAfter:
            reactor.callLater(stopAfter, self.stop_sending)
        self.will_finish = defer.Deferred()
        return will_start, self.will_finish

    def start_loop(self, interval):
        logger.debug("starting loop for %s", self)
        self.loop = LoopingCall(self.send_value)
        self.loop.start(interval)

    def stop_sending(self):
        logger.debug("Board %s Sensor %s finished.", self.board.id, self.ID)
        self.loop.stop()
        self.will_finish.callback("done!")

    def send_value(self):
        """Send the value if it has changed"""
        value = self.get_value()
        if self.last_value != value:
            data = {
                'notifications': [{
                    'path': self.ID,
                    'max-age': 60,
                    'payload': b64encode(self.format_value(value)),
                    'ep': self.board.id,
                    'ct': 'text/plain'
                }]
            }
            d = send(data)
            self.last_value = value
        else:
            d = defer.Deferred()
            d.callback("no change")
        return d


class GeoSensor(Sensor):

    ID_PREFIX = '/3336/0/'

    def get_value(self):
        return self.value

    def send_single(self, path, payload):
        data = {
            'notifications': [{
                'path': path,
                'max-age': 60,
                'payload': b64encode(payload),
                'ep': self.board.id,
                'ct': 'text/plain'
            }]
        }
        return send(data)

    def send_value(self):
        """Send the value if it has changed"""
        value = self.get_value()
        if self.last_value != value:
            sends_will_finish = []
            sensor_data = [
                ("5514", "lat"),
                ("5515", "long"),
                ("5516", "accuracy"),
                ("5750", "type"),
            ]
            one_at_a_time = defer.DeferredSemaphore(1)
            for path_suffix, value_key in sensor_data:
                path = self.ID_PREFIX + path_suffix
                d = one_at_a_time.run(self.send_single,
                                      path,
                                      "%s" % value[value_key])
                sends_will_finish.append(d)

            self.last_value = value
            send_value_will_finish = defer.DeferredList(sends_will_finish)
        else:
            send_value_will_finish = defer.Deferred()
            send_value_will_finish.callback("no change")
        return send_value_will_finish


class TempSensor(Sensor):

    ID = '/3303/0/5700'
    CHANGE_PROB = 0.1
    CHANGE_INCREMENT = 1
    MIN = 20
    MAX = 30

    def get_initial(self):
        return (random.random() * 6.0) + 20

    def format_value(self, value):
        return "%s C" % value


class LightSensor(Sensor):

    ID = '/3301/0/5700'
    CHANGE_PROB = 0.5
    CHANGE_INCREMENT = 1000
    MIN = 0
    MAX = 10000

    def get_initial(self):
        return random.random() * self.MAX

    def format_value(self, value):
        return "%s lux" % value


class HumiditySensor(Sensor):

    ID = '/3304/0/5700'
    CHANGE_PROB = 0.2
    CHANGE_INCREMENT = 5
    MIN = 0
    MAX = 100

    def get_initial(self):
        return (random.random() * 70) + 10

    def format_value(self, value):
        return "%s%%" % value


class AppLabelSensor(Sensor):

    ID = '/26241/0/1'

    def get_value(self):
        return self.value

    def format_value(self, value):
        return "%s" % value


class NetworkResourceSensor(Sensor):

    ID = '/26242/0/1'

    def get_value(self):
        return self.value

    def format_value(self, value):
        return "%s" % json.dumps(value)


class Board(object):

    def __init__(self, id, stopAfter=None, label="fish", geo=None):
        self.id = id
        self.stopAfter = stopAfter
        self.sensors = [
            TempSensor(board=self),
            LightSensor(board=self),
            HumiditySensor(board=self),
            AppLabelSensor(board=self, value=label),
            NetworkResourceSensor(board=self,
                                  value=WIFI_APLIST[random.randint(0, len(WIFI_APLIST) - 1)]['aplist'])
        ]
        if geo is None:
            self.sensors.append(
                GeoSensor(board=self, value={
                    "lat": rand_lat(),
                    "long": rand_long(),
                    "accuracy": 1113,
                    "type": "auto"}))
        else:
            self.sensors.append(GeoSensor(board=self, value=geo))

    def start_sending(self, interval=5.0, stopAfter=None):
        self.will_finish = defer.Deferred()
        sensors_will_finish = []
        if stopAfter is None:
            stopAfter = self.stopAfter

        def start_sending(sensor):
            logger.debug("sensor start_sending: %r", sensor)
            sensor_will_start, sensor_will_finish = sensor.start_sending()
            if sensor_will_finish:
                sensors_will_finish.append(sensor_will_finish)
            return sensor_will_start

        one_at_a_time = defer.DeferredSemaphore(1)
        sensors_will_start = []
        for sensor in self.sensors:
            d = one_at_a_time.run(start_sending, sensor)
            sensors_will_start.append(d)

        will_start = defer.DeferredList(sensors_will_start)

        # When all sensors finish, deregister this board
        all_sensors_finish = defer.DeferredList(sensors_will_finish)

        all_sensors_finish.addCallback(self.deregister) \
                          .addCallback(self.will_finish.callback)
        return will_start, self.will_finish

    def deregister(self):
        logger.debug("Board %s expired.", self.id)
        dereg = {
            'registrations-expired': [
                self.id
            ]
        }
        return send(dereg)


def rand_lat():
    # keep away from the poles because it doesn't show well with the default
    # zoom level
    return random.uniform(-60, 60)


def rand_long():
    return random.uniform(-180, 180)


BOARDS = [
    Board('015d5b32b12f00000000000100100104', stopAfter=10, label="albacore"),
    Board('015d65e83c0b00000000000100100256', stopAfter=10, label="barracuda"),
    Board('015d3cb817c600000000000100100061',
          label="carp",
          geo={"lat": 30.2433,
               "long": -97.8456,
               "accuracy": 11,
               "type": "user"}),
    Board('015d7a220bff00000000000100100309',
          label="dragonfish",
          geo={"lat": 30.38,
               "long": -97.73,
               "accuracy": 1113,
               "type": "user"}),
    Board('015d7b2a9e400000000000010010017e',
          label="eulachon",
          geo={"lat": 52.2053,
               "long": 0.1218,
               "accuracy": 11,
               "type": "user"}),
    Board('015d334883ec00000000000100100075', label="flounder"),
    Board('015d7ec6030b000000000001001003af', label="ghost shark"),
]


def main(argv=None):
    if argv is None:
        argv = sys.argv

    logging.basicConfig()
    log.PythonLoggingObserver().start()

    boards_will_finish = []

    def start_sending(board):
        logger.debug("board start_sending: %r", board.id)
        will_start, will_finish = board.start_sending()
        if will_finish:
            boards_will_finish.append(will_finish)
        return will_start

    one_at_a_time = defer.DeferredSemaphore(1)
    for board in BOARDS:
        one_at_a_time.run(start_sending, board)

    after_boards_are_done = defer.DeferredList(boards_will_finish)

    def stop(_):
        try:
            reactor.stop()
        except ReactorNotRunning:
            pass

    after_boards_are_done.addCallback(stop)

    def deregister_all():
        logger.info("Shutdown all boards.")
        boards_will_deregister = []
        for board in BOARDS:
            board_will_deregister = board.deregister()
            boards_will_deregister.append(board_will_deregister)
        return defer.DeferredList(boards_will_deregister)
    reactor.addSystemEventTrigger('before', 'shutdown', deregister_all)
    reactor.run()


if __name__ == '__main__':
    sys.exit(main())
