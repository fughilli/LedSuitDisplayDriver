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

projectm_command="./projectm_sdl_test \
  --window_x=0 \
  --window_y=0 \
  --channel_count=1 \
  --menu_font_path='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf' \
  --title_font_path='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf' \
  --preset_path='/home/pi/presets'"

libopendrop_command="./libopendrop \
  --window_x=0 \
  --window_y=0"

# Change comment argument to affect which program is run.
echo "COMMAND: <$libopendrop_command $@>"
su -c "$libopendrop_command $*" pi
