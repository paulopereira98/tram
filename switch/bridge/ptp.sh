#!/bin/bash


# E2E or P2P
TYPE="P2P"

NICS="enp1s0 enp21s0 eno2 eno1"

PTP_CONF_DIR="../../ptp/configs/"

cmd="sudo ptp4l -m "

if [ "$TYPE" = "E2E" ]
then
    cmd+="-f ${PTP_CONF_DIR}/E2E-TC.cfg "
else
    cmd+="-f ${PTP_CONF_DIR}/P2P-TC.cfg "
fi

for nic in $NICS
do  
    cmd+="-i $nic "
done

    echo $cmd
    eval $cmd
