
#ifndef _LINUX_ADI9889_I2C_H
#define _LINUX_ADI9889_I2C_H

#define ADI9889_I2C_NAME "adi9889"


typedef enum audio_input_format_t{
	I2S_FORMAT = 0x1,
	SPDIF_FORMAT = 0x2,
}audio_input_format;

#define DEFAULT_AUDIO_INPUT_FORMAT SPDIF

struct adi9889_i2c_platform_data {
	uint32_t version;	/* Use this entry for panels with */
				/* (major << 8 | minor) version or above. */
				/* If non-zero another array entry follows */
	int (*power)(int on);	/* Only valid in first array entry */
	audio_input_format audio_format;

	uint32_t flags;
	unsigned long irqflags;

};

#endif /* _LINUX_SITRONIX_I2C_ST7002_H */

