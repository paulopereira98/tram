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
#include <math.h>

#include <avtp.h>
#include <avtp_aaf.h>
#include "wav.h"


#include "common.h"
#include "avb_alsa.h"
#include "avb_aaf.h"
#include "avb_stats.h"

#define NSEC_PER_SEC		1000000000ULL
#define NSEC_PER_MSEC		1000000ULL

#define TMP_WAV_FILE	"/tmp/avb_perf.wav"

struct sample_entry {
	STAILQ_ENTRY(sample_entry) entries;

	struct timespec tspec;
	uint8_t *pcm_sample;
};


//STAILQ_HEAD(sample_queue, sample_entry)
struct sample_queue {
	struct sample_entry *stqh_first;
	struct sample_entry **stqh_last;
};

int bit_depth_to_aaf(int bit_depth)
{
	switch(bit_depth){
		case 16:
			return AVTP_AAF_FORMAT_INT_16BIT;
		case 24:
			return AVTP_AAF_FORMAT_INT_24BIT;
		case 32:
			return AVTP_AAF_FORMAT_FLOAT_32BIT;
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

uint64_t avb_aaf_hwlatency_to_ns(stream_settings_t *set)
{
	return ((float)set->hw_latency / set->sample_rate) * NSEC_PER_SEC;
}

static void print_settings(FILE *file, stream_settings_t set, bool is_talker)
{

	fprintf(file, "///////////////////////////////////////////////////////\n");
    fprintf(file, "///                    AVB perf                     ///\n");
	fprintf(file, is_talker ? \
				  "///                 AAF Talker Node                 ///\n" : \
				  "///                AAF Listener Node                ///\n");
    fprintf(file, "///////////////////////////////////////////////////////\n\n");

	fprintf(file, "Stream ID       : %16lX\n", 	set.stream_id	);
	fprintf(file, "Sample rate     : %d Hz\n", 	set.sample_rate	);
	fprintf(file, "Bit depth       : %d-bit\n", set.bit_depth	);
	fprintf(file, "Channels        : %d\n", 	set.channels	);
	fprintf(file, "HW latency      : %d samples\n", set.hw_latency);
	fprintf(file, "HW latency      : %.2f ms\n", avb_aaf_hwlatency_to_ns(&set) / (float)NSEC_PER_MSEC);
	fprintf(file, "Analog e2e lat. : %.2f ms\n\n", 
				2.0*(avb_aaf_hwlatency_to_ns(&set) / (float)NSEC_PER_MSEC) + set.max_transit_time);
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


int talker(char ifname[16], uint8_t macaddr[6], int priority, stream_settings_t set, char* adev)
{
	int fd, res;
	struct sockaddr_ll sk_addr;
	uint8_t seq_num = 0;

	snd_pcm_t *pcm_handle;

	struct avtp_stream_pdu *pdu;


	fd = create_talker_socket(priority);
	if (fd < 0)
		return 1;

	res = setup_socket_address(fd, ifname, macaddr, ETH_P_TSN, &sk_addr);
	if (res < 0)
		goto end;

	//setup audio device
	avb_alsa_setup(&pcm_handle, adev, &set, true);

	print_settings(stdout, set, true);
	
	pdu = alloca(set.pdu_size);

	res = init_pdu(pdu, set);
	if (res < 0)
		goto end;



	while (1) {
		ssize_t n;
		uint32_t avtp_time;

		// read from device
		avb_alsa_read(pcm_handle, pdu->avtp_payload, set.frames_per_pdu);

		res = calculate_avtp_time(&avtp_time, set.max_transit_time);
		if (res < 0) {
			fprintf(stderr, "Failed to calculate avtp time\n");
			goto end;
		}

		res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_TIMESTAMP,
								avtp_time);
		if (res < 0)
			goto end;

		res = avtp_aaf_pdu_set(pdu, AVTP_AAF_FIELD_SEQ_NUM, seq_num++);
		if (res < 0)
			goto end;

		// send packet to network
		n = sendto(fd, pdu, set.pdu_size, 0,
				(struct sockaddr *) &sk_addr, sizeof(sk_addr));
		if (n < 0) {
			perror("Failed to send data");
			goto end;
		}

		if (n != set.pdu_size) {
			fprintf(stderr, "wrote %zd bytes, expected %zd\n", n, (size_t)set.pdu_size);
		}
	}
	res = 0;

end:
	//close devices
	avb_alsa_close(pcm_handle);
	close(fd);
	return -res;
}


/* Schedule 'pcm_sample' to be presented at time specified by 'tspec'. */
static int schedule_sample(int fd, struct timespec *tspec, uint8_t *pcm_sample, 
									struct sample_queue *samples, int data_len)
{
	struct sample_entry *entry;
	int res = 0;

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

	// If this was the first entry inserted onto the queue, we need to arm the timer.
	if (STAILQ_FIRST(samples) == entry) {

		res = arm_timer(fd, tspec);
		if (res < 0) {
			STAILQ_REMOVE(samples, entry, sample_entry, entries);
			free(entry);
			return -1;
		}
		res = 1; //tell if buffer was empty
	}

	return res;
}

static bool is_valid_packet(struct avtp_stream_pdu *pdu, stream_settings_t *set, 
															uint8_t *expected_seq)
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
		fprintf(stderr, "\nSequence number mismatch: expected %u, got %" PRIu64 "\n",
							*expected_seq, val64);
		*expected_seq = val64;
	}
	(*expected_seq)++;

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

static int new_packet(snd_pcm_t *pcm_handle, int sk_fd, int timer_fd, struct sample_queue *samples, 
								  uint8_t *expected_seq, stream_settings_t *set, avb_stats_handler_t *stats)
{
	int res;
	ssize_t n;
	uint64_t avtp_time;
	struct timespec arrival, presentation;
	struct avtp_stream_pdu *pdu = alloca(set->pdu_size);
	uint8_t expected_seq_aux;

	res = clock_gettime(CLOCK_REALTIME, &arrival);
	if (res < 0) {
		perror("Failed to get time from PHC");
		return -1;
	}

	memset(pdu, 0, set->pdu_size);

	n = recv(sk_fd, pdu, set->pdu_size, 0);
	if (n < 0 || n != set->pdu_size) {
		perror("Failed to receive data");
		return -1;
	}

	expected_seq_aux = *expected_seq;
	if (!is_valid_packet(pdu, set, expected_seq)) {
		fprintf(stderr, "Dropping packet\n");
		return 0;
	}

	res = avtp_aaf_pdu_get(pdu, AVTP_AAF_FIELD_TIMESTAMP, &avtp_time);
	if (res < 0) {
		fprintf(stderr, "Failed to get AVTP time from PDU\n");
		return -1;
	}

	res = get_presentation_time(avtp_time, &arrival, &presentation);
	if (res == 1){
		// if the frame is late and the buffer is empty
		// discard it and wait for a better one to sync the hardware
		if (STAILQ_EMPTY(samples))
			return 1;
	}
	else if (res < 0)
		return -1;
	
	if (++expected_seq_aux != *expected_seq){
		avb_stats_add_dropped(stats, *expected_seq - expected_seq_aux);
	}

	res = avb_stats_push(stats, &arrival, &presentation);
	if (res < 0) {
		fprintf(stderr, "Failed to push stats\n");
		return -1;
	}

	// schedule_sample will only schedule the frame is the buffer is empty
	res = schedule_sample(timer_fd, &presentation, pdu->avtp_payload, samples, set->data_len);
	if (res < 0)
		return -1;

	return 0;
}

static void play_fragment(snd_pcm_t *pcm_handle, struct sample_queue *samples, uint32_t frames, WavFile *fp);
static inline void play_fragment(snd_pcm_t *pcm_handle, struct sample_queue *samples, uint32_t frames, WavFile *fp)
{
	struct sample_entry *entry;

	entry = STAILQ_FIRST(samples);
	assert(entry != NULL);

	// write to sound card
	// AAF sample = ALSA frame
	avb_alsa_write(pcm_handle, entry->pcm_sample, frames);
	if (fp) // write to wav file
		wav_write(fp, entry->pcm_sample, frames);

	STAILQ_REMOVE_HEAD(samples, entries);
    free(entry->pcm_sample);
	free(entry);

}

int timeout(int fd, snd_pcm_t *pcm_handle, int data_len, struct sample_queue *samples)
{
	ssize_t n;
	uint64_t expirations;
	//struct sample_entry *entry;

	n = read(fd, &expirations, sizeof(uint64_t));
	if (n < 0) {
		perror("Failed to read timerfd");
		return -1;
	}
	assert(expirations == 1);

	return 0;
}

static int setup_session_timer(int timer_fd, struct timespec *timeout_tspec, int rec_time)
{
	int res;

	res = clock_gettime(CLOCK_REALTIME, timeout_tspec);
	if (res < 0) {
		perror("Failed to get time from PHC");
		return -1;
	}
	timeout_tspec->tv_sec += rec_time;
	
	res = arm_timer(timer_fd, timeout_tspec);
	if (res < 0) {
		perror("Failed to arm session timer");
		return -1;
	}
	return 0;
}

static int setup_wav_recorder(WavFile **fp, stream_settings_t set)
{
    *fp = wav_open(TMP_WAV_FILE, WAV_OPEN_WRITE);
	if (*fp == NULL)
		return -1;
	wav_set_format(*fp, (set.bit_depth==32)? WAV_FORMAT_IEEE_FLOAT : WAV_FORMAT_PCM );

    wav_set_sample_size(*fp, ceil(set.bit_depth/8.0));

    wav_set_num_channels(*fp, set.channels);
    wav_set_sample_rate(*fp, set.sample_rate);

    return 0;
}

static int close_wav_recorder(WavFile *fp, struct timespec *start, char *filename)
{
	struct tm tm;
	// filename_timestamp.wav
	char *wav_filename = alloca(strlen(filename) + 31 + strlen(".wav") + 1 );
	char aux_str[21];

	wav_close(fp);

    gmtime_r(&(start->tv_sec), &tm);
	strftime(aux_str, 21, "%Y-%m-%dT%H:%M:%S.", &tm);
	sprintf(wav_filename, "%s_%s%09luZ.wav", filename, aux_str, start->tv_nsec);
	
	return rename(TMP_WAV_FILE, wav_filename);
}

int listener(char *ifname, stream_settings_t set, char *adev, int rec_time, char *filename)
{
	int sk_fd, timer_fd, session_tim_fd, res;
	struct pollfd fds[3];
	uint8_t macaddr[6] = {0,0,0,0,0,0};
	uint8_t expected_seq = 0;

	struct sample_queue samples;
	STAILQ_INIT(&samples);

	snd_pcm_t *pcm_handle;
	bool is_running = false;
	struct sample_entry *entry;

	WavFile *wav_fp = NULL;

	avb_stats_handler_t stats;
	avb_stats_handler_init(&stats);
	avb_stats_data_node_t start_stat_node;

	struct timespec start_tspec, timeout_tspec;

	sk_fd = create_listener_socket(ifname, macaddr, ETH_P_TSN);
	if (sk_fd < 0)
		return 1;

	timer_fd = timerfd_create(CLOCK_REALTIME, 0);
	if (timer_fd < 0) {
		close(sk_fd);
		return 1;
	}
	session_tim_fd = timerfd_create(CLOCK_REALTIME, 0);
	if (session_tim_fd < 0) {
		close(sk_fd);
		close(timer_fd);
		return 1;
	}
	
	// socket
	fds[0].fd = sk_fd;
	fds[0].events = POLLIN;
	// packet timer
	fds[1].fd = timer_fd;
	fds[1].events = POLLIN; //POLLPRI
	// session timer
	fds[2].fd = session_tim_fd;
	fds[2].events = POLLIN;
	
	//setup audio device
	avb_alsa_setup(&pcm_handle, adev, &set, false);

	print_settings(stdout, set, false);

	if (filename[0] != '\0'){
		setup_wav_recorder(&wav_fp, set);
		if (res < 0) {
			goto end;
		}
	}

	//setup session timer
	if (rec_time){
		res = setup_session_timer(session_tim_fd, &timeout_tspec, rec_time);
		if (res < 0) {
			perror("Failed to set session timer");
			goto end;
		}

	}


	// infinite loop
	while (1) {
		res = poll(fds, 3, -1);
		if (res < 0) {
			perror("Failed to poll() fds");
			goto end;
		}

		// Poll socket
		if (fds[0].revents & POLLIN) {
			// new_packet will schedule the frame is the buffer is empty
			res = new_packet(pcm_handle, sk_fd, timer_fd, &samples, &expected_seq, &set, &stats);
			if (res < 0)
				goto end;
		}

		if (is_running){
			// write to sound card
			play_fragment(pcm_handle, &samples, set.frames_per_pdu, wav_fp);

			if (STAILQ_EMPTY(&samples))
				is_running = false;
		}
		else if (fds[1].revents & POLLIN) { // Poll packet timer
			// first frame is scheduled for latency management
			// the following aren't because the playback is synced by hardware
			is_running = true;

			res = timeout(timer_fd, pcm_handle, set.data_len, &samples);
			if (res < 0)
				goto end;
			snd_pcm_prepare(pcm_handle);
			play_fragment(pcm_handle, &samples, set.frames_per_pdu, wav_fp);
			if (STAILQ_EMPTY(&samples))
				is_running = false;
		}

		// Poll session timer
		if (fds[2].revents & POLLIN) {
			// session timeout
			break;
		}

	}


	res = 0;

end:
	res = avb_stats_process_stats(&stats, &set);
	if (res == 0)
		avb_stats_print_stats(stdout, &stats);
	
	//close devices
	if (wav_fp){
		res = avb_stats_get_first(&stats, &start_stat_node);
		if (res == 0)
			avb_stats_get_cap_time(&start_stat_node, &set, &start_tspec);
		close_wav_recorder(wav_fp, &start_tspec, filename);
	}
	avb_stats_clear(&stats);
	avb_alsa_close(pcm_handle);
	close(sk_fd);
	close(timer_fd);
	close(session_tim_fd);
	return -res;
}

