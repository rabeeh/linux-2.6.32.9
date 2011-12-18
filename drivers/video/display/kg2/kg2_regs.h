/*
 * kg2_regs.h - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#ifndef __KG2_REGS_H__
#define __KG2_REGS_H__


typedef enum tagKG2_REG_BASE_TYPE
{
	REG_FPLL	= 0x000,	// Clock Control and FPLL Register						0x000 ~ 0x05C
	REG_PAT		= 0x080,	// Flexiport									0x080 ~ 0x0AA
	REG_IP		= 0x0B0,	// Flexiport									0x0B0 ~ 0x0D6
	REG_PINH	= 0x0D7,	// Flexiport									0x0D7 ~ 0x0D9
	REG_FE		= 0x100,	// Frontend									0x100 ~ 0x162
	REG_FE_DI	= 0x170,	// Frontend - Data Instrumentation						0x170 ~ 0x1DF
	REG_VNR		= 0x200,	// Video Noise Reducer								0x200 ~ 0x229
	REG_CAR		= 0x235,	// Compression Artifact Reducer (CAR)						0x235 ~ 0x23C, 0x280 ~ 0x2C8
	REG_VP		= 0x2F2,	// Video Processor								0x2F2 ~ 0x2F7
	REG_DIN		= 0x300,	// Deinterlacer									0x300 ~ 0x3B1
	REG_DET		= 0x3E0,	// Edge Enhancement								0x3E0 ~ 0x3FA
	REG_DDR		= 0x400,	// SDRam Subsystem								0x400 ~ 0x4BA
	REG_DDR_PHY	= 0x500,	// SDRam PHY Registers								0x500 ~ 0x5CD
	REG_FRC_MAIN	= 0x600,	// Scaler and Frame Rate Conversion						0x600 ~ 0x62D
	REG_SCL_MAIN	= 0x660,	// Scaler and Frame Rate Conversion						0x660 ~ 0x6A5
	REG_FRC_DET	= 0x700,	// Scaler and Frame Rate Conversion						0x700 ~ 0x72D
	REG_SCL_DET	= 0x760,	// Scaler and Frame Rate Conversion						0x760 ~ 0x7A5
	REG_CPCB_ICSC	= 0x800,	// Color Management Unit - Input Color Space Converter				0x800 ~ 0x81E
	REG_CPCB_ACE	= 0x820,	// Color Management Unit - Adaptive Contrast Enhancement (ACE)			0x820 ~ 0x83F
	REG_CMU_FTDC	= 0x840,	// Color Management Unit - Intelligent Color Remapper - FTDC			0x840 ~ 0x887
	REG_CMU_GCSC	= 0x8A0,	// Color Management Unit - Global Color Space Converter and Gamut Compression	0x8A0 ~ 0x8E0
	REG_CMU_GAMMA	= 0x8F0,	// Gamma									0x8F0 ~ 0x8F9
	REG_CMU_HS	= 0x900,	// Color Management Unit - Intelligent Color Remapper - Hue/Sat			0x900 ~ 0x950
	REG_QTC		= 0x980,	// Qdeo True Color (QTC)							0x980 ~ 0x986
	REG_BE		= 0xA00,	// BE Timing Generator								0xA00 ~ 0xA3F, 0xA43, 0xAA0 ~ 0xAA7
	REG_EE		= 0xA40,	// Edge Enhancement - LTI, CTI							0xA40 ~ 0xA42
	REG_BE_FGG	= 0xA48,	// Film Grain Generator								0xA48 ~ 0xA4F
	REG_BE_INT	= 0xA50,	// Interlacer									0xA50 ~ 0xA79
	REG_BE_DNS	= 0xA44,	// Overlay									0xA44 ~ 0xA45, 0xA90 ~ 0xA9A
	REG_NEST	= 0xB00,	// Noise Estimator (NEST)							0xB00 ~ 0xB77
	REG_OP		= 0xD00,	// Output Formatter								0xD00 ~ 0xD2D
	REG_ANA		= 0xF00,	// Analog (Analog Macro Control Registers)					0xF00 ~ 0xF4F
} KG2_REG_BASE;


// Adapted from "88DE2750/sdk/2750_core/private/88DE2750/avc/ddr/bdr/pinselection/src/hwipinselection.h" +

struct tagHWI_MISC_CONTROL_FOR_FLEXIPORT
{
	unsigned char PolarityControlforPin0Clock	: 1;
	unsigned char PolarityControlforPin1Clock	: 1;
	unsigned char InputStreamSelection		: 1;
	unsigned char SourceSelection			: 1;
	unsigned char PolarityInversionforHSync		: 1;
	unsigned char PolarityInversionforVSync		: 1;
	unsigned char PolarityInversionforField		: 1;
	unsigned char Reserved				: 1;
};

typedef struct tagHWI_MISC_CONTROL_FOR_FLEXIPORT HWI_MISC_CONTROL_FOR_FLEXIPORT;

typedef struct tagHWI_FLEXIPORT_PIN_CONTROL_BITS
{
	HWI_MISC_CONTROL_FOR_FLEXIPORT		MiscInputControl;				// PINH_CTRL0		0x0D7	 8-bits
	unsigned char				CCIR656Control;					// PINH_CTRL1		0x0D8	 8-bits
	unsigned char				BT1120Control;					// PINH_CTRL2		0x0D9	 8-bits

} HWI_FLEXIPORT_PIN_CONTROL_BITS;

// Adapted from "88DE2750/sdk/2750_core/private/88DE2750/avc/ddr/bdr/pinselection/src/hwipinselection.h" -

// Adapted from "88DE2750/sdk/2750_core/private/88DE2750/avc/ddr/bdr/datachannel/src/hwidatachannel.h" +

/*This macro defines number of color coefficients in case of front end
color conversion*/
#define MAX_SIZE_COL_COEFF_CHNL        (9)

/*This macro defines number of color offsets in case of front end
color conversion*/
#define MAX_SIZE_COL_OFFSET_CHNL       (3)

/*This structure defines data type,which will represnt 12 bit registers*/
struct tagHWI_DC_REG_12BIT_TYPE
{
	unsigned short Value	: 12;
	unsigned short Reserved	: 4;
};

typedef struct tagHWI_DC_REG_12BIT_TYPE HWI_DC_REG_12BIT_TYPE;

typedef struct tagHWI_FE_CHANNEL_REG
{
	HWI_DC_REG_12BIT_TYPE			FeDcStrX;					// FE_STR_X		0x100	12-bits
	HWI_DC_REG_12BIT_TYPE			FeDcStrY;					// FE_STR_Y		0x102	12-bits
	HWI_DC_REG_12BIT_TYPE			FeDcEndX;					// FE_END_X		0x104	12-bits
	HWI_DC_REG_12BIT_TYPE			FeDcEndY;					// FE_END_Y		0x106	12-bits
	HWI_DC_REG_12BIT_TYPE			FeDcVSamp;					// FE_VSAMP		0x108	12-bits
	unsigned short				FeDcColCoeff[MAX_SIZE_COL_COEFF_CHNL];		// FE_C0 ~ FE_C8	0x10A	14-bits for each
	unsigned short 				FeDcColOffset[MAX_SIZE_COL_OFFSET_CHNL];	// FE_A0 ~ FE_A2	0x11C	16-bits for each
	unsigned char				FeDcCntrl;					// FE_CTRL1		0x122	 8-bits
	unsigned char				FeDcYbl;					// FE_YBLK		0x123	 8-bits
	unsigned char				FeDcCbl;					// FE_CBLK		0x124	 8-bits
	unsigned char				Unused1[49];
	unsigned char				FeDcSyncRef;					// FE_SYNCREF		0x156	 7-bits
	unsigned char				FeDcFrst;					// FE_FRST		0x157	 8-bits
	HWI_DC_REG_12BIT_TYPE			FeDcLrst;					// FE_LRST		0x158	12-bits
	unsigned char				FeDcTgLoadFlg;					// FE_TG_LOAD		0x15A	 1-bit
	unsigned char				FeDcTgDbCtrl;					// FE_TG_DB_CTRL	0x15B	 1-bit
	unsigned short				FeDcDelCtrl;					// FE_DEL_BL		0x15C	16-bits
	unsigned char				FeDeltaHtot;					// FE_DHT		0x15E	 8-bits
	unsigned char 				Unused2[2];
	unsigned char				FeDithCtrl;					// FE_DIT_CTRL		0x161	 1-bit
	unsigned char				FeDithRes;					// FE_DIT_RES		0x162	 8-bits
} HWI_FE_CHANNEL_REG;

// Adapted from "88DE2750/sdk/2750_core/private/88DE2750/avc/ddr/bdr/datachannel/src/hwidatachannel.h" -


#endif
