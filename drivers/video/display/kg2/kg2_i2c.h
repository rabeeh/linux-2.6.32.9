/*
 * kg2_i2c.h - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#ifndef __KG2_I2C_H__
#define __KG2_I2C_H__


int kg2_i2c_read(unsigned char baseaddr, unsigned char subaddr, unsigned char * data, unsigned short dataLen);
int kg2_i2c_write(unsigned char baseaddr, unsigned char subaddr, const unsigned char * data, unsigned short dataLen);


#endif
