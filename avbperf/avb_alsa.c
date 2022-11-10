/* Copyright (C) 2022 Paulo Pereira, Uminho <github.com/paulopereira98>
 * Copyright (C) 2004 Jeff Tranter
 *
 * https://www.linuxjournal.com/article/6735
 */

#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#include "avb_aaf.h"

#define USEC_PER_SEC		1000000ULL
#define USEC_PER_MSEC		1000ULL

// Use the newer ALSA API
#define ALSA_PCM_NEW_HW_PARAMS_API


static uint bit_depth_to_alsa(uint bit_depth) 
{
  switch (bit_depth) {
  case 16:
    return SND_PCM_FORMAT_S16_LE;
  case 24:
    return SND_PCM_FORMAT_S24_LE;
  case 32:
    return SND_PCM_FORMAT_FLOAT_LE;
  }

  return SND_PCM_FORMAT_S16_LE;
}

/**
 * @brief configures the pcm device with the given settings
 *
 * @param pcm_handle handle for pcm device
 * @param device device name string
 * @param set stream settings structure
 * @return error code
 */
int avb_alsa_setup(snd_pcm_t **handle, char *device, stream_settings_t *set, bool is_recorder)
{
	int rc;
	snd_pcm_hw_params_t *params;

	snd_pcm_stream_t mode = SND_PCM_STREAM_PLAYBACK;
	if (is_recorder)
		mode = SND_PCM_STREAM_CAPTURE;


	// Open PCM device.
	rc = snd_pcm_open(handle, device, mode, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}


	// Allocate a hardware parameters object.
	snd_pcm_hw_params_alloca(&params);

	// Fill it in with default values.
	snd_pcm_hw_params_any(*handle, params);

	// Set the desired hardware parameters.

	// Interleaved mode
	snd_pcm_hw_params_set_access(*handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

	// Set bit depth
	snd_pcm_hw_params_set_format(*handle, params, bit_depth_to_alsa(set->bit_depth));

	// Set channel count
	snd_pcm_hw_params_set_channels(*handle, params, set->channels);

	// Set sample rate
	snd_pcm_hw_params_set_rate_near(*handle, params, &(set->sample_rate), NULL);


	// In ALSA, period (or fragment) is the ammount of frames transfered in a single operation

	// Set period size.
	snd_pcm_hw_params_set_period_size_near(*handle, params, &(set->hw_latency), NULL);

	// Write the parameters to the driver
	rc = snd_pcm_hw_params(*handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}

	// Get actual latency
	// Use a buffer large enough to hold one period
	snd_pcm_hw_params_get_period_size(params, &(set->hw_latency), NULL);

}

int avb_alsa_read(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t frames);
inline int avb_alsa_read(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t frames)
{
	int rc;
	rc = snd_pcm_readi(handle, buffer, frames); //read from sound card
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    }
}

int avb_alsa_write(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t frames);
inline int avb_alsa_write(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t frames)
{
	int rc;

	rc = snd_pcm_writei(handle, buffer, frames);

    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr, "error from writei: %s\n", snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
}

void avb_alsa_close(snd_pcm_t *handle)
{
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
}
