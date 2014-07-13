#!/bin/bash
###########################
#Created by Paul 2010.07.20
###########################
source ../../../config_android
echo "KDIR-$MALI_BUILD_HARDWARE:=$ANDROID_KERNEL_DIR" > KDIR_CONFIGURATION
chmod 777 KDIR_CONFIGURATION
echo "$MALI_KERNEL_CONFIG make" > mk
chmod 777 mk
echo "$MALI_KERNEL_CONFIG make clean" > mk_clean
chmod 777 mk_clean
./mk_clean
./mk
