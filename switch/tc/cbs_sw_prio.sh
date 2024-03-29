
#!/bin/bash


IFNAME=enp21s0
MAJNUM=7777

sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1
#sudo tc qdisc show dev $IFNAME parent 0

# Filter placeholder qdisc
sudo tc qdisc add dev $IFNAME clsact

# PRIO
sudo tc qdisc add dev $IFNAME handle $MAJNUM: parent root prio \
    bands 4 \
    priomap 3 3 2 1 0 0 0 0


## queue 3, prio 0,1
#sudo tc qdisc replace dev $IFNAME parent $MAJNUM:4 cbs \
#        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0
#
## queue 2, prio 2
#sudo tc qdisc replace dev $IFNAME parent $MAJNUM:3 cbs \
#        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0

# queue 1, prio 3
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:2 cbs \
        idleslope 500000 sendslope -500000 hicredit 2699 locredit -771

# queue 0, prios 4+
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:1 cbs \
        idleslope 600000 sendslope -400000 hicredit 926 locredit -616



# Show
sudo tc qdisc show dev $IFNAME parent 0
