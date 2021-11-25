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

//TODO 4.1: Add capture Widgets - Mic Jack, DMIC wiht SND_SOC_DAPM_MIC
static const struct snd_soc_dapm_widget test_dapm_capture_widgets[] = {
};

//TODO 4.2: Add playback widgets - Headphone Jack, Speaker_L and Speaker_R
// SND_SOC_DAPM_HP, SND_SOC_DAPM_SPK
static const struct snd_soc_dapm_widget test_dapm_playback_widgets[] = {
};

//TODO 4.3: Add DAPM route for Headphone Left and Headphone Right
static const struct snd_soc_dapm_route audio_map[] = {
};

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
	struct snd_soc_card *card = rtd->card;

	printk("In %s\n", __func__);
	// External Sys clock or Internal Sysclock - 24Mhz
	//Oscillator - 24.576 Mhz
	//TODO 3.1: Set the clock divider for cpu_dai. Use snd_soc_dai_set_clkdiv
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 1, 16);
	if (ret < 0)
		return ret;

	//TODO 3.2: Set the clock direction for cpu dai. Use snd_soc_dai_set_sysclk
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	//TODO 3.3: Set the format for the cpu_dai 
	//Set CPU DAI as master, I2S mode and non-inverted frame sync & bit clock
	// Refer include/sound/soc-dai.h for available formats
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |  SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;

	//TODO 4.4: Register playback widgets with snd_soc_dapm_new_controls
	
	//TODO 4.5:Register  capture widgets with snd_soc_dapm_new_controls

	//TODO 4.6:Register  routes with snd_soc_dapm_add_routes

        snd_soc_dapm_enable_pin(&card->dapm, "Headphone Jack");
        snd_soc_dapm_enable_pin(&card->dapm, "Mic Jack");
        snd_soc_dapm_enable_pin(&card->dapm, "Speaker_L");
        snd_soc_dapm_enable_pin(&card->dapm, "Speaker_R");
        snd_soc_dapm_enable_pin(&card->dapm, "DMIC"),
 	snd_soc_dapm_sync(&card->dapm);

	return 0;
}
//TODO 2.4: Populate the snd_soc_dai_link
// Populate the name, stream_name, codec_dai_name, ops, init and dai_fmt
// DAI format as I2S mode, SoC as master
static struct snd_soc_dai_link evm_dai_dummy = {
	.name = "testdai",
	.stream_name = "dummy stream",
	.codec_dai_name = "codec-hifi",
	//.codec_dai_name = "dummy-hifi",
	.ops = &evm_dummy_ops,
	.init = evm_dummy_init,
	.dai_fmt = SND_SOC_DAIFMT_I2S |  SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_NB_NF,
};

//TODO 2.5: Populate the fields for snd_soc_card
// .dai_link and num_links
static struct snd_soc_card evm_soc_card = {
	.owner = THIS_MODULE,
	.dai_link = &evm_dai_dummy,
	.num_links = 1,
};

static int test_evm_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	u32 clk_gpio;
	int ret = 0;

	printk("In %s\n", __func__);
	clk_gpio = of_get_named_gpio(np,
                        "mcasp_clock_enable", 0);
	if (clk_gpio < 0) {
		printk("Invalid gpio\n");
		return -EINVAL;
	}
	gpio_set_value(clk_gpio, 1);

	//TODO 2.1: Get the audio-codec property and assign it to the codec_of_node
	// field of evm_dai_dummy
	// Use of_parse_phandle api
	evm_dai_dummy.codec_of_node = of_parse_phandle(np, "ti,audio-codec", 0);
	if (!evm_dai_dummy.codec_of_node)
		return -EINVAL;

	//TODO 2.2: Get the controller property and assign it to the cpu_of_node
	// field of evm_dai_dummy
	evm_dai_dummy.cpu_of_node = of_parse_phandle(np, "ti,mcasp-controller", 0);
	if (!evm_dai_dummy.cpu_of_node)
		return -EINVAL;

	//TODO 2.3: Assign the cpu_of_node to platform_of_node field
	evm_dai_dummy.platform_of_node = evm_dai_dummy.cpu_of_node;

	evm_soc_card.dev = &pdev->dev;
	//TODO 2.6: Get the card name with snd_soc_of_parse_card_name
	ret = snd_soc_of_parse_card_name(&evm_soc_card, "ti,model");
	if (ret)
		return ret;

	//TODO 2.7: Register the card with snd_soc_register_card
	ret = snd_soc_register_card(&evm_soc_card);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);

	return ret;
}

static int test_evm_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	//TODO 2.8: Unregister the card with snd_soc_register_card
	return snd_soc_unregister_card(card);
}

//TODO 1.1: Populate the platform driver structure
// Set the compatible property to the string used in the dtb
static const struct of_device_id test_evm_dt_ids[] = {
	{
		.compatible = "test-evm-audio",
	},
	
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, test_evm_dt_ids);

//TODO 1.2: Populate the platform driver structure
// Populate .probe, .remove, & .of_match_table = of_match_ptr
static struct platform_driver test_evm_driver = {
	.probe = test_evm_probe,
	.remove = test_evm_remove,
	.driver		= {
		.name	= "test_evm",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(test_evm_dt_ids),
	},
};

static int __init evm_init(void)
{
	//TODO 1.3: Register the platform driver
	return platform_driver_register(&test_evm_driver);
}

static void __exit evm_exit(void)
{
	//TODO 1.4: Unregister the platform driver
	platform_driver_unregister(&test_evm_driver);
}

module_init(evm_init);
module_exit(evm_exit);

MODULE_DESCRIPTION("ASoC Test Machine Driver");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in");
MODULE_LICENSE("GPL");
