#!/usr/bin/env python3

from multiprocessing import Process
import os
from time import sleep
from dateutil import parser as timeparser
from datetime import datetime
import math
import argparse

NSEC_IN_SEC = 1000000000
AUX_VID_PREFIX = "sync"

def thread_camera(fps :float, dec: int, camIDs :list, indexes :list, time :int):

    cmd = f'../vimbaperf/vimbaperf --stealth --frameRate={fps} --time={time} --decimation={dec} '

    for id in camIDs:
        cmd += f'--cameraID={id} '

    for index in indexes:
        cmd += f'--index={index} '

    cmd += f'--outputFile=tmp/{AUX_VID_PREFIX}'
    print(cmd)
    os.system(cmd)



def thread_audio(interface :str, time:int):

    cmd = f'sudo ../avbperf/avbperf --ifname={interface} -a default --timeout={time} -ftmp/sync_mic'
    sleep(4)
    os.system(cmd)


class MediaFile:
    path :str
    filename :str
    t_stamp_str :str
    t_stamp :int
    offset :int

    def __init__(self, path):
        self.parse(path)

    def parse(self, path):
        self.path=path

        aux = path      # <path/>prefix_id_timestamp.ext
        aux = aux.rsplit('.', 1) #remove extension

        aux = aux[0]
        aux = aux.rsplit('_', 1) #extract timestamp
        self.t_stamp_str = aux[1]
        self.t_stamp = timeparser.parse(self.t_stamp_str).timestamp()

        
        # put id in self.filename
        aux = aux[0]
        aux = aux.rsplit('/', 1)
        self.filename = aux[len(aux)-1] #remove path
        self.filename = self.filename.replace(AUX_VID_PREFIX+'_', '')


def setup_parser():
    parser = argparse.ArgumentParser(description='Sync utility.')
    parser.add_argument('-f', "--frameRate", default=10, type=float, metavar='FPS', 
                        help='Target framerate (default 10 fps)')
    parser.add_argument('-d', "--decimation", default=2, type=int, metavar='FACTOR', 
                        help='On-camera image decimation factor. between 1 and 8 (default = 2)')
    parser.add_argument('-c', "--cameraID", default=[], type=str, metavar='ID', action='append',
                        help='Camera id. More than one can be used (ex: -c id0 -c id1)')
    parser.add_argument('-i', "--index", default=[], type=int, metavar='#', action='append',
                        help='Camera index (default 0). More than one can be used (ex: -i0 -i1)')
    parser.add_argument('-t', "--time", default=0, type=int, metavar='s',
                        help='Duration in time (default = 0 freerun)')
    parser.add_argument('-o', "--outputFile", default="", type=str, metavar='name',
                        help='save stream to video file (name_id.mp4)')
    parser.add_argument('-I', "--ifname", default="", type=str, metavar='ifname',
                        help='Audio AVB Network Interface')
    return parser


def main():
    parser = setup_parser()
    args = parser.parse_args()

    if args.ifname == '':
        print("Audio AVB network interface is required")
        exit(0)

    try:
        os.mkdir("tmp/")
    except FileExistsError:
        pass

    
    cam_p = Process(target=thread_camera, 
                    args=(args.frameRate, args.decimation, args.cameraID, args.index, args.time,))
    mic_p = Process(target=thread_audio, 
                    args=(args.ifname, int(args.time),))

    cam_p.start()
    mic_p.start()

    cam_p.join()
    mic_p.join()
    
    
    #sync files

    videoFiles :list = [] #MediaFile
    audioFile :MediaFile = 0
    for file in os.listdir("./tmp/"):
        if file.endswith(".mp4"):
            videoFiles.append( MediaFile('./tmp/'+file) )
            
    for file in os.listdir("./tmp/"):
        if file.endswith(".wav"):
            audioFile = MediaFile('./tmp/'+file)
            break
        
    

    # find who started last
    latest_start_time = audioFile.t_stamp
    for vfile in videoFiles:
        latest_start_time = max([ latest_start_time , vfile.t_stamp ])

    
    start_stamp_s = datetime.fromtimestamp(math.floor( latest_start_time ))
    start_stamp_ns = math.floor( (latest_start_time - math.floor(latest_start_time))*NSEC_IN_SEC )
    # ISO 8601 format
    latest_stamp = f'{start_stamp_s.isoformat()}.{str(start_stamp_ns) :0>9}Z'


    audio_offset = max([ 0 , latest_start_time - audioFile.t_stamp ])

    for vfile in videoFiles:

        video_offset = max([ 0 , latest_start_time-vfile.t_stamp ])
        cmd = f'ffmpeg '
        cmd += f'-ss {video_offset:.3f} -i "{vfile.path}" -ss {audio_offset:.3f} -i "{audioFile.path}" '

        cmd += f'-map 0:v:0 -map 1:a:0 -c:a aac -c:v copy -shortest '
        cmd += f'-metadata creation_time="{latest_stamp}" '

        if args.outputFile != '':
            vfile.filename = args.outputFile + '_' + vfile.filename
        cmd += f'"./{vfile.filename}.mp4"'

        print(cmd)
        os.system(cmd)

        os.remove(vfile.path)

    os.remove(audioFile.path)
    os.rmdir("tmp/")



if __name__ == '__main__':
    main()