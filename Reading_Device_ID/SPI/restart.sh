#! /bin/bash
#PRU_CORE should be either 0 or 1
PRU_CORE=0
echo "-Restarting PRU"
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
