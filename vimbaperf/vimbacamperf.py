import threading
import sys
import cv2
from typing import Optional
from vimba import *
from vimba.c_binding import vimba_c
import argparse
import time
from datetime import datetime
import random
import string
import os
import math

import vimbastats as vs

USEC_IN_SEC = 1000000
NSEC_IN_SEC = 1000000000
NSEC_IN_MSEC = 1000000




class CamPerf:
    def __init__(self, cam:Camera, str_id:str):
        self.cam = cam
        self.cam_str_id = str_id

        self.stats = vs.Stats()
        self.frameHandler :Handler
        self.video_tmp_name :str = ''
        self.video :cv2.VideoWriter = 0





class Handler:
    def __init__(self, camname, stealth_mode :bool, stats :vs.Stats, video=0):
        self.shutdown_event = threading.Event()
        self.last = 0
        self.camname = camname
        self.stealth_mode = stealth_mode
        self.stats = stats
        self.video = video
        print('handler init')
        
    def __call__(self, cam: Camera, frame: Frame):        
        print('callback')
        if frame.get_status() == FrameStatus.Complete:
            
            local = time.time_ns()
            current = frame.get_timestamp()

            # ignore first frame
            # somme cameras send the first frame with the wrong timestamp
            if self.last==0:
                self.last = current
                if (not self.stealth_mode):
                    print('Cam     Frame        Cam timestamp      Local timestamp '+
                            'Transint Delta (ms)    FPS'
                            , flush=True)
                return

            self.stats.cam_stamps.append(current)
            self.stats.loc_stamps.append(local)

            if (not self.stealth_mode) or self.video:

                frame_cv2 = frame.as_numpy_ndarray()
                #frame.convert_pixel_format(PixelFormat.Bgr8)
                frame_cv2 = cv2.cvtColor(frame_cv2, cv2.COLOR_BAYER_RG2RGB)

                if not self.stealth_mode:
                    frame_id = frame.get_id()-1
                    delta = current-self.last if frame_id != 1 else 0

                    transit = (local - current)/NSEC_IN_MSEC

                    fps =  NSEC_IN_SEC/delta if delta else 0
                    print('{:7} {:5} {:20} {:20} {:8.2f} {:8.4f} {:8.04f}'.format(
                            self.camname, frame_id, current, local, transit, delta/NSEC_IN_MSEC, fps
                            ), flush=True)
                    self.last = current

                    msg = 'Stream from \'{}\''.format(self.camname)
                    cv2.namedWindow(msg, cv2.WINDOW_NORMAL)
                    cv2.imshow(msg, frame_cv2)

                    cv2.waitKey(1)
                    if cv2.getWindowProperty(msg, cv2.WND_PROP_VISIBLE) < 1:
                        self.shutdown_event.set()

                if self.video:
                    self.video.write(frame_cv2)


            cam.queue_frame(frame)