
# Space separated list of nics
NICS="enp1s0 enp21s0 eno1 eno2"
BRIDGE="ovs-br0"

DPDK=false

OVSDEV_PCIID=0000:06:00.0 

# Create bridge
sudo ovs-vsctl add-br $BRIDGE

for nic in $NICS
do  
    echo "Configuring $nic"
    sudo ip link set dev $nic down
    sudo ip link set dev $nic promisc on

    if $DPDK
    then
        sudo ovs-vsctl add-port $BRIDGE $nic \
            -- set Interface $nic type=dpdk "options:dpdk-devargs=${OVSDEV_PCIID}"
    else
        sudo ovs-vsctl add-port $BRIDGE $nic
    fi

    sudo ip link set dev $nic up
done

sudo ip link set dev $BRIDGE up

# enable dhcp for the bridge pc (so it can be accessed)
sudo dhclient $BRIDGE

#sudo ip link set dev $BRIDGE down
#sudo ovs-vsctl del-br $BRIDGE

####
#install
#dpdk-devbind.py --bind=vfio-pci eno1
