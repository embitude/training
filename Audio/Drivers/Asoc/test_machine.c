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
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	struct snd_soc_card *soc_card = rtd->card;
	struct device_node *np = soc_card->dev->of_node;

	unsigned sysclk;
	int ret = 0;

	return 0;

	if (np) {
		ret = of_property_read_u32(np, "ti,codec-clock-rate", &sysclk);
		if (ret < 0)
			return ret;
	}

	ret = snd_soc_dai_set_fmt(codec_dai, AUDIO_FORMAT);
	if( ret < 0 ){
		printk( "-%s(): Codec DAI configuration error, %d\n", __FUNCTION__, ret );
	return ret;
	}

	ret = snd_soc_dai_set_fmt(cpu_dai, AUDIO_FORMAT);
	if( ret < 0 ){
		printk( "-%s(): AP DAI configuration error, %d\n", __FUNCTION__, ret);
	return ret;
	}

	printk("[%s] sysclk = %d\n", __func__, sysclk);

	/* set the codec system clock */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, sysclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* set the CPU system clock */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, sysclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}
static struct snd_soc_ops evm_dummy_ops = {
	.hw_params = evm_dummy_hw_params,
};

static int evm_dummy_init(struct snd_soc_pcm_runtime *rtd)
{
	int ret;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	printk("In %s\n", __func__);

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, 1);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 1, 16);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, 0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_dai_link evm_dai_dummy = {
	.name		= "testdia",
	.stream_name	= "test-hifi",
	.codec_dai_name	= "dummy-hifi",
	.ops            = &evm_dummy_ops,
	.init           = evm_dummy_init,
	.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS |
		SND_SOC_DAIFMT_NB_NF,
};

static const struct of_device_id test_evm_dt_ids[] = {
	{
		.compatible = "test-evm-audio",
		.data = (void *) &evm_dai_dummy,
	},
	
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, test_evm_dt_ids);

/* davinci evm audio machine driver */
static struct snd_soc_card evm_soc_card = {
	.owner = THIS_MODULE,
	.dai_link = &evm_dai_dummy,
	.num_links = 1,
};

static int test_evm_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	u32 machine_ver, clk_gpio;
	int ret = 0;

	clk_gpio = of_get_named_gpio(np, "mcasp_clock_enable", 0);
	if (clk_gpio < 0) {
		dev_err(&pdev->dev, "failed to find mcasp_clock enable GPIO!\n");
		return -EINVAL;
	}
	gpio_set_value(clk_gpio, 1);
	evm_dai_dummy.codec_of_node = of_parse_phandle(np, "ti,audio-codec", 0);
	if (!evm_dai_dummy.codec_of_node)
		return -EINVAL;

	evm_dai_dummy.cpu_of_node = of_parse_phandle(np,
						"ti,mcasp-controller", 0);
	if (!evm_dai_dummy.cpu_of_node)
		return -EINVAL;

	evm_dai_dummy.platform_of_node = evm_dai_dummy.cpu_of_node;

	evm_soc_card.dev = &pdev->dev;
	ret = snd_soc_of_parse_card_name(&evm_soc_card, "ti,model");
	if (ret)
		return ret;

	ret = snd_soc_register_card(&evm_soc_card);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);

	return ret;
}

static int test_evm_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);

	return 0;
}

static struct platform_driver test_evm_driver = {
	.probe		= test_evm_probe,
	.remove		= test_evm_remove,
	.driver		= {
		.name	= "test_evm",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(test_evm_dt_ids),
	},
};

static int __init evm_init(void)
{

	return platform_driver_register(&test_evm_driver);
}

static void __exit evm_exit(void)
{
	platform_driver_unregister(&test_evm_driver);
}

module_init(evm_init);
module_exit(evm_exit);

MODULE_DESCRIPTION("ASoC Test Machine Driver");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in");
MODULE_LICENSE("GPL");
