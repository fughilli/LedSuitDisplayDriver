#!/bin/bash

# Set the below value to 1 to export an strace when running projectM.
USE_STRACE=0

if [[ $USE_STRACE == 1 ]]; then
  su -c "strace /usr/bin/projectM-pulseaudio 1>projectm_trace_log.txt 2>&1" pi
else
  su -c "/usr/bin/projectM-pulseaudio" pi
fi
