#!/bin/bash
set -e # fail the script if a command fails

if [ -z "$1" ]; then
    echo "No firmware image specified"
    exit 1
fi

firmware_image="`pwd`/$1"

if [ ${firmware_image: -4} != ".bin" ]; then
    echo "Firmware image extension is not .bin"
    exit 1
fi

if [ ! -f "$firmware_image" ]; then
    echo "Firmware image does not exist"
    exit 1
fi

PATH=$PATH:/usr/local/bin # This should make node and npm accessible on OSX.
usb_dir=../../../lib/agent/packages/usb
usb_binding=$usb_dir/node_modules/usb/build/Release/usb_bindings.node

case "$(uname -s)" in
   Linux)
     blhost_path=linux/amd64
     ;;
   Darwin)
     blhost_path=mac
     ;;
   *)
     echo "Your operating system is not supported."
     exit 1
     ;;
esac

blhost_usb="../../../lib/bootloader/bin/Tools/blhost/$blhost_path/blhost --usb 0x1d50,0x6121"
blhost_buspal="$blhost_usb --buspal i2c,0x10,100k"

set -x # echo on

#if [ ! -f $usb_binding ]; then
#   cd $usb_dir
#   npm install
#fi

$usb_dir/send-kboot-command.js ping 0x10
$usb_dir/jump-to-slave-bootloader.js
$usb_dir/reenumerate.js buspal
$blhost_buspal get-property 1
$blhost_buspal flash-erase-all-unsecure
$blhost_buspal write-memory 0x0 "$firmware_image"
$blhost_usb reset
$usb_dir/reenumerate.js normalKeyboard
$usb_dir/send-kboot-command.js reset 0x10
