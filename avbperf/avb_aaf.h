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

#ifndef _AVB_AAF_H_
#define _AVB_AAF_H_

#include <stdint.h>

typedef struct {
    uint64_t stream_id;
	uint32_t sample_rate;
    uint32_t bit_depth;
    uint32_t channels;
    uint32_t data_len;
    uint32_t pdu_size;
    uint32_t aaf_sample_rate;
    uint32_t aaf_bit_depth;
    uint32_t hw_latency;
    uint64_t frames_per_pdu;
    uint32_t max_transit_time;
}stream_settings_t;

int bit_depth_to_aaf(int bit_depth);

int sample_rate_to_aaf(int sample_rate);

uint64_t avb_aaf_hwlatency_to_ns(stream_settings_t *set);

int avb_aaf_talker(FILE *term_out, FILE *term_in, char ifname[16], uint8_t macaddr[6], 
										int priority, stream_settings_t set, char* adev);
int avb_aaf_listener(FILE * term_out, FILE* term_in, char *ifname, stream_settings_t set, 
													char *adev, int rec_time, char *filename);

#endif
