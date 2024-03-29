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

# Configure loopback device (for some reason this is needed
# to get low latency from the alsa source)
# pactl load-module module-loopback \
#     source="alsa_input.platform-soc_sound.analog-stereo" \
#     sink="alsa_output.platform-soc_sound.analog-stereo"

set -o errexit
set -o pipefail

# Run setup. This is a no-op if setup is already complete.
./setup_pi.sh

# Size of the visualizations. Larger values increase the computational cost of
# rendering the visualizations. Since the output is aggressively downsampled for
# display on the LED array, ~100x100 should be sufficient.
RASTER_WIDTH=50
RASTER_HEIGHT=50
# Whether or not to run the LED driver module.
RUN_SPI_DRIVER=1
# Whether or not we are running projectM as the visualizer. In this case, some
# additional work is required to setup the window and the audio source.
RUN_PROJECTM=0

export DISPLAY=:0

function echoerr() {
  echo -e "***** " "$@" 1>&2
}

function get_microphone_source() {
  card_name="seeed-2mic-voicecard"
  microphone_source=$(pactl list sources |
    grep -e Name -e "alsa\.card_name = \"${card_name}\"" |
    grep -B1 "${card_name}" | grep -v 'monitor$' |
    sed -n "s/^\s\+Name:\s\+\(.*\)$/\1/p")
  echoerr "Microphone source: $microphone_source"
  echo $microphone_source
}

function load_null_sink() {
  sink_name="ledsuit_sink"
  if [[ -z $(pactl list sinks | grep "${sink_name}") ]]; then
    echoerr "Loading module-null-sink with sink name \"${sink_name}\"... "
    pacmd load-module module-null-sink "sink_name=${sink_name}"
    echoerr "Done."
  fi
  echo "${sink_name}.monitor"
}

function ctrl_c() {
  echoerr "Caught Ctrl-C; cleaning up."
  if [[ $RUN_SPI_DRIVER != 0 ]]; then
    echoerr "Killing spi driver... (${SPI_DRIVER_PID})"
    kill $SPI_DRIVER_PID
  fi
  echoerr "Killing xinit... (${XINIT_PID})"
  sudo kill $XINIT_PID

  if [[ ! -z "${PARECORD_PID}" ]]; then
    echoerr "Killing parecord... (${PARECORD_PID})"
    kill $PARECORD_PID
  fi
}

trap ctrl_c INT

# Default to microphone.
source_type="microphone"
SOURCE_ARG=""

while [[ ! -z "$@" ]]; do
  arg=$1
  shift

  case $arg in
    '-s')
      source_type=$1
      shift
      ;;

    *)
      passthrough_args+="${arg}"
      ;;
  esac
done

case $source_type in
  'microphone')
    SOURCE=$(get_microphone_source)
    SOURCE_ARG="--pulseaudio_source=${SOURCE}"
    ;;

  'network')
    SOURCE=$(load_null_sink)
    SOURCE_ARG="--pulseaudio_source=${SOURCE}"
    ;;

  'custom')
    SOURCE="custom"
    SOURCE_ARG=""
    ;;

  *)
    echoerr "No source type specified!"
    ;;
esac
echoerr "Selected source: $source_type => $SOURCE"

echoerr "Starting visualizations..."
xinit_command="./user_projectm.sh \
  --window_width=$RASTER_WIDTH \
  --window_height=$RASTER_HEIGHT \
  ${SOURCE_ARG} \
  ${passthrough_args[*]}"
sudo -E xinit $xinit_command -- :0 &
XINIT_PID=$!

sleep 5

if [[ $RUN_PROJECTM != 0 ]]; then
  # Wait for projectM to spool up so we can capture its window ID. This is done
  # for two reasons: the LED driver needs to capture the projectM window, so we
  # have it open after passing this block; and we need the window ID for
  # configuring the location of the window.
  while [[ -z $(xdotool search "projectM") ]]; do
    echoerr "Waiting for projectM..."
    sleep 1
  done

  echoerr "projectM started, setting up window"
  sleep 10

  # Hacks to get the input latency into an acceptable range for PulseAudio.
  # Since projectM-pulseaudio doesn't configure the source latency, it will end
  # up using the default, which may be waaaay too long to be useful.
  #
  # (kbalke note: IME, the latency was on the order of 1s and frames arrived
  #  infrequently so as to make the visualizations jump.)
  pactl set-source-volume "${SOURCE}" 100%
  parecord -r -d 2 --latency-msec=10 > /dev/null
  PARECORD_PID=$!

  xset s off
  xset -dpms
  xset s noblank

  # Configure the projectM window location.
  WINIDA=$(xdotool search "projectM" | head -n1)
  WINIDB=$(xdotool search "projectM" | tail -n1)

  xdotool windowmove $WINIDA 0 0
  xdotool windowsize $WINIDA $RASTER_WIDTH $RASTER_HEIGHT
  xdotool windowmove $WINIDB 0 0
  xdotool windowsize $WINIDB $RASTER_WIDTH $RASTER_HEIGHT

  # Move the cursor out of the window area so it doesn't draw to the LED array.
  xdotool mousemove $(($RASTER_WIDTH + 50)) $(($RASTER_HEIGHT + 50))
fi

if [[ $RUN_SPI_DRIVER != 0 ]]; then
  sleep 5

  /home/pi/led_driver --intensity=1 \
    --enable_projectm_controller=false \
    --mapping_file=/home/pi/mapping.binaryproto \
    --raster_x=0 \
    --raster_y=0 \
    --raster_width=$RASTER_WIDTH \
    --raster_height=$RASTER_HEIGHT &
  SPI_DRIVER_PID=$!
fi

wait $XINIT_PID
if [[ $RUN_SPI_DRIVER != 0 ]]; then
  wait $SPI_DRIVER_PID
fi
if [[ ! -z "${PARECORD_PID}" ]]; then
  wait $PARECORD_PID
fi
