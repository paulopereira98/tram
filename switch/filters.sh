#!/bin/bash

IFNAME=enp21s0

MAKO_MAC=00:0f:31:5c:f6:ff
MANTA_MAC=00:0f:31:4d:4b:a3

# Placeholder qdisc
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

sudo tc filter add dev $IFNAME egress prio 1 u32 \
    match ether src $MANTA_MAC \
    action vlan push id 0 protocol 802.1Q priority 2 \
    action skbedit priority 2
