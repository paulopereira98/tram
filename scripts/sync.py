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

def thread_camera(indexes :list):

    cmd = '../vimbaperf/vimbaperf --stealth --frameRate=10 --time=15 --decimation=2 '
    for index in indexes:
        cmd += f'--index={index} '
    cmd += f'--outputFile=tmp/{AUX_VID_PREFIX}'
    print(cmd)
    os.system(cmd)



def thread_audio():

    cmd = f'sudo ../avbperf/avbperf -ienp4s0f1 -a default -t15 -ftmp/sync_mic'
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




def main(argv, argc):


    
    try:
        os.mkdir("tmp/")
    except FileExistsError:
        pass

    
    cam_p = Process(target=thread_camera, args=([0,1],))
    mic_p = Process(target=thread_audio)

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
        cmd += f'"./{vfile.filename}.mp4"'
        print(cmd)
        os.system(cmd)

        os.remove(vfile.path)

    os.remove(audioFile.path)
    os.rmdir("tmp/")



if __name__ == '__main__':
    main()