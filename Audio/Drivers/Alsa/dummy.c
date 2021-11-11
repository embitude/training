#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/hrtimer.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/info.h>
#include <sound/initval.h>

#define MAX_PCM_DEVICES		4
#define MAX_PCM_SUBSTREAMS	128
#define MAX_MIDI_DEVICES	2

/* defaults */
#define MAX_BUFFER_SIZE		(64*1024)
#define MIN_PERIOD_SIZE		64
#define MAX_PERIOD_SIZE		MAX_BUFFER_SIZE
#define USE_FORMATS 		(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE)
#define USE_RATE		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000
#define USE_RATE_MIN		5500
#define USE_RATE_MAX		48000
#define USE_CHANNELS_MIN 	1
#define USE_CHANNELS_MAX 	2
#define USE_PERIODS_MIN 	1
#define USE_PERIODS_MAX 	1024

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */

#define SND_DUMMY	"dummy"

static struct platform_device *devices;

struct snd_dummy {
	struct snd_card *card;
	struct snd_pcm *pcm;
	struct snd_pcm_hardware pcm_hw;
};

/*
 * system timer interface
 */
struct dummy_systimer_pcm {
	spinlock_t lock;
	/* timer stuff */
	struct timer_list timer;
	unsigned int irq_pos;		/* fractional IRQ position */
	unsigned long last_jiffies;
	unsigned int rate;
	/* PCM parameters */
	unsigned int pcm_period_size;
	unsigned int pcm_bps;		/* bytes per second */
	unsigned int pcm_bpj;		/* bytes per jiffy */
	struct snd_pcm_substream *substream;
	unsigned int pcm_buffer_size;
	unsigned int buf_pos;	/* position in buffer */
	unsigned int running;
	unsigned int elapsed :1;
	/* added for waveform: */
	unsigned int wvf_pos;	/* position in waveform array */
	unsigned int wvf_lift;	/* lift of waveform array */
};

// waveform
static char wvfdat[] = {20, 22, 24, 25, 24, 22, 21,
			19, 17, 15, 14, 15, 17, 19,
			20, 127, 22, 19, 17, 15, 19};
static char wvfdat2[] = { 20, 22, 24, 25, 24, 22, 21,
			19, 17, 15, 14, 15, 17, 19,
			20, 127, 22, 19, 17, 15, 19};

static unsigned int wvfsz = sizeof(wvfdat);

static void dummy_systimer_rearm(struct dummy_systimer_pcm *dpcm)
{
	unsigned long tick;

	tick = dpcm->pcm_period_size - dpcm->irq_pos;
	tick /= dpcm->pcm_bpj;
	if (tick == 0) {
		tick = 2;
	};
	printk("## Tick = %lu\n", tick);
	//TODO 4.8: Start the timer. Use mod_timer
	
}

static void dummy_fill_capture_buf(struct dummy_systimer_pcm *dpcm, unsigned int bytes)
{
	char *dst = dpcm->substream->runtime->dma_area;
	int i = 0, mylift = 0, j = 0;

	if (dpcm->running) {
		for (j = 0; j < bytes; j++) {
			mylift = dpcm->wvf_lift * 10 - 10;

			for (i = 0; i < sizeof(wvfdat); i++) {
				wvfdat[i] = wvfdat2[i] + mylift;
			}
			//TODO 5.2: Copy the wvfdat to dst and increment the pointers accordingly

			// we should wrap waveform here..
			if (dpcm->wvf_pos >= wvfsz) {
				dpcm->wvf_pos = 0;
				// also handle lift here..
				dpcm->wvf_lift++;
				if (dpcm->wvf_lift >= 4) dpcm->wvf_lift = 0;
			}
		}
			//TODO 5.3: Check for buf_pos wrap and update it accordingly
	}
}

static void dummy_systimer_update(struct dummy_systimer_pcm *dpcm)
{
	unsigned long delta;
	unsigned int last_pos;

	if (!dpcm->running)
		return;

	delta = jiffies - dpcm->last_jiffies;
	if (!delta)
		return;

	dpcm->last_jiffies += delta;

	last_pos = dpcm->irq_pos;
	//TODO 4.9: Update the irq_pos as bytes per jiffies

	delta = dpcm->irq_pos - last_pos;
	//TODO 4.10: Update the buf_pos

	printk("buf_pos = %u, irq_pos = %u\n", dpcm->buf_pos, dpcm->irq_pos); 
	
	//TODO 4.11: Wrap the pointers if required and set the dpcm->elapsed if period time has elapsed


	//TODO 5.1 Fill the Capture buffer here.
	// Make sure to avoid updating buf_pos above as it would be updated while the buffer is filled
	// Also, this need to be done only for capture
}

static int dummy_systimer_start(struct snd_pcm_substream *substream)
{
	struct dummy_systimer_pcm *dpcm = substream->runtime->private_data;
	spin_lock(&dpcm->lock);
	dpcm->last_jiffies = jiffies;
	dummy_systimer_rearm(dpcm);
	spin_unlock(&dpcm->lock);
	return 0;
}

static int dummy_systimer_stop(struct snd_pcm_substream *substream)
{
	struct dummy_systimer_pcm *dpcm = substream->runtime->private_data;
	printk("Stopping\n");
	spin_lock(&dpcm->lock);
	//TODO 4.17: Delete the timer
	spin_unlock(&dpcm->lock);
	return 0;
}

static int dummy_systimer_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct dummy_systimer_pcm *dpcm = runtime->private_data;
	unsigned int bps, bpj;

	bps = runtime->rate * runtime->channels; 
	bps *= snd_pcm_format_width(runtime->format);
	bps /= 8;
	bpj = bps / HZ;

	printk("Preparing bps = %u, bpj = %u", bps, bpj);
	if (bps <= 0)
		return -EINVAL;

	dpcm->buf_pos = 0;
	//TODO 4.4: Initialize dpcm->pcm_buffer_size (buffer size in bytes)
	// Use frames_to_bytes to convert runtime buffer size (frames) to bytes

	dpcm->irq_pos = 0;
	dpcm->elapsed = 0;

	dpcm->pcm_bps = bps;
	dpcm->pcm_bpj = bpj;
	//TODO 4.5: Initialize dpcm->pcm_period_size (period size in bytes)
	// Use frames_to_bytes to convert runtime period size (frames) to bytes

	return 0;
}

static void dummy_systimer_callback(struct timer_list *t)
{
	struct dummy_systimer_pcm *dpcm = from_timer(dpcm, t, timer);
	unsigned long flags;
	int elapsed = 0;

	spin_lock_irqsave(&dpcm->lock, flags);
	dummy_systimer_update(dpcm);
	dummy_systimer_rearm(dpcm);
	elapsed = dpcm->elapsed;
	dpcm->elapsed = 0;
	spin_unlock_irqrestore(&dpcm->lock, flags);
	//TODO 4.12: Signal period elapsed if period time has been elapsed & stream is running
	// Use api snd_pcm_period_elapsed to signal the period elapsed
		
}

static snd_pcm_uframes_t
dummy_systimer_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	// TODO 4.14: Get the dummy_systimer_pcm from runtime private data
	// Also invoke dummy_systimer_update under spin_lock

	//TODO 4.15: Return the buf_pos in frames
	// Use bytes_to_frames for the conversion and return the same
	return 0;
}

static int dummy_systimer_create(struct snd_pcm_substream *substream)
{
	struct dummy_systimer_pcm *dpcm;

	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (!dpcm)
		return -ENOMEM;
	//TODO 4.2: Initialize runtime private data with dpcm
	//TODO 4.3: Create the timer & set up the callback handler for the same
	//Use timer_setup api for the same
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	printk(KERN_ERR "Timer created\n");

	return 0;
}

static void dummy_systimer_free(struct snd_pcm_substream *substream)
{
	kfree(substream->runtime->private_data);
}

/*
 * PCM interface
 */
static int dummy_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	//TODO 4.6: Get the dummy_systimer_pcm data structure from runtime private data

	printk("Driver: In %s\n", __func__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		printk("Trigger start received\n");
		//TODO 4.7: Start the timer, if 'running' field in dpcm is not set
		// and set the dpcm->running accordingly
		// Invoke dummy_systimer_start to start the timer and return the value returned by it
		return 0;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		printk("Trigger stop received\n");
		//TODO 4.16: Stop the timer (dummy_systimer_stop) & updated the 'running' field
		return 0;
	}
	return -EINVAL;
}

static int dummy_pcm_prepare(struct snd_pcm_substream *substream)
{
	printk("Driver: In %s\n", __func__);
	//TODO 4.2: Initialize the dpcm data structure for keeping track of pointers
	// This is done in dummy_systimer_prepare
	return 0;
}

static snd_pcm_uframes_t dummy_pcm_pointer(struct snd_pcm_substream *substream)
{
	//TODO 4.13: Invoke dummy_systimer_pointer and return the value returned by it
	return 0;
}

static const struct snd_pcm_hardware dummy_pcm_hardware = {
	.info =			(SNDRV_PCM_INFO_MMAP |
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_RESUME |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		USE_FORMATS,
	.rates =		USE_RATE,
	.rate_min =		USE_RATE_MIN,
	.rate_max =		USE_RATE_MAX,
	.channels_min =		USE_CHANNELS_MIN,
	.channels_max =		USE_CHANNELS_MAX,
	.buffer_bytes_max =	MAX_BUFFER_SIZE,
	.period_bytes_min =	MIN_PERIOD_SIZE,
	.period_bytes_max =	MAX_PERIOD_SIZE,
	.periods_min =		USE_PERIODS_MIN,
	.periods_max =		USE_PERIODS_MAX,
	.fifo_size =		0,
};

static int dummy_pcm_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *hw_params)
{
	printk("Driver: In %s\n", __func__);
	//TODO 3.8: Allocate the pages for buffer
    // Use snd_pcm_lib_malloc_pages and return the value returned by it
	return 0;
}

static int dummy_pcm_hw_free(struct snd_pcm_substream *substream)
{
	printk("Driver: In %s\n", __func__);
	//TODO 3.9: Free up the pages for buffer
	// Use snd_pcm_lib_free_pages and return the value returned by it
	return 0;
}

static int dummy_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_dummy *dummy = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	printk("Driver: In %s\n", __func__);

	//TODO 3.6: Assign dummy->pcm_hw to runtime->hw

	//TODO: 4.1 Create the systimer by invoking dpcm_systimer_creat
	if (err < 0)
		return err;


	return 0;
}

static int dummy_pcm_close(struct snd_pcm_substream *substream)
{
	printk("Driver: In %s\n", __func__);
	//TODO 4.18: Free up the systimer
	printk(KERN_ERR "Freeing\n");
	return 0;
}

//TODO 3.3: Populate the pcm operations
// Assign callback handlers for .open, .close, .hw_params, .hw_free, .prepare,
// .trigger, .pointer
static struct snd_pcm_ops dummy_pcm_ops = {
	.ioctl =	snd_pcm_lib_ioctl,
};

static int snd_card_dummy_pcm(struct snd_dummy *dummy, int device,
			      int substreams)
{
	struct snd_pcm *pcm;
	int err = -1;

	//TODO 3.2: Create the new pcm device
	if (err < 0)
		return err;
	dummy->pcm = pcm;
	//TODO 3.4: Register the pcm operations for the pcm device
	// To be done separately for capture & playback

	pcm->private_data = dummy;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Dummy PCM");

	//TODO 3.7: Pre-allocate the space for buffers
	if (err < 0)
		return err;

	return 0;
}

static int snd_dummy_probe(struct platform_device *devptr)
{
	struct snd_card *card;
	struct snd_dummy *dummy;
	int err = -1;

	printk("Driver: In %s\n", __func__);

	//TODO 2.1: Create the sound card instance
	// Alocate the space for struct snd_dummy as a part of this
	if (err < 0)
		return err;

	dummy = card->private_data;
	dummy->card = card;

	//TODO 2.2: Set the driver name, short name and long name for the card
	// card->drver, card->shortname and card->longname

	// TODO 3.1: Create the PCM device and register the ops
	// This is done in snd_card_dummy_pcm. Let's invoke the same
	// Let's use device as 0 and number of substreams to 1
	if (err < 0)
		goto __nodev;

	//TODO 3.5 : Set the PCM Hardware params. Assign the hw params to pcm_hw field

	//TODO 2.3: Register the sound card 
	if (err == 0) {
		platform_set_drvdata(devptr, card);
		return 0;
	}
      __nodev:
	snd_card_free(card);
	return err;
}

static int snd_dummy_remove(struct platform_device *devptr)
{
	printk("Driver: In %s\n", __func__);
	//TODO 2.4: Deregister the sound card 
	return 0;
}

#define SND_DUMMY_DRIVER	"snd_dummy"
// TODO 1.1: Populate the Platform Driver Data Structure
// Assign handlers to .probe and .remove and assign name to .driver.name
static struct platform_driver snd_dummy_driver = {
	.driver		= {
	},
};

static int __init alsa_card_dummy_init(void)
{
	int err = 0;

	// TODO 1.2: Register the platform driver
	if (err < 0)
		return err;

	// TODO 1.3: Register the platform device and assign it devices
	// Use platform_device_register_simple

	if (IS_ERR(devices))
		return -ENODEV;

	return 0;
}

static void __exit alsa_card_dummy_exit(void)
{
	// TODO 1.4: Deregister the platform device

	// TODO 1.5: Deregister the platform driver
}

module_init(alsa_card_dummy_init)
module_exit(alsa_card_dummy_exit)

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Dummy soundcard");
MODULE_LICENSE("GPL");

