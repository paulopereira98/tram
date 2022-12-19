#!/bin/bash


IFNAME=enp21s0
MAJNUM=6666


sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1



# MQPRIO
sudo tc qdisc add dev $IFNAME parent root handle $MAJNUM mqprio \
    num_tc 4 \
    map 3 2 1 0 0 0 0 0  \
    queues 1@0 1@1 1@2 1@3 \
    hw 0


# queue 1, prio 2
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:2 cbs \
        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0 offload 1

# queue 0, prio 4+
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:1 cbs \
        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0 offload 1


# show
sudo tc qdisc show dev $IFNAME parent 0
