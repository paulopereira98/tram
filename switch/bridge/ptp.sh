#!/bin/bash

NICS="enp1s0 enp21s0"

SWITCH_BRIDGE_DIR=${0%/*}
PTP_CONF_DIR=$SWITCH_BRIDGE_DIR/../../ptp/configs/


# E2E or P2P
if [ "$1" = "e2e" ] || [ "$1" = "E2E" ]
then
    TYPE="E2E"
else
    TYPE="P2P"
fi
echo $TYPE mode


cmd="sudo ptp4l -m "

if [ "$TYPE" = "E2E" ]
then
    cmd+="-f ${PTP_CONF_DIR}/E2E-TC.cfg "
else
    cmd+="-P -2 -f ${PTP_CONF_DIR}/P2P-TC.cfg "
fi

for nic in $NICS
do  
    cmd+="-i $nic "
done

    echo $cmd
    eval $cmd
