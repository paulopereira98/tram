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

#include <avtp.h>
#include <avtp_aaf.h>

#include "avb_aaf.h"
#include "common.h"

#define NSEC_PER_SEC		1000000000ULL
#define NSEC_PER_MSEC		1000000ULL

#define BUFFER_T STAILQ_HEAD(sample_queue, sample_entry)

struct sample_entry {
	STAILQ_ENTRY(sample_entry) entries;

	struct timespec tspec;
	uint8_t *pcm_sample;
};



int bit_depth_to_aaf(int bit_depth)
{
	switch(bit_depth){
		case 16:
			return AVTP_AAF_FORMAT_INT_16BIT;
		case 24:
			return AVTP_AAF_FORMAT_INT_24BIT;
	}

	return AVTP_AAF_FORMAT_INT_16BIT;
}


int sample_rate_to_aaf(int sample_rate)
{
	switch(sample_rate){
		case 44100:
			return AVTP_AAF_PCM_NSR_44_1KHZ;
		case 48000:
			return AVTP_AAF_PCM_NSR_48KHZ;
		case 96000:
			return AVTP_AAF_PCM_NSR_96KHZ;
		case 192000:
			return AVTP_AAF_PCM_NSR_192KHZ;
	}

	return AVTP_AAF_PCM_NSR_48KHZ;
}


static int init_pdu(struct avtp_stream_pdu *pdu, stream_settings_t set)
{
	int res;

	res = avtp_aaf_pdu_init(pdu);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_TV, 1);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_STREAM_ID, set.stream_id);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_FORMAT, set.aaf_bit_depth);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_NSR,	set.aaf_sample_rate);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_CHAN_PER_FRAME, set.channels);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_BIT_DEPTH, set.bit_depth);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_STREAM_DATA_LEN, set.data_len);
	if (res < 0)
		return -1;

	res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_SP, AVTP_AAF_PCM_SP_NORMAL);
	if (res < 0)
		return -1;

	return 0;
}


int talker(char ifname[16], uint8_t macaddr[6], int priority, int max_transit_time, stream_settings_t set)
{
	int fd, res;
	struct sockaddr_ll sk_addr;
	uint8_t seq_num = 0;

	struct avtp_stream_pdu *pdu = alloca(set.pdu_size);

	printf("AAF talker node\n");

	fd = create_talker_socket(priority);
	if (fd < 0)
		return 1;

	res = setup_socket_address(fd, ifname, macaddr, ETH_P_TSN, &sk_addr);
	if (res < 0)
		goto err;

	res = init_pdu(pdu, set);
	if (res < 0)
		goto err;

	while (1) {
		ssize_t n;
		uint32_t avtp_time;

		memset(pdu->avtp_payload, 0, set.data_len);

		n = read(STDIN_FILENO, pdu->avtp_payload, set.data_len);
		if (n == 0)
			break;

		if (n != set.data_len) {
			fprintf(stderr, "read %zd bytes, expected %d\n",
								n, set.data_len);
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

		// send packet to network
		n = sendto(fd, pdu, set.pdu_size, 0,
				(struct sockaddr *) &sk_addr, sizeof(sk_addr));
		if (n < 0) {
			perror("Failed to send data");
			goto err;
		}

		if (n != set.pdu_size) {
			fprintf(stderr, "wrote %zd bytes, expected %zd\n",
								n, set.pdu_size);
		}
	}

	close(fd);
	return 0;

err:
	close(fd);
	return 1;
}


/* Schedule 'pcm_sample' to be presented at time specified by 'tspec'. */
static int schedule_sample(int fd, struct timespec *tspec, uint8_t *pcm_sample, BUFFER_T *samples, int data_len)
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

	STAILQ_INSERT_TAIL(samples, entry, entries);

	/* If this was the first entry inserted onto the queue, we need to arm
	 * the timer.
	 */
	if (STAILQ_FIRST(samples) == entry) {
		int res;

		res = arm_timer(fd, tspec);
		if (res < 0) {
			STAILQ_REMOVE(samples, entry, sample_entry, entries);
			free(entry);
			return -1;
		}
	}

	return 0;
}

static bool is_valid_packet(struct avtp_stream_pdu *pdu, stream_settings_t *set, int *expected_seq)
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
		fprintf(stderr, "Version mismatch: expected %u, got %u\n", 0, val32);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_TV, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get tv field: %d\n", res);
		return false;
	}
	if (val64 != 1) {
		fprintf(stderr, "tv mismatch: expected %u, got %" PRIu64 "\n", 1, val64);
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
	if (val64 != set->stream_id) {
		fprintf(stderr, "Stream ID mismatch: expected %" PRIu64 ", got %" PRIu64 "\n",
							set->stream_id, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_SEQ_NUM, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get sequence num field: %d\n", res);
		return false;
	}
	if (val64 != *expected_seq) {
		/* If we have a sequence number mismatch, we simply log the
		 * issue and continue to process the packet. We don't want to
		 * invalidate it since it is a valid packet after all.
		 */
		fprintf(stderr, "Sequence number mismatch: expected %u, got %" PRIu64 "\n",
							*expected_seq, val64);
		*expected_seq = val64;
	}
	*expected_seq++;

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_FORMAT, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get format field: %d\n", res);
		return false;
	}
	if (val64 != set->aaf_bit_depth) {
		fprintf(stderr, "Format mismatch: expected %u, got %" PRIu64 "\n",
					set->aaf_bit_depth, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_NSR, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get sample rate field: %d\n", res);
		return false;
	}
	if (val64 != set->aaf_sample_rate) {
		fprintf(stderr, "Sample rate mismatch: expected %u, got %" PRIu64 "\n",
						set->aaf_sample_rate, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_CHAN_PER_FRAME, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get channels field: %d\n", res);
		return false;
	}
	if (val64 != set->channels) {
		fprintf(stderr, "Channels mismatch: expected %u, got %" PRIu64 "\n",
							set->channels, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_BIT_DEPTH, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get depth field: %d\n", res);
		return false;
	}
	if (val64 != set->bit_depth) {
		fprintf(stderr, "Depth mismatch: expected %u, got %" PRIu64 "\n",
								set->bit_depth, val64);
		return false;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_STREAM_DATA_LEN, &val64);
	if (res < 0) {
		fprintf(stderr, "Failed to get data_len field: %d\n", res);
		return false;
	}
	if (val64 != set->data_len) {
		fprintf(stderr, "Data len mismatch: expected %u, got %" PRIu64 "\n",
							set->data_len, val64);
		return false;
	}

	return true;
}

static int new_packet(int sk_fd, int timer_fd, BUFFER_T *samples, int *expected_seq, stream_settings_t *set)
{
	int res;
	ssize_t n;
	uint64_t avtp_time;
	struct timespec tspec;
	struct avtp_stream_pdu *pdu = alloca(set->pdu_size);

	memset(pdu, 0, set->pdu_size);

	n = recv(sk_fd, pdu, set->pdu_size, 0);
	if (n < 0 || n != set->pdu_size) {
		perror("Failed to receive data");
		return -1;
	}

	if (!is_valid_packet(pdu, set, &expected_seq)) {
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

	res = schedule_sample(timer_fd, &tspec, pdu->avtp_payload, samples, set->data_len);
	if (res < 0)
		return -1;

	return 0;
}

static int timeout(int fd, int data_len, BUFFER_T *samples)
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

	entry = STAILQ_FIRST(samples);
	assert(entry != NULL);

	res = present_data(entry->pcm_sample, data_len);
	if (res < 0)
		return -1;

	STAILQ_REMOVE_HEAD(samples, entries);
    free(entry->pcm_sample);
	free(entry);

	if (!STAILQ_EMPTY(samples)) {
		entry = STAILQ_FIRST(samples);

		res = arm_timer(fd, &entry->tspec);
		if (res < 0)
			return -1;
	}

	return 0;
}

int listener(char ifname[16], stream_settings_t set)
{
	int sk_fd, timer_fd, res;
	struct pollfd fds[2];
	stream_settings_t settings; //will be populated on first new_packet() call
	uint8_t expected_seq = 0;

	BUFFER_T samples;
	STAILQ_INIT(&samples);


	sk_fd = create_listener_socket(ifname, 0, ETH_P_TSN);
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
			res = new_packet(sk_fd, timer_fd, &samples, &expected_seq, &settings);
			if (res < 0)
				goto err;
		}

		if (fds[1].revents & POLLIN) {
			res = timeout(timer_fd, settings.data_len, &samples);
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

