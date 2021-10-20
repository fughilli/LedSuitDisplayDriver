#!/bin/bash

LOG=/home/pi/run_on_boot_log

touch $LOG
chmod a+w $LOG

echo "$(date): This is a test; user='$USER'; runlevel='$(runlevel)'" >> $LOG
if [[ -f /home/pi/.run_on_boot ]]; then
  echo "$(date): Running projectm" >> $LOG
  su -l pi -c "/home/pi/run_projectm.sh -s microphone &"
else
  echo "$(date): .run_on_boot not present; won't start projectm" >> $LOG
fi
