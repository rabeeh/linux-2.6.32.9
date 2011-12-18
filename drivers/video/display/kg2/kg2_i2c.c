/*
 * kg2_i2c.c - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#include <linux/i2c.h>
#include "kg2_i2c.h"

#define DBG_I2C_DATA_PER_LINE   16
#define DBG_I2C_LENGTH_PER_DATA 3
#define DBG_I2C_LENGTH_PER_LINE (DBG_I2C_DATA_PER_LINE * DBG_I2C_LENGTH_PER_DATA + 1)
//#define DEBUG				// For detail debug messages, including I2C write operations


struct i2c_client * i2c_client_kg2 = NULL;


/*-----------------------------------------------------------------------------
 * Description : Read data from KG2 via I2C bus
 * Parameters  : baseaddr - Page/Register access (0x21/0x23 or 0x41/0x43)
 *               subaddr  - Page access: 0x00, Register access: register offset
 *               data     - Page access: page number, Register access: data
 *               dataLen  - Length of data
 * Return Type : =0 - Success
 *               <0 - Fail
 *-----------------------------------------------------------------------------
 */

int kg2_i2c_read(unsigned char baseaddr, unsigned char subaddr,
                 unsigned char * data, unsigned short dataLen)
{
#if defined(DEBUG)
	char input[DBG_I2C_LENGTH_PER_LINE];
	int count = 0;
#endif
	unsigned short i = 0;
	int result = 0;

	for (i = 0; i < dataLen; i++)
	{
		i2c_client_kg2->addr = baseaddr >> 1;
		result = i2c_smbus_read_byte_data(i2c_client_kg2,
		                                  subaddr + i);
		if (result < 0)
		{
			printk(KERN_ERR
			       "Failed to read data via i2c. (baseaddr: 0x%02X, subaddr: 0x%02X)\n",
			       baseaddr, subaddr + i);

			return -1;
		}

		data[i] = (unsigned char) result;

#if defined(DEBUG)
		count += snprintf(input + count,
		                  DBG_I2C_LENGTH_PER_LINE - count,
		                  " %02X",
		                  data[i]);

		if ((i == (dataLen - 1)) || (i % DBG_I2C_DATA_PER_LINE == (DBG_I2C_DATA_PER_LINE - 1)))
		{
			if (i < DBG_I2C_DATA_PER_LINE)
			{
				dev_info(&i2c_client_kg2->dev,
				         "R - %02X %02X%s\n",
				         baseaddr, subaddr, input);
			}
			else
			{
				dev_info(&i2c_client_kg2->dev,
				         "         %s\n",
				         input);
			}

			count = 0;
		}
#endif
	}

	return 0;
}

/*-----------------------------------------------------------------------------
 * Description : Write data to KG2 via I2C bus
 * Parameters  : baseaddr - Page/Register access (0x20/0x22 or 0x40/0x42)
 *               subaddr  - Page access: 0x00, Register access: register offset
 *               data     - Page access: page number, Register access: data
 *               dataLen  - Length of data
 * Return Type : =0 - Success
 *               <0 - Fail
 *-----------------------------------------------------------------------------
 */

int kg2_i2c_write(unsigned char baseaddr, unsigned char subaddr,
                  const unsigned char * data, unsigned short dataLen)
{
#if defined(DEBUG)
	char output[DBG_I2C_LENGTH_PER_LINE];
	int count = 0;
#endif
	unsigned short i = 0;
	int result = 0;

	for (i = 0; i < dataLen; i++)
	{
		i2c_client_kg2->addr = baseaddr >> 1;
		result = i2c_smbus_write_byte_data(i2c_client_kg2,
		                                   subaddr + i,
		                                   data[i]);
		if (result < 0)
		{
			printk(KERN_ERR
			       "Failed to write data via i2c. (baseaddr: 0x%02X, subaddr: 0x%02X, data: 0x%02X)\n",
			       baseaddr, subaddr + i, data[i]);

			return -1;
		}

#if defined(DEBUG)
		count += snprintf(output + count,
		                  DBG_I2C_LENGTH_PER_LINE - count,
		                  " %02X",
		                  data[i]);

		if ((i == (dataLen - 1)) || (i % DBG_I2C_DATA_PER_LINE == (DBG_I2C_DATA_PER_LINE - 1)))
		{
			if (i < DBG_I2C_DATA_PER_LINE)
			{
				dev_info(&i2c_client_kg2->dev,
				         "W - %02X %02X%s\n",
				         baseaddr, subaddr, output);
			}
			else
			{
				dev_info(&i2c_client_kg2->dev,
				         "         %s\n",
				         output);
			}

			count = 0;
		}
#endif
	}

	return 0;
}

