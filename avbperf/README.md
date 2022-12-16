# tram
Network Traffic Management



The program requires superuser permission. 
The raw socket can only be opened by root user for security reasons.

The ubuntu default audio device (pulseaudio) is only started for normal user.
Since the process is started with sudo, the audio device will not be accessible.
to solve this, the pulseaudio device must be aded to oot user.


/etc/systemd/system/pulseaudio.service
```bash
[Unit]
Description=PulseAudio system server

[Service]
Type=notify
ExecStart=pulseaudio --daemonize=no --system --realtime --log-target=journal

[Install]
WantedBy=multi-user.target
```


sudo systemctl --system enable pulseaudio.service
sudo systemctl --system start pulseaudio.service
sudo systemctl --system status pulseaudio.service


/etc/pulse/client.conf 
```bash
#default-server = /var/run/pulse/native
autospawn = no
```

sudo adduser root pulse-access
sudo adduser pulse audio


sudo modprobe snd-aloop


sudo ./avbperf -t -i enp3s0 -d 1c:b7:2c:27:4c:78 -a snd-aloop