#!/usr/bin/env python
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

import sys

standard_f = sys.argv[1]
actual_f = sys.argv[2]

with open(standard_f, 'r') as f:
    standard_messages = [x.rstrip() for x in f.readlines()]

with open(actual_f, 'r') as f:
    actual_messages = f.read()

not_found_msgs = []
for m in standard_messages:
    #Possible improvement: use 're' and do regular expression searches
    if m not in actual_messages:
        not_found_msgs.append(m)

if not_found_msgs:
    print("ERROR: Some expected output was not seen in your %s:" % actual_f)
    print("\n".join(not_found_msgs))
    print("\nIf that missing message is ok, remove it from %s." % standard_f)
    sys.exit(1)
else:
    sys.exit(0)
