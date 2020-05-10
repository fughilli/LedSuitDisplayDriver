#!/bin/bash

set -o errexit
set -o pipefail

bazel build --distinct_host_configuration --config=pi :led_driver
cp ../bazel-bin/led_driver/led_driver /tmp/led_driver_test
chmod +rw /tmp/led_driver_test
bazel run --distinct_host_configuration :mapping_generator -- --export_only --generate_file=$PWD/generate.py --export_file=$PWD/mapping.binaryproto

scp /tmp/led_driver_test mapping.binaryproto pi@ledsuit:~
#ssh pi@ledsuit /home/pi/led_driver_test
