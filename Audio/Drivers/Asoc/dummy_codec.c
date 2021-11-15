#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/soc.h>

static struct snd_soc_dai_driver dummy_dai = {
	.name = "dummy-hifi",
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			   SNDRV_PCM_FMTBIT_S24_LE |
			   SNDRV_PCM_FMTBIT_S32_LE
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE
	},
};

static int dummy_probe(struct snd_soc_component *component)
{
	printk("In %s\n", __func__);
	return 0;
}

static int dummy_set_bias_level(struct snd_soc_component *component, enum snd_soc_bias_level level)
{
	printk("In %s\n", __func__);
	return 0;
}

static struct snd_soc_component_driver soc_component_dev_dummy = {
	.probe			= dummy_probe,
	.set_bias_level		= dummy_set_bias_level,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static int dummy_codec_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev, &soc_component_dev_dummy,
			&dummy_dai, 1);
}

static const struct of_device_id dummy_of_match[] = {
	{ .compatible = "dummy-codec", },
	{ }
};
MODULE_DEVICE_TABLE(of, dummy_of_match);

static struct platform_driver dummy_codec_driver = {
	.probe		= dummy_codec_probe,
	.driver		= {
		.name	= "dummy-codec",
		.of_match_table = dummy_of_match,
	},
};

module_platform_driver(dummy_codec_driver);

MODULE_DESCRIPTION("ASoC Dummy Codec Driver");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_LICENSE("GPL v2");
