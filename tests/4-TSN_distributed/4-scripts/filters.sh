#!/bin/bash

IFNAME=enp3s0

MAKO_MAC="00:0f:31:5c:f6:ff"
MANTA_MAC="00:0f:31:4d:4b:a3"
VLAN=0


# iperf3 ports 5200..5207
for p in {0..7}
do  
    sudo tc filter add dev $IFNAME egress prio 1 u32 \
        match ip dport 520$p 0xffff \
        action vlan push id $VLAN protocol 802.1Q priority $p \
        action skbedit priority $p
done

# PTP L2
    # all messages are multicast to 01:80:c2:00:00:0e
    sudo tc filter add dev $IFNAME egress prio 1 u32 \
        match ether dst 01:80:c2:00:00:0e \
        action vlan push id $VLAN protocol 802.1Q priority 7 \
        action skbedit priority 7
# PTP L4
    # ptp ip ports: 319(events), 320(general) 
    for port in 319 320
    do  
        sudo tc filter add dev $IFNAME egress prio 1 u32 \
            match ip dport $port 0xffff \
            action vlan push id $VLAN protocol 802.1Q priority 7 \
            action skbedit priority 7
    done

# cameras
sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ether src $MAKO_MAC \
    action vlan push id 0 protocol 802.1Q priority 2 \
    action skbedit priority 2

sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ether src $MANTA_MAC \
    action vlan push id 0 protocol 802.1Q priority 3 \
    action skbedit priority 3

# cameras
sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ether src $MAKO_MAC \
    action vlan push id 0 protocol 802.1Q priority 2 \
    action skbedit priority 2

