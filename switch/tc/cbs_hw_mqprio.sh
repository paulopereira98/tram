#!/bin/bash


IFNAME=enp21s0
MAJNUM=6666


sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1

# Filter placeholder qdisc
sudo tc qdisc add dev $IFNAME clsact

# MQPRIO
sudo tc qdisc add dev $IFNAME parent root handle $MAJNUM mqprio \
    num_tc 4 \
    map 3 3 2 1 0 0 0 0 \
    queues 1@0 1@1 1@2 1@3 \
    hw 0

# queue 1, prio 3
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:2 cbs \
        idleslope 500000 sendslope -500000 hicredit 2699 locredit -771 offload 1

# queue 0, prios 4+
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:1 cbs \
        idleslope 600000 sendslope -400000 hicredit 926 locredit -616 offload 1


# show
sudo tc qdisc show dev $IFNAME parent 0
