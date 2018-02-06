#! /bin/bash
#PRU_CORE should be either 0 or 1
PRU_CORE=0

echo "*******************************************************"
echo "This must be compiled on the BEAGLEBONE BLACK itself"
echo "It was tested on 4.4.12-ti-r31 kernel version"
echo "The source code for blinky ie PRU_gpioToggle was taken from"
echo "pru-software-support-package and can be cloned from"
echo "git clone git://git.ti.com/pru-software-support-package/pru-software-support-package.git"
echo "******NOTE: use a resistor >470 ohms to connect to the LED, I have alredy made this mistake."
echo "To continue, press any key:"
read


echo "-Building project"
        make clean
        make

echo "-Placing the firmware"
        cp gen/*.out /lib/firmware/am335x-pru$PRU_CORE-fw

echo "-Configuring pinmux"
        config-pin P8.30 pruout
        config-pin P9.31 pruout
        config-pin P9.27 pruout
        config-pin P9.29 pruout
        config-pin P9.28 pruin
        config-pin P9.30 pruout
        
echo "-Rebooting"
        if [ $PRU_CORE -eq 0 ]
        then
                echo "Rebooting pru-core 0"
                echo "stop" > /sys/class/remoteproc/remoteproc1/state
                echo "start" > /sys/class/remoteproc/remoteproc1/state
        else
                echo "Rebooting pru-core 1"
                echo "stop"  > /sys/class/remoteproc/remoteproc2/state
                echo "start" > /sys/class/remoteproc/remoteproc2/state
        fi

echo "Done."
