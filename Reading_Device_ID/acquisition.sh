#! /bin/bash
echo "**** Starting ****"
echo "Building project"
        gcc rpmsg_acquisition.c -o rpmsg_acquisition
echo "launching project"
        ./rpmsg_acquisition
echo "**** End ****"
