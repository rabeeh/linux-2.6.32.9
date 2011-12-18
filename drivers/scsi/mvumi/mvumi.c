/*
 *
 *		Marvell Linux driver for SAS based RAID controllers
 *
 * Copyright (c) 2006-2008  MARVELL SEMICONDUCTOR, LTD.
 *
 *	   This program is free software; you can redistribute it and/or
 *	   modify it under the terms of the GNU General Public License
 *	   as published by the Free Software Foundation; either version
 *	   2 of the License, or (at your option) any later version.
 *
 * FILE		: mvumi.c
 * Version	: 01.00.00.01-rc1
 *
 * Authors:
 *
 * List of supported controllers
 *
 * OEM	Product Name			VID	DID	SSVID	SSID
 * ---	------------			---	---	----	----
 */
#ifndef LINUX_VERSION_CODE
#	include <linux/version.h>
#endif

#ifndef AUTOCONF_INCLUDED
#	include <linux/config.h>
#endif /* AUTOCONF_INCLUDED */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uio.h>
#include <linux/fs.h>
#include <linux/compat.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/nmi.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_ioctl.h>

#include <linux/idr.h>
#include "mvumi.h"

MODULE_LICENSE("GPL");
MODULE_VERSION(MVUMI_VERSION);
MODULE_AUTHOR("linux@marvell.com");
MODULE_DESCRIPTION("Marvell RAID On Controller SAS Driver");

#define _MVUMI_DUMP 0

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#define mv_use_sg(cmd)	cmd->use_sg
#define mv_rq_bf(cmd)	cmd->request_buffer
#define mv_rq_bf_l(cmd)	cmd->request_bufflen
#else
#define mv_use_sg(cmd)	scsi_sg_count(cmd)
#define mv_rq_bf(cmd)	scsi_sglist(cmd)
#define mv_rq_bf_l(cmd)	scsi_bufflen(cmd)
#endif

static DEFINE_MUTEX(sysfs_lock);
/*
 * PCI ID table for all supported controllers
 */
static struct pci_device_id mvumi_pci_table[] = {
	/* Flash Controller */
	{PCI_DEVICE(PCI_VENDOR_ID_MARVELL, PCI_DEVICE_ID_MARVELL_MV8120)},
	{PCI_DEVICE(PCI_VENDOR_ID_MARVELL, PCI_DEVICE_ID_MARVELL_MV8110)},
	{0}
};

MODULE_DEVICE_TABLE(pci, mvumi_pci_table);


static struct sas_mgmt_info mvumi_mgmt_info;


static u32 mvumi_dbg_lvl=1;
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
/* will wait for atomic value atomic to become zero until timed out */
/* return how much 'timeout' is left or 0 if already timed out */
int mvumi_wait_for_atomic_timeout(atomic_t * atomic, unsigned long timeout)
{
	unsigned intv = HZ / 20;

	while (timeout) {
		if (0 == atomic_read(atomic))
			break;

		if (timeout < intv)
			intv = timeout;
		set_current_state(TASK_INTERRUPTIBLE);
		timeout -= (intv - schedule_timeout(intv));
	}
	return timeout;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11) */
#ifdef MV_DEBUG
static const char *mvumi_dump_sense_key(u8 sense)
{
	switch (sense) {
	case SCSI_SK_NO_SENSE:
		return "SCSI_SK_NO_SENSE";
	case SCSI_SK_RECOVERED_ERROR:
		return "SCSI_SK_RECOVERED_ERROR";
	case SCSI_SK_NOT_READY:
		return "SCSI_SK_NOT_READY";
	case SCSI_SK_MEDIUM_ERROR:
		return "SCSI_SK_MEDIUM_ERROR";
	case SCSI_SK_HARDWARE_ERROR:
		return "SCSI_SK_HARDWARE_ERROR";
	case SCSI_SK_ILLEGAL_REQUEST:
		return "SCSI_SK_ILLEGAL_REQUEST";
	case SCSI_SK_UNIT_ATTENTION:
		return "SCSI_SK_UNIT_ATTENTION";
	case SCSI_SK_DATA_PROTECT:
		return "SCSI_SK_DATA_PROTECT";
	case SCSI_SK_BLANK_CHECK:
		return "SCSI_SK_BLANK_CHECK";
	case SCSI_SK_VENDOR_SPECIFIC:
		return "SCSI_SK_VENDOR_SPECIFIC";
	case SCSI_SK_COPY_ABORTED:
		return "SCSI_SK_COPY_ABORTED";
	case SCSI_SK_ABORTED_COMMAND:
		return "SCSI_SK_ABORTED_COMMAND";
	case SCSI_SK_VOLUME_OVERFLOW:
		return "SCSI_SK_VOLUME_OVERFLOW";
	case SCSI_SK_MISCOMPARE:
		return "SCSI_SK_MISCOMPARE";
	default:
		mv_printk( "Unknown sense key 0x%x.\n", sense);
		return "Unknown sense key";
	}
}

static void mvumi_dump_cmd_info(struct mvumi_msg_frame *frame, u8 detail)
{
	u8 i;
	struct mv_sgl *m_sg = (struct mv_sgl *)&frame->payload[0];

	mv_printk("Device %d,Cdb[%2x,%2x,%2x,%2x, %2x,%2x,%2x,%2x,%2x,%2x,%2x,"
		  "%2x,%2x,%2x,%2x,%2x].\n", frame->device_id,
		  frame->cdb[0], frame->cdb[1], frame->cdb[2], frame->cdb[3],
		  frame->cdb[4], frame->cdb[5], frame->cdb[6], frame->cdb[7],
		  frame->cdb[8], frame->cdb[9], frame->cdb[10], frame->cdb[11],
		  frame->cdb[12], frame->cdb[13], frame->cdb[14],
		  frame->cdb[15]);

		mv_printk("tag=0x%x, flag=0x%x, fun=0x%x, cdb_len=0x%x,"
			"sg_cnts=0x%x, transfer_len=0x%x.\n",
			frame->tag,
			frame->cmd_flag, frame->req_function,
			frame->cdb_length, frame->sg_counts,
			frame->data_transfer_length);

	if (detail) {
		for (i = 0; i < frame->sg_counts; i++) {
			mv_printk("sg[%d][0x%p] baseh[0x%x],basel[0x%x],"
				"size[0x%x],flag[0x%x].\n\n",
				i, m_sg, m_sg->baseaddr_h, m_sg->baseaddr_l,
				m_sg->size, m_sg->flags);
			sgd_inc(m_sg);
		}
	}
}

static void mvumi_dump_ob_info(struct mvumi_rsp_frame *frame)
{
	mv_printk("Device %d:ob frame tag=0x%x,flag=0x%x, status=0x%x.\n",
		  frame->device_id, frame->tag,
		  frame->rsp_flag, frame->req_status);
}
#endif

/* if tag_stack_type!=FIFO_TAG, use FILO,
*  if tag_stack_type==FIFO_TAG, use FIFO, ptr_out is the next
*  tag to get and top is the number of available tags in the
*  stack when use FIFO, get tag from ptr_out and free tag to
* (ptr_out+top)%size
*/

#define FILO_TAG 0x00
#define FIFO_TAG 0x01

static void tag_init(struct tag_stack *st, u16 size)
{
	u16 i;
	MV_ASSERT(size == st->size);
	st->top = size;
	st->tag_stack_type = FILO_TAG;
	st->ptr_out = 0;
	for (i = 0; i < size; i++) {
		st->stack[i] = size - 1 - i;
	}
}

static u16 tag_get_one(struct mv_hba *mhba, struct tag_stack *st)
{
	u16 n_tag, tag;
	unsigned long flags;
	spin_lock_irqsave(&mhba->mv_tag_lock, flags);
	MV_ASSERT(st->top > 0);
	if (st->tag_stack_type == FIFO_TAG) {
		n_tag = st->stack[st->ptr_out++];
		if (st->ptr_out >= st->size)
			st->ptr_out = 0;
		st->top--;
		tag = n_tag;
	} else
		tag = st->stack[--st->top];
	spin_unlock_irqrestore(&mhba->mv_tag_lock, flags);
	return tag;
}

static void tag_release_one(struct mv_hba *mhba, struct tag_stack *st, u16 tag)
{
	unsigned long flags;
	spin_lock_irqsave(&mhba->mv_tag_lock, flags);
	MV_ASSERT(st->top < st->size);
	if (st->tag_stack_type == FIFO_TAG) {
		st->stack[(st->ptr_out + st->top) % st->size] = tag;
		st->top++;
	} else
		st->stack[st->top++] = tag;
	spin_unlock_irqrestore(&mhba->mv_tag_lock, flags);
}

static u8 tag_is_empty(struct tag_stack *st)
{
	if (st->top == 0) {
		return 1;
	}
	return 0;
}

#if 0
static void *mvumi_sgd_kmap(struct scatterlist *ksg, struct mvumi_cmd *cmd)
{
	void *kvaddr = NULL;
	MV_ASSERT(cmd->scmd);	/* must OS command */
	//kvaddr = page_address(ksg->page);
	//if (!kvaddr)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	kvaddr = kmap_atomic(ksg->page, KM_IRQ0);
#else
	kvaddr = kmap_atomic(sg_page(ksg), KM_IRQ0);
#endif
	kvaddr += ksg->offset;
	return kvaddr;
}

static void
mvumi_sgd_kunmap(struct scatterlist *ksg,
		 struct mvumi_cmd *cmd, void *mapped_addr)
{
	//void *kvaddr = NULL;
	MV_ASSERT(cmd->scmd);	/* must OS command */
	//kvaddr = page_address(ksg->page);
	//if (!kvaddr)
	kunmap_atomic(mapped_addr - ksg->offset, KM_IRQ0);
}
#endif

static int mvumi_map_pci_addr(struct pci_dev *dev, void **addr_array)
{
	int i;
	unsigned long addr;
	unsigned long range;

	for (i = 0; i < MAX_BASE_ADDRESS; i++) {
		addr = pci_resource_start(dev, i);
		range = pci_resource_len(dev, i);

		if (pci_resource_flags(dev, i) & IORESOURCE_MEM) {
			addr_array[i] = (void *)ioremap_nocache(addr, range);
			if (!addr_array[i]) {
				mv_printk(" Failed to map IO mem\n");
				return -1;
			}
		} else {
			addr_array[i] = (void *)addr;
		}
		mv_printk("BAR %d : %p.\n", i, addr_array[i]);
	}
	return 0;
}

static void mvumi_unmap_pci_addr(struct pci_dev *dev, void **addr_array)
{
	int i;

	for (i = 0; i < MAX_BASE_ADDRESS; i++)
		if (pci_resource_flags(dev, i) & IORESOURCE_MEM)
			iounmap(addr_array[i]);
}

static struct mvumi_res_mgnt *mvumi_alloc_mem_resource(struct mv_hba *mhba,
						       enum resource_type type,
						       u32 size)
{
	struct mvumi_res_mgnt *res_mgnt =
			vmalloc(sizeof(struct mvumi_res_mgnt));
	if (NULL == res_mgnt) {
		mv_printk(" Failed to allocate memory for mod_res.\n");
		return NULL;
	}
	memset(res_mgnt, 0, sizeof(sizeof(struct mvumi_res_mgnt)));

	switch (type) {
	case RESOURCE_CACHED_MEMORY:
		res_mgnt->virt_addr = vmalloc(size);
		if (NULL == res_mgnt->virt_addr) {
			mv_printk("unable to alloc 0x%x mem.\n", size);
			vfree(res_mgnt);
			return NULL;
		}
		break;

	case RESOURCE_UNCACHED_MEMORY:
		size = ROUNDING(size, 8);
		res_mgnt->virt_addr = (void *)pci_alloc_consistent(mhba->pdev,
								   size,
								   &res_mgnt->
								   bus_addr);
		if (NULL == res_mgnt->virt_addr) {
			mv_printk("unable to alloc 0x%x consistent mem.\n",
				  size);
			vfree(res_mgnt);
			return NULL;
		}
		break;

	default:
		mv_printk("res type %d unknown.\n", type);
		vfree(res_mgnt);
		return NULL;
	}

	memset(res_mgnt->virt_addr, 0, size);
	res_mgnt->type = type;
	res_mgnt->size = size;
	list_add_tail(&res_mgnt->res_entry, &mhba->res_list);

	return res_mgnt;
}

static void mvumi_release_mem_resource(struct mv_hba *mhba)
{
	struct mvumi_res_mgnt *res_mgnt, *tmp;

	list_for_each_entry_safe(res_mgnt, tmp, &mhba->res_list, res_entry) {
		switch (res_mgnt->type) {
		case RESOURCE_UNCACHED_MEMORY:
			pci_free_consistent(mhba->pdev,
					    res_mgnt->size,
					    res_mgnt->virt_addr,
					    res_mgnt->bus_addr);
			break;
		case RESOURCE_CACHED_MEMORY:
			vfree(res_mgnt->virt_addr);
			break;
		default:
			mv_printk("res type %d unknown.\n", res_mgnt->type);
			break;
		}
		list_del(&res_mgnt->res_entry);
		vfree(res_mgnt);
	}
	mhba->flag &= ~MVUMI_FW_ALLOC;
}

/**
 * mvumi_make_sgl -	Prepares  SGL
 * @lhba:		Adapter soft state
 * @scmd:		SCSI command from the mid-layer
 * @m_sg:		SGL to be filled in
 *
 * If successful, this function returns the number of SG elements. Otherwise,
 * it returnes -1.
 */
static int
mvumi_make_sgl(struct mv_hba *mhba, struct scsi_cmnd *scmd, void *sgl_p)
{
	struct scatterlist *sg;
	struct mv_sgl *m_sg;
	int sg_count = 0;
	unsigned int len;
	dma_addr_t busaddr = 0;
	int i;

	if (mv_rq_bf_l(scmd) > (mv_scmd_host(scmd)->max_sectors << 9))
		mv_printk( " request length exceeds "
		       "the maximum alowed value.\n");

	if (0 == mv_rq_bf_l(scmd))
		return 0;

	m_sg = (struct mv_sgl *)sgl_p;
	if (mv_use_sg(scmd)) {
		sg = (struct scatterlist *)mv_rq_bf(scmd);
		sg_count = pci_map_sg(mhba->pdev,
				      sg,
				      mv_use_sg(scmd),
				      scsi_to_pci_dma_dir(scmd->
							  sc_data_direction));

		if (sg_count != mv_use_sg(scmd)) {
			mv_printk(" sg_count(%d) != scmd->use_sg(%d)\n",
			       (unsigned int)sg_count, mv_use_sg(scmd));
		}
		for (i = 0; i < sg_count; i++) {
			busaddr = sg_dma_address(&sg[i]);
			len = sg_dma_len(&sg[i]);
			m_sg->baseaddr_l = cpu_to_le32((u32) busaddr);
			m_sg->baseaddr_h =
			    cpu_to_le32((u32) ((busaddr >> 16) >> 16));
			m_sg->flags = 0;
#if defined(USE_NEW_SGTABLE)
			if ((i + 1) == sg_count)
				m_sg->flags |= SGD_EOT;
			if (mhba->hba_capability & HS_CAPABILITY_SUPPORT_COMPACT_SG)
				m_sg->flags |= SGD_COMPACT;
			sgd_setsz(m_sg, cpu_to_le32(len));
#else
			m_sg->size = cpu_to_le32(len);
#endif
			sgd_inc(m_sg);
		}
	} else {
		mv_printk( " map kernel addr into bus addr.\n");
		scmd->SCp.dma_handle = mv_rq_bf_l(scmd) ?
		    pci_map_single(mhba->pdev, mv_rq_bf(scmd),
				   mv_rq_bf_l(scmd),
				   scsi_to_pci_dma_dir(scmd->sc_data_direction))
		    : 0;
		busaddr = scmd->SCp.dma_handle;
		m_sg->baseaddr_l = cpu_to_le32((u32) busaddr);
		m_sg->baseaddr_h = cpu_to_le32((u32) ((busaddr >> 16) >> 16));
		m_sg->flags = 0;
#if defined(USE_NEW_SGTABLE)
		m_sg->flags |= SGD_EOT;
		if (mhba->hba_capability & HS_CAPABILITY_SUPPORT_COMPACT_SG)
			m_sg->flags |= SGD_COMPACT;
		sgd_setsz(m_sg, cpu_to_le32(mv_rq_bf_l(scmd)));
#else
		m_sg->size = cpu_to_le32(mv_rq_bf_l(scmd));
#endif
		sg_count=1;
	}
	return sg_count;
}

u8 mvumi_add_timer(struct timer_list * timer,
		   u32 sec,
		   void (*function) (unsigned long), unsigned long data)
{
	u64 jif;

	WARN_ON(timer_pending(timer));
	init_timer(timer);
	timer->function = function;
	timer->data = data;

	jif = (u64) (sec * HZ);
	//do_div(jif, 1000);         /* wait in unit of second */
	timer->expires = jiffies + 1 + jif;

	add_timer(timer);
	return 0;
}

void mvumi_del_timer(struct timer_list *timer)
{
	if (timer->function)
		del_timer(timer);
	timer->function = NULL;
}

u64 mvumi_get_time_in_sec(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u64) tv.tv_sec;
}

u32 mvumi_get_msec_of_time(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return (u32) tv.tv_usec * 1000 * 1000;
}

//reset nmi watchdog before delay
static void mvumi_touch_nmi_watchdog(void)
{
#ifdef CONFIG_X86_64
	touch_nmi_watchdog();
#endif /* CONFIG_X86_64 */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 10)
	touch_softlockup_watchdog();
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 10) */
}

void mvumi_sleep_millisecond(void *ext, u32 msec)
{
	u32 tmp = 0;
	u32 mod_msec = 2000;
	mvumi_touch_nmi_watchdog();
	if (msec <= mod_msec)
		mdelay(msec);

	else {
		for (tmp = 0; tmp < msec / mod_msec; tmp++) {
			mdelay(mod_msec);
			mvumi_touch_nmi_watchdog();
		}
		if (msec % mod_msec)
			mdelay(msec % mod_msec);
	}
	mvumi_touch_nmi_watchdog();

}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */

#if defined(SUPPORT_INTERNAL_REQ)
/**
 * mvumi_get_int_cmd -	Get a command from the free pool
 * @lhba:		Adapter soft state
 *
 * Returns a free command from the pool
 */
static struct mvumi_cmd *mvumi_get_int_cmd(struct mv_hba *mhba, int in_irq)
{
	unsigned long flags;
	struct mvumi_cmd *cmd = NULL;

	spin_lock_irqsave(&mhba->int_cmd_pool_lock, flags);

	if (!list_empty(&mhba->int_cmd_pool)) {
		struct mv_sgl *m_sg;
		cmd = list_entry((&mhba->int_cmd_pool)->next,
				 struct mvumi_cmd, queue_pointer);
		list_del_init(&cmd->queue_pointer);
		m_sg = (struct mv_sgl *)&cmd->frame->payload[0];
		if (!in_irq) {
			cmd->data_buf = mhba->int_req_buf;
			m_sg->baseaddr_l = cpu_to_le32(mhba->int_req_phys);
			m_sg->baseaddr_h = cpu_to_le32(
					(mhba->int_req_phys >> 16) >> 16);
		} else {
			cmd->data_buf = mhba->event_int_req_buf;
			m_sg->baseaddr_l = cpu_to_le32(
					mhba->event_int_req_phys);
			m_sg->baseaddr_h = cpu_to_le32(
					(mhba->event_int_req_phys >> 16) >> 16);
		}
	} else
		mv_printk(" Int command pool empty!\n");

	spin_unlock_irqrestore(&mhba->int_cmd_pool_lock, flags);
	return cmd;
}

/**
 * mvumi_return_cmd -	Return a cmd to free command pool
 * @lhba:		Adapter soft state
 * @cmd:		Command packet to be returned to free command pool
 */
static inline void
mvumi_return_int_cmd(struct mv_hba *mhba, struct mvumi_cmd *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&mhba->int_cmd_pool_lock, flags);

	cmd->scmd = NULL;
	list_add_tail(&cmd->queue_pointer, &mhba->int_cmd_pool);

	spin_unlock_irqrestore(&mhba->int_cmd_pool_lock, flags);
}

/**
 * mvumi_teardown_frame_pool -	Destroy the cmd frame DMA pool
 * @lhba:				Adapter soft state
 */
static void mvumi_teardown_int_frame_pool(struct mv_hba *mhba)
{
	int i;
	struct mvumi_cmd *cmd;
	/*
	 * Return all frames to pool
	 */
	for (i = 0; i < mhba->max_int_io; i++) {
		cmd = mhba->int_cmd_list[i];
		if (cmd->frame)
			kfree(cmd->frame);
	}
}

/**
 * mvumi_create_int_frame_pool -	Creates DMA pool for cmd frames
 * @lhba:			Adapter soft state
 *
 * Each command packet has an embedded DMA memory buffer that is used for
 * filling FW frame and the SG list that immediately follows the frame. This
 * function creates those DMA memory buffers for each command packet by using
 * PCI pool facility.
 */
static int mvumi_create_int_frame_pool(struct mv_hba *mhba)
{
	int i;
	void *int_req_buf;
	dma_addr_t int_req_phys;
	struct mvumi_cmd *cmd;
	struct mv_sgl *m_sg;

	int_req_buf = mhba->int_req_buf;
	int_req_phys = mhba->int_req_phys;

	/*
	 * Allocate and attach a frame to each of the commands in cmd_list.
	 * By making cmd->index as the context instead of the &cmd, we can
	 * always use 32bit context regardless of the architecture
	 */
	for (i = 0; i < mhba->max_int_io; i++) {

		cmd = mhba->int_cmd_list[i];
		cmd->frame = kmalloc(mhba->ib_max_entry_size_bytes,
					GFP_KERNEL);
		/*
		 * mvumi_teardown_frame_pool() takes care of freeing
		 * whatever has been allocated
		 */
		if (!cmd->frame) {
			mv_printk(" pci_pool_alloc failed \n");
			mvumi_teardown_int_frame_pool(mhba);
			return -ENOMEM;
		}
		memset(cmd->frame, 0, mhba->ib_max_entry_size_bytes);
		m_sg = (struct mv_sgl *)&cmd->frame->payload[0];
		cmd->frame->sg_counts = 1;
		cmd->data_buf = int_req_buf;
		m_sg->baseaddr_l = cpu_to_le32(int_req_phys);
		m_sg->baseaddr_h = cpu_to_le32((int_req_phys >> 16) >> 16);
		m_sg->flags = 0;
#if defined(USE_NEW_SGTABLE)
		m_sg->flags |= SGD_EOT;
		if (mhba->hba_capability & HS_CAPABILITY_SUPPORT_COMPACT_SG)
			m_sg->flags |= SGD_COMPACT;
		sgd_setsz(m_sg, cpu_to_le32(mhba->int_req_size));
#else
		m_sg->size = cpu_to_le32(mhba->int_req_size);
#endif
	}
	return 0;
}

/**
 * mvumi_free_cmds -	Free all the cmds in the free cmd pool
 * @lhba:		Adapter soft state
 */
static void mvumi_free_int_cmds(struct mv_hba *mhba)
{
	int i;
	/* First free the inter frame pool */
	mvumi_teardown_int_frame_pool(mhba);

	/* Free all the commands in the cmd_list */
	for (i = 0; i < mhba->max_int_io; i++)
		kfree(mhba->int_cmd_list[i]);

	/* Free the int cmd_list buffer itself */
	kfree(mhba->int_cmd_list);
	mhba->int_cmd_list = NULL;

}

/**
 * mvumi_alloc_int_cmds -	Allocates the command packets
 * @lhba:		Adapter soft state
 *
 * Each command that is issued to the FW, whether IO commands from the OS or
 * internal commands like IOCTLs, are wrapped in local data structure called
 * mvumi_cmd. The frame embedded in this mvumi_cmd is actually issued to
 * the FW.
 *
 * Each frame has a 32-bit field called context (tag). This context is used
 * to get back the mvumi_cmd from the frame when a frame gets completed in
 * the ISR. Typically the address of the mvumi_cmd itself would be used as
 * the context. But we wanted to keep the differences between 32 and 64 bit
 * systems to the mininum. We always use 32 bit integers for the context. In
 * this driver, the 32 bit values are the indices into an array cmd_list.
 * This array is used only to look up the mvumi_cmd given the context. The
 * free commands themselves are maintained in a linked list called cmd_pool.
 */
static int mvumi_alloc_int_cmds(struct mv_hba *mhba)
{
	int i;
	int j;
	u32 max_cmd;
	struct mvumi_cmd *cmd;
	max_cmd = mhba->max_int_io;

	/*
	 * lhba->cmd_list is an array of struct mvumi_cmd pointers.
	 * Allocate the dynamic array first and then allocate individual
	 * commands.
	 */
	mhba->int_cmd_list =
	    kcalloc(max_cmd, sizeof(struct mvumi_cmd *), GFP_KERNEL);

	if (!mhba->int_cmd_list) {
		mv_printk(" out of memory for inter cmd\n");
		return -ENOMEM;
	}

	for (i = 0; i < max_cmd; i++) {
		mhba->int_cmd_list[i] = kmalloc(sizeof(struct mvumi_cmd),
						GFP_KERNEL);

		if (!mhba->int_cmd_list[i]) {

			for (j = 0; j < i; j++)
				kfree(mhba->int_cmd_list[j]);

			kfree(mhba->int_cmd_list);
			mhba->int_cmd_list = NULL;

			return -ENOMEM;
		}
	}

	/*
	 * Add all the commands to command pool (lhba->cmd_pool)
	 */
	for (i = 0; i < max_cmd; i++) {
		cmd = mhba->int_cmd_list[i];
		memset(cmd, 0, sizeof(struct mvumi_cmd));
		cmd->index = i;
		cmd->mhba = mhba;

		list_add_tail(&cmd->queue_pointer, &mhba->int_cmd_pool);
	}

	/*
	 * Create a frame pool and assign one frame to each cmd
	 */
	if (mvumi_create_int_frame_pool(mhba)) {
		mv_printk(" Error creating frame DMA pool\n");
		mvumi_free_int_cmds(mhba);
		return -ENOMEM;
	}

	return 0;
}

#endif /* #if defined(SUPPORT_INTERNAL_REQ) */

/**
 * mvumi_get_cmd -	Get a command from the free pool
 * @lhba:		Adapter soft state
 *
 * Returns a free command from the pool
 */
static struct mvumi_cmd *mvumi_get_cmd(struct mv_hba *mhba)
{
	unsigned long flags;
	struct mvumi_cmd *cmd = NULL;

	spin_lock_irqsave(&mhba->cmd_pool_lock, flags);

	if (!list_empty(&mhba->cmd_pool)) {
		cmd = list_entry((&mhba->cmd_pool)->next,
				 struct mvumi_cmd, queue_pointer);
		list_del_init(&cmd->queue_pointer);
	} else
		mv_printk(KERN_ERR " Command pool empty!\n");

	spin_unlock_irqrestore(&mhba->cmd_pool_lock, flags);
	return cmd;
}

/**
 * mvumi_return_cmd -	Return a cmd to free command pool
 * @lhba:		Adapter soft state
 * @cmd:		Command packet to be returned to free command pool
 */
static inline void mvumi_return_cmd(struct mv_hba *mhba, struct mvumi_cmd *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&mhba->cmd_pool_lock, flags);

	cmd->scmd = NULL;
	list_add_tail(&cmd->queue_pointer, &mhba->cmd_pool);
	//list_add(&cmd->queue_pointer, &mhba->cmd_pool);
	spin_unlock_irqrestore(&mhba->cmd_pool_lock, flags);
}

/**
 * mvumi_teardown_frame_pool -	Destroy the cmd frame DMA pool
 * @lhba:				Adapter soft state
 */
static void mvumi_teardown_frame_pool(struct mv_hba *mhba)
{
	int i;
	struct mvumi_cmd *cmd;
	/*
	 * Return all frames to pool
	 */
	for (i = 0; i < mhba->max_io; i++) {

		cmd = mhba->cmd_list[i];
		if (cmd->frame)
			kfree(cmd->frame);

	}
}

/**
 * mvumi_create_frame_pool -	Creates DMA pool for cmd frames
 * @lhba:			Adapter soft state
 *
 * Each command packet has an embedded DMA memory buffer that is used for
 * filling FW frame and the SG list that immediately follows the frame. This
 * function creates those DMA memory buffers for each command packet by using
 * PCI pool facility.
 */
static int mvumi_create_frame_pool(struct mv_hba *mhba)
{
	int i;
	struct mvumi_cmd *cmd;
	/*
	 * Allocate and attach a frame to each of the commands in cmd_list.
	 * By making cmd->index as the context instead of the &cmd, we can
	 * always use 32bit context regardless of the architecture
	 */
	for (i = 0; i < mhba->max_io; i++) {
		cmd = mhba->cmd_list[i];
		cmd->index = i;
		cmd->frame = kmalloc(mhba->ib_max_entry_size_bytes,
					GFP_KERNEL);
		/*
		 * mvumi_teardown_frame_pool() takes care of freeing
		 * whatever has been allocated
		 */
		if (!cmd->frame) {
			mv_printk("failed alloc cmd[0x%x] frame.\n.", i);
			mvumi_teardown_frame_pool(mhba);
			return -ENOMEM;
		}
		memset(cmd->frame, 0, mhba->ib_max_entry_size_bytes);
	}

	return 0;
}

/**
 * mvumi_free_cmds -	Free all the cmds in the free cmd pool
 * @lhba:		Adapter soft state
 */
static void mvumi_free_cmds(struct mv_hba *mhba)
{
	int i;
	/* First free the FW frame pool */
	mvumi_teardown_frame_pool(mhba);

	/* Free all the commands in the cmd_list */
	for (i = 0; i < mhba->max_io; i++)
		kfree(mhba->cmd_list[i]);

	/* Free the cmd_list buffer itself */
	kfree(mhba->cmd_list);
	mhba->cmd_list = NULL;

	INIT_LIST_HEAD(&mhba->cmd_pool);
}

/**
 * mvumi_alloc_cmds -	Allocates the command packets
 * @lhba:		Adapter soft state
 *
 * Each command that is issued to the FW, whether IO commands from the OS or
 * internal commands like IOCTLs, are wrapped in local data structure called
 * mvumi_cmd. The frame embedded in this mvumi_cmd is actually issued to
 * the FW.
 *
 * Each frame has a 32-bit field called context (tag). This context is used
 * to get back the mvumi_cmd from the frame when a frame gets completed in
 * the ISR. Typically the address of the mvumi_cmd itself would be used as
 * the context. But we wanted to keep the differences between 32 and 64 bit
 * systems to the mininum. We always use 32 bit integers for the context. In
 * this driver, the 32 bit values are the indices into an array cmd_list.
 * This array is used only to look up the mvumi_cmd given the context. The
 * free commands themselves are maintained in a linked list called cmd_pool.
 */
static int mvumi_alloc_cmds(struct mv_hba *mhba)
{
	int i;
	int j;
	u32 max_cmd;
	struct mvumi_cmd *cmd;

	max_cmd = mhba->max_io;

	/*
	 * lhba->cmd_list is an array of struct mvumi_cmd pointers.
	 * Allocate the dynamic array first and then allocate individual
	 * commands.
	 */
	mhba->cmd_list =
	    kcalloc(max_cmd, sizeof(struct mvumi_cmd *), GFP_KERNEL);

	if (!mhba->cmd_list) {
		mv_printk(" out of memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < max_cmd; i++) {
		mhba->cmd_list[i] = kmalloc(sizeof(struct mvumi_cmd),
					    GFP_KERNEL);

		if (!mhba->cmd_list[i]) {

			for (j = 0; j < i; j++)
				kfree(mhba->cmd_list[j]);

			kfree(mhba->cmd_list);
			mhba->cmd_list = NULL;
			mv_printk(" out of memory for alloc cmd list\n");

			return -ENOMEM;
		}
	}

	/*
	 * Add all the commands to command pool (lhba->cmd_pool)
	 */
	for (i = 0; i < max_cmd; i++) {
		cmd = mhba->cmd_list[i];
		memset(cmd, 0, sizeof(struct mvumi_cmd));
		cmd->index = i;
		cmd->mhba = mhba;
		list_add_tail(&cmd->queue_pointer, &mhba->cmd_pool);
	}

	/*
	 * Create a frame pool and assign one frame to each cmd
	 */
	if (mvumi_create_frame_pool(mhba)) {
		mv_printk(" Error creating frame DMA pool\n");
		mvumi_free_cmds(mhba);
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL(mvumi_alloc_cmds);

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
static int mvumi_get_ib_list_entry(struct mv_hba *mhba, void **ib_entry)
{
	void *regs = mhba->mmio;
	u32 ib_rp_reg;
	u32 cur_tag;
	u32 inb_available_count=0;

	if(_IS_CHIP_8180(mhba)){
		if (atomic_read(&mhba->fw_outstanding) >= mhba->max_io - 1) {
			mv_printk("firmware io overflow.\n");
			return -1;
		}
		inb_available_count=readl(mhba->ib_shadow);
		if(inb_available_count !=mhba->list_num_io){
			return -1;
			}
		if(inb_available_count<=mhba->ib_cur_count++){
			return -1;
			}
	}else
	{
		if (atomic_read(&mhba->fw_outstanding) >= mhba->max_io ) {
			mv_printk("firmware io overflow.\n");
			return -1;
		}
	ib_rp_reg = mr32(CLA_INB_READ_POINTER);

	/* no free slot to use */
	if (((ib_rp_reg & CL_SLOT_NUM_MASK) ==
	     (mhba->ib_cur_slot & CL_SLOT_NUM_MASK)) &&
	    ((ib_rp_reg & CL_POINTER_TOGGLE) !=
	     (mhba->ib_cur_slot & CL_POINTER_TOGGLE))) {
		mv_printk("no free slot to use.\n");
		return -1;
	}
	}
	cur_tag = mhba->ib_cur_slot & CL_SLOT_NUM_MASK;
	cur_tag++;
	if (cur_tag >= mhba->list_num_io) {
		cur_tag -= mhba->list_num_io;
		mhba->ib_cur_slot ^= CL_POINTER_TOGGLE;
	}

	/* modify current slot value */
	mhba->ib_cur_slot &= ~CL_SLOT_NUM_MASK;
	mhba->ib_cur_slot |= (cur_tag & CL_SLOT_NUM_MASK);
	*ib_entry = (void *)((u8 *) mhba->ib_list +
			cur_tag * mhba->ib_max_entry_size_bytes);
	atomic_inc(&mhba->fw_outstanding);

	if(is_workround_dev(mhba)) {
		/* !!! work around for slot 0, can not work */
		if ((cur_tag == 0) && (mhba->list_workaround_flag & MU_WA_IB_LIST)) {
			*ib_entry = (void *)((u8 *) mhba->ib_list +
				mhba->list_num_io * mhba->ib_max_entry_size_bytes);
		}
		mhba->list_workaround_flag |= MU_WA_IB_LIST;
	}

	return 0;
}

static u8 mvumi_send_ib_list_entry(struct mv_hba *mhba)
{
	void *regs = mhba->mmio;
#if 1
	writel((u32)0xfff,mhba->ib_shadow);
	mhba->ib_cur_count=0;
	mw32(CLA_INB_WRITE_POINTER, mhba->ib_cur_slot);
#else
	/* work wround for M-bus */
	mw32(CLA_INB_WRITE_POINTER, mhba->ib_current_slot);
	mw32(CPU_PCIEA_TO_ARM_DRBL_REG, MV_BIT(7));
#endif

	return 0;
}

static u8 mvumi_receive_ob_list_entry(struct mv_hba *mhba)
{
	u32 ob_read_reg, ob_write_req;
	u32 cur_tag, assign_tag_end;
	u32 i;
	struct mv_ob_data_pool *free_ob_pool;
	void *p_outb_frame, *regs;
	struct mvumi_rsp_frame *ob_frame;

	regs = mhba->mmio;
	ob_read_reg = mr32(CLA_OUTB_READ_POINTER);
	if(_IS_CHIP_8180(mhba))
		ob_write_req=readl(mhba->ob_shadow);
	else
	ob_write_req = mr32(CLA_OUTB_COPY_POINTER);
	cur_tag = mhba->ob_cur_slot & CL_SLOT_NUM_MASK;	/* bit (0~11) indicate the number */
	assign_tag_end = ob_write_req & CL_SLOT_NUM_MASK;

	/* check if wrap around */
	if(_IS_CHIP_8180(mhba)){
		if(assign_tag_end<cur_tag){
			assign_tag_end+=mhba->list_num_io;
			} else if(assign_tag_end==cur_tag)
				return MV_TRUE;
		}else{
	if ((ob_write_req & CL_POINTER_TOGGLE) !=
	    (mhba->ob_cur_slot & CL_POINTER_TOGGLE)) {
		assign_tag_end += mhba->list_num_io;

	}
	}

	for (i = (assign_tag_end - cur_tag); i != 0; i--) {
		cur_tag++;
		if (cur_tag >= mhba->list_num_io) {
			cur_tag -= mhba->list_num_io;
			mhba->ob_cur_slot ^= CL_POINTER_TOGGLE;	/* set toggle for current slot */
		}

		free_ob_pool = (struct mv_ob_data_pool *)list_get_first_entry(
		    		&mhba->ob_data_pool_list,
				struct mv_ob_data_pool,
				queue_pointer);

		if (!free_ob_pool) {
			if (cur_tag == 0) {
				cur_tag = mhba->list_num_io - 1;
				mhba->ob_cur_slot ^= CL_POINTER_TOGGLE;	/* set toggle for current slot */
			} else {
				cur_tag -= 1;
			}

			break;
		}

		/* Get Outbound List and copy to free pool */
		p_outb_frame = (void *)((u8 *) mhba->ob_list +
			     cur_tag * mhba->ob_max_entry_size_bytes);
		memcpy(free_ob_pool->ob_data, p_outb_frame,
			mhba->ob_max_entry_size_bytes);

		if(is_workround_dev(mhba)) {
			/* !!! work around for slot 0, can not work */
			if ((cur_tag == 0)
			    && (mhba->list_workaround_flag & MU_WA_OB_LIST)) {

				/* Get Outbound List and copy to free pool */
				p_outb_frame =
				    (void *)((u8 *) mhba->ob_list +
					     mhba->list_num_io *
					     mhba->ob_max_entry_size_bytes);
				memcpy(free_ob_pool->ob_data, p_outb_frame,
				       mhba->ob_max_entry_size_bytes);
			}

			mhba->list_workaround_flag |= MU_WA_OB_LIST;
		}

		ob_frame = (struct mvumi_rsp_frame *)p_outb_frame;
		//mvumi_dump_ob_info(ob_frame);
		/* Queue OutB frame to g_outb_list */
		list_add_tail(&free_ob_pool->queue_pointer,
			      &mhba->free_ob_list);
	}

	/* modify current slot value         */
	mhba->ob_cur_slot &= ~CL_SLOT_NUM_MASK;
	mhba->ob_cur_slot |= (cur_tag & CL_SLOT_NUM_MASK);
	mw32(CLA_OUTB_READ_POINTER, mhba->ob_cur_slot);

	return 1;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
static void mvumi_reset(void *regs)
{
	/* disable all isr */
	mw32(CPU_ENPOINTA_MASK_REG, 0);
	if (mr32(CPU_ARM_TO_PCIEA_MSG1) != HANDSHAKE_DONESTATE)
		return;

	/* perform soft-reset */
	mw32(CPU_ARM_TO_PCIEA_MSG1,0);
	mw32(CPU_PCIEA_TO_ARM_DRBL_REG, DRBL_SOFT_RESET);
	return;
}

static u8 mvumi_start(struct mv_hba *mhba);

/**
 * mvumi_wait_for_outstanding -	Wait for all outstanding cmds
 * @lhba:				Adapter soft state
 *
 * This function waits for upto MVUMI_RESET_WAIT_TIME seconds for FW to
 * complete all its outstanding commands. Returns error if one or more IOs
 * are pending after this time period. It also marks the controller dead.
 */
static int mvumi_wait_for_outstanding(struct mv_hba *mhba)
{
	mhba->fw_state = FW_STATE_ABORT;
	mvumi_reset(mhba->mmio);
	msleep(10000);
	if(mvumi_start(mhba))
		return FAILED;
	else
		return SUCCESS;
}

/**
 * mvumi_generic_reset -	Generic reset routine
 * @scmd:			Mid-layer SCSI command
 *
 * This routine implements a generic reset handler for device, bus and host
 * reset requests. Device, bus and host specific reset handlers can use this
 * function after they do their specific tasks.
 */
static int mvumi_generic_reset(struct scsi_cmnd *scmd)
{
	int ret_val;
	struct mv_hba *mhba;

	mhba = (struct mv_hba *)scmd->device->host->hostdata;

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2, 6, 18)
	scmd_printk(KERN_NOTICE, scmd, " RESET -%ld cmd=%x retries=%x\n",
		    scmd->serial_number, scmd->cmnd[0], scmd->retries);
	#else
	mv_printk(KERN_NOTICE, scmd, " RESET -%ld cmd=%x retries=%x\n",
		    scmd->serial_number, scmd->cmnd[0], scmd->retries);
	#endif

	if (mhba->hw_crit_error) {
		mv_printk( " cannot recover from previous reset "
		       "failures\n");
		return FAILED;
	}

	ret_val = mvumi_wait_for_outstanding(mhba);
	if (ret_val == SUCCESS)
		mv_printk( " reset successful\n");
	else
		mv_printk( " failed to do reset\n");

	return ret_val;
}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/**
 * mvumi_complete_int_cmd -	Completes an internal command
 * @lhba:			Adapter soft state
 * @cmd:			Command to be completed
 *
 * The mvumi_issue_blocked_cmd() function waits for a command to complete
 * after it issues a command. This function wakes up that waiting routine by
 * calling wake_up() on the wait queue.
 */
static void mvumi_complete_int_cmd(struct mv_hba *mhba, struct mvumi_cmd *cmd)
{
	mvumi_del_timer(&cmd->eh_timer);
	mvumi_return_int_cmd(mhba, cmd);
	/* FIXED ME */
}

static int mvumi_issue_blocked_cmd(struct mv_hba *mhba, struct mvumi_cmd *cmd)
{
	u8 ret = 0;
	unsigned long flags;
	cmd->cmd_status = REQ_STATUS_PENDING;
	if (cmd->sync_cmd){
		mv_printk("last blocked cmd not finished.\n");
		MV_ASSERT(0);
		return 0;
	}
	cmd->sync_cmd = 1;

	mhba->instancet->fire_cmd(mhba, cmd);

	/* MVUMI_INTERNAL_CMD_WAIT_TIME second timeout */
	ret=wait_event_timeout(mhba->int_cmd_wait_q,
			(cmd->cmd_status != REQ_STATUS_PENDING),
			MVUMI_INTERNAL_CMD_WAIT_TIME * HZ);
	
	if (cmd->sync_cmd) {
		spin_lock_irqsave(&mhba->mv_timer_lock, flags);
		if(cmd->sync_cmd){
		cmd->sync_cmd = 0;
		if(mhba->tag_cmd[cmd->frame->tag]){
			mhba->tag_cmd[cmd->frame->tag] = 0;
			mv_printk("TIMEOUT:release tag [%d]\n",cmd->frame->tag);
			tag_release_one(mhba, &mhba->tag_pool, cmd->frame->tag);
		}
		mv_printk("TIMEOUT:release a internal command\n");
		spin_lock_irqsave(&mhba->mv_waiting_req_lock,flags);
		if(cmd->queue_pointer.next!=LIST_POISON1 && cmd->queue_pointer.prev!=LIST_POISON2			
			&&cmd->queue_pointer.next!=&cmd->queue_pointer && cmd->queue_pointer.prev!=&cmd->queue_pointer
				&&cmd->queue_pointer.prev!=NULL && cmd->queue_pointer.next!=NULL)
			{
				mv_printk("TIMEOUT:A internal command doesn't send!\n");
				list_del_init(&cmd->queue_pointer);
			} else
				atomic_dec(&mhba->fw_outstanding);
		spin_unlock_irqrestore(&mhba->mv_waiting_req_lock,flags);
		mvumi_complete_int_cmd(mhba, cmd);
		
		}
		spin_unlock_irqrestore(&mhba->mv_timer_lock, flags);
	}

	return ret;
}
/* ----------------------------------------------------------------- */

/**
 * mvumi_release_fw -	Reverses the FW initialization
 * @intance:			Adapter soft state
 */
static void mvumi_release_fw(struct mv_hba *mhba)
{
	mvumi_free_cmds(mhba);
	mvumi_release_mem_resource(mhba);
#if defined(SUPPORT_INTERNAL_REQ)
	mvumi_free_int_cmds(mhba);
#endif
	mvumi_unmap_pci_addr(mhba->pdev, mhba->base_addr);
	pci_release_regions(mhba->pdev);
}

/**
 * mvumi_flush_cache -	Requests FW to flush all its caches
 * @lhba:			Adapter soft state
 */
static u8 mvumi_flush_cache(struct mv_hba *mhba)
{
	struct mvumi_cmd *cmd;
	struct mvumi_msg_frame *frame;
	u8 device_id;
	mv_printk("start mvumi_flush_cache.\n");

	for(device_id=0;device_id<mhba->max_target_id;device_id++){
	cmd = mvumi_get_int_cmd(mhba, 0);
	if (!cmd)
		return -1;
	cmd->scmd = 0;
	cmd->mhba = mhba;
	cmd->cmd_status = REQ_STATUS_PENDING;
	frame = cmd->frame;

	frame->req_function = CL_FUN_SCSI_CMD;
	frame->device_id = device_id;//VIRTUAL_DEVICE_ID;
	frame->cmd_flag = CMD_FLAG_NON_DATA;
	frame->sg_counts = 0;
	frame->data_transfer_length = 0;
	frame->cdb_length = MAX_CDB_SIZE;
	memset(frame->cdb, 0, MAX_CDB_SIZE);
	frame->cdb[0] = SCSI_CMD_MARVELL_SPECIFIC;
	//frame->cdb[1] = CDB_CORE_MODULE;
	frame->cdb[2] = CDB_CORE_SHUTDOWN;
	/*
	 * Issue the command to the FW
	 */
	mvumi_issue_blocked_cmd(mhba, cmd);
	if (cmd->cmd_status != REQ_STATUS_SUCCESS) {
		mv_printk("device %d mvumi_flush_cache failed, status=0x%x.\n",
			  device_id, cmd->cmd_status);
		break;
	}
	}
	return 0;
}
static u8 mvumi_inquiry(struct mv_hba *mhba, unsigned int id, unsigned int lun)
{
	struct mvumi_cmd *cmd;
	struct mvumi_msg_frame *frame;
	cmd = mvumi_get_int_cmd(mhba, 0);
	if (!cmd)
		return -1;

	cmd->scmd = 0;
	cmd->mhba = mhba;

	frame = cmd->frame;
	frame->req_function = CL_FUN_SCSI_CMD;
	frame->device_id = MVUMI_DEV_INDEX(id, lun);
	frame->cmd_flag = CMD_FLAG_DATA_IN;
	frame->sg_counts = 1;
	frame->data_transfer_length = 0x24;
	frame->cdb_length = 6;
	memset(frame->cdb, 0, MAX_CDB_SIZE);
	frame->cdb[0] = INQUIRY;
	frame->cdb[4] = frame->data_transfer_length;

	mvumi_issue_blocked_cmd(mhba, cmd);
	return cmd->cmd_status;
}

static void mvumi_rescan(struct mv_hba *mhba, unsigned int event)
{
	struct scsi_device *sdev;
	if (event == EVENT_DEVICE_REMOVAL) {
		shost_for_each_device(sdev, mhba->shost) {
			if (mvumi_inquiry(mhba, sdev->id, sdev->lun)) {
				sdev = scsi_device_lookup(mhba->shost, 0,
							sdev->id, 0);
				if (sdev) {
					mv_printk(" remove disk %d-%d-%d.\n",
							0, sdev->id, 0);
					scsi_remove_device(sdev);
					scsi_device_put(sdev);
				} else
					mv_printk(" no disk to remove\n");
			}
		}
	} else if (event == EVENT_DEVICE_ARRIVAL) {
		unsigned int id;
		unsigned int lun;
		for(id = 0; id < mhba->shost->max_id; id++)
			for(lun = 0; lun < mhba->shost->max_lun; lun++) {
				sdev = scsi_device_lookup(mhba->shost,
								0, id, lun);
				if (!mvumi_inquiry(mhba, id, lun)) {
					if (!sdev) {
						scsi_add_device(mhba->shost, 0,
							id, lun);
						mv_printk(" add disk "
							"%d-%d-%d.\n",
							0, id, lun);
					}
				} else {
					if (sdev) {
						mv_printk(" remove disk"
							"%d-%d-%d.\n",
							0, sdev->id, sdev->lun);
						scsi_remove_device(sdev);
						scsi_device_put(sdev);
					}
				}
			}
	}
}

static int mvumi_handle_hotplug(struct mv_hba *mhba, u16 devid, int status)
{
	struct scsi_device *sdev;
	if (status == DEVICE_OFFLINE) {
		sdev = scsi_device_lookup(mhba->shost, 0,
				devid, 0);
		if (sdev) {
			mv_printk(" remove disk %d-%d-%d.\n",
					0, sdev->id, 0);
			scsi_remove_device(sdev);
			scsi_device_put(sdev);
		} else
			mv_printk(" no disk[%d] to remove\n", devid);
		return 1;
	} else if (status == DEVICE_ONLINE) {
		sdev = scsi_device_lookup(mhba->shost,
				0, devid, 0);
		if (!sdev) {
			scsi_add_device(mhba->shost, 0,
				devid, 0);
			mv_printk(" add disk "
				"%d-%d-%d.\n",
				0, devid, 0);
		} else {
			mv_printk(" don't add disk"
				"%d-%d-%d.\n",
				0, devid, 0);
		}
		return 1;
	}
	return 0;
}

/**
 * mvumi_shutdown_controller -	Instructs FW to shutdown the controller
 * @lhba:				Adapter soft state
 */
static void mvumi_shutdown_controller(struct mv_hba *mhba)
{
	/* FIXED ME */
	return;
}

static u8
mvumi_calculate_checksum(struct mv_handshake_header *p_header, u16 len)
{
	u8 *ptr_U8;
	u8 ret = 0, i;

	ptr_U8 = (u8 *) p_header->frame_content;
	for (i = 0; i < len; i++) {
		ret ^= *ptr_U8;
		ptr_U8++;
	}

	return ret;
}

void mvumi_hs_build_page(struct mv_hba *mhba,
			 struct mv_handshake_header *hs_header)
{
	struct mv_handshake_page2 *hs_page2;
	struct mv_handshake_page4 *hs_page4;
	struct mv_handshake_page3 *hs_page3;

	switch (hs_header->page_code) {
	case HS_PAGE_HOST_INFO:
		hs_page2 = (struct mv_handshake_page2 *)hs_header;
		hs_header->frame_length = sizeof(struct mv_handshake_page2) - 4;
		memset(hs_header->frame_content, 0, hs_header->frame_length);
		hs_page2->host_type = MV_OS_TYPE_LINUX;
		hs_page2->host_ver.ver_major = VER_MAJOR;
		hs_page2->host_ver.ver_minor = VER_MINOR;
		hs_page2->host_ver.ver_oem = VER_OEM;
		hs_page2->host_ver.ver_build = VER_BUILD;
		hs_page2->system_io_bus = 0;
		hs_page2->slot_number = 0;
		hs_page2->intr_level = 0;
		hs_page2->intr_vector = 0;
		hs_page2->seconds_since1970 = mvumi_get_time_in_sec();
		hs_header->checksum =
		    HS_SET_CHECKSUM(hs_header, hs_header->frame_length);
		break;

	case HS_PAGE_FIRM_CTL:
		hs_page3 = (struct mv_handshake_page3 *)hs_header;
		hs_header->frame_length = sizeof(struct mv_handshake_page3) - 4;
		memset(hs_header->frame_content, 0, hs_header->frame_length);
		hs_header->checksum =
		    HS_SET_CHECKSUM(hs_header, hs_header->frame_length);
		break;

	case HS_PAGE_CL_INFO:
		hs_page4 = (struct mv_handshake_page4 *)hs_header;
		hs_header->frame_length = sizeof(struct mv_handshake_page4) - 4;
		memset(hs_header->frame_content, 0, hs_header->frame_length);
		hs_page4->ib_baseaddr_l = mhba->ib_list_phys;
		if (!IS_CHIP_8180(mhba))
			hs_page4->ob_baseaddr_l = mhba->ob_list_phys-4;
		else
			hs_page4->ob_baseaddr_l = mhba->ob_list_phys;

		hs_page4->ib_baseaddr_h = (mhba->ib_list_phys >> 16) >> 16;
		hs_page4->ob_baseaddr_h = (mhba->ob_list_phys >> 16) >> 16;
		hs_page4->ib_entry_size = mhba->ib_max_entry_size_setting;
		hs_page4->ob_entry_size = mhba->ob_max_entry_size_setting;
		hs_page4->ob_depth = mhba->list_num_io;
		hs_page4->ib_depth = mhba->list_num_io;
		hs_header->checksum =
		    HS_SET_CHECKSUM(hs_header, hs_header->frame_length);
		
		mv_printk("****ib max size=%d\n",mhba->ib_max_entry_size_bytes);
		
		mv_printk("****ob max size=%d \n",mhba->ob_max_entry_size_bytes);
#if 0
		mv_printk("ib_baseaddr_l=0x%x,"
			"ib_baseaddr_h=0x%x,"
			"ob_baseaddr_h=0x%x,"
			"ob_baseaddr_h=0x%x.\n",
			hs_page4->ib_baseaddr_l,
			hs_page4->ib_baseaddr_h ,
			hs_page4->ob_baseaddr_l ,
			hs_page4->ob_baseaddr_h);

		mv_printk("ib_list=0x%p,ob_list=0x%p.\n",mhba->ib_list,
			mhba->ob_list);
#endif

		break;

	default:
		break;
	}
	return;
}

static int mvumi_init_data(struct mv_hba *mhba);

static int mvumi_hs_process_page(struct mv_hba *mhba,
				struct mv_handshake_header *hs_header)
{
	struct mv_handshake_page1 *hs_page1;
	u8 page_checksum;
	int ret = 0;

	page_checksum =
		HS_SET_CHECKSUM(hs_header, hs_header->frame_length);
	if (page_checksum != hs_header->checksum) {
		//Page Error by checksum
		return -1;
	}

	switch (hs_header->page_code) {
	case HS_PAGE_FIRM_CAP:
		hs_page1 = (struct mv_handshake_page1 *)hs_header;
		if (mhba->list_num_io > hs_page1->cl_inout_list_depth)
		    mhba->list_num_io = hs_page1->cl_inout_list_depth;
		if (hs_page1->cl_in_max_entry_size < CL_IN_ENTRY_SIZE_SETTING) {
			mhba->ib_max_entry_size_setting =
				hs_page1->cl_in_max_entry_size;
			mhba->ib_max_entry_size_bytes =
				CL_LIST_ENTRY_SIZE_BYTES(hs_page1->cl_in_max_entry_size);
			mv_printk("****ib max size=%d set=%d\n",mhba->ib_max_entry_size_bytes,hs_page1->cl_in_max_entry_size);
		}
		if (hs_page1->cl_out_max_entry_size < CL_OUT_ENTRY_SIZE_SETTING) {
			mhba->ob_max_entry_size_setting =
				hs_page1->cl_out_max_entry_size;
			mhba->ob_max_entry_size_bytes =
				CL_LIST_ENTRY_SIZE_BYTES(hs_page1->cl_out_max_entry_size);
			mv_printk("****ob max size=%d,set=%d\n",mhba->ob_max_entry_size_bytes,hs_page1->cl_out_max_entry_size);
		}
		/* TBD: transfer_size could be more flexible in the future.
		 *	driver may need to split request or consolidate before
		 *	sending to firmware */
		if (hs_page1->max_transfer_size < MVUMI_MAX_TRANSFER_SIZE)
			mhba->max_transfer_size = hs_page1->max_transfer_size;
		if (hs_page1->max_devices_support<MVUMI_MAX_TARGET_NUMBER)
			mhba->max_target_id = hs_page1->max_devices_support;
		if (hs_page1->max_io_support < mhba->max_io)
			mhba->max_io = hs_page1->max_io_support;

		mhba->hba_capability = hs_page1->capability;

		// work around for Vili A0 - can only support one IO at a time
		if (IS_CHIP_VILI(mhba)) {
				mhba->max_io = 1;
		}
#if 0
		mv_printk("ob_max_entry_size_setting=0x%x,"
			"ib_max_entry_size_bytes =0x%x,"
			"ob_max_entry_size_setting=0x%x,"
			"ob_max_entry_size_bytes=0x%x.\n",
			mhba->ib_max_entry_size_setting,
			mhba->ib_max_entry_size_bytes ,
			mhba->ob_max_entry_size_setting,
			mhba->ob_max_entry_size_bytes);

		mv_printk("fw capacity=0x%x,"
			"max_transfer_size=0x%x,"
			"list_num_io=%d,"
			"max_target_id=%d,"
			"max_io=%d.\n",
			mhba->hba_capability,
			mhba->max_transfer_size,
			mhba->list_num_io,
			mhba->max_target_id,
			mhba->max_io);
#endif
		break;
	default:
		break;
	}
	return ret;
}

static void mvumi_alloc_hs_page(struct mv_hba *mhba)
{
	mhba->handshake_page = mhba->handshake_page_base;
	mhba->handshake_page_phys = mhba->handshake_page_phys_base;
}

/**
 * mvumi_handshake -	Move the FW to READY state
 * @lhba:				Adapter soft state
 *
 * During the initialization, FW passes can potentially be in any one of
 * several possible states. If the FW in operational, waiting-for-handshake
 * states, driver must take steps to bring it to ready state. Otherwise, it
 * has to wait for the ready state.
 */
static int mvumi_handshake(struct mv_hba *mhba)
{
	u32 hs_state, tmp, hs_fun;
	struct mv_handshake_header *hs_header;
	void *regs = mhba->mmio;

	if (mhba->fw_state == FW_STATE_STARTING)
		hs_state = HS_S_START;
	else {
		tmp = mr32(CPU_ARM_TO_PCIEA_MSG0);
		hs_state = HS_GET_STATE(tmp);
		mv_printk("handshake host state = 0x%x.\n",   hs_state);
		if (HS_GET_STATUS(tmp) != HS_STATUS_OK) {
			mhba->fw_state = FW_STATE_STARTING;
			return -1;
		}
	}

	hs_fun = 0;
	switch (hs_state) {
	case HS_S_START:
		mhba->fw_state = FW_STATE_HANDSHAKING;
		HS_SET_STATUS(hs_fun, HS_STATUS_OK);
		HS_SET_STATE(hs_fun, HS_S_RESET);
		mw32(CPU_PCIEA_TO_ARM_MSG1, HANDSHAKE_SIGNATURE);
		mw32(CPU_PCIEA_TO_ARM_MSG0, hs_fun);
		mw32(CPU_PCIEA_TO_ARM_DRBL_REG, DRBL_HANDSHAKE);
		break;

	case HS_S_RESET:
		mvumi_alloc_hs_page(mhba);
		mw32(CPU_PCIEA_TO_ARM_MSG1, (u32) mhba->handshake_page_phys);
		mw32(CPU_ARM_TO_PCIEA_MSG1,
				(mhba->handshake_page_phys >> 16) >> 16);
		HS_SET_STATUS(hs_fun, HS_STATUS_OK);
		HS_SET_STATE(hs_fun, HS_S_PAGE_ADDR);
		mw32(CPU_PCIEA_TO_ARM_MSG0, hs_fun);
		mw32(CPU_PCIEA_TO_ARM_DRBL_REG, DRBL_HANDSHAKE);

		break;

	case HS_S_PAGE_ADDR:
	case HS_S_QUERY_PAGE:
	case HS_S_SEND_PAGE:
		hs_header = (struct mv_handshake_header *) mhba->handshake_page;
		if (hs_header->page_code == HS_PAGE_FIRM_CAP) {
			mhba->hba_total_pages =
				((struct mv_handshake_page1 *)hs_header)->total_pages;

			// for backward compatibility
			if (mhba->hba_total_pages == 0)
				mhba->hba_total_pages = HS_PAGE_TOTAL-1;

		}

		if (hs_state == HS_S_QUERY_PAGE) {
			if (mvumi_hs_process_page(mhba, hs_header)) {
				HS_SET_STATE(hs_fun, HS_S_ABORT);
				return -1;
			}
		} else if (hs_state == HS_S_PAGE_ADDR){
			hs_header->page_code = 0;
			/* initial value - will be set by firmware after the first handshaking
			 * page */
			mhba->hba_total_pages = HS_PAGE_TOTAL-1;

		}

		//verify next fw_state
		if ((hs_header->page_code + 1) <= mhba->hba_total_pages) {
			hs_header->page_code++;
			if (hs_header->page_code != HS_PAGE_FIRM_CAP) {
				mvumi_hs_build_page(mhba, hs_header);
				HS_SET_STATE(hs_fun, HS_S_SEND_PAGE);
			} else {
				HS_SET_STATE(hs_fun, HS_S_QUERY_PAGE);
			}
		} else {
			HS_SET_STATE(hs_fun, HS_S_END);
		}
		HS_SET_STATUS(hs_fun, HS_STATUS_OK);
		mw32(CPU_PCIEA_TO_ARM_MSG0, hs_fun);
		mw32(CPU_PCIEA_TO_ARM_DRBL_REG, DRBL_HANDSHAKE);
		break;

	case HS_S_END:

		/* Set communication list ISR */
		tmp = mr32(CPU_ENPOINTA_MASK_REG);
		tmp |= INT_MAP_COMAOUT | INT_MAP_COMAERR;
		mw32(CPU_ENPOINTA_MASK_REG, tmp);
		writel(mhba->list_num_io,mhba->ib_shadow);
		/* Set InBound List Avaliable count shadow */
		mw32(CLA_INB_AVAL_COUNT_BASEL, mhba->ib_shadow_phys);
		mw32(CLA_INB_AVAL_COUNT_BASEH,
		     (mhba->ib_shadow_phys >> 16) >> 16);

		if (IS_CHIP_8180(mhba)) {
			/* Set OutBound List Avaliable count shadow */
			writel((u32)((mhba->list_num_io-1)|CL_POINTER_TOGGLE ),mhba->ob_shadow);
			mw32(0x5B0, mhba->ob_shadow_phys);
			mw32(0x5B4, (mhba->ob_shadow_phys >> 16) >> 16);
		}
		#ifdef MU_OB_COALESCING
				mw32(0x568,(u32)0x10040);
		#endif

		mhba->ib_cur_slot = (mhba->list_num_io - 1) | CL_POINTER_TOGGLE;
		mhba->ob_cur_slot = (mhba->list_num_io - 1) | CL_POINTER_TOGGLE;

		if(is_workround_dev(mhba))
			mhba->list_workaround_flag = 0;

		mhba->fw_state = FW_STATE_STARTED;

		break;
	default:
		return -1;
	}
	return 0;
}

static u8 mvumi_handshake_event(struct mv_hba *mhba)
{
	void *regs = mhba->mmio;
	u32 isr_status;		// isr_bits;
	unsigned long before;
	before = jiffies;
	mvumi_handshake(mhba);
	do {
    	isr_status = mr32(CPU_MAIN_INT_CAUSE_REG);
		isr_status = mhba->instancet->read_fw_status_reg(mhba->mmio);

		if (mhba->fw_state == FW_STATE_STARTED)
			return 0;	/* handshake finished */
		if (time_after(jiffies, before + FW_MAX_DELAY * HZ)) {
			mv_printk("no handshake response at 0x%x.\n",
				  mhba->fw_state);
			mv_printk("falied global_isr=0x%x,isr_status=0x%x.\n",
				  mhba->global_isr, isr_status);
			return -1;
		}
		rmb();
		msleep(1);
	} while (!(isr_status & DRBL_HANDSHAKE_ISR));

	return 0;
}

static u8 mvumi_check_handshake(struct mv_hba *mhba)
{
	void *regs = mhba->mmio;
	u32 tmp;
#if 0
	if (mr32(CPU_ARM_TO_PCIEA_MSG1) != HANDSHAKE_READYSTATE) {
		mvumi_add_timer(&mhba->hs_timer, 1, mvumi_check_handshake,
				mhba);
		return 0;
	}
#else

	unsigned long before;
	before = jiffies;

	/* Check handshake signature */

	tmp = mr32(CPU_ARM_TO_PCIEA_MSG1);
	while ((tmp != HANDSHAKE_READYSTATE) && (tmp != HANDSHAKE_DONESTATE)) {
		if (tmp != HANDSHAKE_READYSTATE) {
			mw32(CPU_PCIEA_TO_ARM_DRBL_REG,   DRBL_MU_RESET);
		}
		if (time_after(jiffies, before + FW_MAX_DELAY * HZ)) {
			mv_printk(" invalid handshake signature 0x%x.\n",
			       tmp);
			return -1;
		}
		msleep(1);
		rmb();
		tmp = mr32(CPU_ARM_TO_PCIEA_MSG1);
	}

#endif
	mv_printk("Handshake signature = 0x%x.\n", mr32(CPU_ARM_TO_PCIEA_MSG1));

	/* Start firmware handshake */
	mhba->fw_state = FW_STATE_STARTING;
	mv_printk("start firmware handshake.\n");
	while (1) {
		if (mvumi_handshake_event(mhba)) {
			mv_printk("start firmware failed at state 0x%x.\n",
				  mhba->fw_state);
			return -1;
		}

		if (mhba->fw_state == FW_STATE_STARTED)
			break;
	}
	mv_printk("mhba->fw_state at  0x%x.\n",   mhba->fw_state);
	return 0;
}

static int mvumi_request_irq(struct mv_hba *mhba);

static u8 mvumi_start(struct mv_hba *mhba)
{
	void *regs = mhba->mmio;
	u32 tmp;
	/* clear Door bell */
	tmp = mr32(CPU_ARM_TO_PCIEA_DRBL_REG);
	mw32(CPU_ARM_TO_PCIEA_DRBL_REG, tmp);

	mw32(CPU_ARM_TO_PCIEA_MASK_REG, 0x3FFFFFFF);
	tmp=mr32(CPU_ENPOINTA_MASK_REG)|INT_MAP_DL_CPU2PCIEA;
	mw32(CPU_ENPOINTA_MASK_REG,tmp);
	if (mvumi_check_handshake(mhba)) {
		goto err_wait_cmpl;
	}

	mv_printk("CLA_INB_LIST_BASEL=0x%x.\n",mr32(CLA_INB_LIST_BASEL));
	mv_printk("CLA_INB_LIST_BASEH=0x%x.\n",mr32(CLA_INB_LIST_BASEH));
	mv_printk("CLA_OUTB_LIST_BASEL=0x%x.\n",mr32(CLA_OUTB_LIST_BASEL));
	mv_printk("CLA_OUTB_LIST_BASEH=0x%x.\n",mr32(CLA_OUTB_LIST_BASEH));
	mv_printk("firmware handshake ok.\n");
	return 0;

err_wait_cmpl:
	mv_printk( "firmware handshake failed.\n");
	return -1;
}

#if defined(SUPPORT_EVENT)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
#define MV_INIT_WORK(w, f, d)	INIT_WORK(w, f)
#else
#define MV_INIT_WORK(w, f, d)	INIT_WORK(w, f, (void *) d)
#endif


#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
static void mvumi_proc_msg(struct work_struct *work)
{
	struct mvumi_events_wq *mu_ev =
		container_of(work, struct mvumi_events_wq, work_q);
#else
static void mvumi_proc_msg(void *arg)
{
	struct mvumi_events_wq *mu_ev = arg;
#endif
	struct mv_hba *mhba = mu_ev->mhba;
	struct mvumi_hotplug_event *param = mu_ev->param;
	u16 size = param->size;
	const unsigned long *ar_bitmap;
	const unsigned long *re_bitmap;
	int index;

	if (mhba->flag & MVUMI_FW_ATTACH) {
		index = -1;
		ar_bitmap = (const unsigned long *) param->bitmap;
		re_bitmap = (const unsigned long *) &param->bitmap[size >> 3];

		mutex_lock(&mhba->sas_discovery_mutex);
		do {
			index = find_next_zero_bit(ar_bitmap, size, index + 1);
			if (index >= size)
				break;
			mvumi_handle_hotplug(mhba, index, DEVICE_ONLINE);
		} while (1);

		index = -1;
		do {
			index = find_next_zero_bit(re_bitmap, size, index + 1);
			if (index >= size)
				break;
			mvumi_handle_hotplug(mhba, index, DEVICE_OFFLINE);
		} while (1);
		mutex_unlock(&mhba->sas_discovery_mutex);
	}
	kfree(mu_ev);
}

static void mvumi_launch_mesg(struct mv_hba *mhba,
				struct mvumi_hotplug_event *param)
{
	struct mvumi_events_wq *mu_ev;
	int size = (param->size >> 2) + sizeof(struct mvumi_hotplug_event);
	mu_ev = kzalloc(sizeof(*mu_ev), GFP_ATOMIC);
	if (!mu_ev)
		goto launch_abort;
	mu_ev->param = kzalloc(size, GFP_ATOMIC);
	if (!mu_ev->param) {
		kfree(mu_ev);
		goto launch_abort;
	}
	MV_INIT_WORK(&mu_ev->work_q, mvumi_proc_msg, mu_ev);
	mu_ev->mhba = mhba;
	mu_ev->event = EVENT_LOG_GENERATED;
	memcpy(mu_ev->param, param, size);
	schedule_work(&mu_ev->work_q);
	return;
launch_abort:
	mv_printk("Failed to allocate memory for operation!");
	return;
}

static void mvumi_notification(struct mv_hba *mhba,
			u8 msg,
			void *buffer)
{
	if (msg == APICDB1_HOST_GETEVENT) {
		struct mvumi_hotplug_event *param = buffer;
#if 0//_MVUMI_DUMP
		print_hex_dump("", "", DUMP_PREFIX_NONE, 16, 1,
			(void *) param,
			(param->size >> 2) + sizeof(struct mvumi_hotplug_event),
			1);
#endif
		mvumi_launch_mesg(mhba, param);
	}
}

static int mvumi_get_event(struct mv_hba *mhba, u8 msg)
{
	struct mvumi_cmd *cmd;
	struct mvumi_msg_frame *frame;
	cmd = mvumi_get_int_cmd(mhba, 1);
	if (!cmd)
		return -1;
	cmd->scmd = NULL;
	cmd->mhba = mhba;
	frame = cmd->frame;

	frame->device_id = VIRTUAL_DEVICE_ID;
	frame->cmd_flag = CMD_FLAG_DATA_IN;
	frame->req_function = CL_FUN_SCSI_CMD;
	frame->cdb_length = MAX_CDB_SIZE;
	frame->sg_counts = 1;
	frame->data_transfer_length = sizeof(struct mvumi_event_req);
	memset(frame->cdb, 0, MAX_CDB_SIZE);
	frame->cdb[0] = APICDB0_EVENT;
	frame->cdb[1] = msg;
	mvumi_issue_blocked_cmd(mhba, cmd);
	return cmd->cmd_status;

}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
static void mvumi_scan_events(struct work_struct *work)
{
	struct mvumi_events_wq *mu_ev =
		container_of(work, struct mvumi_events_wq, work_q);
#else
static void mvumi_scan_events(void *arg)
{
	struct mvumi_events_wq *mu_ev = arg;
#endif
	struct mv_hba *mhba = mu_ev->mhba;
	u8 msg = mu_ev->event;

	mutex_lock(&mhba->sas_discovery_mutex);
	mvumi_get_event(mhba, msg);
	mutex_unlock(&mhba->sas_discovery_mutex);
	kfree(mu_ev);
}

static void mvumi_launch_events(struct mv_hba *mhba, u8 msg)
{
	struct mvumi_events_wq *mu_ev;
	mu_ev = kzalloc(sizeof(*mu_ev), GFP_ATOMIC);
	if (mu_ev) {
		MV_INIT_WORK(&mu_ev->work_q, mvumi_scan_events, mu_ev);
		mu_ev->mhba = mhba;
		mu_ev->event = msg;
		mu_ev->param = NULL;
		schedule_work(&mu_ev->work_q);
	} else
		mv_printk( "can't get all events.\n");
}

#endif /* #if defined(SUPPORT_EVENT) */

/**
 * mvumi_complete_cmd -	Completes a command
 * @lhba:			Adapter soft state
 * @cmd:			Command to be completed
 */
static void mvumi_complete_cmd(struct mv_hba *mhba, struct mvumi_cmd *cmd)
{
	struct scsi_cmnd *scmd = cmd->scmd;
	struct mvumi_sense_data *sense_buf =
	    (struct mvumi_sense_data *)scmd->sense_buffer;
	mvumi_del_timer(&cmd->eh_timer);
	if (cmd->scmd)
		cmd->scmd->SCp.ptr = NULL;
	if ((cmd->cmd_status != REQ_STATUS_SUCCESS)
	    && (cmd->frame->cdb[0] != INQUIRY))
		mv_printk("device %d:cmd=0x%x,cmd->cmd_status=0x%x.\n",
			  cmd->frame->device_id, cmd->frame->cdb[0],
			  cmd->cmd_status);

	switch (cmd->cmd_status) {
	case REQ_STATUS_SUCCESS:
		scmd->result = (DID_OK << 16);
		break;
	case REQ_STATUS_MEDIA_ERROR:
		scmd->result = (DID_BAD_TARGET << 16);
		break;
	case REQ_STATUS_BUSY:
		scmd->result = (DID_BUS_BUSY << 16);
		break;
	case REQ_STATUS_NO_DEVICE:
		scmd->result = (DID_NO_CONNECT << 16);
		break;
	case REQ_STATUS_HAS_SENSE:
		/* Sense buffer data is valid already. */
		scmd->result = (DRIVER_SENSE << 24) | (DID_OK << 16);
		sense_buf->valid = 1;

#ifdef MV_DEBUG
		mv_printk(" sense response %x SK %s length %x "
			  "ASC %x ASCQ %x.\n", ((u8 *) sense_buf)[0],
			  mvumi_dump_sense_key(((u8 *) sense_buf)[2]),
			  ((u8 *) sense_buf)[7],
			  ((u8 *) sense_buf)[12], ((u8 *) sense_buf)[13]);
#endif
		break;
	default:
		scmd->result = DRIVER_INVALID << 24;
		scmd->result |= DID_ABORT << 16;
		break;
	}

	if (mv_rq_bf_l(scmd)) {
		if (mv_use_sg(scmd)) {
			pci_unmap_sg(mhba->pdev,
				     mv_rq_bf(scmd),
				     mv_use_sg(scmd),
				     scsi_to_pci_dma_dir(scmd->
							 sc_data_direction));
		} else {
			pci_unmap_single(mhba->pdev,
					 scmd->SCp.dma_handle,
					 mv_rq_bf_l(scmd),
					 scsi_to_pci_dma_dir(scmd->
							     sc_data_direction));

			scmd->SCp.dma_handle = 0;
		}
	}
	cmd->scmd->scsi_done(scmd);
	mvumi_return_cmd(mhba, cmd);
}

void
mvumi_translate_status(struct mvumi_rsp_frame *ob_frame, struct mvumi_cmd *cmd)
{
	struct mvumi_sense_data *sense_buf;
	if (ob_frame->req_status == SCSI_STATUS_GOOD)
		cmd->cmd_status = REQ_STATUS_SUCCESS;
	else if (ob_frame->req_status == SCSI_STATUS_BUSY)
		cmd->cmd_status = REQ_STATUS_BUSY;
	else if (ob_frame->req_status == SCSI_STATUS_CHECK_CONDITION) {
		sense_buf = (struct mvumi_sense_data *)ob_frame->payload;

		if ((sense_buf->sense_key == SCSI_SK_HARDWARE_ERROR) &&
		    (sense_buf->additional_sense_code ==
		     			SCSI_ASC_LOGICAL_UNIT_NOT_RESP_TO_SEL))
			cmd->cmd_status = REQ_STATUS_NO_DEVICE;
		else if (sense_buf->sense_key == SCSI_SK_ILLEGAL_REQUEST)
			cmd->cmd_status = REQ_STATUS_INVALID_REQUEST;
		else if (sense_buf->sense_key == SCSI_SK_VENDOR_SPECIFIC)
			cmd->cmd_status = REQ_STATUS_ERROR;
		else {
			//mv_printk("get sense key = %X\n",
			//	sense_buf->sense_key);
			cmd->cmd_status = REQ_STATUS_ERROR_WITH_SENSE;
			if ((cmd->scmd) /* os command */ &&
			    ob_frame->rsp_flag & CL_RSP_FLAG_SENSEDATA)
				memcpy(cmd->scmd->sense_buffer,
				       ob_frame->payload,
				       sizeof(struct mvumi_sense_data));
		}
	}
	else
		cmd->cmd_status = REQ_STATUS_ERROR;

	if ((cmd->scmd) /* os command */ &&
	    (ob_frame->rsp_flag & CL_RSP_FLAG_SENSEDATA) &&
	    (ob_frame->req_status == SCSI_STATUS_CHECK_CONDITION)) {
		memcpy(cmd->scmd->sense_buffer,
		       ob_frame->payload, sizeof(struct mvumi_sense_data));
	}
	return;
}
static void mvumi_fire_cmd(struct mv_hba *mhba, struct mvumi_cmd *mvcmd);

static u8 mvumi_handle_clob(struct mv_hba *mhba)
{
	struct mvumi_rsp_frame *ob_frame;
	struct mvumi_cmd *cmd;
	struct mv_ob_data_pool *pool;
	while (!list_empty(&mhba->free_ob_list)) {
		pool = list_get_first_entry(&mhba->free_ob_list,
					  struct mv_ob_data_pool,
					  queue_pointer);
		
		list_add_tail(&pool->queue_pointer, &mhba->ob_data_pool_list);

		ob_frame = (struct mvumi_rsp_frame *)&pool->ob_data[0];
		cmd = mhba->tag_cmd[ob_frame->tag];

	    	//MV_ASSERT(cmd != 0);
	    	if(cmd==NULL){
			mv_printk(" got a TAG [%d] with NO command\n",ob_frame->tag);
			return 0;
	    		} else
	    		atomic_dec(&mhba->fw_outstanding);
		mhba->tag_cmd[ob_frame->tag] = 0;
		tag_release_one(mhba, &mhba->tag_pool, ob_frame->tag);
		mvumi_translate_status(ob_frame, cmd);
		if ((cmd->cmd_status != REQ_STATUS_SUCCESS)
		    && (cmd->frame->cdb[0] != INQUIRY)) {
			//mvumi_dump_cmd_info(cmd->frame, MV_FALSE);
			//mvumi_dump_ob_info(ob_frame);
		}
		/* Check is OS command or internal command */
		if (cmd->scmd) {
			if ((cmd->cmd_status == REQ_STATUS_SUCCESS)
			    && (cmd->frame->cdb[0] == INQUIRY)) {
			}

#if 0
			if (cmd->frame->cdb[0] == READ_CAPACITY) {
				u32 *buffer;
				struct scatterlist *sg =
				    (struct scatterlist *)mv_rq_bf(cmd->scmd);
				buffer = (u32 *) mvumi_sgd_kmap(sg, cmd);

				mv_printk
				    ("device %d read capacity size=0x%x,sector=0x%x.\n",
				     cmd->frame->device_id, buffer[0],
				     buffer[1]);
				mvumi_sgd_kunmap(sg, cmd, buffer);
			}
#endif
			mvumi_complete_cmd(mhba, cmd);
		} else {
			if (cmd->frame->cdb[0] == APICDB0_EVENT)
				mvumi_notification(mhba, cmd->frame->cdb[1],
						cmd->data_buf);
			if (cmd->sync_cmd) {
				cmd->sync_cmd = 0;
				wake_up(&mhba->int_cmd_wait_q);
			}
			mvumi_complete_int_cmd(mhba, cmd);
		}
	}
	mvumi_fire_cmd(mhba,NULL);
	return 0;
}

EXPORT_SYMBOL(mvumi_handle_clob);

/**
 * mvumi_complete_cmd_dpc	 -	Returns FW's controller structure
 * @instance_addr:			Address of adapter soft state
 *
 * Tasklet to complete cmds
 */
static void mvumi_complete_cmd_dpc(unsigned long instance_addr)
{
	u32 isr_bit, isr_status;
	struct mv_hba *mhba = (struct mv_hba *)instance_addr;
	unsigned long flags;
	//void *regs = mhba->mmio;
	/* If we have already declared adapter dead, donot complete cmds */
	if (mhba->hw_crit_error)
		return;

	spin_lock_irqsave(&mhba->mv_isr_lock, flags);
	isr_bit = mhba->global_isr;
	isr_status = mhba->mv_isr_status;
	mhba->global_isr = 0;
	mhba->mv_isr_status = 0;
	spin_unlock_irqrestore(&mhba->mv_isr_lock, flags);
	if (!isr_bit){
		return;
	}
	if (isr_bit & INT_MAP_DL_CPU2PCIEA) {
		if (isr_status & DRBL_HANDSHAKE_ISR) {
			mv_printk("Enter handler shake again!.\n");
			mvumi_handshake(mhba);
		}

#ifdef SUPPORT_INTERNAL_REQ
		if (isr_status & DRBL_BUS_CHANGE) {
			mv_printk("DRBL_BUS_CHANGE isr\n");
			/* reenumerate the bus by issuing INQUIRY commands
			 * Need to get device id */
			mvumi_launch_events(mhba, APICDB1_HOST_GETEVENT);
		}
		if (isr_status & DRBL_EVENT_NOTIFY)
			mvumi_launch_events(mhba, APICDB1_EVENT_GETEVENT);
#endif
	}

	if (isr_bit & INT_MAP_COMAERR) {
		mv_printk( "firmware error.\n");
	}
	spin_lock_irqsave(&mhba->mv_timer_lock, flags);
	if (isr_bit & INT_MAP_COMAOUT) {
		mvumi_receive_ob_list_entry(mhba);
	}
	if (mhba->fw_state == FW_STATE_STARTED) {
		mvumi_handle_clob(mhba);
	}
	spin_unlock_irqrestore(&mhba->mv_timer_lock, flags);
	
}

static int mvumi_isr_handler(void *devp)
{
	struct mv_hba *mhba = (struct mv_hba *)devp;
	/*
	 * Check if it is our interrupt
	 * Clear the interrupt
	 */
	if (mhba->instancet->clear_intr(mhba))
		return IRQ_NONE;

	if (mhba->hw_crit_error)
		goto out_done;
	#ifndef  SUPPORT_TASKLET
	mvumi_complete_cmd_dpc((long)mhba);
	#else
	/*
	 * Schedule the tasklet for cmd completion
	 */
	tasklet_schedule(&mhba->isr_tasklet);
	#endif
out_done:
	return IRQ_HANDLED;

}

/**
 * mvumi_isr_entry -	Processes all completed commands
 * @devp:				Adapter soft state
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
static irqreturn_t mvumi_isr_entry(int irq, void *devp, struct pt_regs *regs)
{
	return mvumi_isr_handler(devp);
}

#else
static irqreturn_t mvumi_isr_entry(int irq, void *devp)
{
	return mvumi_isr_handler(devp);
}
#endif /* #if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19) */

static int mvumi_request_irq(struct mv_hba *mhba)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 19)
	return request_irq(mhba->pdev->irq, mvumi_isr_entry, IRQF_SHARED,
			   "mvumi", mhba);
#else
	return request_irq(mhba->pdev->irq, mvumi_isr_entry, SA_SHIRQ, "mvumi",
			   mhba);
#endif
}

static int mvumi_pre_init(struct mv_hba *mhba)
{
	mhba->handshake_page_base =
			(void *)pci_alloc_consistent(mhba->pdev,
				HSP_MAX_SIZE,
				&mhba->handshake_page_phys_base);
	if (!mhba->handshake_page_base) {
		mv_printk(" Failed to allocate memory for handshake\n");
		return -1;
	}
	return 0;
}

static int mvumi_set_hs_page(struct mv_hba *mhba)
{
	/*
	* borrow InBound List memory for hand shake temporarily
	* promise the page at same 512K rounding area.
	*/
	if (mhba->handshake_page_base)
		pci_free_consistent(mhba->pdev,
				    HSP_MAX_SIZE,
				    mhba->handshake_page_base,
				    mhba->handshake_page_phys_base);
	mhba->handshake_page_base = mhba->ib_list;
	mhba->handshake_page_phys_base = mhba->ib_list_phys;
	return 0;
}

/**
 * mvumi_init_data -	Initialize requested date for FW
 */
static int mvumi_init_data(struct mv_hba *mhba)
{
	struct mv_ob_data_pool *ob_pool;
	struct mvumi_res_mgnt *res_mgnt;
	struct mvumi_event_entry *event_pool;
	u32 i;
	u32 tmp_size, offset;
	void *virmem;
	dma_addr_t p;
	void *v;
	if (mhba->flag & MVUMI_FW_ALLOC)
		return 0;
	tmp_size=128+mhba->ib_max_entry_size_bytes * mhba->max_io +16 + sizeof(u32) ;
	tmp_size+=8+mhba->ob_max_entry_size_bytes * mhba->max_io+4;
	if(is_workround_dev(mhba)) {
		tmp_size+=8+mhba->ib_max_entry_size_bytes *1;
		tmp_size+=8+mhba->ob_max_entry_size_bytes *1;
	}
	res_mgnt = mvumi_alloc_mem_resource(mhba,
			RESOURCE_UNCACHED_MEMORY, tmp_size);
	if (!res_mgnt) {
		mv_printk(" Failed to allocate memory for inbound list\n");
		goto fail_alloc_dma_buf;
	}

	//ib_list
	p=res_mgnt->bus_addr;
	v=res_mgnt->virt_addr;
	offset = ROUNDING(res_mgnt->bus_addr, 128) - res_mgnt->bus_addr;
	p+=offset;
	v=(u8 *)v+offset;
 	mhba->ib_list = (u8 *)v;
 	mhba->ib_list_phys = p;
	//ib_shadow
	v=(u8 *)v+mhba->ib_max_entry_size_bytes * mhba->max_io;
	p=p+mhba->ib_max_entry_size_bytes * mhba->max_io;
	if(is_workround_dev(mhba)) {
		v=(u8 *)v+mhba->ib_max_entry_size_bytes * 1;
		p=p+mhba->ib_max_entry_size_bytes * 1;
	}

	offset = ROUNDING(p, 8) - p;
	p+=offset;
	v=(u8 *)v+offset;
	mhba->ib_shadow =(u8 *)v;
	mhba->ib_shadow_phys =p;
	//ob_shadow
	p+=sizeof(u32);
	v=(u8 *)v+sizeof(u32);
	if(!IS_CHIP_8180(mhba)){
		offset = ROUNDING(p, 4) - p;
		p+=offset;
		v=(u8 *)v+offset;
		mhba->ob_shadow=v;
		mhba->ob_shadow_phys=p;
		p+=4;
		v=(u8 *)v+4;
		} else{
		offset = ROUNDING(p,8) - p;
		p+=offset;
		v=(u8 *)v+offset;
		mhba->ob_shadow=v;
		mhba->ob_shadow_phys=p;
		p+=8;
		v=(u8 *)v+8;
			}
	mhba->ob_list=v;
	mhba->ob_list_phys=p;
	tmp_size = sizeof(struct mv_ob_data_pool) * mhba->max_io + 8;
	res_mgnt = mvumi_alloc_mem_resource(mhba,
			RESOURCE_CACHED_MEMORY, tmp_size);
	if (!res_mgnt) {
		mv_printk(" Failed to allocate memory for"
			  " outbound data buffer\n");
		goto fail_alloc_dma_buf;
	}
	virmem = res_mgnt->virt_addr;
	for (i = mhba->max_io; i != 0; i--) {
		ob_pool = (struct mv_ob_data_pool *)virmem;
		list_add_tail(&ob_pool->queue_pointer,
			      &mhba->ob_data_pool_list);
		virmem = (u8 *) virmem + sizeof(struct mv_ob_data_pool);
	}

#ifdef SUPPORT_EVENT
	/* event buffer */
	tmp_size = sizeof(struct mvumi_event_entry) * MAX_EVENTS;
	res_mgnt = mvumi_alloc_mem_resource(mhba,
			RESOURCE_CACHED_MEMORY, tmp_size);
	if (!res_mgnt) {
		mv_printk(" Failed to allocate memory for "
			  "mvumi_event_entry.\n");
		goto fail_alloc_dma_buf;
	}
	virmem = res_mgnt->virt_addr;
	mhba->num_stored_events = 0;
	mhba->num_pool_events = 0;
	for (i = 0; i < MAX_EVENTS; i++) {
		event_pool = (struct mvumi_event_entry *)virmem;
		list_add_tail(&event_pool->queue_pointer, &mhba->free_events);
		virmem = (u8 *) virmem + sizeof(struct mvumi_event_entry);
	}
#endif /* #ifdef SUPPORT_EVENT */

#ifdef SUPPORT_INTERNAL_REQ
	/* Internal Request DataBuffer */
	mhba->int_req_size = SECTOR_LENGTH * MAX_INTERNAL_REQ;
	tmp_size = mhba->int_req_size * 2 + 8;

	res_mgnt = mvumi_alloc_mem_resource(mhba,
				RESOURCE_UNCACHED_MEMORY, tmp_size);
	if (!res_mgnt) {
		mv_printk(" Failed to allocate memory for "
			  "internal request buffer\n");
		mhba->int_req_size = 0;
		goto fail_alloc_dma_buf;
	}
	offset = ROUNDING(res_mgnt->bus_addr, 8) - res_mgnt->bus_addr;
	mhba->int_req_buf = (u8 *) res_mgnt->virt_addr + offset;
	mhba->int_req_phys = res_mgnt->bus_addr + offset;
	mhba->event_int_req_buf = mhba->int_req_buf + mhba->int_req_size;
	mhba->event_int_req_phys = res_mgnt->bus_addr + mhba->int_req_size;
#endif

	/* initialize tag stack */
	mhba->tag_pool.stack = (u16 *) mhba->tag_num;
	mhba->tag_pool.size = (mhba->max_io + MAX_INTERNAL_REQ);
	tag_init(&mhba->tag_pool, (mhba->max_io + MAX_INTERNAL_REQ));
	mhba->flag |= MVUMI_FW_ALLOC;
	
	return 0;

fail_alloc_dma_buf:
	mvumi_release_mem_resource(mhba);
	return -1;

}
#if 0
static void dwordcpy(void * to, const void * from, size_t n)
{
        u32 *p_dst=(u32*)to;
        u32 *p_src=(u32*)from;
        n/=sizeof(u32);
        while(n--)
                *p_dst++=*p_src++;
}
#endif
static void mvumi_handle_timer(struct mvumi_cmd *cmd)
	{
        struct mv_hba *mhba=cmd->mhba;
        unsigned long flags;
        if((mhba->fw_state==FW_STATE_STARTED)&&(atomic_read( &mhba->fw_outstanding)))
        {
        	    mv_printk("%s:%d outstanding=%d\n",__FUNCTION__,__LINE__,atomic_read( &mhba->fw_outstanding));
                spin_lock_irqsave(&mhba->mv_timer_lock, flags);
                mvumi_receive_ob_list_entry(mhba);
                mvumi_handle_clob(mhba);
                spin_unlock_irqrestore(&mhba->mv_timer_lock, flags);
         }
        return ;
}

/**
 * mvumi_fire_cmd -	Sends command to the FW
 */
static enum MV_QUEUE_COMMAND_RESULT
mvumi_send_command(struct mv_hba *mhba, struct mvumi_cmd *cmd)
{
	void *ib_entry;
	struct mvumi_msg_frame *ib_frame;
	ib_frame = cmd->frame;
	if (mhba->fw_state != FW_STATE_STARTED) {
		mv_printk("firmware not ready.\n");
		//MV_ASSERT(0);
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}

	if (tag_is_empty(&mhba->tag_pool)){
		mv_printk("no free tag.\n");
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}
	if (mvumi_get_ib_list_entry(mhba, &ib_entry)){
		//mv_printk("mvumi_get_ib_list_entry failed.\n");
		return MV_QUEUE_COMMAND_RESULT_NO_RESOURCE;
	}

	cmd->frame->tag = tag_get_one(mhba, &mhba->tag_pool);
//	mvumi_dump_cmd_info(cmd->frame, MV_FALSE);
	mvumi_add_timer(&cmd->eh_timer,5,(void *)mvumi_handle_timer,(long)cmd);
	mhba->tag_cmd[cmd->frame->tag] = cmd;
	memcpy(ib_entry, ib_frame, mhba->ib_max_entry_size_bytes);
	//dwordcpy(ib_entry, ib_frame, mhba->ib_max_entry_size_bytes);
	//tmp=(struct mvumi_msg_frame *)(ib_entry);
	//tmp->tag=ib_frame->tag;
	/* Request is sent to the hardware and not finished yet. */
	return MV_QUEUE_COMMAND_RESULT_SENT;

}

static void mvumi_fire_cmd(struct mv_hba *mhba, struct mvumi_cmd *mvcmd)
{
	u16 num_of_cl_sent = 0;
	struct mvumi_cmd *cmd=mvcmd;
	unsigned long flags;
	enum MV_QUEUE_COMMAND_RESULT result;
	
	spin_lock_irqsave(&mhba->mv_waiting_req_lock,flags);
	if(cmd){
		list_add_tail(&cmd->queue_pointer, &mhba->waiting_req_list);
		}


	while (!list_empty(&mhba->waiting_req_list)) {
		cmd = list_get_first_entry(&mhba->waiting_req_list,
					 struct mvumi_cmd, queue_pointer);
		result = mvumi_send_command(mhba, cmd);
		switch (result) {
		case MV_QUEUE_COMMAND_RESULT_SENT:
			num_of_cl_sent++;
			break;
		case MV_QUEUE_COMMAND_RESULT_FINISHED:
			mvumi_complete_cmd(mhba, cmd);
			break;
		case MV_QUEUE_COMMAND_RESULT_FULL:
		case MV_QUEUE_COMMAND_RESULT_NO_RESOURCE:
			list_add(&cmd->queue_pointer, &mhba->waiting_req_list);
			if (num_of_cl_sent > 0) {
				mvumi_send_ib_list_entry(mhba);
			}
			spin_unlock_irqrestore(&mhba->mv_waiting_req_lock,flags);
			return;
		}
	}
	if (num_of_cl_sent > 0) {
		mvumi_send_ib_list_entry(mhba);
	}
	spin_unlock_irqrestore(&mhba->mv_waiting_req_lock,flags);
}

/**
 * mvumi_enable_intr -	Enables interrupts
 * @regs:			FW register set
 */
static void mvumi_enable_intr(void *regs)
{
	u32 mask;
	mw32(CPU_ARM_TO_PCIEA_MASK_REG, 0x3FFFFFFF);

	mask = mr32(CPU_ENPOINTA_MASK_REG);
	mask |= INT_MAP_DL_CPU2PCIEA | INT_MAP_COMAOUT | INT_MAP_COMAERR;
	mw32(CPU_ENPOINTA_MASK_REG, mask);

}

/**
 * mvumi_disable_intr -Disables interrupt
 * @regs:			FW register set
 */
static void mvumi_disable_intr(void *regs)
{
	u32 mask;
	mw32(CPU_ARM_TO_PCIEA_MASK_REG, 0);

	mask = mr32(CPU_ENPOINTA_MASK_REG);
	mask &= ~(INT_MAP_DL_CPU2PCIEA | INT_MAP_COMAOUT | INT_MAP_COMAERR);
	mw32(CPU_ENPOINTA_MASK_REG, mask);
}

/**
 * mvumi_clear_intr -	Check & clear interrupt
 * @regs:				FW register set
 */
static int mvumi_clear_intr(void *extend)
{
	struct mv_hba *mhba = (struct mv_hba *)extend;
	u32 status, isr_status = 0,tmp=0;
	unsigned long flags;

	void *regs = mhba->mmio;
	/*
	 * Check if it is our interrupt
	 */
	status = mr32(CPU_MAIN_INT_CAUSE_REG);

	if (!(status & INT_MAP_MU)) {
		return 1;
	}

	if (status & INT_MAP_COMAERR) {
		/* Clear ISR */
		tmp = mr32(CLA_ISR_CAUSE);
		if (tmp & (CLIC_IN_ERR_IRQ | CLIC_OUT_ERR_IRQ)) {
			mw32(CLA_ISR_CAUSE,
			     (tmp &
			      (CLIC_IN_ERR_IRQ | CLIC_OUT_ERR_IRQ)));
		}
	} 
	if (status & INT_MAP_COMAOUT) {
		/* Clear ISR */
		tmp = mr32(CLA_ISR_CAUSE);
		if (tmp & CLIC_OUT_IRQ) {
			mw32(CLA_ISR_CAUSE, (tmp & CLIC_OUT_IRQ));
		}
	} 
	if (status & INT_MAP_DL_CPU2PCIEA) {
		/* Clear ISR */
		isr_status = mr32(CPU_ARM_TO_PCIEA_DRBL_REG);
		if (isr_status) {
			mw32(CPU_ARM_TO_PCIEA_DRBL_REG, isr_status);
		}
	} 

	spin_lock_irqsave(&mhba->mv_isr_lock, flags);
	mhba->global_isr = status;
	mhba->mv_isr_status = isr_status;
	spin_unlock_irqrestore(&mhba->mv_isr_lock, flags);

	/*
	 * Clear the interrupt by writing back the same value
	 */
	//mw32(CPU_MAIN_INT_CAUSE_REG, status);

	return 0;
}

/**
 * mvumi_read_fw_status_reg - returns the current FW status value
 * @regs:			FW register set
 */
static u32 mvumi_read_fw_status_reg(void *regs)
{
	u32 status;
	status = mr32(CPU_ARM_TO_PCIEA_DRBL_REG);
	if (status)
		mw32(CPU_ARM_TO_PCIEA_DRBL_REG, status);

	return status;
}

static struct mvumi_instance_template mvumi_instance_template = {
	.fire_cmd = mvumi_fire_cmd,
	.enable_intr = mvumi_enable_intr,
	.disable_intr = mvumi_disable_intr,
	.clear_intr = mvumi_clear_intr,
	.read_fw_status_reg = mvumi_read_fw_status_reg,
};

static int mvumi_slave_configure(struct scsi_device *sdev)
{
	/* FIXED ME */
	return 0;
}

/**
 * mvumi_build_frame -	Prepares a direct cdb (DCDB) command
 * @lhba:		Adapter soft state
 * @scmd:		SCSI command
 * @cmd:		Command to be prepared in
 *
 * This function prepares CDB commands. These are typcially pass-through
 * commands to the devices.
 */
static u8
mvumi_build_frame(struct mv_hba *mhba, struct scsi_cmnd *scmd,
		  struct mvumi_cmd *cmd)
{
	struct mvumi_msg_frame *pframe;
	u32 max_sg_len = mhba->ib_max_entry_size_bytes -
				sizeof(struct mvumi_msg_frame) + 4;
	cmd->scmd = scmd;
	cmd->mhba = mhba;
	cmd->cmd_status = REQ_STATUS_PENDING;
	pframe = cmd->frame;
	pframe->device_id = MVUMI_DEV_INDEX(mv_scmd_target(scmd),
					    mv_scmd_lun(scmd));

	/*
	 * Set three flags: CMD_FLAG_NON_DATA
	 *                  CMD_FLAG_DATA_IN
	 *                  CMD_FLAG_DMA
	 * currently data in/out all go thru DMA
	 */
	pframe->cmd_flag = 0;
	switch (scmd->sc_data_direction) {
	case DMA_NONE:
		pframe->cmd_flag |= CMD_FLAG_NON_DATA;
		break;
	case DMA_FROM_DEVICE:
		pframe->cmd_flag |= CMD_FLAG_DATA_IN;
		break;
	case DMA_TO_DEVICE:
		pframe->cmd_flag |= CMD_FLAG_DATA_OUT;
		break;
	case DMA_BIDIRECTIONAL:
		mv_printk( "mvumi : unexpected DMA_BIDIRECTIONAL.\n");
		break;
	default:
		break;
	}

	pframe->cdb_length = scmd->cmd_len;
	memcpy(pframe->cdb, scmd->cmnd, pframe->cdb_length);
	pframe->req_function = CL_FUN_SCSI_CMD;

	/*
	 * Construct SGL
	 */
	pframe->sg_counts =
			(u8) mvumi_make_sgl(mhba, scmd, &pframe->payload[0]);
	pframe->data_transfer_length = mv_rq_bf_l(scmd);
	if ((u32) pframe->sg_counts * sizeof(struct mv_sgl) > max_sg_len) {
		mv_printk("SG counts %d is too large, max_sg_len is %d.\n",
			  pframe->sg_counts, max_sg_len);
		return 0;
	}

	return -1;
}

/**
 * mvumi_queue_command -	Queue entry point
 * @scmd:			SCSI command to be queued
 * @done:			Callback entry point
 */
static int
mvumi_queue_command(struct scsi_cmnd *scmd, void (*done) (struct scsi_cmnd *))
{
	struct mvumi_cmd *cmd;
	struct mv_hba *mhba;
	mhba = (struct mv_hba *)
	    scmd->device->host->hostdata;
	/* Don't process if we have already declared adapter dead */
	if (mhba->hw_crit_error)
		return SCSI_MLQUEUE_HOST_BUSY;


	scmd->scsi_done = done;
	scmd->result = 0;

	if (MVUMI_DEV_INDEX(mv_scmd_target(scmd), mv_scmd_lun(scmd))
				> mhba->max_target_id ||
				mv_scmd_channel(scmd)) {
		scmd->result = DID_BAD_TARGET << 16;
		goto out_done;
	}

#if 1
/* workround for VILI : not support REPORT_LUNS */
	if((scmd->cmnd[0] == REPORT_LUNS) && (IS_CHIP_VILI(mhba))){
		scmd->result = DID_BAD_TARGET << 16;
		goto out_done;
	}
#endif


	cmd = mvumi_get_cmd(mhba);
	if (!cmd)
		return SCSI_MLQUEUE_HOST_BUSY;

	/* build firmware frame */
	if (!mvumi_build_frame(mhba, scmd, cmd))
		goto out_return_cmd;

	cmd->scmd = scmd;
	scmd->SCp.ptr = (char *)cmd;

	/*
	 * Issue the command to the FW
	 */
	mhba->instancet->fire_cmd(mhba, cmd);
	return 0;

out_return_cmd:
	mvumi_return_cmd(mhba, cmd);
out_done:
	done(scmd);
	return 0;
}

/**
 * mvumi_reset_device -	Device reset handler entry point
 */
static int mvumi_reset_device(struct scsi_cmnd *scmd)
{
	int ret;
	mv_printk("line %d: %s.\n", __LINE__, __FUNCTION__);
	/*
	 * First wait for all commands to complete
	 */
	ret = mvumi_generic_reset(scmd);

	return ret;
}

/**
 * mvumi_reset_bus_host -	Bus & host reset handler entry point
 */
static int mvumi_reset_bus_host(struct scsi_cmnd *scmd)
{
	int ret;
	mv_printk("line %d: %s.\n", __LINE__, __FUNCTION__);

	/*
	 * First wait for all commands to complete
	 */
	ret = mvumi_generic_reset(scmd);

	return ret;
}

/**
 * mvumi_reset_timer - quiesce the adapter if required
 * @scmd:		scsi cmnd
 *
 * Sets the FW busy flag and reduces the host->can_queue if the
 * cmd has not been completed within the timeout period.
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static enum scsi_eh_timer_return mvumi_reset_timer(struct scsi_cmnd *scmd)
#else
static enum blk_eh_timer_return mvumi_reset_timer(struct scsi_cmnd* scmd)
#endif
{
	struct mvumi_cmd *cmd = (struct mvumi_cmd *)scmd->SCp.ptr;
	struct mv_hba *mhba;
	unsigned long flags;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 10)
	if (time_after(jiffies, jiffies +
		       (MVUMI_DEFAULT_CMD_TIMEOUT * 2) * HZ)) {
		return EH_NOT_HANDLED;
	}
#else
	if (time_after(jiffies, scmd->jiffies_at_alloc +
		       (MVUMI_DEFAULT_CMD_TIMEOUT * 2) * HZ)) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
			return BLK_EH_NOT_HANDLED;
		#else
			return EH_NOT_HANDLED;
		#endif
	}
#endif
	mhba = cmd->mhba;
	spin_lock_irqsave(&mhba->mv_timer_lock, flags);
	if (cmd->cmd_status == REQ_STATUS_PENDING){
               
		if(mhba->tag_cmd[cmd->frame->tag]){
			mhba->tag_cmd[cmd->frame->tag] = 0;
			tag_release_one(mhba, &mhba->tag_pool, cmd->frame->tag);
		}
		mv_printk("reset timer :a command faild\n");
		spin_lock(&mhba->mv_waiting_req_lock);
		if(cmd->queue_pointer.next!=LIST_POISON1 && cmd->queue_pointer.prev!=LIST_POISON2			
			&&cmd->queue_pointer.next!=&cmd->queue_pointer && cmd->queue_pointer.prev!=&cmd->queue_pointer
				&&cmd->queue_pointer.prev!=NULL && cmd->queue_pointer.next!=NULL)
			{
				mv_printk("TIMEOUT:A command doesn't send!\n");
				list_del_init(&cmd->queue_pointer);
			} else
				atomic_dec(&mhba->fw_outstanding);		
		spin_unlock(&mhba->mv_waiting_req_lock);
		mvumi_complete_cmd(mhba, cmd);      
	}
	spin_unlock_irqrestore(&mhba->mv_timer_lock, flags);
	if (!(mhba->flag & MVUMI_FW_BUSY)) {
		/* FW is busy, throttle IO */
		spin_lock_irqsave(mhba->shost->host_lock, flags);

		mhba->last_time = jiffies;
		mhba->flag |= MVUMI_FW_BUSY;

		spin_unlock_irqrestore(mhba->shost->host_lock, flags);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	return EH_RESET_TIMER;
#else
	return BLK_EH_RESET_TIMER;
#endif
}

/**
 * mvumi_bios_param - Returns disk geometry for a disk
 * @sdev: 		device handle
 * @bdev:		block device
 * @capacity:		drive capacity
 * @geom:		geometry parameters
 */
static int
mvumi_bios_param(struct scsi_device *sdev, struct block_device *bdev,
		 sector_t capacity, int geom[])
{
	int heads;
	int sectors;
	sector_t cylinders;
	unsigned long tmp;
	/* Default heads (64) & sectors (32) */
	heads = 64;
	sectors = 32;

	tmp = heads * sectors;
	cylinders = capacity;

	sector_div(cylinders, tmp);

	/*
	 * Handle extended translation size for logical drives > 1Gb
	 */

	if (capacity >= 0x200000) {
		heads = 255;
		sectors = 63;
		tmp = heads * sectors;
		cylinders = capacity;
		sector_div(cylinders, tmp);
	}

	geom[0] = heads;
	geom[1] = sectors;
	geom[2] = cylinders;

	return 0;
}

/*
 * Scsi host template for marvell raid sas driver
 */
static struct scsi_host_template mvumi_template = {

	.module = THIS_MODULE,
	.name = "Marvell Storage Controller",
	.slave_configure = mvumi_slave_configure,
	.queuecommand = mvumi_queue_command,
	.eh_device_reset_handler = mvumi_reset_device,
	.eh_bus_reset_handler = mvumi_reset_bus_host,
	.eh_host_reset_handler = mvumi_reset_bus_host,
#if  LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 7) && \
     LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 16)
	.eh_timed_out = mvumi_reset_timer,
#endif
	.bios_param = mvumi_bios_param,
	//.can_queue                   =  MVUMI_MAX_REQUEST_NUMBER,
	//.this_id                     =  MVUMI_DEFAULT_INIT_ID,
	//.max_sectors                 =  (MVUMI_MAX_REQUEST_NUMBER * PAGE_SIZE)/SECTOR_LENGTH,
	//.sg_tablesize                =  MVUMI_MAX_SG_ENTRY,
	//.cmd_per_lun                 =  MVUMI_MAX_REQUEST_PER_LUN,
	// .use_clustering              =  MVUMI_USE_CLUSTERING,
	//.emulated                    =  MVUMI_EMULATED,
};

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
static struct scsi_transport_template mvumi_transport_template = {
	.eh_timed_out = mvumi_reset_timer,
};
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */

/**
 * mvumi_init_fw -	Initializes the FW
 * @lhba:		Adapter soft state
 *
 * This is the main function for initializing firmware.
 */
static int mvumi_init_fw(struct mv_hba *mhba)
{
	/*
	 * Map the message registers
	 */
	if (pci_request_regions(mhba->pdev, MV_DRIVER_NAME)) {
		mv_printk(" IO memory region busy!\n");
		return -EBUSY;
	}
	if (mvumi_map_pci_addr(mhba->pdev, mhba->base_addr))
		goto fail_ioremap;

	mhba->mmio = mhba->base_addr[0];
	mhba->chip_device_id = mhba->pdev->device;	/* check device id for different product */
	switch (mhba->pdev->device) {
	case PCI_DEVICE_ID_MARVELL_MV8120:
	case PCI_DEVICE_ID_MARVELL_MV8110:
		mhba->hba_capability = 0;
		mhba->instancet = &mvumi_instance_template;
		mhba->max_num_sge = MVUMI_MAX_SG_ENTRY;
		mhba->max_transfer_size = MVUMI_MAX_TRANSFER_SIZE;
		mhba->max_target_id = 1;
		mhba->max_io = 1;
		mhba->list_num_io = 1;
		mhba->max_int_io = 1;

		break;
	default:
		mv_printk(" device 0x%x not supported !\n",
		       mhba->pdev->device);
		mhba->instancet = NULL;
		goto	fail_device_id;
	}
	mv_printk("DEVICE_ID_%04X is found.\n", mhba->pdev->device);
	mhba->ib_max_entry_size_bytes = CL_IN_ENTRY_SIZE;
	mhba->ob_max_entry_size_bytes = CL_OUT_ENTRY_SIZE;
	mhba->ib_max_entry_size_setting = CL_IN_ENTRY_SIZE_SETTING;
	mhba->ob_max_entry_size_setting = CL_OUT_ENTRY_SIZE_SETTING;
	mv_printk("ib max=%d ob max=%d\n",CL_IN_ENTRY_SIZE,CL_OUT_ENTRY_SIZE);
	mv_printk("ib max seting=%d ob max seting=%d\n",CL_IN_ENTRY_SIZE_SETTING,CL_OUT_ENTRY_SIZE_SETTING);
	if (mvumi_pre_init(mhba))
		goto fail_init_data;

	/*
	* Get sizes, so initialize FW data
	*/
	if(mvumi_init_data(mhba))
		goto fail_init_data;
	/*
	 * We expect the FW state to be READY
	 */
	if (mvumi_start(mhba))
		goto fail_ready_state;

	mvumi_set_hs_page(mhba);

	/*
	 * Create a pool of commands
	 */
	if (mvumi_alloc_cmds(mhba))
		goto fail_alloc_cmds;

#if defined(SUPPORT_INTERNAL_REQ)
	/*
	 * Create a pool of int commands
	 */
	if (mvumi_alloc_int_cmds(mhba))
		goto fail_alloc_int_cmds;
#endif
#ifdef  SUPPORT_TASKLET

	/*
	 * Setup tasklet for cmd completion
	 */

	tasklet_init(&mhba->isr_tasklet, mvumi_complete_cmd_dpc,
		     (unsigned long)mhba);
#endif

	return 0;

fail_alloc_int_cmds:
	mvumi_free_cmds(mhba);

fail_alloc_cmds:
fail_ready_state:
	mvumi_release_mem_resource(mhba);
fail_init_data:
fail_device_id:
	mvumi_unmap_pci_addr(mhba->pdev, mhba->base_addr);
fail_ioremap:
	pci_release_regions(mhba->pdev);

	return -EINVAL;
}

/**
 * mvumi_io_attach -	Attaches this driver to SCSI mid-layer
 * @lhba:		Adapter soft state
 */
static int mvumi_io_attach(struct mv_hba *mhba)
{
	struct Scsi_Host *host = mhba->shost;

	/*
	 * Export parameters required by SCSI mid-layer
	 */
	host->irq = mhba->pdev->irq;
	host->unique_id = mhba->unique_id;
	host->can_queue = (mhba->max_io - 1) ? (mhba->max_io - 1) : 1;
	host->this_id = mhba->init_id;
	host->sg_tablesize = mhba->max_num_sge;
	host->max_sectors = mhba->max_transfer_size / SECTOR_LENGTH;
	host->cmd_per_lun =  (mhba->max_io - 1) ? (mhba->max_io - 1) : 1;
	host->max_channel = MVUMI_MAX_CHANNELS;
	host->max_id = mhba->max_target_id;
	host->max_lun = MVUMI_MAX_LUN;
	host->max_cmd_len = 16;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 15)
	host->transportt = &mvumi_transport_template;
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16) */

	/*
	 * Notify the mid-layer about the new controller
	 */
	if (scsi_add_host(host, &mhba->pdev->dev)) {
		mv_printk(" scsi_add_host failed\n");
		return -ENODEV;
	}

	/*
	 * Trigger SCSI to scan our drives
	 */
	scsi_scan_host(host);
	mhba->flag |= MVUMI_FW_ATTACH;
	return 0;
}
/**
 * mvumi_probe_one -	PCI hotplug entry point
 * @pdev:		PCI device structure
 * @id:			PCI ids of supported hotplugged adapter
 */
static int __devinit
mvumi_probe_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rval;
	struct Scsi_Host *host;
	struct mv_hba *mhba;

	/*
	 * Announce PCI information
	 */
	mv_printk(" %#4.04x:%#4.04x:%#4.04x:%#4.04x: ",
		  pdev->vendor, pdev->device, pdev->subsystem_vendor,
		  pdev->subsystem_device);

	mv_printk("bus %d:slot %d:func %d\n",
		  pdev->bus->number, PCI_SLOT(pdev->devfn),
		  PCI_FUNC(pdev->devfn));

	/*
	 * PCI prepping: enable device set bus mastering and dma mask
	 */
	rval = pci_enable_device(pdev);

	if (rval) {
		return rval;
	}

	pci_set_master(pdev);

	/*
	 * All our contollers are capable of performing 64-bit DMA
	 */
	if (IS_DMA64) {
		if (pci_set_dma_mask(pdev, DMA_64BIT_MASK) != 0) {

			if (pci_set_dma_mask(pdev, DMA_32BIT_MASK) != 0)
				goto fail_set_dma_mask;
		}
	} else {
		if (pci_set_dma_mask(pdev, DMA_32BIT_MASK) != 0)
			goto fail_set_dma_mask;
	}

	host = scsi_host_alloc(&mvumi_template, sizeof(struct mv_hba));

	if (!host) {
		mv_printk(" scsi_host_alloc failed\n");
		goto fail_alloc_instance;
	}

	mhba = (struct mv_hba *)host->hostdata;
	memset(mhba, 0, sizeof(*mhba));
	/*
	 * Initialize locks and queues
	 */
	INIT_LIST_HEAD(&mhba->cmd_pool);
#if defined(SUPPORT_INTERNAL_REQ)
	INIT_LIST_HEAD(&mhba->int_cmd_pool);
#endif

	INIT_LIST_HEAD(&mhba->ob_data_pool_list);
	INIT_LIST_HEAD(&mhba->free_ob_list);
	INIT_LIST_HEAD(&mhba->res_list);
	INIT_LIST_HEAD(&mhba->waiting_req_list);
	INIT_LIST_HEAD(&mhba->complete_req_list);
	INIT_LIST_HEAD(&mhba->stored_events);
	INIT_LIST_HEAD(&mhba->free_events);
	INIT_LIST_HEAD(&mhba->events_pool);
	atomic_set(&mhba->fw_outstanding, 0);

#if defined(SUPPORT_INTERNAL_REQ)
	init_waitqueue_head(&mhba->int_cmd_wait_q);
#endif
	init_waitqueue_head(&mhba->abort_cmd_wait_q);
	init_waitqueue_head(&mhba->handshake_wait_q);

	init_completion(&mhba->handshake_cmpl);
	init_timer(&mhba->hs_timer);

	spin_lock_init(&mhba->cmd_pool_lock);
#if defined(SUPPORT_INTERNAL_REQ)
	spin_lock_init(&mhba->int_cmd_pool_lock);
	spin_lock_init(&mhba->event_list_lock);
#endif
	spin_lock_init(&mhba->mv_isr_lock);
	spin_lock_init(&mhba->mv_tag_lock);
	spin_lock_init(&mhba->mv_timer_lock);
	sema_init(&mhba->ioctl_sem, MVUMI_INT_CMDS);
	mutex_init(&mhba->sas_discovery_mutex);
	spin_lock_init(&mhba->mv_waiting_req_lock);
	/*
	 * Initialize PCI related and misc parameters
	 */
	mhba->pdev = pdev;
	mhba->shost = host;
	mhba->unique_id = pdev->bus->number << 8 | pdev->devfn;
	mhba->init_id = MVUMI_DEFAULT_INIT_ID;

	mhba->flag = 0;
	mhba->last_time = 0;
	mhba->adapter_number = 0;

	/*
	 * Initialize Firmware
	 */
	if (mvumi_init_fw(mhba))
		goto fail_init_fw;
	/*
	 * Register IRQ
	 */
	if (mvumi_request_irq(mhba)) {
		mv_printk(" Failed to register IRQ\n");
		goto fail_irq;
	}
	/* Enable hba interrupt */
	mhba->instancet->enable_intr(mhba->mmio);

	/*
	 * Store mhba in PCI softstate
	 */
	pci_set_drvdata(pdev, mhba);

	/*
	 * Add this controller to mvumi_mgmt_info structure so that it
	 * can be exported to management applications
	 */
	mvumi_mgmt_info.count++;
	mvumi_mgmt_info.mhba[mvumi_mgmt_info.max_index] = mhba;
	mvumi_mgmt_info.max_index++;

#ifdef SUPPORT_INTERNAL_REQ
	/*
	 * get & clearup original hotplug events.
	 */
	mvumi_get_event(mhba, APICDB1_HOST_GETEVENT);
#endif
	/*
	 * Register with SCSI mid-layer
	 */
	if (mvumi_io_attach(mhba))
		goto fail_io_attach;
	mv_printk("probe mvumi driver successfully.\n");
	return 0;
	
fail_io_attach:
	mvumi_mgmt_info.count--;
	mvumi_mgmt_info.mhba[mvumi_mgmt_info.max_index] = NULL;
	mvumi_mgmt_info.max_index--;

	pci_set_drvdata(pdev, NULL);
	mhba->instancet->disable_intr(mhba->mmio);
	free_irq(mhba->pdev->irq, mhba);

	mvumi_release_fw(mhba);

fail_irq:
fail_init_fw:
	scsi_host_put(host);

fail_alloc_instance:
fail_set_dma_mask:
	pci_disable_device(pdev);

	return -ENODEV;
}

/**
 * mvumi_detach_one -	PCI hot"un"plug entry point
 * @pdev:		PCI device structure
 */
static void mvumi_detach_one(struct pci_dev *pdev)
{
	int i;
	struct Scsi_Host *host;
	struct mv_hba *mhba;
	mhba = pci_get_drvdata(pdev);
	host = mhba->shost;
	scsi_remove_host(mhba->shost);
	mvumi_flush_cache(mhba);
	mvumi_shutdown_controller(mhba);
	#ifdef  SUPPORT_TASKLET
	tasklet_kill(&mhba->isr_tasklet);
	#endif

	/*
	 * Take the lhba off the lhba array. Note that we will not
	 * decrement the max_index. We let this array be sparse array
	 */
	for (i = 0; i < mvumi_mgmt_info.max_index; i++) {
		if (mvumi_mgmt_info.mhba[i] == mhba) {
			mvumi_mgmt_info.count--;
			mvumi_mgmt_info.mhba[i] = NULL;

			break;
		}
	}

	mhba->instancet->disable_intr(mhba->mmio);

	free_irq(mhba->pdev->irq, mhba);

	mvumi_release_fw(mhba);

	scsi_host_put(host);

	pci_set_drvdata(pdev, NULL);

	pci_disable_device(pdev);
	mv_printk("driver is removed!.\n");

	return;
}

/**
 * mvumi_shutdown -	Shutdown entry point
 * @device:		Generic device structure
 */
static void mvumi_shutdown(struct pci_dev *pdev)
{
	struct mv_hba *mhba = pci_get_drvdata(pdev);
	mvumi_flush_cache(mhba);
}
#if LINUX_VERSION_CODE <=KERNEL_VERSION(2, 6, 9)
static int mvumi_suspend(struct pci_dev *pdev,u32 state)

#else
static int mvumi_suspend(struct pci_dev *pdev,pm_message_t state)
#endif
{

	struct mv_hba *mhba=NULL;
	mv_printk("%s\n",__FUNCTION__);

	mhba=pci_get_drvdata(pdev);
	mvumi_flush_cache(mhba);
	mvumi_shutdown_controller(mhba);
	#ifdef  SUPPORT_TASKLET
	tasklet_kill(&mhba->isr_tasklet);
	#endif
	pci_set_drvdata(pdev, mhba);
	mhba->instancet->disable_intr(mhba->mmio);
	free_irq(mhba->pdev->irq, mhba);
#if LINUX_VERSION_CODE <=KERNEL_VERSION(2, 6, 9)
	pci_save_state(pdev,mhba->pmstate);
#else
	pci_save_state(pdev);
#endif
	pci_disable_device(pdev);
#if LINUX_VERSION_CODE <=KERNEL_VERSION(2, 6, 9)
	pci_set_power_state(pdev, state);
#else
	pci_set_power_state(pdev, pci_choose_state(pdev, state));
#endif

	return 0;
}
static int mvumi_resume(struct pci_dev *pdev)
{

	int rval;
	struct mv_hba *mhba=NULL;
	mv_printk("%s\n",__FUNCTION__);
	mhba = pci_get_drvdata(pdev);
#if LINUX_VERSION_CODE <=KERNEL_VERSION(2, 6, 9)
	pci_set_power_state(pdev, 0);
	pci_enable_wake(pdev, 0, 0);
	pci_restore_state(pdev,mhba->pmstate);
#else
	pci_set_power_state(pdev, PCI_D0);
	pci_enable_wake(pdev, PCI_D0, 0);
	pci_restore_state(pdev);
#endif
	/*
	 * PCI prepping: enable device set bus mastering and dma mask
	 */
	rval = pci_enable_device(pdev);

	if (rval) {
		mv_printk(" Enable device failed\n");
		return rval;
	}

	pci_set_master(pdev);

	if (IS_DMA64) {
		if (pci_set_dma_mask(pdev, DMA_64BIT_MASK) != 0) {

			if (pci_set_dma_mask(pdev, DMA_32BIT_MASK) != 0)
				goto fail_set_dma_mask;
		}
	} else {
		if (pci_set_dma_mask(pdev, DMA_32BIT_MASK) != 0)
			goto fail_set_dma_mask;
	}

	/*
	 * We expect the FW state to be READY
	 */
	if (mvumi_start(mhba))
		goto fail_set_dma_mask;
	#ifdef  SUPPORT_TASKLET
	/*
	 * Setup tasklet for cmd completion
	 */
	tasklet_init(&mhba->isr_tasklet, mvumi_complete_cmd_dpc,
		     (unsigned long)mhba);
	#endif
	/*
	 * Register IRQ
	 */
		if (mvumi_request_irq(mhba)) {
			mv_printk(" Failed to register IRQ\n");
			goto fail_set_dma_mask;
		}
	/* Enable hba interrupt */
	mhba->instancet->enable_intr(mhba->mmio);
	/*
	 * Initiate AEN (Asynchronous Event Notification)
	 */
#ifdef SUPPORT_INTERNAL_REQ
			/*
			 * get & clearup original hotplug events.
			 */
			mvumi_get_event(mhba, APICDB1_HOST_GETEVENT);
#endif

	return 0;

fail_set_dma_mask:
	pci_disable_device(pdev);

	return -ENODEV;
}
/*
 * PCI hotplug support registration structure
 */
static struct pci_driver mvumi_pci_driver = {

	.name = "mv_" MV_DRIVER_NAME,
	.id_table = mvumi_pci_table,
	.probe = mvumi_probe_one,
	.remove = __devexit_p(mvumi_detach_one),
	#if LINUX_VERSION_CODE >KERNEL_VERSION(2, 6, 9)
	.shutdown = mvumi_shutdown,
	#endif
	.suspend=mvumi_suspend,
	.resume=mvumi_resume,
};

/*
 * Sysfs driver attributes
 */
static ssize_t mvumi_sysfs_show_version(struct device_driver *dd, char *buf)
{
	return snprintf(buf, strlen(MVUMI_VERSION) + 2, "%s\n", MVUMI_VERSION);
}

static DRIVER_ATTR(version, S_IRUGO, mvumi_sysfs_show_version, NULL);

static ssize_t
mvumi_sysfs_show_release_date(struct device_driver *dd, char *buf)
{
	return snprintf(buf, strlen(MVUMI_RELDATE) + 2, "%s\n", MVUMI_RELDATE);
}

static DRIVER_ATTR(release_date, S_IRUGO, mvumi_sysfs_show_release_date, NULL);

static ssize_t mvumi_sysfs_show_dbg_lvl(struct device_driver *dd, char *buf)
{
	return sprintf(buf, "%u", mvumi_dbg_lvl);
}

static ssize_t
mvumi_sysfs_set_dbg_lvl(struct device_driver *dd, const char *buf, size_t count)
{
	int retval = count;
	sscanf(buf, "%u", &mvumi_dbg_lvl);
	if(mvumi_dbg_lvl== 0){
		printk(KERN_DEBUG " disable dbg_lvl\n");
	}else{
		printk(KERN_DEBUG " enable dbg_lvl\n");
	}

	return retval;
}

static DRIVER_ATTR(dbg_lvl, S_IRUGO | S_IWUGO, mvumi_sysfs_show_dbg_lvl,
		   mvumi_sysfs_set_dbg_lvl);

/**
 * mvumi_init - Driver load entry point
 */
static int __init mvumi_init(void)
{
	int rval;
	/*
	 * Announce driver version and other information
	 */
	mv_printk("%s %s\n", MVUMI_VERSION, MVUMI_RELDATE);

	memset(&mvumi_mgmt_info, 0, sizeof(mvumi_mgmt_info));

	/*
	 * Register ourselves as PCI hotplug module
	 */
	rval = pci_register_driver(&mvumi_pci_driver);

	if (rval) {
		mv_printk(" PCI hotplug regisration failed \n");
		goto err_pcidrv;
	}
	rval = driver_create_file(&mvumi_pci_driver.driver,
				  &driver_attr_version);
	if (rval)
		goto err_dcf_attr_ver;
	rval = driver_create_file(&mvumi_pci_driver.driver,
				  &driver_attr_release_date);
	if (rval)
		goto err_dcf_rel_date;
	rval = driver_create_file(&mvumi_pci_driver.driver,
				  &driver_attr_dbg_lvl);
	if (rval)
		goto err_dcf_dbg_lvl;

	return rval;

err_dcf_dbg_lvl:
	driver_remove_file(&mvumi_pci_driver.driver, &driver_attr_release_date);
err_dcf_rel_date:
	driver_remove_file(&mvumi_pci_driver.driver, &driver_attr_version);

err_dcf_attr_ver:
	//pci_unregister_driver(&mvumi_pci_driver);

err_pcidrv:
	//unregister_chrdev(mvumi_mgmt_majorno, MV_DRIVER_NAME);
	return rval;
}

/**
 * mvumi_exit - Driver unload entry point
 */
static void __exit mvumi_exit(void)
{
	driver_remove_file(&mvumi_pci_driver.driver, &driver_attr_dbg_lvl);
	driver_remove_file(&mvumi_pci_driver.driver, &driver_attr_release_date);
	driver_remove_file(&mvumi_pci_driver.driver, &driver_attr_version);

	pci_unregister_driver(&mvumi_pci_driver);
}

module_init(mvumi_init);
module_exit(mvumi_exit);
