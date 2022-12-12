
#!/bin/bash


IFNAME=enp21s0

sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1
#sudo tc qdisc show dev $IFNAME parent 0


# PRIO
sudo tc qdisc add dev $IFNAME handle 1: parent root prio \
    bands 4 \
    priomap 3 2 1 0 0 0 0 0


# queue 0
sudo tc qdisc replace dev $IFNAME parent 1:4 handle 7773 cbs \
        idleslope 3000 sendslope -997000 hicredit 24503974 locredit -64906 offload 0

# queue 1
sudo tc qdisc replace dev $IFNAME parent 1:3 handle 7772 cbs \
        idleslope 200000 sendslope -800000 hicredit 771 locredit -32551 offload 0

# queue 2
sudo tc qdisc replace dev $IFNAME parent 1:2 handle 7771 cbs \
        idleslope 500000 sendslope -500000 hicredit 771 locredit -32551 offload 0


# queue 3
sudo tc qdisc replace dev $IFNAME parent 1:1 handle 7770 cbs \
        idleslope 900000 sendslope -100000 hicredit 1388 locredit -6510 offload 0


# Show
sudo tc qdisc show dev $IFNAME parent 0
