

from vimba import *
import statistics


USEC_IN_SEC = 1000000
NSEC_IN_SEC = 1000000000
NSEC_IN_MSEC = 1000000


class Stat:
	min :float
	max :float
	mean:float
	std :float


class CamStats:
	StatFrameRate           :float
	StatFrameDelivered      :int
	StatFrameDropped        :int
	StatFrameRescued        :int
	StatFrameShoved         :int
	StatFrameUnderrun       :int
	StatLocalRate           :float
	StatPacketErrors        :int
	StatPacketMissed        :int
	StatPacketReceived      :int
	StatPacketRequested     :int
	StatPacketResent        :int
	StatPacketUnavailable   :int
	StatTimeElapsed         :int


	def read(self, cam: Camera):
		with cam:
			self.StatFrameRate         = cam.StatFrameRate.get()
			self.StatFrameDelivered    = cam.StatFrameDelivered.get()
			self.StatFrameDropped      = cam.StatFrameDropped.get()
			self.StatFrameRescued      = cam.StatFrameRescued.get()
			self.StatFrameShoved       = cam.StatFrameShoved.get()
			self.StatFrameUnderrun     = cam.StatFrameUnderrun.get()
			self.StatLocalRate         = cam.StatLocalRate.get()
			self.StatPacketErrors      = cam.StatPacketErrors.get()
			self.StatPacketMissed      = cam.StatPacketMissed.get()
			self.StatPacketReceived    = cam.StatPacketReceived.get()
			self.StatPacketRequested   = cam.StatPacketRequested.get()
			self.StatPacketResent      = cam.StatPacketResent.get()
			self.StatPacketUnavailable = cam.StatPacketUnavailable.get()
			self.StatTimeElapsed       = cam.StatTimeElapsed.get()

	def print(self, file):
		print("\nCamera Statistics")
		print(f"    Stat Frame Rate         : {self.StatFrameRate      :.6f} fps", file=file)
		print(f"    Stat Frames Delivered   : {self.StatFrameDelivered     }", file=file)
		print(f"    Stat Frames Dropped     : {self.StatFrameDropped       }", file=file)
		print(f"    Stat Frames Rescued     : {self.StatFrameRescued       }", file=file)
		print(f"    Stat Frames Shoved      : {self.StatFrameShoved        }", file=file)
		print(f"    Stat Frames Underrun    : {self.StatFrameUnderrun      }", file=file)
		print(f"    Stat Local Rate         : {self.StatLocalRate      :.6f} fps", file=file)
		print(f"    Stat Packets Errors     : {self.StatPacketErrors       }", file=file)
		print(f"    Stat Packets Missed     : {self.StatPacketMissed       }", file=file)
		print(f"    Stat Packets Received   : {self.StatPacketReceived     }", file=file)
		print(f"    Stat Packets Requested  : {self.StatPacketRequested    }", file=file)
		print(f"    Stat Packets Resent     : {self.StatPacketResent       }", file=file)
		print(f"    Stat Packets Unavailable: {self.StatPacketUnavailable  }", file=file)
		print(f"    Stat Time Elapsed       : {self.StatTimeElapsed        }", file=file)

        

class Stats:
	def __init__(self):
		self.cam_stamps = []
		self.loc_stamps = []
		self.frame_count = 0
		# units in ms
		self.cam_delta = Stat()
		self.cam_fps = Stat()
		self.loc_delta = Stat()
		self.loc_fps = Stat()
		self.travel = Stat()

		self.cam_stats = CamStats()

	def append(self, locStamp, camStamp):
		self.cam_stamps.append(camStamp)
		self.loc_stamps.append(locStamp)

	def process(self):

		if len(self.cam_stamps) <= 2:
			return

		self.frame_count = len(self.cam_stamps)
		
		#cam Statistics
		deltas = [self.cam_stamps[n]-self.cam_stamps[n-1] for n in range(1,len(self.cam_stamps))]
		fps =  [NSEC_IN_SEC/deltas[n] for n in range(0,len(deltas))]

		self.cam_delta.min  = min(deltas)/NSEC_IN_MSEC
		self.cam_delta.max  = max(deltas)/NSEC_IN_MSEC
		self.cam_delta.mean = statistics.mean(deltas)/NSEC_IN_MSEC
		self.cam_delta.std  = statistics.stdev(deltas)/NSEC_IN_MSEC
		self.cam_fps.min    = min(fps)
		self.cam_fps.max    = max(fps)
		self.cam_fps.mean   = statistics.mean(fps)
		self.cam_fps.std    = statistics.stdev(fps)

		#loc Statistics
		deltas = [self.loc_stamps[n]-self.loc_stamps[n-1] for n in range(1,len(self.loc_stamps))]
		fps =  [NSEC_IN_SEC/deltas[n] for n in range(0,len(deltas))]

		self.loc_delta.min  = min(deltas)/NSEC_IN_MSEC
		self.loc_delta.max  = max(deltas)/NSEC_IN_MSEC
		self.loc_delta.mean = statistics.mean(deltas)/NSEC_IN_MSEC
		self.loc_delta.std  = statistics.stdev(deltas)/NSEC_IN_MSEC
		self.loc_fps.min    = min(fps)
		self.loc_fps.max    = max(fps)
		self.loc_fps.mean   = statistics.mean(fps)
		self.loc_fps.std    = statistics.stdev(fps)

		#travel Statistics
		travel_times = [self.loc_stamps[n]-self.cam_stamps[n] for n in range(0,len(self.cam_stamps))]

		self.travel.min    = min(travel_times)/NSEC_IN_MSEC
		self.travel.max    = max(travel_times)/NSEC_IN_MSEC
		self.travel.mean   = statistics.mean(travel_times)/NSEC_IN_MSEC
		self.travel.std    = statistics.stdev(travel_times)/NSEC_IN_MSEC
	

	def print(self, file):
		#file=sys.stdout

		self.cam_stats.print(file)

		print(f"\nFrame count : {self.frame_count}", file=file)

		if self.frame_count == 0:
			return
				
		print(f"\nCamera")
		print(f"    Delta min   : {self.cam_delta.min  :10.6f} ms", file=file)
		print(f"    Delta max   : {self.cam_delta.max  :10.6f} ms", file=file)
		print(f"    Delta mean  : {self.cam_delta.mean :10.6f} ms", file=file)
		print(f"    Delta stdev : {self.cam_delta.std  :10.6f} ms", file=file)
		print(f"    Fps min     : {self.cam_fps.min    :10.6f} fps", file=file)
		print(f"    Fps max     : {self.cam_fps.max    :10.6f} fps", file=file)
		print(f"    Fps mean    : {self.cam_fps.mean   :10.6f} fps", file=file)
		print(f"    Fps stdev   : {self.cam_fps.std    :10.6f} Hz", file=file)

		print("\nLocal")
		print(f"    Delta min   : {self.loc_delta.min  :10.6f} ms", file=file)
		print(f"    Delta max   : {self.loc_delta.max  :10.6f} ms", file=file)
		print(f"    Delta mean  : {self.loc_delta.mean :10.6f} ms", file=file)
		print(f"    Delta stdev : {self.loc_delta.std  :10.6f} ms", file=file)
		print(f"    Fps min     : {self.loc_fps.min    :10.6f} fps", file=file)
		print(f"    Fps max     : {self.loc_fps.max    :10.6f} fps", file=file)
		print(f"    Fps mean    : {self.loc_fps.mean   :10.6f} fps", file=file)
		print(f"    Fps stdev   : {self.loc_fps.std    :10.6f} Hz", file=file)

		print("\nTravel")
		print(f"    Travel min  : {self.travel.min  :8.3f} ms", file=file)
		print(f"    Travel max  : {self.travel.max  :8.3f} ms", file=file)
		print(f"    Travel mean : {self.travel.mean :8.3f} ms", file=file)
		print(f"    Travel stdev: {self.travel.std  :8.3f} ms", file=file)
