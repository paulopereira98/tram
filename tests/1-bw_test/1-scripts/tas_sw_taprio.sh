

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
    sched-entry S FF 300000 \
    sched-entry S FF 300000 \
    sched-entry S FF 300000 \
    sched-entry S FF 3000000 \
    flags 0x1 \
    clockid CLOCK_TAI

# sched-entry <command 'S'> <gatemask (Hex)> <interval (ns)>

# gatemasks:
#          queues:   3210
# 00 0000 0000  0000 0000   useless
# 01 0000 0000  0000 0001   queues >= 3
# 02 0000 0000  0000 0010   queue  2
# 03 0000 0000  0000 0011   queues 2, >= 3
# 04 0000 0000  0000 0100   queue  1
# 05 0000 0000  0000 0101   queues 1, >= 3
# 06 0000 0000  0000 0110   queues 1, 2
# 07 0000 0000  0000 0111   queues 1, 2, >= 3

# 08 0000 0000  0000 1000   best-effort
# 09 0000 0000  0000 1001   best-effort + queues >= 3
# 0A 0000 0000  0000 1010   best-effort + queue  2
# 0B 0000 0000  0000 1011   best-effort + queues 2, >= 3
# 0C 0000 0000  0000 1100   best-effort + queue  1
# 0D 0000 0000  0000 1101   best-effort + queues 1, >= 3
# 0E 0000 0000  0000 1110   best-effort + queues 1, 2
# 0F 0000 0000  0000 1111   best-effort + queues 1, 2, >= 3
#....

  
sudo tc qdisc show dev $IFNAME parent 0