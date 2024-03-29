

VLAN_ID=0
VLAN_NAME="vlan$VLAN_ID"

if [ -z "$1" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]
then
    echo "Configs vlan0 on given interface"
    echo "Usage:"
    echo "$0 ifname"
else

    cmd="sudo ip link add link $1 \
                name $VLAN_NAME type vlan id $VLAN_ID \
                egress-qos-map 0:0 1:1 2:2 3:3 4:4 5:5 6:6 7:7"
    echo $cmd
    eval $cmd

    cmd="sudo ip link set vlan0 up"
    echo $cmd
    eval $cmd
fi
