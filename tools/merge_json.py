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

import argparse
import json
import sys

def print_json(data):
    print(json.dumps(data,
                     separators=(',',': '),
                     indent=4,
                     sort_keys=True))

def merge_json(orig_fname, new_fname):
    with open(orig_fname, 'r') as forig:
        data_orig = json.load(forig)
    with open(new_fname, 'r') as fnew:
        data_new = json.load(fnew)
    # You *do not* want to do this:
    # data_orig.update(data_new)
    # You will lose more settings than you want.
    for k in data_new:
        if type(data_new[k]) == dict:
            if k not in data_orig or type(data_orig[k]) != dict:
                data_orig[k] = {}
            for i in data_new[k]:
                if type(data_new[k][i]) == dict:
                    if i not in data_orig[k] or type(data_orig[k][i]) != dict:
                        data_orig[k][i] = {}
                    for j in data_new[k][i]:
                        data_orig[k][i][j] = data_new[k][i][j]
                else:
                    data_orig[k][i] = data_new[k][i]
        else:
            data_orig[k] = data_new[k]
    return data_orig

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Update a JSON file using another JSON file.')
    parser.add_argument('original_file', type=str, help='Original JSON file.')
    parser.add_argument('new_file', type=str, help='New JSON file.')
    args = parser.parse_args()

    # merge the json
    merged = merge_json(args.original_file, args.new_file)

    # print the merged json to stdout
    print_json(merged)
