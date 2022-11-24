import threading
import typing
import cv2
from typing import Optional
from vimba import *
from vimba.c_binding import vimba_c
import time
import copy
import queue

import vimbastats as vs

USEC_IN_SEC = 1000000
NSEC_IN_SEC = 1000000000
NSEC_IN_MSEC = 1000000

FRAME_QUEUE_SIZE = 50



class CamPerf(threading.Thread):
    def __init__(self, cam:Camera, str_id:str):
        threading.Thread.__init__(self)

        self.cam = cam
        self.cam_str_id = str_id

        self.stats = vs.Stats()
        self.video_tmp_name :str = ''
        self.video :cv2.VideoWriter = 0



class FrameGrabber(threading.Thread):
    def __init__(self, camObj: CamPerf, frameQueue: queue.Queue, stealth :bool, time :int):
        threading.Thread.__init__(self)

        #self.cam = camObj.cam
        self.camObj = camObj
        self.frameQueue = frameQueue
        self.stealth = stealth
        self.trigger_stamp = 0
        self.time = time

        self.lastTime = 0

        self.shutdown_event = threading.Event()

    def __call__(self, cam: Camera, frame: Frame):
        # callback

        localTime = time.time_ns()
        currentTime = frame.get_timestamp()

        if frame.get_status() == FrameStatus.Complete:
            # ignore first frame
            # somme cameras send the first frame with the wrong timestamp
            if self.lastTime==0:
                self.lastTime = currentTime
                if (not self.stealth):
                    print('Cam     Frame        Cam timestamp      Local timestamp '+
                            'Transint Delta (ms)    FPS'
                            , flush=True)
                cam.queue_frame(frame)
                return

            #queue the frame
            if not self.frameQueue.full():
                frame_cpy = copy.deepcopy(frame)
                self.try_put_frame(frame_cpy)

            # store stats
            self.camObj.stats.append(localTime, currentTime)
            # print log
            if not self.stealth:
                frame_id = frame.get_id()-1
                delta = currentTime-self.lastTime if frame_id != 1 else 0

                transit = (localTime - currentTime)/NSEC_IN_MSEC

                fps =  NSEC_IN_SEC/delta if delta else 0
                print('{:7} {:5} {:20} {:20} {:8.2f} {:8.4f} {:8.04f}'.format(
                        self.camObj.cam_str_id, frame_id, currentTime, localTime, transit, 
                        delta/NSEC_IN_MSEC, fps), flush=True)
                self.last = currentTime

        cam.queue_frame(frame)

    def try_put_frame(self, frame: Optional[Frame]):
        try:
            self.frameQueue.put_nowait((self.camObj.cam_str_id, frame))

        except queue.Full:
            pass

    def stop(self):
        self.shutdown_event.set()


    def run(self):

        now = time.time_ns()
        delay_s = (self.trigger_stamp - now) / NSEC_IN_SEC if self.trigger_stamp else 0
        timeout = self.time + delay_s if self.time else None

        try:
            with self.camObj.cam as cam:
                try:
                    cam.PtpAcquisitionGateTime.set(self.trigger_stamp)
                    cam.start_streaming(handler=self, buffer_count=10)
                    self.shutdown_event.wait(timeout)

                finally:
                    cam.stop_streaming()
                    self.camObj.stats.cam_stats.read(cam)
                    vimba_c.call_vimba_c("VmbCaptureQueueFlush", cam._Camera__handle)
                    vimba_c.call_vimba_c("VmbFrameRevokeAll", cam._Camera__handle)
                    # restore trimmer mode
                    cam.PtpAcquisitionGateTime.set(0)

        except VimbaCameraError:
            pass

        finally:
            self.try_put_frame(None)


class FrameDisplayer(threading.Thread):
    def __init__(self, frame_queue: queue.Queue, camObjs :typing.Dict[str, float], stealth :bool):
        threading.Thread.__init__(self)

        self.log = Log.get_instance()
        self.frame_queue = frame_queue
        self.camObjs = camObjs
        self.stealth_mode = stealth

    def run(self):

        if not self.stealth_mode:
            for camObj in self.camObjs.values():
                cv2.namedWindow(camObj.cam_str_id, cv2.WINDOW_NORMAL)

        alive = True
        while alive:
            # Update current state by dequeuing all currently available frames.
            frames_left = self.frame_queue.qsize()
            while frames_left:

                try:
                    cam_str_id, frame = self.frame_queue.get_nowait()

                except queue.Empty:
                    break

                # Add/Remove frame from current state.
                if frame:
                    video = self.camObjs[cam_str_id].video
                    if (not self.stealth_mode) or video:
                        frame_cv2 = frame.as_numpy_ndarray()
                        #frame.convert_pixel_format(PixelFormat.Bgr8)
                        frame_cv2 = cv2.cvtColor(frame_cv2, cv2.COLOR_BAYER_RG2RGB)

                        if not self.stealth_mode:
                            if cv2.getWindowProperty(cam_str_id, cv2.WND_PROP_VISIBLE) < 1:
                                alive = False
                            cv2.imshow(cam_str_id, frame_cv2)

                            cv2.waitKey(10)
                        if video:
                            video.write(frame_cv2)

                else:
                    alive = False
                    if not self.stealth_mode:
                        cv2.destroyWindow(cam_str_id)

                frames_left -= 1
