/*
    This driver supports Chrontel CH7025/CH7026 TV/VGA Encoder.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.
*/

enum input_format{
	CH7025_RGB888,
	CH7025_RGB666,
	CH7025_RGB565,
	CH7025_RGB555,
	CH7025_DVO,
	CH7025_YCbCr422_8bit,
	CH7025_YCbCr422_10bit,
	CH7025_YCbCr444_8bit,
	CH7025_Consecutive_RGB666 = 9,
	CH7025_Consecutive_RGB565,
	CH7025_Consecutive_RGB555
};
enum output_format{
	CH7025_NTSC_M,
	CH7025_NTSC_J,
	CH7025_NTSC_443,
	CH7025_PAL_B,
	CH7025_PAL_M,
	CH7025_PAL_N,
	CH7025_PAL_Nc,
	CH7025_PAL_60,
	CH7025_VGA_OUT //we only have CVBS output on AV-D1
};
enum RGB_SWAP{
	CH7025_RGB_ORDER,
	CH7025_RBG_ORDER,	
	CH7025_GRB_ORDER,
	CH7025_GBR_ORDER,
	CH7025_BRG_ORDER,
	CH7025_BGR_ORDER,
};
struct input_stream_info{
	int xres;
	int yres;
	enum input_format iformat;
	enum output_format oformat;
	enum RGB_SWAP swap;
};
void ch7025_set_input_stream(struct input_stream_info *info);
void ch7025_enable(bool enable);
