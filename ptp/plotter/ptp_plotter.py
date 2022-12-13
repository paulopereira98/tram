#!/usr/bin/env python3

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
import sys
import matplotlib.pyplot as plt
import argparse
import typing
from cycler import cycle


pattern = 'ptp4l\[(.+)\]: master offset\s+(-?[0-9]+) s([012]) freq\s+([+-]\d+) path delay\s+(-?\d+)$'
test_string = 'ptp4l[17498.733]: master offset    2551157 s0 freq -127500 path delay    481272'


class Stat:
    def __init__(self, time, offset, state, freq, delay) -> None:
        self.time           :float  = time
        self.masterOffset   :int    = offset
        self.state          :int    = state
        self.freq           :int    = freq
        self.pathDelay      :int    = delay

class PtpStats:
    def __init__(self, filename, label) -> None:
        self.filename :str = filename
        self.label :str = label
        self.stats :list[Stat] = []

def displayPlot(filesStats :typing.List[PtpStats]):
    
    linecycler = cycle(["-","--","-.",":"])

    fig, axs = plt.subplots(3)
    fig.set_figheight(6)
    fig.set_figwidth(12)

    axs[0].set(xlabel='time (s)', ylabel='Offset (ns)')
    axs[2].set(xlabel='time (s)', ylabel='Path Delay (ns)')
    axs[1].set(xlabel='time (s)', ylabel='Frequency adj. (ppb)')

    for file in filesStats:
        time_arr    =[]
        offset_arr  =[]
        freq_arr    =[]
        delay_arr   =[]
        for stat in file.stats:
            time_arr    .append(stat.time)
            offset_arr  .append(stat.masterOffset)
            freq_arr    .append(stat.freq)
            delay_arr   .append(stat.pathDelay)
        linestyle = next(linecycler)
        axs[0].plot(time_arr, offset_arr,   label = file.label, linestyle=linestyle)
        axs[1].plot(time_arr, freq_arr,     label = file.label, linestyle=linestyle)
        axs[2].plot(time_arr, delay_arr,    label = file.label, linestyle=linestyle)


    # Hide x labels and tick labels for top plots and y ticks for right plots.
    for ax in axs.flat:
        ax.legend()
        ax.label_outer()

    plt.show()


def setup_parser():
    parser = argparse.ArgumentParser(description='PTP plotter.')
    parser.add_argument('-f', "--file", default=[], type=str, action='append',
                        help='ptp4l logfile. (multiple files can be added).')
    parser.add_argument('-l', "--label", default=[], type=str, action='append',
                        help='Label for the corresponind log file. (multiple files can be added, \
                            must be specified in order)')
    parser.add_argument('-t', "--time", default=0, type=int, metavar='s',
                        help='Max time to plot in seconds')
    return parser


def main():
    parser = setup_parser()
    args = parser.parse_args()


    filesStats :list[PtpStats] = []

    print(args.time)
    for fileName, label in zip(args.file, args.label):
         
        runStats = PtpStats(fileName, label)
        filesStats.append(runStats)
        
        file = open(fileName, 'r')
        lines = file.readlines()
        file.close()
        startTime = 0.0

        for line in lines:
            if not 'master offset' in line:
                continue
            # Regex search
            res = re.search(pattern, line)# if pattern was matched
            if res:
                # Capture result
                kernelTime   = float(res.group(1))
                masterOffset = int(res.group(2))
                state        = int(res.group(3))
                freq         = int(res.group(4))
                pathDelay    = int(res.group(5))    


                # state 1 is stepping
                # state 2 is servo control
                if (state == 2) :

                    if startTime == 0.0:
                        startTime = kernelTime #store first time

                    if (float(kernelTime)-startTime) < args.time if args.time > 0 else True :

                        stat = Stat(kernelTime - startTime, masterOffset, state, freq, pathDelay)
                        runStats.stats.append(stat)

            # if issue in patter
            else:
                print("Regex error:", line)
                sys.exit()

    displayPlot(filesStats)




if __name__ == '__main__':
    main()