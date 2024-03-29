#!/bin/bash


# Space separated list of nics
NICS="enp1s0 enp21s0 eno1 eno2"
BRIDGE="br0"


# Create bridge
sudo brctl addbr $BRIDGE

for nic in $NICS
do  
    echo "Configuring $nic"
    sudo ip link set dev $nic down
    sudo ip link set dev $nic promisc on
    sudo brctl addif $BRIDGE $nic
    sudo ifconfig $nic mtu 1600 up 
    sudo ip link set dev $nic up
done

sudo ip link set dev $BRIDGE up

# enable dhcp for the bridge pc (so it can be accessed)
sudo dhclient $BRIDGE

brctl show $BRIDGE

# Delete bridge:
# 
# sudo ip link set dev $BRIDGE down
# sudo brctl delbr $BRIDGE
