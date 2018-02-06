#! /bin/bash
#PRU_CORE should be either 0 or 1

cd XData/SPI
PRU_CORE=0
echo "*****************************************************"
echo "**             Building PRU Program                **"
echo "*****************************************************"
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
echo "*****************************************************"
echo "**              Building Userspace                 **"
echo "*****************************************************"

cd ../
echo "**** Starting ****"
echo "Building project"
        gcc rpmsg_acquisition.c -o rpmsg_acquisition
echo "launching project"
        ./rpmsg_acquisition
echo "*****************************************************"
echo "**                    DONE                         **"
echo "*****************************************************"
