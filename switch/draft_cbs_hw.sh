sudo tc qdisc del dev enp1s0 parent root

sudo tc qdisc show dev enp1s0
sudo tc qdisc add dev enp1s0 handle 6666: parent root mqprio num_tc 3 \
            map 0 0 1 2 2 2 2 2 2 2 2 2 2 2 2 2 \
            queues 1@0 1@1 2@2 \
            hw 0


sudo tc qdisc replace dev enp1s0 parent 6666:1 handle 7777 cbs \
         idleslope 100000 sendslope -900000 hicredit 155 locredit -58591 offload 1
sudo tc qdisc replace dev enp1s0 parent 6666:2 handle 8888 cbs \
         idleslope 3000 sendslope -997000 hicredit 24503974 locredit -64906 offload 1

         
sudo tc filter add dev enp1s0 egress prio 1 u32 match ip dport 5201 0xffff flowid 6666:1
sudo tc filter add dev enp1s0 egress prio 1 u32 match ip dport 5202 0xffff flowid 6666:2

lsmod | grep cbs
