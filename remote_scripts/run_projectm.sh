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

RASTER_X=100
RASTER_Y=100

export DISPLAY=:0

RUN_SPI_DRIVER=1

#echo "Reconfiguring sound module..."
#su -c "./reconfigure_source.sh" pi

echo "Starting projectM..."
#xinit /home/pi/trace_projectm.sh $* -- :0 &
xinit /home/pi/user_projectm.sh $* -- :0 &
#xinit /usr/bin/projectM-pulseaudio $* -- :0 1>run_projectm_log.txt 2>&1 &
XINIT_PID=$!

trap ctrl_c INT

function ctrl_c() {
    echo "Caught Ctrl-C"
    if [[ $RUN_SPI_DRIVER != 0 ]]; then
    echo "Killing spi driver..."
    kill $SPI_DRIVER_PID
    fi
    echo "Killing xinit..."
    kill $XINIT_PID
    echo "Killing parecord..."
    kill $PARECORD_PID
}

while [[ -z $(xdotool search "projectM") ]]
do
    echo "Waiting for projectM..."
    sleep 1
done

echo "projectM started, setting up window"
sleep 10

if [[ $RUN_SPI_DRIVER != 0 ]]; then
    DISPLAY=:0 /home/pi/led_driver --intensity=0.2 --visual_interest_threshold=0.05 \
                                   --calculation_period_ms=200 --cooldown_duration=10 \
                                   --moving_average_minimum_invocations=10 \
                                   --enable_projectm_controller=true \
                                   --mapping_file=/home/pi/mapping.binaryproto &
    SPI_DRIVER_PID=$!
fi

# Hacks to get the input latency into an acceptable range for PulseAudio.
pactl set-source-volume alsa_input.platform-soc_sound.analog-stereo 100%
parecord -r -d 2 --latency-msec=10 > /dev/null
PARECORD_PID=$!

xset s off
xset -dpms
xset s noblank

WINIDA=$(xdotool search "projectM" | head -n1)
WINIDB=$(xdotool search "projectM" | tail -n1)

xdotool windowmove $WINIDA 0 0
xdotool windowsize $WINIDA $RASTER_X $RASTER_Y
xdotool windowmove $WINIDB 0 0
xdotool windowsize $WINIDB $RASTER_X $RASTER_Y

xdotool mousemove $(($RASTER_X + 50)) $(($RASTER_Y + 50))

wait $XINIT_PID
if [[ $RUN_SPI_DRIVER != 0 ]]; then
wait $SPI_DRIVER_PID
fi
