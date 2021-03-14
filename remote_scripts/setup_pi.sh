#!/bin/bash

set -o errexit
set -o pipefail

# Whether or not to clean up temporary directories used for setup.
cleanup=0
# Scratch directory into which to instantiate temporary state.
setup_directory=".setup_scratch"

# Packages to install with `apt-get`.
packages="libsdl2-dev libpulse-dev xserver-xorg xinit libglm-dev git \
  libxdo-dev libraspberrypi-dev pulseaudio-utils xdotool pulseaudio \
  x11-xserver-utils pulseaudio-module-bluetooth pulseaudio-module-zeroconf \
  gdbserver valgrind"

# Groups to add the user "pi" to.
groups="lp"

done_something=0

function cleanup_dir {
  dir_to_cleanup="$1"
  if [[ $cleanup == 1 ]]; then
    echo "Cleaning up $dir_to_cleanup."
    rm -Rf "$dir_to_cleanup"
  else
    echo "Skipping cleanup of $dir_to_cleanup."
  fi
}

function is_group_member() {
  group=$1
  user=$2
  grep "^${group}:x:[0-9]\+:.*${user}.*$" /etc/group 1> /dev/null 2>&1
}

for group in $groups; do
  if ! is_group_member "${group}" pi; then
    echo "Adding \"${user}\" to group \"${group}\"".
    usermod -a -G "${group}" pi
  fi
done

# Stuff that doesn't need to happen in any particular directory.
#
# Install packages. Check for existence of each package before invoking install.
{
  needs_install=0
  for i in $packages; do
    if ! (dpkg -s "${i}" 1> /dev/null 2>&1); then
      needs_install=1
      break
    fi
  done

  if [[ $needs_install == 1 ]]; then
    sudo apt-get update
    sudo apt-get install -y $packages
  fi
}

# Stuff that happens inside of the setup temporary directory.
{
  # Instantiate setup directory.
  mkdir -p "$setup_directory"

  (
    if [[ -z $(lsmod | grep snd_soc_seeed_voicecard) ]]; then
      echo -n "Installing SeeedStudio Voicecard driver... "
      voicecard_directory="voicecard"
      if [[ ! -d "$setup_directory/$voicecard_directory" ]]; then
        git clone https://github.com/respeaker/seeed-voicecard.git \
          "$setup_directory/$voicecard_directory"
      else
        echo -n "Using existing cloned repo at "
        echo "\"$setup_directory/$voicecard_directory\"."
      fi
      (
        cd "$setup_directory/$voicecard_directory"
        sudo ./install.sh --compat-kernel
      )
      echo "Done installing SeeedStudio Voicecard driver."
      done_something=1
    else
      echo "SeedStudio Voicecard driver is already installed, skipping "
      echo "installation..."
    fi
  )

  cleanup_dir "$setup_directory"
}

{
  if ! cmp config.txt.src /boot/config.txt; then
    echo -n "Installing config.txt... "
    if [[ ! -f /boot/config.txt.bak ]]; then
      sudo mv /boot/config.txt{,.bak}
      echo "Backup taken at \"/boot/config.txt.bak\"."
    fi
    sudo cp config.txt.src /boot/config.txt
    echo "Done installing config.txt."
    done_something=1
  else
    echo "\"/boot/config.txt\" matches source, skipping installation."
  fi
}

{
  if ! cmp default.pa.src /etc/pulse/default.pa; then
    echo -n "Installing default.pa... "
    if [[ ! -f /etc/pulse/default.pa.bak ]]; then
      sudo mv /etc/pulse/default.pa{,.bak}
      echo "Backup taken at \"/etc/pulse/default.pa.bak\"."
    fi
    sudo cp default.pa.src /etc/pulse/default.pa
    echo "Done installing default.pa."
    done_something=1
  else
    echo "\"/etc/pulse/default.pa\" matches source, skipping installation."
  fi
}

# Fix hostname
{
  if [[ ! -z $(echo /etc/hostname | grep raspberrypi) ]]; then
    echo -n "Configuring hostname... "
    sudo sh -c 'echo "ledsuit" > /etc/hostname'
    echo "Done."
    done_something=1
  fi
  if [[ ! -z $(echo /etc/hosts | grep raspberrypi) ]]; then
    echo -n "Configuring hosts file... "
    sudo sh -c 'sed -i "s/raspberrypi/ledsuit/g" /etc/hosts'
    echo "Done."
    done_something=1
  fi
}

if [[ $done_something == 1 ]]; then
  echo "Rebooting for changes to take effect..."
  sudo reboot
else
  echo "Configuration already completed."
fi
