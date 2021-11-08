#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

//#define DEBUG

#ifdef DEBUG
static snd_output_t *output = NULL;
#endif

int main (int argc, char *argv[])
{
	int i;
	int err;
	short buf[128];
	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	unsigned int val = 44100;

	if (argc < 2) {
		printf("Usage: %s <device>\n", argv[0]);
		return -1;
	}

	/* Open PCM device for playback. */
	if ((err = snd_pcm_open(&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
				argv[1],
				snd_strerror(err));
		exit (1);
	}
	snd_pcm_hw_params_alloca(&hw_params);

	/* Fill it in with default values. */
	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Interleaved mode */
	if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Signed 16-bit little-endian format */
	if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* 44100 bits/second sampling rate (CD quality) */
	if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &val, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Two channels (stereo) */
	if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 2)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	/* Write the parameters to the driver */
	if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
				snd_strerror (err));
		exit (1);
	}

	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
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
		if ((err = snd_pcm_readi(capture_handle, buf, 128)) != 128) {
			fprintf (stderr, "read from audio interface failed (%s)\n",
					snd_strerror (err));
			exit (1);
		}
#ifdef DEBUG
		snd_pcm_dump(capture_handle, output);
#endif
	}

	snd_pcm_close (capture_handle);
	exit (0);
}
