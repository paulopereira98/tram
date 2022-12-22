




# AVB


<br>

```mermaid
    graph LR;

    akg(Microphone<br><i>AKG CK 99 L<i/>);
    foc(Audio interface<br><i>Focusrite Scarlet2i2<i/>);
    PC(AVB endpoint<br><i>asus.local<i/>);
    server(Edge Server<br><i>msi.local<i/>);
    sw(Switch<br><i><i/>);

    akg-->foc-->PC-->sw;
    sw-->server;
```

sudo nice -n -20 ./avbperf -i enp3s0 -m100 -t10
<img src="images/TEK00005.PNG"/>


sudo nice -n -20 ./avbperf -i enp3s0 -m10 -t10
<img src="images/TEK00007.PNG"/>

sudo nice -n -20 ./avbperf -i enp3s0 -c2 -b24 -m10 -t10
<img src="images/TEK00010.PNG"/>

sudo nice -n -20 ./avbperf -i enp3s0 -c2 -r96 -m10 -t10
<img src="images/TEK00012.PNG"/>

