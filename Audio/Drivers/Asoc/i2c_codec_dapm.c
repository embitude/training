#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/wm8960.h>
#include "i2c_codec.h"

#include <sound/soc.h>

static const struct reg_default codec_reg_defaults[] = {
	{  0x0, 0x00a7 },
	{  0x1, 0x00a7 },
	{  0x2, 0x0000 },
	{  0x3, 0x0000 },
	{  0x4, 0x0000 },
	{  0x5, 0x0008 },
	{  0x6, 0x0000 },
	{  0x7, 0x000a },
	{  0x8, 0x01c0 },
	{  0x9, 0x0000 },
	{  0xa, 0x00ff },
	{  0xb, 0x00ff },

	{ 0x10, 0x0000 },
	{ 0x11, 0x007b },
	{ 0x12, 0x0100 },
	{ 0x13, 0x0032 },
	{ 0x14, 0x0000 },
	{ 0x15, 0x00c3 },
	{ 0x16, 0x00c3 },
	{ 0x17, 0x01c0 },
	{ 0x18, 0x0000 },
	{ 0x19, 0x0000 },
	{ 0x1a, 0x0000 },
	{ 0x1b, 0x0000 },
	{ 0x1c, 0x0000 },
	{ 0x1d, 0x0000 },

	{ 0x20, 0x0100 },
	{ 0x21, 0x0100 },
	{ 0x22, 0x0050 },

	{ 0x25, 0x0050 },
	{ 0x26, 0x0000 },
	{ 0x27, 0x0000 },
	{ 0x28, 0x0000 },
	{ 0x29, 0x0000 },
	{ 0x2a, 0x0040 },
	{ 0x2b, 0x0000 },
	{ 0x2c, 0x0000 },
	{ 0x2d, 0x0050 },
	{ 0x2e, 0x0050 },
	{ 0x2f, 0x0000 },
	{ 0x30, 0x0002 },
	{ 0x31, 0x0037 },

	{ 0x33, 0x0080 },
	{ 0x34, 0x0008 },
	{ 0x35, 0x0031 },
	{ 0x36, 0x0026 },
	{ 0x37, 0x00e9 },
};

#define codec_reset(c)	regmap_write(c, CODEC_RESET, 0)

struct codec_priv {
	struct regmap *regmap;
};

static struct snd_soc_dai_driver dummy_dai = {
	.name = "codec-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
		       SNDRV_PCM_FMTBIT_S24_LE |
		       SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
		       SNDRV_PCM_FMTBIT_S24_LE |
		       SNDRV_PCM_FMTBIT_S32_LE,
	},
};

static const DECLARE_TLV_DB_SCALE(dac_tlv, -12750, 50, 1);
static const DECLARE_TLV_DB_SCALE(adc_tlv, -9750, 50, 1);
static const DECLARE_TLV_DB_SCALE(out_tlv, -12100, 100, 1);

static const struct snd_kcontrol_new codec_snd_controls[] = {

        SOC_DOUBLE_R_TLV("Playback Volume", CODEC_LDAC, CODEC_RDAC,
                 0, 255, 0, dac_tlv),
        SOC_DOUBLE_R_TLV("ADC PCM Capture Volume", CODEC_LADC, CODEC_RADC,
                0, 255, 0, adc_tlv),
        SOC_DOUBLE_R("Capture Switch", CODEC_LINVOL, CODEC_RINVOL,
        7, 1, 1),
        SOC_SINGLE("DAC Mute", CODEC_DACCTL1, 3, 1, 0),
        SOC_DOUBLE_R_TLV("Headphone Playback Volume", CODEC_LOUT1, CODEC_ROUT1,
                 0, 127, 0, out_tlv),
};
//TODO 5.1: Add the Widgets for
// Left DAC, Right DAC, LOUT1 PGA, ROUT1 PGA, HP_L, HP_R, Left Output Mixer, Right Output Mixer
// Use SND_SOC_DAPM_DAC, SND_SOC_DAPM_PGA, SND_SOC_DAPM_OUTPUT, SND_SOC_DAPM_MIXER
static const struct snd_soc_dapm_widget codec_dapm_widgets[] = {
};

//TODO 5.2: Add the DAPM routes for
// Left DAC -> Left Mixer -> Left PGA -> HP_L
// Right DAC -> Right Mixer -> Right PGA -> HP_R
static const struct snd_soc_dapm_route audio_paths[] = {
};

static bool codec_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CODEC_RESET:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config codec_regmap = {
	.reg_bits = 7,
	.val_bits = 9,
	.max_register = CODEC_PLL4,

	.reg_defaults = codec_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(codec_reg_defaults),
	.cache_type = REGCACHE_RBTREE,

	.volatile_reg = codec_volatile,
};

static int codec_set_bias_level(struct snd_soc_component *component, enum snd_soc_bias_level level)
{
	struct codec_priv *codec_priv = snd_soc_component_get_drvdata(component);
	printk("In %s\n", __func__);
	regmap_write(codec_priv->regmap, CODEC_POWER2, 0x01F8);
	return 0;
}

#define USE_EARPHONE_MIC 1
#define USE_BOARD_MIC 0

static int codec_probe(struct snd_soc_component *component)
{
	struct codec_priv *codec_priv = snd_soc_component_get_drvdata(component);
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(component);
	int res;

	printk("In %s\n", __func__);

	//Set Power Source
#if USE_BOARD_MIC
	res =  regmap_write(codec_priv->regmap, CODEC_POWER1, 0x00E8);
#elif USE_EARPHONE_MIC
	res =  regmap_write(codec_priv->regmap, CODEC_POWER1, 0x00D4);
#endif
	res += regmap_write(codec_priv->regmap, CODEC_POWER2, 0x01F8);
	res += regmap_write(codec_priv->regmap, CODEC_POWER3, 0x003C);

	if(res != 0)  {
		printk("Source set fail !!\r\n");
		printk("Error code: %d\r\n",res);
		return res;
	}

	//Configure clock
	//MCLK->div1->SYSCLK->DAC/ADC
	regmap_write(codec_priv->regmap, CODEC_CLOCK1, 0x0000);

	//Audio Interface
	//I2S format 16 bits word length
	regmap_write(codec_priv->regmap, CODEC_IFACE1, 0x0002);

	//Input PGA
	regmap_write(codec_priv->regmap, CODEC_LINVOL, 0x003F | 0x0100);
	regmap_write(codec_priv->regmap, CODEC_RINVOL, 0x003F | 0x0100);

	//Input Signal Path
#if USE_BOARD_MIC
	regmap_write(codec_priv->regmap, CODEC_LINPATH, 0x0008 | 0x0100);
	regmap_write(codec_priv->regmap, CODEC_RINPATH, 0x0000);
#elif USE_EARPHONE_MIC
	regmap_write(codec_priv->regmap, CODEC_LINPATH, 0x0000);
	regmap_write(codec_priv->regmap, CODEC_RINPATH, 0x0008 | 0x0100);
#endif
	//Input Boost Mixer
	regmap_write(codec_priv->regmap, CODEC_INBMIX1, 0x0000);
	regmap_write(codec_priv->regmap, CODEC_INBMIX2, 0x0000);

	//ADC Digital Volume Control
	regmap_write(codec_priv->regmap, CODEC_LADC, 0x00C3 | 0x0100);
	regmap_write(codec_priv->regmap, CODEC_RADC, 0x00C3 | 0x0100);

#if USE_BOARD_MIC
	regmap_write(codec_priv->regmap, CODEC_ADDCTL1, 0x01C4);
#elif USE_EARPHONE_MIC
	regmap_write(codec_priv->regmap, CODEC_ADDCTL1, 0x01C8);
#endif

	//ALC Control
	//Noise Gate Control
	regmap_write(codec_priv->regmap, CODEC_NOISEG, 0x00F9);

	//OUTPUT SIGNAL PATH
	//Digital Volume Control
	regmap_write(codec_priv->regmap, CODEC_LDAC, 0x0000 | 0x0100);
	regmap_write(codec_priv->regmap, CODEC_RDAC, 0x0000 | 0x0100);

	//DAC Soft-Mute Control
	regmap_write(codec_priv->regmap, CODEC_DACCTL1, 0x0000);
	regmap_write(codec_priv->regmap, CODEC_DACCTL2, 0x0000);
	//3D Stereo Enhancement Function
	regmap_write(codec_priv->regmap, CODEC_3D, 0x0000);
	//Analogue Output Control
	regmap_write(codec_priv->regmap, CODEC_CLASSD1, 0x00F7);

	// Playback

	//Configure ADC/DAC
	regmap_write(codec_priv->regmap, CODEC_DACCTL1, 0x0000);

	//Configure HP_L and HP_R OUTPUTS
	regmap_write(codec_priv->regmap, CODEC_LOUT1, 0x006F | 0x0100);  //LOUT1 Volume Set
	regmap_write(codec_priv->regmap, CODEC_ROUT1, 0x006F | 0x0100);  //ROUT1 Volume Set

	//Configure SPK_RP and SPK_RN
	regmap_write(codec_priv->regmap, CODEC_LOUT2, 0x007A | 0x0100); //Left Speaker Volume
	regmap_write(codec_priv->regmap, CODEC_ROUT2, 0x007A | 0x0100); //Right Speaker Volume

	//Enable the OUTPUTS
	regmap_write(codec_priv->regmap, CODEC_CLASSD1, 0x00F7); //Enable Class D Speaker Outputs

	//Configure DAC volume
	regmap_write(codec_priv->regmap, CODEC_LDAC, 0x00FF | 0x0100);
	regmap_write(codec_priv->regmap, CODEC_RDAC, 0x00FF | 0x0100);

	//Configure MIXER
	regmap_write(codec_priv->regmap, CODEC_LOUTMIX, 1<<8 | 1<<7);
	regmap_write(codec_priv->regmap, CODEC_ROUTMIX, 1<<8 | 1<<7);

	//Jack Detect
	regmap_write(codec_priv->regmap, CODEC_ADDCTL2, 1<<6 | 0<<5);
	regmap_update_bits(codec_priv->regmap, CODEC_ADDCTL1, 0x3, 0x3);
	regmap_write(codec_priv->regmap, CODEC_ADDCTL4, 0x0009);

	//TODO 4.1: //Register the Kcontrols using API snd_soc_add_component_controls
	snd_soc_add_component_controls(component, codec_snd_controls, ARRAY_SIZE(codec_snd_controls)); 

	//TODO 5.3: Add the dapm widgets with snd_soc_dapm_new_controls

	//TODO 5.4: Add the dapm routes with snd_soc_dapm_add_routes

	return 0;
}

//TODO 3.1: Register the probe for codec initialization
static struct snd_soc_component_driver soc_component_dev_dummy = {
	.probe = codec_probe,
	.set_bias_level = codec_set_bias_level,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static int i2c_codec_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct codec_priv *codec_priv;
	int ret;

	printk("In %s\n", __func__);

	codec_priv = devm_kzalloc(&i2c->dev, sizeof(struct codec_priv), GFP_KERNEL);
	if (codec_priv == NULL)
		return -ENOMEM;

	//TODO 2.1: Register the managed regmap interface and assign to codec_priv->regmap
	// Use devm_regmap_init_i2c api
	codec_priv->regmap = devm_regmap_init_i2c(i2c, &codec_regmap);
	if (IS_ERR(codec_priv->regmap))
		return PTR_ERR(codec_priv->regmap);
	//TODO 2.2: Reset the codec with codec_reset api
	ret = codec_reset(codec_priv->regmap);
	if (ret != 0) {
		dev_err(&i2c->dev, "Failed to issue reset\n");
		return ret;
	}

	i2c_set_clientdata(i2c, codec_priv);
	
	//TODO 2.3: Register the codec with devm_snd_soc_register_component
	return devm_snd_soc_register_component(&i2c->dev, &soc_component_dev_dummy, 
			&dummy_dai, 1); 
}

static int i2c_codec_remove(struct i2c_client *client)
{
	return 0;
}

//0x1a I2C1
//TODO 1.1: Populate the i2c_device_id with the string used in device tree
static const struct i2c_device_id codec_i2c_id[] = {
	{ "i2c-codec", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, codec_i2c_id);

//TODO 1.2: Populate the i2c_driver data structure
// Use id_table for binding the driver. Register probe & remove
static struct i2c_driver i2c_codec_driver = {
	.probe = i2c_codec_probe,
	.remove = i2c_codec_remove,
	.driver		= {
		.name	= "i2c-codec",
	},
	.id_table = codec_i2c_id,
};

module_i2c_driver(i2c_codec_driver);

MODULE_DESCRIPTION("ASoC I2C Codec Driver");
MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_LICENSE("GPL v2");
