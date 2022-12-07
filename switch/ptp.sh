
/home/tsnswitch/linuxptp/configs/gPTP.cfg

sudo ptp4l -f /usr/share/doc/linuxptp/configs/gPTP.cfg -i enp21s0 --step_threshold=1 -m
sudo ptp4l -f /home/tsnswitch/linuxptp/configs/gPTP.cfg -i enp21s0 --step_threshold=1 -m



sudo phc2sys -s enp21s0 -c CLOCK_REALTIME --step_threshold=1 --transportSpecific=1 -w -m

sudo phc2sys -c enp21s0 -s CLOCK_REALTIME --step_threshold=1 --transportSpecific=1 -w -m
