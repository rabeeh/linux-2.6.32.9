
#ifndef _ANX7150__I2C__INTF_H
#define _ANX7150__I2C__INTF_H
#include "ANX7150_Sys7150.h"

#define PORT0_ADDR	0x60
#define PORT1_ADDR	0x68
#define ANX7150_PORT0_ADDR	0x72  //ANX7150
#define ANX7150_PORT1_ADDR	0x7A  //ANX7150

extern unsigned char i2c_dev_addr;
char i2c_read_reg(unsigned char offset, unsigned char *d);
char i2c_write_reg(unsigned char offset, unsigned char d);
void i2c_init(void);
void i2c_set_dev_addr(unsigned char daddr);
char i2c_write_p0_reg(unsigned char offset, unsigned char d);
char i2c_write_p1_reg(unsigned char offset, unsigned char d);
char i2c_write_p0_regs(unsigned char offset, unsigned char *dat, unsigned char len);
char i2c_write_p1_regs(unsigned char offset, unsigned char *dat, unsigned char len);
char i2c_read_p0_reg(unsigned char offset, unsigned char *d);
char i2c_read_p1_reg(unsigned char offset, unsigned char *d);
char i2c_read_p0_regs(unsigned char offset, unsigned char *dat, unsigned char len);
char i2c_read_p1_regs(unsigned char offset, unsigned char *dat, unsigned char len);

void ANX7150_i2c_init(void);
void ANX7150_i2c_set_dev_addr(BYTE daddr);
BYTE ANX7150_i2c_write_p0_reg(BYTE offset, BYTE d);
BYTE ANX7150_i2c_write_p1_reg(BYTE offset, BYTE d);

BYTE ANX7150_i2c_read_p0_reg(BYTE offset, BYTE *d);
BYTE ANX7150_i2c_read_p1_reg(BYTE offset, BYTE *d);
#endif

