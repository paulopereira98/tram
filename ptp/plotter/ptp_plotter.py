
# https://www.geeksforgeeks.org/plot-multiple-lines-in-matplotlib/
# Parser script to plot PTP delay
#
# Vincent Jordan
# 2020.10.12
#
# Paulo Pereira
# 2022.12.13
#   -Changed regex pattern to match current ptp4l desktop implementation
#   -Added matplotlib plotting
#
# Run with:
# ptp4l options -m -s > file
# cat file | grep "master offset" | python3 ptp_plotter.py
 
import re
import fileinput
import sys
import matplotlib.pyplot as plt


#minKernelTime = 0
#maxKernelTime = 1000000000
duration = 60*10 #10 minutes
#minKernelTime = 0
#maxKernelTime = 1000000000

pattern = 'ptp4l\[(.+)\]: master offset\s+(-?[0-9]+) s([012]) freq\s+([+-]\d+) path delay\s+(-?\d+)$'
test_string = 'ptp4l[17498.733]: master offset    2551157 s0 freq -127500 path delay    481272'

kernelTime_arr = []
masterOffset_arr = []
freq_arr = []
pathDelay_arr = []
startTime = 0.0

# Gnuplot data header
print('# time, offset, freq, pathDelay')

for line in fileinput.input():
    # Regex search
    res = re.search(pattern, line)# if pattern was matched
    if res:
        # Capture result
        kernelTime   = res.group(1)
        masterOffset = res.group(2)
        state        = res.group(3)
        freq         = res.group(4)
        pathDelay    = res.group(5)    

        if startTime == 0.0:
            startTime = float(kernelTime) #store first time

        # state 1 is stepping
        # state 2 is servo control
        if (state == '2') and ( (float(kernelTime)-startTime) < duration) :
            #print(kernelTime, masterOffset, freq, pathDelay)
            kernelTime_arr  .append(float(kernelTime) - startTime)
            masterOffset_arr.append(int(masterOffset))
            freq_arr        .append(int(freq))
            pathDelay_arr   .append(int(pathDelay))

    # if issue in patter
    else:
        print("Regex error:", line)
        sys.exit()



plt.plot(kernelTime_arr, masterOffset_arr,  label = 'Offset',       linestyle="-")
plt.plot(kernelTime_arr, freq_arr,          label = 'Frequency',    linestyle="--")
plt.plot(kernelTime_arr, pathDelay_arr,     label = 'Path Delay',   linestyle="-.")
plt.legend()
plt.show()
