
#!/bin/bash


IFNAME=enp21s0
MAJNUM=7777

sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1
#sudo tc qdisc show dev $IFNAME parent 0


# PRIO
sudo tc qdisc add dev $IFNAME handle $MAJNUM: parent root prio \
    bands 4 \
    priomap 3 3 2 2 1 1 0 0


# queue 0
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:4 cbs \
        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0

# queue 1
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:3 cbs \
        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0

# queue 2
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:2 cbs \
        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0

# queue 3
sudo tc qdisc replace dev $IFNAME parent $MAJNUM:1 cbs \
        idleslope 1000000 sendslope 0 hicredit 1542 locredit 0



# Show
sudo tc qdisc show dev $IFNAME parent 0
