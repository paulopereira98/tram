
### PTP
Biggest packet: 
    Announce packet in OSI L4
    Packet size: 106+4 Bytes

    bandwidth per period: Announce + Sync + Follow_Up + Delay_Req + Delay_Resp
    106 + 86 + 86 + 96 = 374 Bytes
    including 802.1Q tags: 390 Bytes
    average bandwidth: <3.5k kbps/period
    period: 1 sec

### AVT Manta G-917C
##### Full res
    resolution: 3384 x 2710 pixels
    image size: 9170640 Bytes
    packets per frame: 6255+1
    burst size: ~10 MBytes
    average bandwidth: ~80 Mbps/fps
##### Decimation by 2
    resolution: 1692 x 1354 pixels
    image size: 2290968 Bytes
    packets per frame: 1562+1
    burst size: ~2.5 MBytes
    average bandwidth: ~20 Mbps/fps


### AVT Mako G-234C
##### Full res
    resolution: 1936 x 1216 pixels
    image size: 2354176 Bytes
    packets per frame: 1608+1
    burst size: ~2.5 MBytes
    average bandwidth: ~20 Mbps/fps
##### Decimation by 2
    resolution: 968 x 608 pixels
    image size: 588544 Bytes
    packets per frame: 401+1
    burst size: ~650 kBytes
    average bandwidth: ~20 Mbps/fps


### AVB
##### Stereo 16-bit @ 48 kHz

##### Stereo 24-bit @ 96 kHz