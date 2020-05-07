#!/bin/bash

bazel build --config=pi :led_driver

cp ../bazel-bin/led_driver/led_driver /tmp/led_driver_test
chmod +rw /tmp/led_driver_test
scp /tmp/led_driver_test pi@ledsuit:~
#ssh pi@ledsuit /home/pi/led_driver_test
