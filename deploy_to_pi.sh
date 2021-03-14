#!/bin/bash
#
# LED Suit Driver - Embedded host driver software for Kevin's LED suit controller.
# Copyright (C) 2019-2020 Kevin Balke
#
# This file is part of LED Suit Driver.
#
# LED Suit Driver is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LED Suit Driver is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LED Suit Driver.  If not, see <http://www.gnu.org/licenses/>.
#

set -o errexit
set -o pipefail

# Build the LED driver and projectM.
bazelisk build --distinct_host_configuration --config=pi \
  :led_driver \
  :projectm_sdl_test #\
  #-c opt

# Generate the LED mapping proto.
bazelisk run --distinct_host_configuration :mapping_generator -- --export_only \
  --generate_file=$PWD/generate.py --export_file=$PWD/mapping.binaryproto

# Set permissions.
cp ../bazel-out/armeabihf-opt/bin/led_driver/led_driver \
  /tmp/led_driver
cp ../bazel-out/armeabihf-opt/bin/led_driver/projectm_sdl_test \
  /tmp/projectm_sdl_test

chmod +rw /tmp/led_driver
chmod +rw /tmp/projectm_sdl_test

# Copy to the RPi.
rsync --update --checksum -va \
  /tmp/led_driver \
  /tmp/projectm_sdl_test \
  mapping.binaryproto \
  remote_scripts/* \
  remote_config/* \
pi@ledsuit:~
