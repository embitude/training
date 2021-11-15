#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/platform_data/edma.h>
#include <linux/i2c.h>
#include <linux/of_platform.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/mach-types.h>

#include <linux/edma.h>
#include <linux/of_gpio.h>

#define AUDIO_FORMAT (SND_SOC_DAIFMT_I2S | \
		SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_NB_NF)

static int evm_dummy_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params)
{
	printk("In %s\n", __func__);
	return 0;
}
static struct snd_soc_ops evm_dummy_ops = {
	.hw_params = evm_dummy_hw_params,
};

static int evm_dummy_init(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	printk("In %s\n", __func__);

	//TODO 3.1: Set the clock divider for cpu_dai. Use snd_soc_dai_set_clkdiv
	if (ret < 0)
		return ret;

	//TODO 3.2: Set the clock direction for cpu dai. Use snd_soc_dai_set_sysclk
	if (ret < 0)
		return ret;

	//TODO 3.3: Set the format for the cpu_dai 
	//Set CPU DAI as master, I2S mode and non-inverted frame sync & bit clock
	// Refer include/sound/soc-dai.h for available formats
	if (ret < 0)
		return ret;

	return 0;
}
//TODO 2.4: Populate the snd_soc_dai_link
// Populate the name, stream_name, codec_dai_name, ops, init and dai_fmt
// DAI format as I2S mode, SoC as master
static struct snd_soc_dai_link evm_dai_dummy = {
};

//TODO 2.5: Populate the fields for snd_soc_card
// .dai_link and num_links
static struct snd_soc_card evm_soc_card = {
	.owner = THIS_MODULE,
};

static int test_evm_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	u32 clk_gpio;
	int ret = 0;

	printk("In %s\n", __func__);

	//TODO 2.1: Get the audio-codec property and assign it to the codec_of_node
	// field of evm_dai_dummy
	// Use of_parse_phandle api
	if (!evm_dai_dummy.codec_of_node)
		return -EINVAL;

	//TODO 2.2: Get the controller property and assign it to the cpu_of_node
	// field of evm_dai_dummy
	if (!evm_dai_dummy.cpu_of_node)
		return -EINVAL;

	//TODO 2.3: Assign the cpu_of_node to platform_of_node field

	evm_soc_card.dev = &pdev->dev;
	//TODO 2.6: Get the card name with snd_soc_of_parse_card_name
	if (ret)
		return ret;

	//TODO 2.7: Register the card with snd_soc_register_card
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);

	return ret;
}

static int test_evm_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	//TODO 2.8: Unregister the card with snd_soc_register_card

	return 0;
}

//TODO 1.1: Populate the platform driver structure
// Set the compatible property to the string used in the dtb
static const struct of_device_id test_evm_dt_ids[] = {
	{
	},
	
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, test_evm_dt_ids);

//TODO 1.2: Populate the platform driver structure
// Populate .probe, .remove, & .of_match_table = of_match_ptr
static struct platform_driver test_evm_driver = {
	.driver		= {
		.name	= "test_evm",
		.owner	= THIS_MODULE,
	},
};

static int __init evm_init(void)
{
	//TODO 1.3: Register the platform driver
	return 0;
}

static void __exit evm_exit(void)
{
	//TODO 1.4: Unregister the platform driver
}

module_init(evm_init);
module_exit(evm_exit);

MODULE_DESCRIPTION("ASoC Test Machine Driver");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in");
MODULE_LICENSE("GPL");
