
#include <assert.h>
#include <argp.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>

#include "avtp.h"
#include "avtp_aaf.h"
#include "common.h"

#include "avb_aaf.h"


static char ifname[16];
static bool talker_mode = false;
static uint64_t stream_id= 0xAABBCCDDEEFF0001;
static int bit_depth = 16;
static int sample_rate = 48000;
static int channels = 2;
static char device[32] = "default";
static int hw_latency = 128;

static uint8_t macaddr[6];
static int priority = 2;
static int max_transit_time = 100;

static int record_time = 0;
static char record_file[64] = "";


static struct argp_option options[] = {
	{"ifname"			, 'i', "IFNAME"	, 0, "Network Interface", 1},
    {"stream-id"    	, 's', "ID"     , 0, "AAF stream ID (default: AABBCCDDEEFF0001)", 2},
    {"bit-depth"    	, 'b', "bits"   , 0, "Bit depth := {16 | 24} (default: 16)", 3},
    {"sample-rate"  	, 'r', "Hz"     , 0, "Sample rate := {44100 | 48000 | 96000 | 192000} (default: 48000 Hz)", 4},
    {"channels"     	, 'c', "NUM"    , 0, "Channel count (default: 2)", 5},
	{"audio-device"		, 'a', "PCM"    , 0, "Audio device PCM name (default: default)", 6},
	{"hw-latency"		, 'l', "frames" , 0, "Hardware latency in frames. must be base 2 (default:128)", 7},
    {"talker"  			, 9	 , 0       	, 0, "Enable Talker mode. (Default: listener mode)", 9},
	{ 0 }
};
static struct argp_option options_talker[] = {
	{"dst-addr"         , 'd', "MACADDR", 0, "Destination MAC address", 11},
	{"prio"             , 'p', "NUM"    , 0, "SO_PRIORITY to be set in socket (default: 2)", 12},
	{"max-transit-time" , 'm', "MSEC"   , 0, "Maximum Transit Time in ms. (default: 100)", 13},
	{ 0 }
};
static struct argp_option options_listener[] = {
	{"timeout"  		, 't', "s"      , 0, "Run only for n seconds (default = 0 freerun)", 21},
	{"file"  			, 'f', "name" 	, 0, "Record audio to file (./<name>.wav)", 22},
	{"transit-time" 	, 'T', "MSEC"   , 0, "Transit Time in ms, for stats only (must match talker)", 23},
	{ 0 }
};

static error_t parser(int key, char *arg, struct argp_state *state)
{
	static bool got_iface = 0;
	int res;

	switch (key) {

		case 'i':  // --ifname
			strncpy(ifname, arg, sizeof(ifname) - 1);
			got_iface = true;
			break;
		
		case 'a': // --audio-device
			strncpy(device, arg, sizeof(device) - 1);
			break;

		case 'l': // --hw-latency
			// latency will be set by the hw device to the nearest allowed value
			hw_latency = atoi(arg);
			break;
		
		case 9: // --talker
			talker_mode = true;
			break;

		case 's': // --stream-id
			res = sscanf(arg, "%16lx", &stream_id);
			break;
		
		case 'b': // --bit-depth
			bit_depth = atoi(arg);
			if (bit_depth != 16 && bit_depth != 24 && bit_depth != 32) {
				fprintf(stderr, "Invalid bit depth\n");
				exit(EXIT_FAILURE);
			}
			break;

		case 'r': // --sample-rate
			sample_rate = atoi(arg);
			break;

		case 'c': // --channels
			channels = atoi(arg);
			break;

		case ARGP_KEY_END:
			if(!got_iface){
				fprintf(stderr, "Network interface is mandatory\n");
				exit(EXIT_FAILURE);
			}
			break;
	}
	return 0;
}

static error_t parser_talker(int key, char *arg, struct argp_state *state)
{
	static bool got_mac = false;
	int res;

	switch (key) {

		case 'd': // --dst-addr
			res = sscanf(arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
						&macaddr[0], &macaddr[1], &macaddr[2],
						&macaddr[3], &macaddr[4], &macaddr[5]);
			if (res != 6) {
				fprintf(stderr, "Invalid address\n");
				exit(EXIT_FAILURE);
			}
			got_mac = true;
			break;

		case 'p': // --prio
			priority = atoi(arg);
			if (priority < 0 || priority >= 7) {
				fprintf(stderr, "Invalid priority\n");
				exit(EXIT_FAILURE);
			}
			break;

		case 'm': // --max-transit-time
			max_transit_time = atoi(arg);
			break;

		case ARGP_KEY_END:
			if(talker_mode && !got_mac){
				fprintf(stderr, "Mac address is mandatory\n");
				exit(EXIT_FAILURE);
			}
			break;
	}
	return 0;
}
static error_t parser_listener(int key, char *arg, struct argp_state *state)
{
	int res;

	switch (key) {
		case 'T': // --transit-time
			max_transit_time = atoi(arg);
			break;

		case 't': // --timeout
			record_time = atoi(arg);
			break;
			
		case 'f': // --file
			strcpy(record_file, arg);
			break;
		
		case ARGP_KEY_END:
			if(record_file[0]!='\0' && record_time == 0){
				fprintf(stderr, "Timeout is required when recording a WAV file.\n");
				exit(EXIT_FAILURE);
			}
			break;
	}
	return 0;
}

static struct argp argp_talker = { options_talker, parser_talker};
static struct argp argp_listener = { options_listener, parser_listener};

static struct argp_child argp_childs[] = {
	{&argp_talker	, 0, "Takler options:"	, 10},
	{&argp_listener	, 0, "Listener options:", 20},
	{0}
};
static struct argp argp = { options, parser, 0, 0, argp_childs };


int main(int argc, char *argv[])
{

	argp_parse(&argp, argc, argv, 0, NULL, NULL);

	//these values will be updated later once hw_latency has been confirmed by ALSA
	int data_len = ceil(bit_depth/8.0) * channels; 				// * frames_per_pdu 
	int pdu_size = sizeof(struct avtp_stream_pdu) + data_len; 	// 

	stream_settings_t settings = {
		.stream_id = stream_id,
		.sample_rate = sample_rate,
		.bit_depth = bit_depth,
		.channels = channels,
		.data_len = data_len, 			//this value will be updated later by avb_alsa_setup()
		.pdu_size = pdu_size, 			//this value will be updated later by avb_alsa_setup()
		.aaf_sample_rate = sample_rate_to_aaf(sample_rate),
		.aaf_bit_depth = bit_depth_to_aaf(bit_depth),
		.max_transit_time = max_transit_time,
		.hw_latency = hw_latency, 		//this value will be updated later by avb_alsa_setup()
		.frames_per_pdu = hw_latency	//this value will be updated later by avb_alsa_setup()
	};
	/* 
			For the purposes of this project, 
		the number of frames sent in a single packet (frames_per_pdu)
		will match the hardware buffer size/latency (hw_latency).
			A frame is a set of samples captured at the same instant 
		(i.e.: Left and Right audio channels)
	*/
    if(talker_mode)
        return avb_aaf_talker(stdout, stdin, ifname, macaddr, priority, settings, device);
    else
        return avb_aaf_listener(stdout, stdin, ifname, settings, device, record_time, record_file);

}
