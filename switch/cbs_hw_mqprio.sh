

IFNAME=enp21s0

MAKO_MAC=00:0f:31:5c:f6:ff
MANTA_MAC=00:0f:31:5c:f6:ff
CAM_MAC=$MAKO_MAC

sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1
#sudo tc qdisc show dev $IFNAME parent 0


# MQPRIO
sudo tc qdisc add dev $IFNAME handle 1: parent root mqprio \
    num_tc 4 \
    map 3 2 1 0 0 0 0 0 \
    queues 1@0 1@1 1@2 1@3 \
    hw 0

# PRIO
#sudo tc qdisc add dev $IFNAME handle 1: parent root prio \
#    bands 4 \
#    priomap 0 1 2 3 0 0 0 0


# queue 0
sudo tc qdisc replace dev $IFNAME parent 1:4 handle 7773 cbs \
        idleslope 3000 sendslope -997000 hicredit 24503974 locredit -64906 offload 0

# queue 1
sudo tc qdisc replace dev $IFNAME parent 1:3 handle 7772 cbs \
        idleslope 200000 sendslope -800000 hicredit 771 locredit -32551 offload 0

# queue 2
sudo tc qdisc replace dev $IFNAME parent 1:2 handle 7771 cbs \
        idleslope 500000 sendslope -500000 hicredit 771 locredit -32551 offload 1


# queue 3
sudo tc qdisc replace dev $IFNAME parent 1:1 handle 7770 cbs \
        idleslope 900000 sendslope -100000 hicredit 1388 locredit -6510 offload 1



#filters
sudo tc qdisc add dev $IFNAME clsact

sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ip dport 5200 0xffff \
    action vlan push id 0 protocol 802.1Q priority 0 \
    action skbedit priority 0

sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ip dport 5201 0xffff \
    action vlan push id 0 protocol 802.1Q priority 1 \
    action skbedit priority 1

sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ip dport 5202 0xffff \
    action vlan push id 0 protocol 802.1Q priority 2 \
    action skbedit priority 2

sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ip dport 5203 0xffff \
    action vlan push id 0 protocol 802.1Q priority 3 \
    action skbedit priority 3

# cameras
sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ether src $MAKO_MAC \
    action vlan push id 0 protocol 802.1Q priority 2 \
    action skbedit priority 2


sudo tc qdisc show dev $IFNAME parent 0