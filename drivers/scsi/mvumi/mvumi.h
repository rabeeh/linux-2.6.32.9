/*
 *
 *		Linux driver for SAS based RAID controllers
 *
 * Copyright (c) 2006-2008  MARVELL SEMICONDUCTOR, LTD.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * FILE		: mvumi.c
 */

#ifndef MV_RAID_UMI_H
#define MV_RAID_UMI_H
#define MV_DEBUG	1

/*
 * FW reports the maximum of number of commands that it can accept (maximum
 * commands that can be outstanding) at any time. The driver must report a
 * lower number to the mid layer because it can issue a few internal commands
 * itself (E.g, AEN, abort cmd, IOCTLs etc). The number of commands it needs
 * is shown below
 */
#define MVUMI_INT_CMDS			1

/*
 * ===============================
 * MV SAS driver definitions
 * ===============================
 */
/* for communication with OS/SCSI mid layer only */
#define MVUMI_MAX_TARGET_NUMBER			128
#define MVUMI_MAX_CHANNELS			0
#define MVUMI_DEFAULT_INIT_ID			(-1)
#define MVUMI_MAX_LUN				1
#define MVUMI_MAX_LD				MVUMI_MAX_TARGET_NUMBER
#define MVUMI_MAX_REQUEST_NUMBER		32
#define MVUMI_MAX_SG_ENTRY			30
#define MVUMI_MAX_TRANSFER_SIZE			(128 * 1024)
#define MVUMI_MAX_REQUEST_PER_LUN		MVUMI_MAX_REQUEST_NUMBER
#define MVUMI_USE_CLUSTERING			DISABLE_CLUSTERING
#define MVUMI_EMULATED				0


#define SUPPORT_VIRTUAL_DEVICE       1
#define VIRTUAL_DEVICE_ID            (MVUMI_MAX_LD - 1)
#define USE_NEW_SGTABLE				1

/* driver capabilities */
#define SUPPORT_INTERNAL_REQ	1
#define FW_MAX_DELAY		5

#define SUPPORT_EVENT                1
#if defined(SUPPORT_EVENT)
#define MAX_EVENTS                      20
#define MAX_EVENT_PARAMS                4
#define MAX_EVENTS_RETURNED             6
#endif	/* #if defined(SUPPORT_EVENT) */

#define MAX_BASE_ADDRESS                6
//#define MV_MAX_TRANSFER_SIZE            (32*1024)
#define MAX_INTERNAL_REQ            2

#define LIST_WORK_AROUND
//#define DMA_WORK_AROUND

#define MAX_MGMT_ADAPTERS		1024
#define MAX_IOCTL_SGE			16
#define SENSE_INFO_BUFFER_SIZE		32

#ifdef MV_DEBUG
#define mv_printk(format, arg...)		\
	do {	if(mvumi_dbg_lvl) 	\
		printk( "%s:" format, MV_DRIVER_NAME, ## arg);	\
		}while(0)
#else
static inline int
mv_printk(const char * fmt, ...)
{
	return 0;
}
#endif


/*
 * Marvell SAS Driver meta data
 */
//#define MVUMI_VERSION				"2.00.00.00-rc1"
#define MVUMI_RELDATE				"Jan 22, 2008"
#define VER_MAJOR       	2
#define VER_MINOR       	0
#define VER_OEM		4
#define VER_BUILD       	9
#define VER_TEST        ""
/* call VER_VAR_TO_STRING */
#define NUM_TO_STRING(num1, num2, num3, num4) #num1"."#num2"."#num3"."#num4
#define VER_VAR_TO_STRING(major, minor, oem, build) NUM_TO_STRING(major, \
								  minor, \
								  oem,   \
								  build)

#define MVUMI_VERSION   VER_VAR_TO_STRING(VER_MAJOR, VER_MINOR,       \
					     VER_OEM, VER_BUILD) VER_TEST

#define MV_DRIVER_NAME				"mvumi"

/*
 * Device IDs
 */
#ifndef PCI_VENDOR_ID_MARVELL
#define PCI_VENDOR_ID_MARVELL		0x11ab
#endif
#define PCI_DEVICE_ID_MARVELL_MV8480	0x8480
#define PCI_DEVICE_ID_MARVELL_MV8180	0x8180
#define PCI_DEVICE_ID_MARVELL_MV8120	0x8120
#define PCI_DEVICE_ID_MARVELL_MV8110	0x8110

/*
 * =====================================
 * Marvell SAS  firmware definitions
 * =====================================
 */

#define SECTOR_LENGTH 512
/*
 * Marvell Universal Message Interface (UMI) defines a messaging interface
 * between host and Marvell IOP RAID or non-RAID products.
 * Commands are issued using "message frames".
 */



#define MVUMI_DBG_LVL				1

#define MVUMI_FW_BUSY				(1U << 0)
#define MVUMI_FW_ATTACH				(1U << 1)
#define MVUMI_FW_ALLOC				(1U << 2)

/*
 * When SCSI mid-layer calls driver's reset routine, driver waits for
 * MVUMI_RESET_WAIT_TIME seconds for all outstanding IO to complete. Note
 * that the driver cannot _actually_ abort or reset pending commands. While
 * it is waiting for the commands to complete, it prints a diagnostic message
 * every MVUMI_RESET_NOTICE_INTERVAL seconds
 */
#define MVUMI_RESET_WAIT_TIME			180
#define MVUMI_INTERNAL_CMD_WAIT_TIME		45
#define	MVUMI_RESET_NOTICE_INTERVAL		5
#define MVUMI_IOCTL_CMD			0
#define MVUMI_DEFAULT_CMD_TIMEOUT		15
#define MVUMI_POLL_TIMEOUT_SECS         10
#define MVUMI_HANDSHAKE_TIMEOUT			40

/*
 * FW can accept both 32 and 64 bit SGLs. We want to allocate 32/64 bit
 * SGLs based on the size of dma_addr_t
 */
#define IS_DMA64				(sizeof(dma_addr_t) == 8)

#define MV_TRUE                           1
#define MV_FALSE                          0

#define MV_BIT(x)                         (1U << (x))
#define ROUNDING_MASK(x, mask)  (((x)+(mask))&~(mask))
#define ROUNDING(value, align)  ROUNDING_MASK(value,   \
						 (typeof(value)) (align-1))


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#define mv_scmd_host(cmd)    cmd->device->host
#define mv_scmd_channel(cmd) cmd->device->channel
#define mv_scmd_target(cmd)  cmd->device->id
#define mv_scmd_lun(cmd)     cmd->device->lun
#else
#define mv_scmd_host(cmd)    cmd->host
#define mv_scmd_channel(cmd) cmd->channel
#define mv_scmd_target(cmd)  cmd->target
#define mv_scmd_lun(cmd)     cmd->lun
#endif

#define MAX_CDB_SIZE                           16

#define MV_ASSERT(_x_)  BUG_ON(!(_x_))
#define LO_BUSADDR(x) ((u32)(x))
#define HI_BUSADDR(x) (sizeof(dma_addr_t)>4? (u64)(x) >> 32 : 0)
#ifndef scsi_to_pci_dma_dir
#define scsi_to_pci_dma_dir(scsi_dir) ((int)(scsi_dir))
#endif


enum MV_QUEUE_COMMAND_RESULT {
    MV_QUEUE_COMMAND_RESULT_FINISHED    = 0,
    MV_QUEUE_COMMAND_RESULT_FULL,
    MV_QUEUE_COMMAND_RESULT_NO_RESOURCE,
    MV_QUEUE_COMMAND_RESULT_SENT,
} ;

#define mr32(offset)	readl(regs+offset)
#define mw32(offset,val)	writel((val),regs+offset)
#define mw32_f(offset, val) do{	\
	writel((val), regs + offset);	\
	readl(regs+offset);	\
	} while (0)

#define MU_ENTRY_DEPTH              16

// list entry size in DWORDs
#define CL_LIST_ENTRY_SIZE_DWORDS(p)    (1 << (p))

// list entry size in BYTEs
#define CL_LIST_ENTRY_SIZE_BYTES(p)	(CL_LIST_ENTRY_SIZE_DWORDS(p) << 2)

/*
 * ===============================
 * Marvell SAS driver definitions
 * ===============================
 */
enum {
UMI_STATE_IDLE			= 0,
UMI_STATE_STARTING		= 1,
UMI_STATE_HANDSHAKING		= 2,
UMI_STATE_STARTED		= 3,

CL_IN_ENTRY_SIZE_SETTING	= 8,
CL_OUT_ENTRY_SIZE_SETTING	= 4,
CL_IN_ENTRY_SIZE		= CL_LIST_ENTRY_SIZE_BYTES(CL_IN_ENTRY_SIZE_SETTING),
CL_OUT_ENTRY_SIZE		= CL_LIST_ENTRY_SIZE_BYTES(CL_OUT_ENTRY_SIZE_SETTING),

MV_OS_TYPE_BIOS			= 1,
MV_OS_TYPE_WINDOWS		= 2,
MV_OS_TYPE_LINUX		= 3,


/*******************************************/

/* ARM Mbus Registers Map		      	   */

/*******************************************/
CPU_MAIN_INT_CAUSE_REG  = 0x20200,
CPU_MAIN_IRQ_MASK_REG   = 0x20204,
CPU_MAIN_FIQ_MASK_REG   = 0x20208,
CPU_ENPOINTA_MASK_REG   = 0x2020C,
CPU_ENPOINTB_MASK_REG   = 0x20210,

INT_MAP_COMBOUT         = MV_BIT(11),
INT_MAP_COMBIN          = MV_BIT(10),
INT_MAP_COMBERR         = MV_BIT(9),
INT_MAP_COMAOUT         = MV_BIT(8),
INT_MAP_COMAIN          = MV_BIT(7),
INT_MAP_COMAERR         = MV_BIT(6),

INT_MAP_COMAINT         = (INT_MAP_COMAOUT | INT_MAP_COMAERR),
INT_MAP_COMBINT         = (INT_MAP_COMBOUT | INT_MAP_COMBIN | INT_MAP_COMBERR),
INT_MAP_COMINT          = (INT_MAP_COMAINT | INT_MAP_COMBINT),

INT_MAP_DL_CPU2PCIEB    = MV_BIT(3),
INT_MAP_DL_PCIEB2CPU    = MV_BIT(2),
INT_MAP_DL_CPU2PCIEA    = MV_BIT(1),
INT_MAP_DL_PCIEA2CPU    = MV_BIT(0),

/*******************************************/

/* ARM Doorbell Registers Map			   */

/*******************************************/
CPU_PCIEA_TO_ARM_DRBL_REG   = 0x20400,
CPU_PCIEA_TO_ARM_MASK_REG   = 0x20404,
CPU_ARM_TO_PCIEA_DRBL_REG   = 0x20408,
CPU_ARM_TO_PCIEA_MASK_REG   = 0x2040C,
CPU_PCIEB_TO_ARM_DRBL_REG   = 0x20410,
CPU_PCIEB_TO_ARM_MASK_REG   = 0x20414,
CPU_ARM_TO_PCIEB_DRBL_REG   = 0x20418,
CPU_ARM_TO_PCIEB_MASK_REG   = 0x2041C,

DRBL_HANDSHAKE              = MV_BIT(0),
DRBL_SOFT_RESET             = MV_BIT(1),
DRBL_BUS_CHANGE             = MV_BIT(2),
DRBL_EVENT_NOTIFY           = MV_BIT(3),
DRBL_MU_RESET		=	MV_BIT(4),
DRBL_HANDSHAKE_ISR          = DRBL_HANDSHAKE,

CPU_PCIEA_TO_ARM_MSG0       = 0x20430,
CPU_PCIEA_TO_ARM_MSG1       = 0x20434,
CPU_ARM_TO_PCIEA_MSG0       = 0x20438,
CPU_ARM_TO_PCIEA_MSG1       = 0x2043C,
CPU_PCIEB_TO_ARM_MSG0       = 0x20440,
CPU_PCIEB_TO_ARM_MSG1       = 0x20444,
CPU_ARM_TO_PCIEB_MSG0       = 0x20448,
CPU_ARM_TO_PCIEB_MSG1       = 0x2044C,

/*******************************************/

/* ARM Communication List Registers Map    */

/*******************************************/
CLA_INB_LIST_BASEL          = 0x500,
CLA_INB_LIST_BASEH          = 0x504,
CLA_INB_AVAL_COUNT_BASEL    = 0x508,
CLA_INB_AVAL_COUNT_BASEH    = 0x50C,
CLA_INB_DESTI_LIST_BASEL    = 0x510,
CLA_INB_DESTI_LIST_BASEH    = 0x514,
CLA_INB_WRITE_POINTER       = 0x518,
CLA_INB_READ_POINTER        = 0x51C,
CLA_INB_AVAL_COUNT          = 0x520,
CLA_INB_DESTI_UPDATE        = 0x524,
CLA_INB_INTF_CONF           = 0x528,
CLA_INB_INTF_CTL            = 0x52C,

CLA_OUTB_LIST_BASEL         = 0x530,
CLA_OUTB_LIST_BASEH         = 0x534,
CLA_OUTB_SOURCE_LIST_BASEL  = 0x538,
CLA_OUTB_SOURCE_LIST_BASEH  = 0x53C,
CLA_OUTB_WRITE_POINTER      = 0x540,
CLA_OUTB_COPY_POINTER       = 0x544,
CLA_OUTB_AVAL_COUNT         = 0x54C,
CLA_OUTB_READ_POINTER       = 0x548,
CLA_OUTB_INTF_CONF          = 0x550,
CLA_OUTB_INTF_CTL           = 0x554,

CLA_ISR_CAUSE               = 0x560,
CLA_ISR_MASK                = 0x564,

CLA_INTF_SEL0               = 0x570,
CLA_INTF_SEL1               = 0x574,
CLA_INTF_SEL2               = 0x578,
CLA_INTF_SEL3               = 0x57C,

CLA_INB_LIST_ERR_INF        = 0x590,
CLA_INB_LIST_ERR_ADDRL      = 0x594,
CLA_INB_LIST_ERR_ADDRH      = 0x598,
CLA_OUTB_LIST_ERR_INF       = 0x5A0,
CLA_OUTB_LIST_ERR_ADDRL     = 0x5A4,
CLA_OUTB_LIST_ERR_ADDRH     = 0x5A8,

INT_MAP_MU                  = (INT_MAP_DL_CPU2PCIEA | INT_MAP_COMAINT),

CL_POINTER_TOGGLE           = MV_BIT(12),

CLIC_IN_IRQ                 = MV_BIT(0),
CLIC_OUT_IRQ                = MV_BIT(1),
CLIC_IN_ERR_IRQ             = MV_BIT(8),
CLIC_OUT_ERR_IRQ            = MV_BIT(12),
CLIC_ASSERET_IRQ            = (CLIC_IN_IRQ | CLIC_OUT_IRQ | CLIC_IN_ERR_IRQ
								| CLIC_OUT_ERR_IRQ),

CL_SLOT_NUM_MASK            = 0xFFF,



/*******************************************/
/* ARM RESETl Registers Map			   */
/*******************************************/

CPU_RSTOUTN_EN_REG				= 0xF1400,
CPU_RSTOUTN_MASK_REG			= 0x20108,
CPU_SYS_SOFT_RST_REG			= 0x2010C,


CRMR_RST_OUT_DISABLE			= MV_BIT(24),
CRMR_SOFT_RST_OUT_MASK		= MV_BIT(2),
CSSRR_SYSTEM_SOFT_RST			= MV_BIT(0),


/*
* Command flag is the flag for the CDB command itself
*/
/* The first 16 bit can be determined by the initiator. */
CMD_FLAG_NON_DATA                 = MV_BIT(0),  /* 1-non data;
						0-data command */
CMD_FLAG_DMA                      = MV_BIT(1),  /* 1-DMA */
CMD_FLAG_PIO					  = MV_BIT(2),  /* 1-PIO */
CMD_FLAG_DATA_IN                  = MV_BIT(3),  /* 1-host read data */
CMD_FLAG_DATA_OUT                 = MV_BIT(4),	 /* 1-host write data */
CMD_FLAG_SMART                    = MV_BIT(5),  /* 1-SMART command;
						0-non SMART command*/

/*
* The last 16 bit only can be set by the target. Only core driver knows
* the device characteristic.
*/
CMD_FLAG_NCQ                      = MV_BIT(16),
CMD_FLAG_TCQ                      = MV_BIT(17),
CMD_FLAG_48BIT                    = MV_BIT(18),
CMD_FLAG_PACKET                   = MV_BIT(19),  /* ATAPI packet cmd */
CMD_FLAG_SCSI_PASS_THRU           = MV_BIT(20),
CMD_FLAG_ATA_PASS_THRU            = MV_BIT(21),


SCSI_CMD_MARVELL_SPECIFIC         = 0xE1,
CDB_CORE_MODULE                   = 0x1,
CDB_CORE_SOFT_RESET_1			  = 0x1,
CDB_CORE_SOFT_RESET_0			  = 0x2,
CDB_CORE_IDENTIFY                 = 0x3,
CDB_CORE_SET_UDMA_MODE            = 0x4,
CDB_CORE_SET_PIO_MODE             = 0x5,
CDB_CORE_ENABLE_WRITE_CACHE       = 0x6,
CDB_CORE_DISABLE_WRITE_CACHE      = 0x7,
CDB_CORE_ENABLE_SMART             = 0x8,
CDB_CORE_DISABLE_SMART            = 0x9,
CDB_CORE_SMART_RETURN_STATUS      = 0xA,
CDB_CORE_SHUTDOWN                 = 0xB,
CDB_CORE_ENABLE_READ_AHEAD        = 0xC,
CDB_CORE_DISABLE_READ_AHEAD       = 0xD,
CDB_CORE_READ_LOG_EXT             = 0xE,
CDB_CORE_TASK_MGMT                = 0xF,
CDB_CORE_SMP                      = 0x10,
CDB_CORE_PM_READ_REG			= 0x11,
CDB_CORE_PM_WRITE_REG			= 0x12,
CDB_CORE_RESET_DEVICE			= 0x13,
CDB_CORE_RESET_PORT				= 0x14,
CDB_CORE_OS_SMART_CMD			= 0x15,
SMP_CDB_USE_ADDRESS             = 0x01,



/* CDB definitions */
APICDB0_ADAPTER                        = 0xF0,
APICDB0_LD                             = 0xF1,
APICDB0_BLOCK                          = 0xF2,
APICDB0_PD                             = 0xF3,
APICDB0_EVENT                          = 0xF4,
APICDB0_DBG                            = 0xF5,
APICDB0_FLASH                          = 0xF6,

/* for Adapter */
APICDB1_ADAPTER_GETCOUNT               = 0,
APICDB1_ADAPTER_GETINFO                = (APICDB1_ADAPTER_GETCOUNT + 1),
APICDB1_ADAPTER_GETCONFIG              = (APICDB1_ADAPTER_GETCOUNT + 2),
APICDB1_ADAPTER_SETCONFIG              = (APICDB1_ADAPTER_GETCOUNT + 3),
APICDB1_ADAPTER_POWER_STATE_CHANGE     = (APICDB1_ADAPTER_GETCOUNT + 4),
APICDB1_ADAPTER_MAX                    = (APICDB1_ADAPTER_GETCOUNT + 5),

/* for LD */
APICDB1_LD_CREATE                      = 0,
APICDB1_LD_GETMAXSIZE                  = (APICDB1_LD_CREATE + 1),
APICDB1_LD_GETINFO                     = (APICDB1_LD_CREATE + 2),
APICDB1_LD_GETTARGETLDINFO             = (APICDB1_LD_CREATE + 3),
APICDB1_LD_DELETE                      = (APICDB1_LD_CREATE + 4),
APICDB1_LD_GETSTATUS                   = (APICDB1_LD_CREATE + 5),
APICDB1_LD_GETCONFIG                   = (APICDB1_LD_CREATE + 6),
APICDB1_LD_SETCONFIG                   = (APICDB1_LD_CREATE + 7),
APICDB1_LD_STARTREBUILD                = (APICDB1_LD_CREATE + 8),
APICDB1_LD_STARTCONSISTENCYCHECK       = (APICDB1_LD_CREATE + 9),
APICDB1_LD_STARTINIT                   = (APICDB1_LD_CREATE + 10),
APICDB1_LD_STARTMIGRATION              = (APICDB1_LD_CREATE + 11),
APICDB1_LD_BGACONTROL                  = (APICDB1_LD_CREATE + 12),
APICDB1_LD_WIPEMDD                     = (APICDB1_LD_CREATE + 13),
APICDB1_LD_GETSPARESTATUS              = (APICDB1_LD_CREATE + 14),
APICDB1_LD_SETGLOBALSPARE              = (APICDB1_LD_CREATE + 15),
APICDB1_LD_SETLDSPARE                  = (APICDB1_LD_CREATE + 16),
APICDB1_LD_REMOVESPARE                 = (APICDB1_LD_CREATE + 17),
APICDB1_LD_HD_SETSTATUS                = (APICDB1_LD_CREATE + 18),
APICDB1_LD_SHUTDOWN                    = (APICDB1_LD_CREATE + 19),
APICDB1_LD_HD_FREE_SPACE_INFO          = (APICDB1_LD_CREATE + 20),
APICDB1_LD_HD_GETMBRINFO               = (APICDB1_LD_CREATE + 21),
APICDB1_LD_SIZEOF_MIGRATE_TARGET       = (APICDB1_LD_CREATE + 22),
APICDB1_LD_TARGET_LUN_TYPE			   = (APICDB1_LD_CREATE + 23),
APICDB1_LD_HD_MPCHECK                  = (APICDB1_LD_CREATE + 24),
APICDB1_LD_HD_GETMPSTATUS              = (APICDB1_LD_CREATE + 25),
APICDB1_LD_HD_GET_RCT_COUNT            = (APICDB1_LD_CREATE + 26),
APICDB1_LD_HD_RCT_REPORT               = (APICDB1_LD_CREATE + 27),
APICDB1_LD_HD_START_DATASCRUB          = (APICDB1_LD_CREATE + 28),	// temp
APICDB1_LD_HD_BGACONTROL	           = (APICDB1_LD_CREATE + 29),	// temp
APICDB1_LD_HD_GETBGASTATUS             = (APICDB1_LD_CREATE + 30),	// temp
// Added reserved for future expansion so that MRU/CLI can be backward compatible with drier.
APICDB1_LD_RESERVED1				   = (APICDB1_LD_CREATE + 31),
APICDB1_LD_RESERVED2				   = (APICDB1_LD_CREATE + 32),
APICDB1_LD_RESERVED3				   = (APICDB1_LD_CREATE + 33),
APICDB1_LD_RESERVED4				   = (APICDB1_LD_CREATE + 34),
APICDB1_LD_RESERVED5				   = (APICDB1_LD_CREATE + 35),

APICDB1_LD_MAX                         = (APICDB1_LD_CREATE + 36),

/* for PD */
APICDB1_PD_GETHD_INFO                  = 0,
APICDB1_PD_GETEXPANDER_INFO            = (APICDB1_PD_GETHD_INFO + 1),
APICDB1_PD_GETPM_INFO                  = (APICDB1_PD_GETHD_INFO + 2),
APICDB1_PD_GETSETTING                  = (APICDB1_PD_GETHD_INFO + 3),
APICDB1_PD_SETSETTING                  = (APICDB1_PD_GETHD_INFO + 4),
APICDB1_PD_BSL_DUMP                    = (APICDB1_PD_GETHD_INFO + 5),
APICDB1_PD_RESERVED1				   = (APICDB1_PD_GETHD_INFO + 6),	// not used
APICDB1_PD_RESERVED2				   = (APICDB1_PD_GETHD_INFO + 7),	// not used
APICDB1_PD_GETSTATUS                   = (APICDB1_PD_GETHD_INFO + 8),
APICDB1_PD_GETHD_INFO_EXT              = (APICDB1_PD_GETHD_INFO + 9),	// APICDB1_PD_GETHD_INFO extension
APICDB1_PD_MAX                         = (APICDB1_PD_GETHD_INFO + 10),

/* Sub command for APICDB1_PD_SETSETTING */
APICDB4_PD_SET_WRITE_CACHE_OFF         = 0,
APICDB4_PD_SET_WRITE_CACHE_ON          = 1,
APICDB4_PD_SET_SMART_OFF               = 2,
APICDB4_PD_SET_SMART_ON                = 3,
APICDB4_PD_SMART_RETURN_STATUS         = 4,
APICDB4_PD_SET_SPEED_3G				   = 5,
APICDB4_PD_SET_SPEED_1_5G			   = 6,

/* for Block */
APICDB1_BLOCK_GETINFO                  = 0,
APICDB1_BLOCK_HD_BLOCKIDS              = (APICDB1_BLOCK_GETINFO + 1),
APICDB1_BLOCK_MAX                      = (APICDB1_BLOCK_GETINFO + 2),

/* for event */
APICDB1_HOST_GETEVENT                  = 1,
APICDB1_EVENT_GETEVENT                 = 0,
APICDB1_EVENT_MAX                      = (APICDB1_EVENT_GETEVENT + 1),

/* for DBG */
APICDB1_DBG_PDWR                       = 0,
APICDB1_DBG_MAP                        = (APICDB1_DBG_PDWR + 1),
APICDB1_DBG_MAX                        = (APICDB1_DBG_PDWR + 2),

/* for FLASH */
APICDB1_FLASH_BIN                      = 0,

/* for passthru commands
Cdb[0]: APICDB0_PASS_THRU_CMD_SCSI or APICDB0_PASS_THRU_CMD_ATA
Cdb[1]: APICDB1 (Data flow)
Cdb[2]: TargetID MSB
Cdb[3]: TargetID LSB
Cdb[4]-Cdb[15]: SCSI/ATA command is embedded here
	SCSI command: SCSI command Cdb bytes is in the same order as the spec
	ATA Command:
		Features = pReq->Cdb[0];
		Sector_Count = pReq->Cdb[1];
		LBA_Low = pReq->Cdb[2];
		LBA_Mid = pReq->Cdb[3];
		LBA_High = pReq->Cdb[4];
		Device = pReq->Cdb[5];
		Command = pReq->Cdb[6];

		if necessary:
		Feature_Exp = pReq->Cdb[7];
		Sector_Count_Exp = pReq->Cdb[8];
		LBA_Low_Exp = pReq->Cdb[9];
		LBA_Mid_Exp = pReq->Cdb[10];
		LBA_High_Exp = pReq->Cdb[11];
*/
APICDB0_PASS_THRU_CMD_SCSI			      = 0xFA,
APICDB0_PASS_THRU_CMD_ATA				  = 0xFB,

APICDB1_SCSI_NON_DATA					  = 0x00,
APICDB1_SCSI_PIO_IN						  = 0x01, /* goes with Read Long */
APICDB1_SCSI_PIO_OUT					  = 0x02, /* goes with Write Long */

APICDB1_ATA_NON_DATA					  = 0x00,
APICDB1_ATA_PIO_IN						  = 0x01,
APICDB1_ATA_PIO_OUT						  = 0x02,
};


#define SGD_DOMAIN_MASK	0xF0000000L

#define SGD_EOT			(1L<<27)	/* End of table */
#define SGD_COMPACT		(1L<<26)	/* Compact (12 bytes) SG format, not verified yet */
#define SGD_WIDE		(1L<<25)	/* 32 byte SG format */
#define SGD_X64			(1L<<24)	/* the 2nd part of SGD_WIDE */
#define SGD_NEXT_TBL	(1L<<23)	/* Next SG table format */
#define SGD_VIRTUAL		(1L<<22)	/* Virtual SG format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFTBL		(1L<<21)	/* sg table reference format, either 32 or 64 bit is determined during compile time. */
#define SGD_REFSGD		(1L<<20)	/* sg item reference format */
#define SGD_VP			(1L<<19)	/* virtual and physical, not verified yet */
#define SGD_VWOXCTX		(1L<<18)	/* virtual without translation context */
#define SGD_PCTX		(1L<<17)	/* sgd_pctx_t, 64 bit only */


struct mv_sgl {
	u32	baseaddr_l;
	u32 baseaddr_h;
	u32 flags;
	u32	size;
};

#define SIZEOF_COMPACT_SGD	12

struct mv_compact_sgl {
	u32	baseaddr_l;
	u32 baseaddr_h;
	u32	flags;
};

#define GET_COMPACT_SGD_SIZE(sgd)	\
	((((struct mv_compact_sgl*)(sgd))->flags) & 0x3FFFFFL)

#define SET_COMPACT_SGD_SIZE(sgd,sz) do {			\
	(((struct mv_compact_sgl*)(sgd))->flags) &= ~0x3FFFFFL;	\
	(((struct mv_compact_sgl*)(sgd))->flags) |= (sz);		\
} while(0)

#define sgd_getsz(sgd,sz) do {				\
	if( (sgd)->flags & SGD_COMPACT )		\
		(sz) = GET_COMPACT_SGD_SIZE(sgd);	\
	else (sz) = (sgd)->size;				\
} while(0)

#define sgd_setsz(sgd,sz) do {				\
	if( (sgd)->flags & SGD_COMPACT )		\
		SET_COMPACT_SGD_SIZE(sgd,sz);		\
	else (sgd)->size = (sz);				\
} while(0)


#define sgd_inc(sgd) do {	\
	if( (sgd)->flags & SGD_COMPACT )					\
		sgd = (struct mv_sgl *)(((unsigned char*) (sgd)) + 12);	\
	else if( (sgd)->flags & SGD_WIDE )					\
		sgd = (struct mv_sgl *)(((unsigned char*) (sgd)) + 32);	\
	else sgd = (struct mv_sgl *)(((unsigned char*) (sgd)) + 16);	\
} while(0)


struct mv_sgl_t {
	u8 max_entry;
	u8 valid_entry;
	u8 flag;
	u8 reserved0;
	u32 byte_count;
	struct mv_sgl* entry_ptr;
};


struct mvumi_res_mgnt {
	struct list_head res_entry;
	dma_addr_t bus_addr;
	void *virt_addr;
	u32 size;
	u16 type;          /* enum Resource_Type */
	u16 align;
};

/* Resource type */
enum resource_type {
	RESOURCE_CACHED_MEMORY = 0,
	RESOURCE_PCI_DMA_MEMORY,
	RESOURCE_UNCACHED_MEMORY
};

struct mvumi_hs_msg {
	void *data;
	u32 *msg;
	u32 *param;
	struct  list_head msg_list;
};

#define MSG_QUEUE_DEPTH	20
struct mvumi_msg_queue {
	spinlock_t lock;
	struct list_head free;
	struct list_head tasks;
	struct mvumi_hs_msg msgs[MSG_QUEUE_DEPTH];
};



/*
 * SCSI status
 */
#define SCSI_STATUS_GOOD                        0x00
#define SCSI_STATUS_CHECK_CONDITION             0x02
#define SCSI_STATUS_CONDITION_MET               0x04
#define SCSI_STATUS_BUSY                        0x08
#define SCSI_STATUS_INTERMEDIATE                0x10
#define SCSI_STATUS_INTERMEDIATE_MET            0x14
#define SCSI_STATUS_RESERVATION_CONFLICT        0x18
#define SCSI_STATUS_FULL                        0x28
#define SCSI_STATUS_ACA_ACTIVE                  0x30
#define SCSI_STATUS_ABORTED                     0x40


/*
 * SCSI sense key
 */
#define SCSI_SK_NO_SENSE                        0x00
#define SCSI_SK_RECOVERED_ERROR                 0x01
#define SCSI_SK_NOT_READY                       0x02
#define SCSI_SK_MEDIUM_ERROR                    0x03
#define SCSI_SK_HARDWARE_ERROR                  0x04
#define SCSI_SK_ILLEGAL_REQUEST                 0x05
#define SCSI_SK_UNIT_ATTENTION                  0x06
#define SCSI_SK_DATA_PROTECT                    0x07
#define SCSI_SK_BLANK_CHECK                     0x08
#define SCSI_SK_VENDOR_SPECIFIC                 0x09
#define SCSI_SK_COPY_ABORTED                    0x0A
#define SCSI_SK_ABORTED_COMMAND                 0x0B
#define SCSI_SK_VOLUME_OVERFLOW                 0x0D
#define SCSI_SK_MISCOMPARE                      0x0E
#ifdef _XOR_DMA
#define SCSI_SK_DMA					0x0F
#endif

/*
 * SCSI additional sense code
 */
#define SCSI_ASC_NO_ASC                         0x00
#define SCSI_ASC_LUN_NOT_READY                  0x04
#define SCSI_ASC_ECC_ERROR                      0x10
#define SCSI_ASC_ID_ADDR_MARK_NOT_FOUND         0x12
#define SCSI_ASC_INVALID_OPCODE                 0x20
#define SCSI_ASC_LBA_OUT_OF_RANGE               0x21
#define SCSI_ASC_INVALID_FEILD_IN_CDB           0x24
#define SCSI_ASC_LOGICAL_UNIT_NOT_SUPPORTED     0x25
#define SCSI_ASC_LOGICAL_UNIT_NOT_RESP_TO_SEL	0x05
#define SCSI_ASC_INVALID_FIELD_IN_PARAMETER     0x26
#define SCSI_ASC_INTERNAL_TARGET_FAILURE        0x44
#define SCSI_ASC_FAILURE_PREDICTION_THRESHOLD_EXCEEDED	0x5D


/*
 * SCSI additional sense code qualifier
 */
#define SCSI_ASCQ_NO_ASCQ                       0x00
#define SCSI_ASCQ_INTERVENTION_REQUIRED         0x03
#define SCSI_ASCQ_MAINTENANCE_IN_PROGRESS       0x80
#define SCSI_ASCQ_HIF_GENERAL_HD_FAILURE		0x10

struct mvumi_sense_data {
	u8 error_eode:7;
	u8 valid:1;
	u8 segment_number;
	u8 sense_key:4;
	u8 reserved:1;
	u8 incorrect_length:1;
	u8 end_of_media:1;
	u8 file_mark:1;
	u8 information[4];
	u8 additional_sense_length;
	u8 command_specific_information[4];
	u8 additional_sense_code;
	u8 additional_sense_code_qualifier;
	u8 field_replaceable_unit_code;
	u8 sense_key_specific[3];
};



#define REQ_STATUS_SUCCESS                      0x0
#define REQ_STATUS_NOT_READY                    0x1
#define REQ_STATUS_MEDIA_ERROR                  0x2
#define REQ_STATUS_BUSY                         0x3
#define REQ_STATUS_INVALID_REQUEST              0x4
#define REQ_STATUS_INVALID_PARAMETER            0x5
#define REQ_STATUS_NO_DEVICE                    0x6
/* Sense data structure is the SCSI "Fixed format sense datat" format. */
#define REQ_STATUS_HAS_SENSE                    0x7
#define REQ_STATUS_ERROR                        0x8
#define REQ_STATUS_ERROR_WITH_SENSE             0x10
/* Request initiator must set the status to REQ_STATUS_PENDING. */
#define REQ_STATUS_PENDING                      0x80
#define REQ_STATUS_RETRY                        0x81
#define REQ_STATUS_REQUEST_SENSE                0x82


struct mvumi_cmd {
	struct list_head queue_pointer;
	struct mvumi_msg_frame *frame;
	//struct mvumi_rsp_frame *rsp_frame;
	//dma_addr_t frame_phys_addr;
	//u8 *sense;
	//dma_addr_t sense_phys_addr;

	u32 index;
	u8 sync_cmd;
	u8 cmd_status;
	u16 abort_aen;
	struct timer_list eh_timer;
	struct scsi_cmnd *scmd;
	struct mv_hba *mhba;
	//u32 frame_count;
#if defined(DMA_WORK_AROUND)
	void *d_buf;
#endif
	//u8 os_cmd;
	void *data_buf;
};

#define MVUMI_IOC_FIRMWARE	_IOWR('M', 1, struct mvumi_iocpacket)
#define MVUMI_IOC_GET_AEN	_IOW('M', 3, struct mvumi_aen)


struct mvumi_pass_thru_direct {
	u16 length;
	u8 scsi_status;
	u8 path_id;
	u8 target_id;
	u8 lun;
	u8 cdb_length;
	u8 sense_info_length;
	u8 dataIn;
	u32 data_transfer_length;
	u32 timeout_value;
	void *data_buffer;
	u32 sense_info_offset;
	u8  cdb[16];
};


struct mvumi_pass_thru_direct_with_buff {
	struct mvumi_pass_thru_direct sptd;
	unsigned long filler;
	u8  sense_buffer[SENSE_INFO_BUFFER_SIZE];
};

#define MVUMI_IOCTL_DEFAULT_FUN		0x00
#define	MVUMI_IOCTL_GET_VIRTURL_ID	(MVUMI_IOCTL_DEFAULT_FUN + 1)
#define	MVUMI_IOCTL_GET_HBA_COUNT	(MVUMI_IOCTL_DEFAULT_FUN + 2)
#define	MVUMI_IOCTL_LOOKUP_DEV		(MVUMI_IOCTL_DEFAULT_FUN + 3)
#define MVUMI_IOCTL_MAX			(MVUMI_IOCTL_DEFAULT_FUN + 4)

#if 0
#define MVUMI_IOCTL_API		_IOWR('M', 1, struct mvumi_pass_thru_direct_with_buff)
#define	MVUMI_IOCTL_LOOKUP_DEV	_IOW('M', 2, struct scsi_idlun)
#define	MVUMI_IOCTL_GET_INFO	_IOW('M', 3, struct mv_hba)
#endif

struct sas_mgmt_info {

	u16 count;
	struct mv_hba *mhba[MAX_MGMT_ADAPTERS];
	int max_index;
};


/*
 * the function type of the in bound frame
 */
#define CL_FUN_SCSI_CMD    0x1
#define CL_FUN_BIOS_CMD    0x2
#define CL_FUN_API_CMD    0x3

#define MVUMI_IOCTL_DATA_OUT          0
#define MVUMI_IOCTL_DATA_IN           1
#define MVUMI_IOCTL_DATA_UNSPECIFIED  2

struct mvumi_msg_frame
{
	u16 device_id;
	u16 tag;

	u8 cmd_flag;
	u8 req_function;
	u8 cdb_length;
	u8 sg_counts;

	u32 data_transfer_length;
	u32 reserved_1;

	u8 cdb[MAX_CDB_SIZE];

	u32 payload[1];
	//u32 reserved_2[119];

};

/*
 * the respond flag for data_payload of the out bound frame
 */
#define CL_RSP_FLAG_NODATA    0x0
#define CL_RSP_FLAG_SENSEDATA    0x1

struct mvumi_rsp_frame
{
	u16 device_id;
	u16 tag;


	u8 req_status; 	/* driver shall translate it into OS specific */
	u8 rsp_flag; /* Indicates the type of Data_Payload.*/
	u8 reserved[2];
	u32 payload[1];

};

/*
 * State is the state of the MU
 */
#define FW_STATE_IDLE           0
#define FW_STATE_STARTING       1
#define FW_STATE_HANDSHAKING    2
#define FW_STATE_STARTED        3
#define FW_STATE_ABORT          4

/*
 * State is the work around status of the CL
 */
#if defined(LIST_WORK_AROUND)
#define MU_WA_IB_LIST   1           /* work around for InBound List */
#define MU_WA_OB_LIST   2           /* work around for OutBound List */
#endif

struct mv_ob_data_pool {
	struct list_head 		queue_pointer;
	u8 ob_data[CL_OUT_ENTRY_SIZE];
} ;

struct version_info {
	u32 ver_major;
	u32 ver_minor;
	u32 ver_oem;
	u32 ver_build;
};


struct mv_handshake_frame {
	u16 size; /* size of myself, could be used for version control */

	/* host information */
	u8 host_type; 	/* BIOS, Windows, Linux, Endianess etc */
	u8 reserved_1[1];
	struct version_info host_ver; /* bios or driver version */

	/* controller information */
	u32 system_io_bus;
	u32 slot_number;
	u32 intr_level;
	u32 intr_vector;

	/* communication list configuration */
	u32 ib_baseaddr_l;
	u32 ib_baseaddr_h;
	u32 ob_baseaddr_l;
	u32 ob_baseaddr_h;

	u8 ib_entry_size; /* refer to SPEC, 0=1DW, 1=2DW, 7=128DW */
	u8 ob_entry_size;/* refer to SPEC, 0=1DW, 1=2DW, 7=128DW */
	u8 ob_depth;
	u8 ib_depth;

	/* system date/time */
	u64 seconds_since1970;
};

struct mv_handshake_header {
	u8 page_code;
	u8 checksum;
	u16	frame_length;
	u32	frame_content[1];
};

/*
 * the page code type of the handshake header
 */
#define HS_PAGE_FIRM_CAP    0x1
#define HS_PAGE_HOST_INFO   0x2
#define HS_PAGE_FIRM_CTL    0x3
#define HS_PAGE_CL_INFO     0x4
#define HS_PAGE_TOTAL       0x5

/* code for hs_capability */
#define HS_CAPABILITY_CACHE_SUPPORTED		MV_BIT(0)
#define HS_CAPABILITY_EVENT_NOTIFICATION	MV_BIT(1)
#define HS_CAPABILITY_HOST_MEM_REQUIRED		MV_BIT(2)
#define HS_CAPABILITY_LIST_WORKAROUND		MV_BIT(3)
#define HS_CAPABILITY_SUPPORT_COMPACT_SG	MV_BIT(4)

#define HSP_SIZE(i)					\
		sizeof(struct mv_handshake_page##i)

#define HSP_MAX_SIZE ({					\
	int size, m1, m2;				\
	m1 = max(HSP_SIZE(1), HSP_SIZE(3));		\
	m2 = max(HSP_SIZE(2), HSP_SIZE(4));		\
	size = max(m1, m2);				\
	size;						\
})

/* The format of the page code for Firmware capability */
struct mv_handshake_page1 {
	u8 pagecode;
	u8 checksum;
	u16	frame_length;

	u16	number_of_ports;
	u16	max_devices_support;
	u16	max_io_support;
	u16	umi_ver;
	u32	max_transfer_size;
	struct version_info fw_ver;
	u8	cl_in_max_entry_size;
	u8	cl_out_max_entry_size;
	u8	cl_inout_list_depth;
	u8	total_pages;
	u16	capability;
	u16	reserved1;
};


/* The format of the page code for Host information */
struct mv_handshake_page2 {
	u8 pagecode;
	u8 checksum;
	u16	frame_length;

	u8 host_type;
	u8 reserved[3];
	struct version_info	host_ver;
	u32	system_io_bus;
	u32	slot_number;
	u32	intr_level;
	u32	intr_vector;
	u64	seconds_since1970;
};

/*
 * define the OS type of the Host side
 */
#define MV_OS_TYPE_BIOS     1
#define MV_OS_TYPE_WINDOWS  2
#define MV_OS_TYPE_LINUX    3


/* The format of the page code for firmware control  */
struct mv_handshake_page3 {
	u8 pagecode;
	u8 checksum;
	u16	frame_length;
	u16	control;
	u8 reserved[2];
	u32	host_bufferaddr_l;
	u32	host_bufferaddr_h;
	u32	host_eventaddr_l;
	u32	host_eventaddr_h;
};


struct mv_handshake_page4 {
	u8 pagecode;
	u8 checksum;
	u16 frame_length;
	u32	ib_baseaddr_l;
	u32	ib_baseaddr_h;
	u32	ob_baseaddr_l;
	u32	ob_baseaddr_h;
	u8 ib_entry_size;
	u8 ob_entry_size;
	u8 ob_depth;
	u8 ib_depth;
};

#define HANDSHAKE_SIGNATURE     0x5A5A5A5AL
#define HANDSHAKE_READYSTATE    0x55AA5AA5L
#define HANDSHAKE_DONESTATE     0x55AAA55AL

/* HandShake Status definition */
#define HS_STATUS_OK        1       /* Indicates communication OK. */
#define HS_STATUS_ERR       2       /* Indicates communication with error. */
#define HS_STATUS_INVALID   3       /* Indicates received a invalid data. */

/* HandShake State/Cmd definition */
#define HS_S_ABORT              7
#define HS_S_END                6   /* Indicates request with data in Msg1. */
#define HS_S_SEND_PAGE          5   /* Notifies the other side to finish handshake procedure. */
#define HS_S_QUERY_PAGE         4
#define HS_S_PAGE_ADDR          3   /* Notifies the other side to finish handshake procedure. */
#define HS_S_RESET              2   /* Notifies the other side to restart handshake procedure. */
#define HS_S_START              1   /* Starts the handshake procedure. */
#define HS_PAGE_VERIFY_SIZE     128

#define HS_GET_STATE(a)         (a & 0xFFFF)
#define HS_GET_STATUS(a)        ((a & 0xFFFF0000) >> 16)
#define HS_SET_STATE(a, b)      (a |= (b & 0xFFFF))
#define HS_SET_STATUS(a, b)     (a |= ((b & 0xFFFF) << 16))
#define HS_SET_CHECKSUM(a, b)   mvumi_calculate_checksum(a, b)

/****************************************
 *         Perceived Severity
 ****************************************/

#define SEVERITY_UNKNOWN    0
#define SEVERITY_OTHER      1
#define SEVERITY_INFO       2
#define SEVERITY_WARNING    3  /* used when its appropriate to let the
				  user decide if action is needed */
#define SEVERITY_MINOR      4  /* indicate action is needed, but the
				  situation is not serious at this time */
#define SEVERITY_MAJOR      5  /* indicate action is needed NOW */
#define SEVERITY_CRITICAL   6  /* indicate action is needed NOW and the
				  scope is broad */
#define SEVERITY_FATAL      7  /* indicate an error occurred, but it's too
				  late to take remedial action */

#define DEVICE_OFFLINE	0
#define DEVICE_ONLINE	1

/* Module event type */
enum mvumi_module_event {
	EVENT_MODULE_ALL_STARTED = 0,
	EVENT_DEVICE_CACHE_MODE_CHANGED,
	EVENT_DEVICE_ARRIVAL,
	EVENT_DEVICE_REMOVAL,
	EVENT_LOG_GENERATED,
};

struct mvumi_hotplug_event
{
	u16	size;
	u8	dummy[2];
	u8	bitmap[0];
};

struct mvumi_notif_param {
    void *p_param;
    u16    hi;
    u16    lo;

    /* for event processing */
    u32    event_id;
    u16    dev_id;
    u8     severity_lvl;
    u8     param_count;
};


struct mvumi_driver_event {
	u32 time_stamp;
	u32 sequence_no; /* (contiguous in a single adapter) */
	u32 event_id;    /* 1st 16 bits - Event class */
                            /* last 16 bits - Event code of this particular
			       Event class */
	u8 severity;
	u8 adapter_id;
	u16 device_id;   /* Device ID relate to the event
			       class (HD ID, LD ID etc) */
	u32 params[MAX_EVENT_PARAMS]; /* Additional information if
					     ABSOLUTELY necessary. */
};

// Event support sense code
struct mvumi_driver_event_v2
{
    struct mvumi_driver_event  event_v1;                /* same as the current one */
    u8        sense_data_length;      /* actual length of SenseData.  Driver set it to 0 if no SenseData */
    u8        Reserved1;
    u8        sense_data[30];        /* (24+6) just for making this structure on 64-bits boundary */
};

struct mvumi_event_req {
	u8 count; /* [OUT] # of actual events returned */
	u8 reserved[3];
	struct mvumi_driver_event_v2  events[MAX_EVENTS_RETURNED];
};

struct mvumi_event_entry {
	struct list_head queue_pointer;
	struct mvumi_driver_event_v2 event;
};

struct mvumi_events_wq {
	struct work_struct work_q;
	struct mv_hba *mhba;
	enum mvumi_module_event event;
	void *param;
};

#define MVUMI_DEV_INDEX(id, lun)	(((u16)(id)) | (((u16)(lun)) << 8))
#define DEV_ID_TO_TARGET_ID(_dev_id)	((u8)((_dev_id) & 0x00FF))
#define DEV_ID_TO_LUN(_dev_id)		((u8) (((_dev_id) & 0xFF00) >> 8))

static inline struct list_head *List_GetFirst(struct list_head *head)
{
    struct list_head * one = NULL;
    if (list_empty(head))
	return NULL;

    one = head->next;
    list_del(one);
    return one;
}

static inline struct list_head *list_getlast(struct list_head *head)
{
    struct list_head * one = NULL;
    if (list_empty(head))
	return NULL;

    one = head->prev;
    list_del(one);
    return one;
}

#define list_get_first_entry(head, type, member)	\
	list_entry(List_GetFirst(head), type, member)

#define list_get_last_entry(head, type, member)	\
	list_entry(list_getlast(head), type, member)

struct mv_hba;
struct mvumi_instance_template {
	void (*fire_cmd)(struct mv_hba *, struct mvumi_cmd *);
	void (*enable_intr)(void *) ;
	void (*disable_intr)(void *);
	int (*clear_intr)(void *);
	u32 (*read_fw_status_reg)(void *);
};

struct tag_stack {
	u16 *stack;
	u16 top;
	u16 size;
	u16 ptr_out;
	u8 tag_stack_type;
	u8 reserved[1];
};


#define MAX_HS_SIZE	(512 * 1024)
struct mv_hba {
	u32 *reply_queue;
	dma_addr_t reply_queue_h;

	void *base_addr[MAX_BASE_ADDRESS];	/* Internal register memory base address */
	void *mmio;
	s8 init_id;
	u8 host_id;
	dev_t dev_num;
	struct cdev mv_cdev;

	struct mvumi_cmd **cmd_list;
	struct list_head cmd_pool;
	spinlock_t cmd_pool_lock;
	struct semaphore ioctl_sem;

	struct Scsi_Host *shost;

	wait_queue_head_t int_cmd_wait_q;
	wait_queue_head_t abort_cmd_wait_q;
	wait_queue_head_t handshake_wait_q;
	struct completion handshake_cmpl;
	struct pci_dev *pdev;
	u32 unique_id;

	atomic_t fw_outstanding;
	u32 hw_crit_error;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
	atomic_t  hba_sync;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */

	struct mvumi_instance_template *instancet;
#ifdef  SUPPORT_TASKLET
	struct tasklet_struct isr_tasklet;
#endif
	u8 flag;
	unsigned long last_time;

	u8 adapter_number;

	void *ib_list;
	dma_addr_t ib_list_phys;

	void *ob_list;
	dma_addr_t ob_list_phys;

	void *ib_shadow;
	dma_addr_t ib_shadow_phys;

	void *ob_shadow;
	dma_addr_t ob_shadow_phys;

	void *handshake_page_base;
	dma_addr_t handshake_page_phys_base;
	void *handshake_page;
	dma_addr_t handshake_page_phys;

	spinlock_t mv_isr_lock;
	u32 global_isr;
	u32 mv_isr_status;

#if defined(DMA_WORK_AROUND)
	void *dma_pool;
	dma_addr_t dma_pool_phys;
#endif
#if LINUX_VERSION_CODE <=KERNEL_VERSION(2, 6, 9)
	u32 pmstate[16];
#endif
	u32 ib_cur_count;

#if defined(SUPPORT_INTERNAL_REQ)
	u32 max_int_io;
	struct mvumi_cmd **int_cmd_list;
	struct list_head int_cmd_pool;
	spinlock_t int_cmd_pool_lock;
	struct dma_pool *int_frame_dma_pool;
	struct dma_pool *int_sense_dma_pool;
	void *int_req_buf;
	dma_addr_t int_req_phys;
	void *event_int_req_buf;
	dma_addr_t event_int_req_phys;
	u32	int_req_size;
	spinlock_t event_list_lock;
	void *event_pool_start;
	void *event_pool_end;
#endif

	/* firmware capacity */
	u16 max_num_sge;
	u32 max_io;
	u32 list_num_io;
	u16	max_target_id;
	u32 ib_max_entry_size_bytes;
	u32 ob_max_entry_size_bytes;
	u32 ib_max_entry_size_setting;
	u32 ob_max_entry_size_setting;
	u32 max_transfer_size;
	u8 hba_total_pages;
	u16	hba_capability;		/* see HS_CAPABILITY_XXX */


	/* firmware setting */
	u32 ib_cur_slot;
	u32 ob_cur_slot;
	u32 fw_state;


	struct list_head ob_data_pool_list;
	struct list_head free_ob_list;
#if defined(LIST_WORK_AROUND)
	u32	list_workaround_flag;
#endif
	struct list_head res_list;
	struct timer_list hs_timer;

	struct list_head waiting_req_list;	/* Waiting queue */
	struct list_head complete_req_list;		/* Free queue */
	spinlock_t  mv_timer_lock;

	/* event */
	struct list_head stored_events;
	struct list_head free_events;
	struct list_head events_pool;
	u8 num_stored_events;
	u16 num_pool_events;
	u8 reserved_2[3];	/* make the structure 8 byte aligned */
	/* struct work_struct work_q; */	/* polling all events */
	struct mvumi_msg_queue msg_q;
	unsigned short	chip_device_id;

	struct tag_stack tag_pool;
	u16 tag_num[MVUMI_MAX_REQUEST_NUMBER + MAX_INTERNAL_REQ];
	struct mvumi_cmd *tag_cmd[MVUMI_MAX_REQUEST_NUMBER + MAX_INTERNAL_REQ];
	struct mutex	sas_discovery_mutex;
	spinlock_t mv_tag_lock;
	spinlock_t mv_waiting_req_lock;
};

#define is_workround_dev(h)	\
	(h->hba_capability & HS_CAPABILITY_LIST_WORKAROUND)
#define IS_CHIP_8180(h)	((h->pdev->device == PCI_DEVICE_ID_MARVELL_MV8180)||\
			(h->pdev->device == PCI_DEVICE_ID_MARVELL_MV8120) ||\
			(h->pdev->device == PCI_DEVICE_ID_MARVELL_MV8110))
#define _IS_CHIP_8180(h)	(h->pdev->device == PCI_DEVICE_ID_MARVELL_MV8180)
#define IS_CHIP_VILI(h)	((h->pdev->device == PCI_DEVICE_ID_MARVELL_MV8120) ||(h->pdev->device == PCI_DEVICE_ID_MARVELL_MV8110))

struct mvumi_direct {
	unsigned short length;
	unsigned char  scsi_status;
	unsigned char  path_id;
	unsigned char  target_id;
	unsigned char  lun;
	unsigned char  cdb_length;
	unsigned char  sense_info_length;
	unsigned char  datain;
	unsigned long  data_transfer_length;
	unsigned long  timeout_value;
	void           *data_buffer;
	unsigned long  sense_info_offset;
	unsigned char  cdb[16];
};

struct mvumi_scsi_buffer{
	struct mvumi_direct	sptd;
	unsigned long		filler;
	unsigned char		sense_buffer[SENSE_INFO_BUFFER_SIZE];
};

#ifdef MV_SIMLATION
typedef struct _Link_Endpoint
{
	u16      DevID;
	u8       DevType;         /* Refer to DEVICE_TYPE_xxx, (additional
					type like EDGE_EXPANDER and
					FANOUT_EXPANDER might be added). */
	u8       PhyCnt;          /* Number of PHYs for this endpoint.
					Greater than 1 if it is wide port. */
	u8       PhyID[8];    /* Assuming wide port has
						    max of 8 PHYs. */
	u8       SAS_Address[8];  /* Filled with 0 if not SAS device. */
	u8       Reserved1[8];
} Link_Endpoint, * PLink_Endpoint;

typedef struct _Link_Entity
{
	Link_Endpoint    Parent;
	u8            Reserved[8];
	Link_Endpoint    Self;
} Link_Entity,  *PLink_Entity;


typedef struct _HD_Info
{
	Link_Entity     Link;             /* Including self DevID & DevType */
	u8           AdapterID;
	u8           Status;           /* Refer to HD_STATUS_XXX */
	u8           HDType;           /* HD_Type_xxx, replaced by new driver with ConnectionType & DeviceType */
	u8           PIOMode;		  /* Max PIO mode */
	u8           MDMAMode;		  /* Max MDMA mode */
	u8           UDMAMode;		  /* Max UDMA mode */
	u8           ConnectionType;	  /* DC_XXX, ConnectionType & DeviceType in new driver to replace HDType above */
	u8           DeviceType;	      /* DT_XXX */

	u32          FeatureSupport;   /* Support 1.5G, 3G, TCQ, NCQ, and
					     etc, MV_BIT related */
	u8           Model[40];
	u8           SerialNo[20];
	u8           FWVersion[8];
	u64          Size;             /* unit: 1KB */
	u8           WWN[8];          /* ATA/ATAPI-8 has such definitions
					     for the identify Buffer */
	u8           CurrentPIOMode;		/* Current PIO mode */
	u8           CurrentMDMAMode;	/* Current MDMA mode */
	u8           CurrentUDMAMode;	/* Current UDMA mode */
	u8			Reserved3[5];
//	u32			FeatureEnable;

	u8           Reserved4[80];
}HD_Info, *PHD_Info;

#endif


#endif				/*MV_RAID_UMI_H */
