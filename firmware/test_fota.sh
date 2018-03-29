#!/bin/bash

# This assumes the board:
#  is connected,
#  has firmware that should be able to do firmware over-the-air (FOTA),
#  is connected to internet, and registered with mbed cloud,

# Set new version to look for in output
# to verify successful update
./json_set.py mbed_app.json config.version.value '"2.0"'

# Compile and start update campaign
make campaign

# Give time for mbed cloud and device to update
sleep 150

# Check board status - creates "my_boot.log"
./test.sh

# Check version from board boot log
grep 'Workplace Environmental Monitor version:' my_boot.log | grep '2.0'
rc=$?
if [[ ${rc} = 0 ]]; then
  echo "Successful firmware over-the-air update!"
else
  echo "Error: FOTA did not complete in the expected time."
fi
