#!/bin/zsh

if [ $# -eq 0 ]; then
  if [ "$test_device" = "" ]; then
    echo "You must supply the serial number of device to test or set <test_device> to the serial number you want to test."
    exit 1
  else
    serial=$test_device
  fi
else
  serial=$1
fi

blink_cmd="{ \"serialNumber\" : \"$serial\" , \"duration\" : 10 }"

webtoken=`./login.sh | jq -r '.access_token'`

curl -X POST "https://localhost:16001/api/v1/device/$serial/blink" \
      -H  "accept: application/json" \
      -H "Authorization: Bearer $webtoken" \
      --insecure -d "$blink_cmd"