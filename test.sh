#!/bin/bash

TARGET="UBLOX_EVK_ODIN_W2"
BAUD_RATE="115200"
OUTPUT_LOG="my_boot.log"
STANDARD_LOG="standard_boot.log"

# Find the serial port where board is connected
location="$(mbedls | grep ${TARGET} | awk '{print $8}')"
if [[ -z ${location} ]]; then
    echo "ERROR: No ${TARGET} found using 'mbedls'. Is board connected? Is mbedls installed?"
    exit 1
fi

date > ${OUTPUT_LOG}
# this mbed command returns a non-zero error code, but that is ok for our use case.
mbedhtrun --sync=0 -p ${location} --serial-output-file ${OUTPUT_LOG} --skip-flashing --baud-rate=${BAUD_RATE}
# remove standard mbed boot lines
grep -v BOOT ${OUTPUT_LOG} > temptemp.log
mv temptemp.log ${OUTPUT_LOG}
# change line endings to unix
sed $'s/\r$//' my_boot.log > temptemp.log
mv temptemp.log ${OUTPUT_LOG}
echo "Serial output stored in ${OUTPUT_LOG}."

# Check for error messages
if ! [[ -z "$(grep -i error ${OUTPUT_LOG})" ]]; then
  echo "ERROR: Found error message in the serial output."
  exit 1
fi

# Compare boot log to the standard log
python comparelogs.py ${STANDARD_LOG} ${OUTPUT_LOG}
rc=$?
if [[ ${rc} = 0 ]]; then
  echo "Looks good. All lines in ${STANDARD_LOG} were found in your ${OUTPUT_LOG}."
else
  echo "ERROR: Comparison between your ${OUTPUT_LOG} and the ${STANDARD_LOG} failed."
  exit 1
fi
