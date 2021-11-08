#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

//#define DEBUG

#ifdef DEBUG
static snd_output_t *output = NULL;
#endif

int main(int argc, char *argv[])
{
	int i;
	int err;
	short buf[128];
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;
	unsigned int val = 44100;
#if 0
	unsigned int buffer_time = 500000;       /* ring buffer length in us */
	unsigned int period_time = 125000;       /* period time in us */
#endif

	if (argc < 2) {
		printf("Usage: %s <device>\n", argv[0]);
		return -1;
	}

	/* Open PCM device for playback. */
	if ((err = snd_pcm_open(&playback_handle, argv[1], SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n",  argv[1],
				snd_strerror (err));
		exit (1);
	}

	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&hw_params);

	/* Fill it in with default values. */
	if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Interleaved mode */
	if ((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Signed 16-bit little-endian format */
	if ((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* 44100 bits/second sampling rate (CD quality) */
	if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &val, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Two channels (stereo) */
	if ((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2)) < 0)
	{
		fprintf (stderr, "cannot set channel count (%s)\n",
				snd_strerror (err));
		exit (1);
	}
#if 0
    err = snd_pcm_hw_params_set_period_time_near(playback_handle, hw_params, &period_time, 0);
    if (err < 0) {
        printf("Unable to set period time %u for playback: %s\n", period_time, snd_strerror(err));
        return err;
    }
    err = snd_pcm_hw_params_set_buffer_time_near(playback_handle, hw_params, &buffer_time, 0);
    if (err < 0) {
        printf("Unable to set buffer time %u for playback: %s\n", buffer_time, snd_strerror(err));
        return err;
    }
#endif

	/* Write the parameters to the driver */
	if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
				snd_strerror (err));
		exit (1);
	}
	if ((err = snd_pcm_prepare(playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
				snd_strerror (err));
		exit (1);
	}

#ifdef DEBUG
	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		printf("Output failed: %s\n", snd_strerror(err));
		return 0;
	}
#endif

	for (i = 0; i < 10; ++i) {
		if ((err = snd_pcm_writei(playback_handle, buf, 128)) != 128) {
			fprintf (stderr, "write to audio interface failed (%s)\n",
					snd_strerror (err));
			exit (1);
		}
#ifdef DEBUG
		snd_pcm_dump(playback_handle, output);
#endif
	}
	printf("Playback done\n");

	snd_pcm_close(playback_handle);
	exit (0);
}
