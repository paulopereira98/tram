/* Copyright (C) 2022 Paulo Pereira, Uminho <github.com/paulopereira98>
 * Copyright (C) 2004 Jeff Tranter
 *
 * https://www.linuxjournal.com/article/6735
 */

#ifndef _AVB_ALSA_H_
#define _AVB_ALSA_H_

#include <alsa/asoundlib.h>


int avb_alsa_setup(snd_pcm_t **handle, char *device, uint sample_rate, uint bit_depth, 
						uint channels, void *buffer, snd_pcm_uframes_t *latency, bool is_recorder);

int avb_alsa_read(snd_pcm_t *handle, void *buffer, uint data_len, snd_pcm_uframes_t *frames);

int avb_alsa_write(snd_pcm_t *handle, void *buffer, uint data_len, snd_pcm_uframes_t *frames);

void avb_alsa_close(snd_pcm_t *handle, void *buffer);

#endif
