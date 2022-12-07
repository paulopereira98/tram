

IFNAME=enp21s0

sudo tc qdisc del dev $IFNAME parent root
sudo tc qdisc del dev $IFNAME parent ffff:fff1
#sudo tc qdisc show dev $IFNAME parent 0


# TAPRIO
sudo tc qdisc add dev $IFNAME handle 1: parent root taprio \
    num_tc 4 \
    map 3 2 1 0 0 0 0 0 \
    queues 1@0 1@1 1@2 1@3 \
    base-time 1000000000 \
    sched-entry S 01 300000 \
    sched-entry S 02 300000 \
    sched-entry S 03 300000 \
    sched-entry S 0F 3000000 \
    clockid CLOCK_TAI

# sched-entry <command 'S'> <gatemask (Hex)> <interval (ns)>

# gatemasks:
#           prios:   012>=3
# 00 0000 0000  0000 0000   useless
# 01 0000 0000  0000 0001   prios >= 3
# 02 0000 0000  0000 0010   prio  2
# 03 0000 0000  0000 0011   prios 2, >= 3
# 04 0000 0000  0000 0100   prio  1
# 05 0000 0000  0000 0101   prios 1, >= 3
# 06 0000 0000  0000 0110   prios 1, 2
# 07 0000 0000  0000 0111   prios 1, 2, >= 3

# 08 0000 0000  0000 1000   best-effort
# 09 0000 0000  0000 1001   best-effort + prios >= 3
# 0A 0000 0000  0000 1010   best-effort + prio  2
# 0B 0000 0000  0000 1011   best-effort + prios 2, >= 3
# 0C 0000 0000  0000 1100   best-effort + prio  1
# 0D 0000 0000  0000 1101   best-effort + prios 1, >= 3
# 0E 0000 0000  0000 1110   best-effort + prios 1, 2
# 0F 0000 0000  0000 1111   best-effort + prios 1, 2, >= 3
#....




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

#sudo tc filter add dev $IFNAME egress prio 1 u32 match ether src 00:0f:31:5c:f6:ff action skbedit priority 3


sudo tc qdisc show dev $IFNAME parent 0