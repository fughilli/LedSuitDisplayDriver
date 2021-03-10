#!/bin/bash

set -o errexit
set -o pipefail

if [[ -f ~/.ledsuit_is_configured ]]; then
  echo "Configuration already completed."
  exit 0
fi

cleanup=1
setup_directory=".setup_scratch"
packages="vim python python3 python3-pip python-pip mesa-utils libsdl2-dev \
  libpulse-dev xserver-xorg xinit libglm-dev git libxdo-dev libraspberrypi-dev \
  pulseaudio-utils xdotool pulseaudio x11-xserver-utils"
python_packages="absl-py numpy scipy"

# Stuff that doesn't need to happen in any particular directory.
needs_install=0
for i in $packages; do
  if ! (dpkg -s "${i}" 1>/dev/null 2>&1); then
    needs_install=1
    break
  fi
done

if [[ $needs_install == 1 ]]; then
sudo apt-get update
sudo apt-get install -y $packages
fi

sudo -H pip3 install $python_packages

function cleanup_dir {
  $dir_to_cleanup="$1"
  if [[ $cleanup == 1 ]]; then
    echo "Cleaning up $dir_to_cleanup..."
    rm "$dir_to_cleanup"
  else
    echo "Skipping cleanup of $dir_to_cleanup."
  fi
}

(
  mkdir -p "$setup_directory"
  cd "$setup_directory"

  (
    echo "Installing SeeedStudio Voicecard driver..."
    voicecard_directory="voicecard"
    if [[ ! -d "$voicecard_directory" ]]; then
      git clone https://github.com/respeaker/seeed-voicecard.git \
        "$voicecard_directory"
    else
      echo "Using existing cloned repo at \"$(pwd)/$voicecard_directory\"."
    fi
    (
      cd "$voicecard_directory"
      sudo ./install.sh --compat-kernel
    )
    cleanup_dir "$voicecard_directory"
    echo "Done installing SeeedStudio Voicecard driver."
  )
)

(
  if [[ ! -f /boot/config.txt.bak ]]; then
    echo "Installing config.txt..."
    sudo mv /boot/config.txt{,.bak}
    echo "Backup taken at \"/boot/config.txt.bak\"."
    sudo cp config.txt.src /boot/config.txt
    echo "Done installing config.txt."
  else
    echo -n "Backup at \"/boot/config.txt.bak\" already exists, skipping "
    echo "installation..."
  fi
)

# Fix hostname
sudo sh -c 'echo "ledsuit" > /etc/hostname'
sudo sh -c 'sed -i "s/raspberrypi/ledsuit/g" /etc/hosts'

echo "Please reboot for changes to take effect."

touch ~/.ledsuit_is_configured
