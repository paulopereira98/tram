/* 
 * Copyright (c) 2022, Paulo Pereira, Uminho
 * Copyright (c) 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of Intel Corporation nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



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


struct sample_entry {
	STAILQ_ENTRY(sample_entry) entries;

	struct timespec tspec;
	uint8_t *pcm_sample;
};

static STAILQ_HEAD(sample_queue, sample_entry) samples;
static uint8_t expected_seq;



#define NSEC_PER_SEC		1000000000ULL
#define NSEC_PER_MSEC		1000000ULL

static char ifname[16];
static uint8_t macaddr[6];
static bool talker_mode = false;
static uint64_t stream_id= 0xAABBCCDDEEFF0001;//endianess

static int priority = 2;
static int max_transit_time = 100;
static int bit_depth = 16;
static int sample_rate = 48000;
static int channels = 2;

static int data_len;
static int pdu_size;


static struct argp_option options[] = {
	{"ifname"           , 'i', "IFNAME" , 0, "Network Interface", 1},
    {"stream-id"        , 's', "ID"     , 0, "AAF stream ID (default: AABBCCDDEEFF0001)", 2},
    {"talker-mode"      , 't', 0       	, 0, "Enable Talker mode. (Default: listener mode)", 3},


	{ 0 }
};
static struct argp_option options_talker[] = {
	{"dst-addr"         , 'd', "MACADDR", 0, "Stream Destination MAC address", 0},
	{"prio"             , 'p', "NUM"    , 0, "SO_PRIORITY to be set in socket (default: 2)", 0},
	{"max-transit-time" , 'm', "MSEC"   , 0, "Maximum Transit Time in ms (default: 100)", 1},

    {"bit-depth"        , 'b', "bits"   , 0, "Stream bit depth (default: 16)", 2},
    {"sample-rate"      , 'r', "Hz"     , 0, "Stream sample rate (default: 48000 Hz)", 3},
    {"channels"         , 'c', "NUM"    , 0, "Stream channel count (default: 2)", 4},
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
		data_len = bit_depth/8 * channels;
		pdu_size = sizeof(struct avtp_stream_pdu) + data_len;
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


static int init_pdu(struct avtp_stream_pdu *pdu)
{
	int res;

	printf("AAF talker: streamID: %d\n", talker_mode);

	res = avtp_aaf_pdu_init(pdu);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_TV, 1);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_STREAM_ID, stream_id);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_FORMAT,
						AVTP_AAF_FORMAT_INT_16BIT);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_NSR,
						AVTP_AAF_PCM_NSR_48KHZ);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_CHAN_PER_FRAME,
								channels);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_BIT_DEPTH, 16);//TODO
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_STREAM_DATA_LEN, data_len);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_SP, AVTP_AAF_PCM_SP_NORMAL);
	if (res < 0)
		return -1;

	return 0;
}

int talker()
{
	int fd, res;
	struct sockaddr_ll sk_addr;
	struct avtp_stream_pdu *pdu = alloca(pdu_size);
	uint8_t seq_num = 0;

	printf("AAF talker node\n");

	fd = create_talker_socket(priority);
	if (fd < 0)
		return 1;

	res = setup_socket_address(fd, ifname, macaddr, ETH_P_TSN, &sk_addr);
	if (res < 0)
		goto err;

	res = init_pdu(pdu);
	if (res < 0)
		goto err;

	while (1) {
		ssize_t n;
		uint32_t avtp_time;

		memset(pdu->avtp_payload, 0, data_len);

		n = read(STDIN_FILENO, pdu->avtp_payload, data_len);
		if (n == 0)
			break;

		if (n != data_len) {
			fprintf(stderr, "read %zd bytes, expected %d\n",
								n, data_len);
		}

		res = calculate_avtp_time(&avtp_time, max_transit_time);
		if (res < 0) {
			fprintf(stderr, "Failed to calculate avtp time\n");
			goto err;
		}

		res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_TIMESTAMP,
								avtp_time);
		if (res < 0)
			goto err;

		res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_SEQ_NUM, seq_num++);
		if (res < 0)
			goto err;

		n = sendto(fd, pdu, pdu_size, 0,
				(struct sockaddr *) &sk_addr, sizeof(sk_addr));
		if (n < 0) {
			perror("Failed to send data");
			goto err;
		}

		if (n != pdu_size) {
			fprintf(stderr, "wrote %zd bytes, expected %zd\n",
								n, pdu_size);
		}
	}

	close(fd);
	return 0;

err:
	close(fd);
	return 1;
}


/* Schedule 'pcm_sample' to be presented at time specified by 'tspec'. */
static int schedule_sample(int fd, struct timespec *tspec, uint8_t *pcm_sample)
{
	struct sample_entry *entry;

	entry = malloc(sizeof(*entry));
	if (!entry) {
		fprintf(stderr, "Failed to allocate memory\n");
		return -1;
	}
    entry->pcm_sample = malloc(data_len);

	entry->tspec.tv_sec = tspec->tv_sec;
	entry->tspec.tv_nsec = tspec->tv_nsec;
	memcpy(entry->pcm_sample, pcm_sample, data_len);

	STAILQ_INSERT_TAIL(&samples, entry, entries);

	/* If this was the first entry inserted onto the queue, we need to arm
	 * the timer.
	 */
	if (STAILQ_FIRST(&samples) == entry) {
		int res;

		res = arm_timer(fd, tspec);
		if (res < 0) {
			STAILQ_REMOVE(&samples, entry, sample_entry, entries);
			free(entry);
			return -1;
		}
	}

	return 0;
}

static bool is_valid_packet(struct avtp_stream_pdu *pdu)
{
	struct avtp_common_pdu *common = (struct avtp_common_pdu *) pdu;
	uint64_t val64;
	uint32_t val32;
	int res;

	res = avtp_pdu_get(common, AVTP_FIELD_SUBTYPE, &val32);
	if (res < 0) {
		fprintf(stderr, "Failed to get subtype field: %d\n", res);
		return false;
	}
	if (val32 != AVTP_SUBTYPE_AAF) {
		fprintf(stderr, "Subtype mismatch: expected %u, got %u\n",
						AVTP_SUBTYPE_AAF, val32);
		return false;
	}

	res = avtp_pdu_get(common, AVTP_FIELD_VERSION, &val32);
	if (res < 0) {
		fprintf(stderr, "Failed to get version field: %d\n", res);
		return false;
	}
	if (val32 != 0) {
		fprintf(stderr, "Version mismatch: expected %u, got %u\n",
								0, val32);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_TV, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get tv field: %d\n", res);
		return false;
	}
	if (val64 != 1) {
		fprintf(stderr, "tv mismatch: expected %u, got %" PRIu64 "\n",
								1, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_SP, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get sp field: %d\n", res);
		return false;
	}
	if (val64 != AVTP_AAF_PCM_SP_NORMAL) {
		fprintf(stderr, "sp mismatch: expected %u, got %" PRIu64 "\n",
						AVTP_AAF_PCM_SP_NORMAL, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_STREAM_ID, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get stream ID field: %d\n", res);
		return false;
	}
	if (val64 != stream_id) {
		fprintf(stderr, "Stream ID mismatch: expected %" PRIu64 ", got %" PRIu64 "\n",
							stream_id, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_SEQ_NUM, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get sequence num field: %d\n", res);
		return false;
	}

	if (val64 != expected_seq) {
		/* If we have a sequence number mismatch, we simply log the
		 * issue and continue to process the packet. We don't want to
		 * invalidate it since it is a valid packet after all.
		 */
		fprintf(stderr, "Sequence number mismatch: expected %u, got %" PRIu64 "\n",
							expected_seq, val64);
		expected_seq = val64;
	}

	expected_seq++;

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_FORMAT, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get format field: %d\n", res);
		return false;
	}
	if (val64 != AVTP_AAF_FORMAT_INT_16BIT) {
		fprintf(stderr, "Format mismatch: expected %u, got %" PRIu64 "\n",
					AVTP_AAF_FORMAT_INT_16BIT, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_NSR, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get sample rate field: %d\n", res);
		return false;
	}
	if (val64 != AVTP_AAF_PCM_NSR_48KHZ) {
		fprintf(stderr, "Sample rate mismatch: expected %u, got %" PRIu64 "\n",
						AVTP_AAF_PCM_NSR_48KHZ, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_CHAN_PER_FRAME, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get channels field: %d\n", res);
		return false;
	}
	if (val64 != channels) {
		fprintf(stderr, "Channels mismatch: expected %u, got %" PRIu64 "\n",
							channels, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_BIT_DEPTH, &val64);//TODO
	if (res < 0) {
		fprintf(stderr, "Failed to get depth field: %d\n", res);
		return false;
	}
	if (val64 != 16) {
		fprintf(stderr, "Depth mismatch: expected %u, got %" PRIu64 "\n",
								16, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_STREAM_DATA_LEN, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get data_len field: %d\n", res);
		return false;
	}
	if (val64 != data_len) {
		fprintf(stderr, "Data len mismatch: expected %u, got %" PRIu64 "\n",
							data_len, val64);
		return false;
	}

	return true;
}

static int new_packet(int sk_fd, int timer_fd)
{
	int res;
	ssize_t n;
	uint64_t avtp_time;
	struct timespec tspec;
	struct avtp_stream_pdu *pdu = alloca(pdu_size);

	memset(pdu, 0, pdu_size);

	n = recv(sk_fd, pdu, pdu_size, 0);
	if (n < 0 || n != pdu_size) {
		perror("Failed to receive data");
		return -1;
	}

	if (!is_valid_packet(pdu)) {
		fprintf(stderr, "Dropping packet\n");
		return 0;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_TIMESTAMP, &avtp_time);
	if (res < 0) {
		fprintf(stderr, "Failed to get AVTP time from PDU\n");
		return -1;
	}

	res = get_presentation_time(avtp_time, &tspec);
	if (res < 0)
		return -1;

	res = schedule_sample(timer_fd, &tspec, pdu->avtp_payload);
	if (res < 0)
		return -1;

	return 0;
}

static int timeout(int fd)
{
	int res;
	ssize_t n;
	uint64_t expirations;
	struct sample_entry *entry;

	n = read(fd, &expirations, sizeof(uint64_t));
	if (n < 0) {
		perror("Failed to read timerfd");
		return -1;
	}

	assert(expirations == 1);

	entry = STAILQ_FIRST(&samples);
	assert(entry != NULL);

	res = present_data(entry->pcm_sample, data_len);
	if (res < 0)
		return -1;

	STAILQ_REMOVE_HEAD(&samples, entries);
    free(entry->pcm_sample);
	free(entry);

	if (!STAILQ_EMPTY(&samples)) {
		entry = STAILQ_FIRST(&samples);

		res = arm_timer(fd, &entry->tspec);
		if (res < 0)
			return -1;
	}

	return 0;
}

int listener()
{
	int sk_fd, timer_fd, res;
	struct pollfd fds[2];


	STAILQ_INIT(&samples);

	sk_fd = create_listener_socket(ifname, macaddr, ETH_P_TSN);
	if (sk_fd < 0)
		return 1;

	timer_fd = timerfd_create(CLOCK_REALTIME, 0);
	if (timer_fd < 0) {
		close(sk_fd);
		return 1;
	}

	fds[0].fd = sk_fd;
	fds[0].events = POLLIN;
	fds[1].fd = timer_fd;
	fds[1].events = POLLIN;

	while (1) {
		res = poll(fds, 2, -1);
		if (res < 0) {
			perror("Failed to poll() fds");
			goto err;
		}

		if (fds[0].revents & POLLIN) {
			res = new_packet(sk_fd, timer_fd);
			if (res < 0)
				goto err;
		}

		if (fds[1].revents & POLLIN) {
			res = timeout(timer_fd);
			if (res < 0)
				goto err;
		}
	}

	return 0;

err:
	close(sk_fd);
	close(timer_fd);
	return 1;
}



int main(int argc, char *argv[])
{


	argp_parse(&argp, argc, argv, 0, NULL, NULL);
    if(talker_mode)
        return talker();
    else
        return listener();

}
