
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

#include "avtp.h"
#include "avtp_aaf.h"
#include "common.h"

#include "avb_aaf.h"


static char ifname[16];
static uint8_t macaddr[6];
static bool talker_mode = false;
static uint64_t stream_id= 0xAABBCCDDEEFF0001;//endianess

static int priority = 2;
static int max_transit_time = 100;
static int bit_depth = 16;
static int sample_rate = 48000;
static int channels = 2;


static struct argp_option options[] = {
	{"ifname"           , 'i', "IFNAME" , 0, "Network Interface", 1},
    {"stream-id"        , 's', "ID"     , 0, "AAF stream ID (default: AABBCCDDEEFF0001)", 2},
    {"bit-depth"        , 'b', "bits"   , 0, "Stream bit depth := {16 | 24} (default: 16)", 2},
    {"sample-rate"      , 'r', "Hz"     , 0, "Stream sample rate := {44100 | 48000 | 96000 | 192000} (default: 48000 Hz)", 3},
    {"channels"         , 'c', "NUM"    , 0, "Stream channel count (default: 2)", 4},
    {"talker-mode"      , 't', 0       	, 0, "Enable Talker mode. (Default: listener mode)", 3},
	{ 0 }
};
static struct argp_option options_talker[] = {
	{"dst-addr"         , 'd', "MACADDR", 0, "Stream Destination MAC address", 0},
	{"prio"             , 'p', "NUM"    , 0, "SO_PRIORITY to be set in socket (default: 2)", 0},
	{"max-transit-time" , 'm', "MSEC"   , 0, "Maximum Transit Time in ms (default: 100)", 1},
	{ 0 }
};


static error_t parser(int key, char *arg, struct argp_state *state)
{
	static bool got_mac = 0, got_iface = 0;
	int res;

	switch (key) {
	case 'd':
		res = sscanf(arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
					&macaddr[0], &macaddr[1], &macaddr[2],
					&macaddr[3], &macaddr[4], &macaddr[5]);
		if (res != 6) {
			fprintf(stderr, "Invalid address\n");
			exit(EXIT_FAILURE);
		}
		got_mac = true;
		break;

	case 'i':
		strncpy(ifname, arg, sizeof(ifname) - 1);
		got_iface = true;
		break;
	
	case 't':
		talker_mode = true;
		break;

    case 's':
        res = sscanf(arg, "%16lx", &stream_id);
		break;

	case 'p':
		priority = atoi(arg);
        if (priority < 0 || priority >= 7) {
			fprintf(stderr, "Invalid priority\n");
			exit(EXIT_FAILURE);
		}
		break;

	case 'm':
		max_transit_time = atoi(arg);
		break;
    
    case 'b':
		bit_depth = atoi(arg);
        if (bit_depth != 16 && bit_depth != 24 && bit_depth != 32) {
			fprintf(stderr, "Invalid bit depth\n");
			exit(EXIT_FAILURE);
		}
		break;
    case 'r':
		sample_rate = atoi(arg);
		break;
    case 'c':
		channels = atoi(arg);
		break;
	case ARGP_KEY_END:
		if(talker_mode && !got_mac){
			fprintf(stderr, "Mac address is mandatory\n");
			exit(EXIT_FAILURE);
		}
		if(!got_iface){
			fprintf(stderr, "Network interface is mandatory\n");
			exit(EXIT_FAILURE);
		}
		break;
	}
	return 0;
}


static struct argp argp_talker = { options_talker, parser};

static struct argp_child argp_childs[] = {
	{&argp_talker	, 0, "Takler options:", 5},
	{0}
};
static struct argp argp = { options, parser, 0, 0, argp_childs };


int main(int argc, char *argv[])
{

	argp_parse(&argp, argc, argv, 0, NULL, NULL);

	int data_len = bit_depth/8 * channels;
	int pdu_size = sizeof(struct avtp_stream_pdu) + data_len;

	stream_settings_t settings = {
		.stream_id = stream_id,
		.sample_rate = sample_rate,
		.bit_depth = bit_depth,
		.channels = channels,
		.data_len = data_len,
		.pdu_size = pdu_size,
		.aaf_sample_rate = sample_rate_to_aaf(sample_rate),
		.aaf_bit_depth = bit_depth_to_aaf(bit_depth)
	};

    if(talker_mode)
        return talker(ifname, macaddr, priority, max_transit_time, settings);
    else
        return listener(ifname, settings);

}

//TODO: add frames per packet
