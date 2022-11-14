/* Copyright (C) 2022 Paulo Pereira, Uminho <github.com/paulopereira98>
 * Copyright (C) 2004 Jeff Tranter
 *
 * https://www.linuxjournal.com/article/6735
 */

#ifndef _AVB_ALSA_H_
#define _AVB_ALSA_H_

#include <alsa/asoundlib.h>
#include "avb_aaf.h"

int avb_alsa_setup(snd_pcm_t **handle, char *device, stream_settings_t *set, bool is_recorder);

int avb_alsa_read(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t frames);

int avb_alsa_write(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t frames);

void avb_alsa_close(snd_pcm_t *handle);

#endif
