#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/soc.h>

//TODO 1.4: Populate the snd_soc_dai_driver structure
//Populate the .name, .playback and .capture
//For playback & capture, populate .channels_min, .channels_max
//.rates, .formats
static struct snd_soc_dai_driver dummy_dai = {
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

//TODO 1.3: Populate the snd_soc_component_driver structure
//Initialize the set_bias_level & probe callback handlers
static struct snd_soc_component_driver soc_component_dev_dummy = {
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static int dummy_codec_probe(struct platform_device *pdev)
{
	//TODO 1.5: Register the codec with devm_snd_soc_register_component
	printk("In %s\n", __func__);
	return 0; 
}
//TODO 1.1: Populate the of_device_id structure with compatible string
static const struct of_device_id dummy_of_match[] = {
	{ },
	{ }
};
MODULE_DEVICE_TABLE(of, dummy_of_match);

//TODO 1.2: Populate the platform_driver data structure
// Use of_match_table for binding
static struct platform_driver dummy_codec_driver = {
	.driver		= {
		.name	= "dummy-codec",
	},
};

module_platform_driver(dummy_codec_driver);

MODULE_DESCRIPTION("ASoC Dummy Codec Driver");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_LICENSE("GPL v2");
