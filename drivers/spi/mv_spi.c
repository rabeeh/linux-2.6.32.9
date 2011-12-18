/*
 * mv_spi.c -- Marvell SPI controller driver
 *
 * Author: Saeed Bishara <saeed@marvell.com>
 * Copyright (C) 2007-2008 Marvell Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/spi/spi.h>
#include <linux/spi/mv_spi.h>
#include <asm/unaligned.h>

#define DRIVER_NAME			"mv_spi"

#define MV_NUM_CHIPSELECTS		2 /* only one slave is supported*/
#define MV_SPI_WAIT_RDY_MAX_LOOP	10000 /* in usec */
#define MV_SPI_MAX_TRANS_SIZE		32
#define MV_SPI_CTRL_REG		0x180
#define  MV_SPI_CTRL_START	(1 << 0) /* start transfer */
#define  MV_SPI_CTRL_3W4W	(1 << 1) /* 0 - 4 wire, 1 - 3 wire */
#define  MV_SPI_CTRL_SEL	(1 << 2) /* SPI slave select */
#define  MV_SPI_CTRL_ENABLE	(1 << 3) /* SPI enable */
#define  MV_SPI_CTRL_TX_LSB	(1 << 4) /* 1 - tx from bit 0 to higher */
#define  MV_SPI_CTRL_RX_LSB	(1 << 5) /* 1 - rx from bit 0 to higher */
#define  MV_SPI_CTRL_PHA	(1 << 7) /* 1 - sample at falling edge */
#define  MV_SPI_CTRL_TX_BITS	(0xFF << 8) /* tx bits */
#define  MV_SPI_CTRL_RX_BITS	(0xFF << 16) /* rx bits */
#define  MV_SPI_CTRL_CLKDIV	(0xFF << 24) /* Clock Divider */

#define MV_SPI_TX_DATA_REG		0x184
#define MV_SPI_RX_DATA_REG		0x140
#define MV_LCD_CLK_DIV_REG		0x1A8
#define MV_LCD_CLK_DIV_MASK		0xFFFF
#define MV_LCD_INT_STATUS_REG		0x1c4
#define  MV_LCD_INT_STATUS_SPI_DONE	(1 << 18)


struct mv_spi {
	struct work_struct	work;

	/* Lock access to transfer list.	*/
	spinlock_t		lock;

	struct list_head	msg_queue;
	struct spi_master	*master;
	void __iomem		*base;
	unsigned int		max_speed;
	unsigned int		min_speed;
	struct mv_spi_info	*spi_info;
};

static struct workqueue_struct *mv_spi_wq;

static inline void __iomem *spi_reg(struct mv_spi *mv_spi, u32 reg)
{
	return mv_spi->base + reg;
}

static inline void
mv_spi_setbits(struct mv_spi *mv_spi, u32 reg, u32 mask)
{
	void __iomem *reg_addr = spi_reg(mv_spi, reg);
	u32 val;

	val = readl(reg_addr);
	val |= mask;
	writel(val, reg_addr);
}

static inline void
mv_spi_clrbits(struct mv_spi *mv_spi, u32 reg, u32 mask)
{
	void __iomem *reg_addr = spi_reg(mv_spi, reg);
	u32 val;

	val = readl(reg_addr);
	val &= ~mask;
	writel(val, reg_addr);
}

static int mv_spi_baudrate_set(struct spi_device *spi, unsigned int speed)
{
	u32 clk_hz;
	u32 rate;
	u32 reg;
	unsigned int lcd_clk_div;
	struct mv_spi *mv_spi;

	mv_spi = spi_master_get_devdata(spi->master);

	clk_hz = mv_spi->spi_info->clk;
	/* the spi clock drived from LCD fast clock/CLK_INT_DIV */
	lcd_clk_div = readl(spi_reg(mv_spi, MV_LCD_CLK_DIV_REG));
	lcd_clk_div &= MV_LCD_CLK_DIV_MASK;
	clk_hz /= lcd_clk_div;


	/*
	 * the supported clock div value from 2 up to 0xFF
	 * round up as we look for equal or less speed
	 */
	rate = DIV_ROUND_UP(clk_hz, speed);

	/* check if requested speed is too small */
	if (rate > 0xFF)
		return -EINVAL;

	if (rate < 2)
		rate = 2;

	reg = readl(spi_reg(mv_spi, MV_SPI_CTRL_REG));
	reg = ((reg & ~MV_SPI_CTRL_CLKDIV) | (rate << 24));
	writel(reg, spi_reg(mv_spi, MV_SPI_CTRL_REG));

	return 0;
}

/*
 * called only when no transfer is active on the bus
 */
static int
mv_spi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct mv_spi *mv_spi;
	unsigned int speed = spi->max_speed_hz;
	unsigned int bits_per_word = spi->bits_per_word;
	int	rc;

	mv_spi = spi_master_get_devdata(spi->master);

	if ((t != NULL) && t->speed_hz)
		speed = t->speed_hz;

	if ((t != NULL) && t->bits_per_word)
		bits_per_word = t->bits_per_word;

	rc = mv_spi_baudrate_set(spi, speed);

	return rc;
}

static void mv_spi_set_cs(struct mv_spi *mv_spi, int enable)
{
	if (enable)
		mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, MV_SPI_CTRL_SEL);
	else
		mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, MV_SPI_CTRL_SEL);

}

static inline int mv_spi_wait_till_ready(struct mv_spi *mv_spi)
{
	int i;

	for (i = 0; i < MV_SPI_WAIT_RDY_MAX_LOOP; i++) {
		u32 int_status = readl(spi_reg(mv_spi, MV_LCD_INT_STATUS_REG));
		
		if (int_status & MV_LCD_INT_STATUS_SPI_DONE)
			return 1;
		else
			udelay(1);
	}

	return -1;
}

static inline int
mv_spi_write_read_8bit(struct spi_device *spi,
			  const u8 **tx_buf, u8 **rx_buf)
{
	void __iomem *tx_reg, *rx_reg, *int_reg;
	struct mv_spi *mv_spi;

	mv_spi = spi_master_get_devdata(spi->master);
	tx_reg = spi_reg(mv_spi, MV_SPI_TX_DATA_REG);
	rx_reg = spi_reg(mv_spi, MV_SPI_RX_DATA_REG);
	int_reg = spi_reg(mv_spi, MV_LCD_INT_STATUS_REG);

	/* clear the interrupt cause register */
	writel(~MV_LCD_INT_STATUS_SPI_DONE, int_reg);

	if (tx_buf && *tx_buf) {
		writel(*(*tx_buf)++ , tx_reg);
		mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, 0x7 << 8);
	}
	else
		mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, 0xFF << 8);

	if (rx_buf && *rx_buf)
		mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, 0x7 << 16);
	else
		mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, 0xFF << 16);

	mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, MV_SPI_CTRL_START);

	if (mv_spi_wait_till_ready(mv_spi) < 0) {
		dev_err(&spi->dev, "TXS timed out\n");
		dev_err(&spi->dev, "SPI CTRL %x\n", 
			readl(spi_reg(mv_spi, MV_SPI_CTRL_REG)));
		return -1;
	}
	mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, MV_SPI_CTRL_START);
	/* clear the interrupt cause register */
	writel(~MV_LCD_INT_STATUS_SPI_DONE, int_reg);
	if (rx_buf && *rx_buf)
		*(*rx_buf)++ = readl(rx_reg);

	return 1;
}
#if 0
static inline int
mv_spi_write_read_16bit(struct spi_device *spi,
			  const u16 **tx_buf, u16 **rx_buf)
{
	void __iomem *tx_reg, *rx_reg, *int_reg;
	struct mv_spi *mv_spi;

	mv_spi = spi_master_get_devdata(spi->master);
	tx_reg = spi_reg(mv_spi, MV_SPI_TX_DATA_REG);
	rx_reg = spi_reg(mv_spi, MV_SPI_RX_DATA_REG);
	int_reg = spi_reg(mv_spi, MV_LCD_INT_STATUS_REG);


	/* clear the interrupt cause register */
	writel(MV_LCD_INT_STATUS_SPI_DONE, int_reg);

	if (tx_buf && *tx_buf) {
		writel(*(*tx_buf)++ , tx_reg);
		mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, 0xf << 8);
	}
	else {
		mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, 0xFF << 8);
	}

	if (rx_buf && *rx_buf) {
		mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, 0xf << 16);
	}
	else {
		mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, 0xFF << 16);
	}

	mv_spi_setbits(mv_spi, MV_SPI_CTRL_REG, MV_SPI_CTRL_START);

	if (mv_spi_wait_till_ready(mv_spi) < 0) {
		dev_err(&spi->dev, "TXS timed out\n");
		dev_err(&spi->dev, "SPI CTRL %x\n", readl(spi_reg(mv_spi, MV_SPI_CTRL_REG))  );
		return -1;
	}
	mv_spi_clrbits(mv_spi, MV_SPI_CTRL_REG, MV_SPI_CTRL_START);
	if (rx_buf && *rx_buf) {
		u16 data = readl(rx_reg);

		mdelay(10);
		*(*rx_buf)++ = be16_to_cpu(data);
	}

	return 1;
}
#endif
static unsigned int
mv_spi_write_read(struct spi_device *spi, struct spi_transfer *xfer)
{
	struct mv_spi *mv_spi;
	unsigned int count;
	int word_len;

	mv_spi = spi_master_get_devdata(spi->master);
	word_len = spi->bits_per_word;
	count = xfer->len;

	if(xfer->bits_per_word)
		word_len = xfer->bits_per_word;

	if (word_len == 8) {
		const u8 *tx = xfer->tx_buf;
		u8 *rx = xfer->rx_buf;

		do {
			if (mv_spi_write_read_8bit(spi, &tx, &rx) < 0)
				goto out;
			count--;
		} while (count);
	}
#if 0
 else if (word_len == 16) {
		const u16 *tx = xfer->tx_buf;
		u16 *rx = xfer->rx_buf;

		do {
			if (mv_spi_write_read_16bit(spi, &tx, &rx) < 0)
				goto out;
			count -= 2;
		} while (count);
	}
#endif
out:
	return xfer->len - count;
}


static void mv_spi_work(struct work_struct *work)
{
	struct mv_spi *mv_spi =
		container_of(work, struct mv_spi, work);

	spin_lock_irq(&mv_spi->lock);
	while (!list_empty(&mv_spi->msg_queue)) {
		struct spi_message *m;
		struct spi_device *spi;
		struct spi_transfer *t = NULL;
		int par_override = 0;
		int status = 0;
		int cs_active = 0;

		m = container_of(mv_spi->msg_queue.next, struct spi_message,
				 queue);

		list_del_init(&m->queue);
		spin_unlock_irq(&mv_spi->lock);

		spi = m->spi;

		/* Load defaults */
		status = mv_spi_setup_transfer(spi, NULL);

		if (status < 0)
			goto msg_done;

		list_for_each_entry(t, &m->transfers, transfer_list) {
			if (par_override || t->speed_hz || t->bits_per_word) {
				par_override = 1;
				status = mv_spi_setup_transfer(spi, t);
				if (status < 0)
					break;
				if (!t->speed_hz && !t->bits_per_word)
					par_override = 0;
			}

			if (!cs_active) {
				mv_spi_set_cs(mv_spi, 0);
				cs_active = 1;
			}

			if (t->len)
				m->actual_length +=
					mv_spi_write_read(spi, t);

			if (t->delay_usecs)
				udelay(t->delay_usecs);

			if (t->cs_change) {
				mv_spi_set_cs(mv_spi, 0);
				cs_active = 0;
			}
		}

msg_done:
		if (cs_active)
			mv_spi_set_cs(mv_spi, 0);

		m->status = status;
		m->complete(m->context);

		spin_lock_irq(&mv_spi->lock);
	}

	spin_unlock_irq(&mv_spi->lock);
}

static int __devinit mv_spi_reset(struct mv_spi *mv_spi)
{
	/* Verify that the CS is deasserted */
	mv_spi_set_cs(mv_spi, 0);

	return 0;
}

static int mv_spi_setup(struct spi_device *spi)
{
	struct mv_spi *mv_spi;

	mv_spi = spi_master_get_devdata(spi->master);

	if (spi->mode) {
		dev_err(&spi->dev, "setup: unsupported mode bits %x\n",
			spi->mode);
		return -EINVAL;
	}

	if (spi->bits_per_word == 0)
		spi->bits_per_word = 8;

	if ((spi->max_speed_hz == 0)
			|| (spi->max_speed_hz > mv_spi->max_speed))
		spi->max_speed_hz = mv_spi->max_speed;

	if (spi->max_speed_hz < mv_spi->min_speed) {
		dev_err(&spi->dev, "setup: requested speed too low %d Hz "
			"(min speed %d Hz)\n", spi->max_speed_hz,
			mv_spi->min_speed);
		return -EINVAL;
	}

	/*
	 * baudrate & width will be set mv_spi_setup_transfer
	 */
	return 0;
}

static int mv_spi_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct mv_spi *mv_spi;
	struct spi_transfer *t = NULL;
	unsigned long flags;

	m->actual_length = 0;
	m->status = 0;

	/* reject invalid messages and transfers */
	if (list_empty(&m->transfers) || !m->complete)
		return -EINVAL;

	mv_spi = spi_master_get_devdata(spi->master);

	list_for_each_entry(t, &m->transfers, transfer_list) {
		unsigned int bits_per_word = spi->bits_per_word;

		if (t->tx_buf == NULL && t->rx_buf == NULL && t->len) {
			dev_err(&spi->dev,
				"message rejected : "
				"invalid transfer data buffers\n");
			goto msg_rejected;
		}

		if ((t != NULL) && t->bits_per_word)
			bits_per_word = t->bits_per_word;

		if ((bits_per_word != 8) /*&& (bits_per_word != 16)*/) {
			dev_err(&spi->dev,
				"message rejected : "
				"invalid transfer bits_per_word (%d bits)\n",
				bits_per_word);
			goto msg_rejected;
		}

		if (t->speed_hz && t->speed_hz < mv_spi->min_speed) {
			dev_err(&spi->dev,
				"message rejected : "
				"device min speed (%d Hz) exceeds "
				"required transfer speed (%d Hz)\n",
				mv_spi->min_speed, t->speed_hz);
			goto msg_rejected;
		}
	}


	spin_lock_irqsave(&mv_spi->lock, flags);
	list_add_tail(&m->queue, &mv_spi->msg_queue);
	queue_work(mv_spi_wq, &mv_spi->work);
	spin_unlock_irqrestore(&mv_spi->lock, flags);

	return 0;
msg_rejected:
	/* Message rejected and not queued */
	m->status = -EINVAL;
	if (m->complete)
		m->complete(m->context);
	return -EINVAL;
}

int __devinit mv_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mv_spi *spi;
	struct resource *r;
	struct mv_spi_info *spi_info;
	int status = 0;
	unsigned int lcd_clk_div;

	spi_info = pdev->dev.platform_data;

	master = spi_alloc_master(&pdev->dev, sizeof *spi);
	if (master == NULL) {
		dev_dbg(&pdev->dev, "master allocation failed\n");
		return -ENOMEM;
	}

	if (pdev->id != -1)
		master->bus_num = pdev->id;

	master->setup = mv_spi_setup;
	master->transfer = mv_spi_transfer;
	master->num_chipselect = MV_NUM_CHIPSELECTS;

	dev_set_drvdata(&pdev->dev, master);

	spi = spi_master_get_devdata(master);
	spi->master = master;
	spi->spi_info = spi_info;

	if (spi_info->clk == 0) {
		pr_debug("mv_spi: Bad Clk value.\n");
		status = -EINVAL;
		goto out;
	}
#if 0
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		status = -ENODEV;
		goto out;
	}
#endif
#if 0
	if (!request_mem_region(r->start, (r->end - r->start) + 1,
				pdev->dev.bus_id)) {
		status = -EBUSY;
		printk("request_mem_region failed\n");
		goto out;
	}
#endif
	spi->base = ioremap(0xf1820000, SZ_1K);
	printk("ioremap: %p\n", spi->base);
	if(spi->base == 0){
		status = -EBUSY;
		printk("ioremap failed\n");
		goto out;
	}
	lcd_clk_div = readl(spi_reg(spi, MV_LCD_CLK_DIV_REG));
	lcd_clk_div &= MV_LCD_CLK_DIV_MASK;

	if (lcd_clk_div == 0){
		status = -EBUSY;
		printk("lcd clk div not set. check if lcd driver loaded \n");
		goto out;

	}
	spi->max_speed = DIV_ROUND_UP(spi_info->clk/lcd_clk_div, 2);
	spi->min_speed = DIV_ROUND_UP(spi_info->clk/lcd_clk_div, 0xFF);

	INIT_WORK(&spi->work, mv_spi_work);

	spin_lock_init(&spi->lock);
	INIT_LIST_HEAD(&spi->msg_queue);

	if (mv_spi_reset(spi) < 0)
		goto out_rel_mem;

	writel(MV_SPI_CTRL_ENABLE | /*MV_SPI_CTRL_PHA |*/ 0xFF << 24,
	       spi_reg(spi, MV_SPI_CTRL_REG));

	status = spi_register_master(master);
	if (status < 0)
		goto out_rel_mem;

	return status;

out_rel_mem:
	printk("error while loading mv_spi driver \n");
//	release_mem_region(r->start, (r->end - r->start) + 1);

out:
	spi_master_put(master);
	return status;
}


static int __exit mv_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct mv_spi *spi;
	struct resource *r;

	master = dev_get_drvdata(&pdev->dev);
	spi = spi_master_get_devdata(master);

	cancel_work_sync(&spi->work);

//	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
//	release_mem_region(r->start, (r->end - r->start) + 1);

	spi_unregister_master(master);

	return 0;
}

MODULE_ALIAS("platform:" DRIVER_NAME);

struct platform_driver mv_spi_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.remove		= __exit_p(mv_spi_remove),
};

int __devinit mv_spi_init(void)
{
	mv_spi_wq = create_singlethread_workqueue(
				mv_spi_driver.driver.name);
	if (mv_spi_wq == NULL)
		return -ENOMEM;

	return platform_driver_probe(&mv_spi_driver, mv_spi_probe);
}
//module_init(mv_spi_init);

void __exit mv_spi_exit(void)
{
	flush_workqueue(mv_spi_wq);
	platform_driver_unregister(&mv_spi_driver);

	destroy_workqueue(mv_spi_wq);
}
//module_exit(mv_spi_exit);

MODULE_DESCRIPTION("Marvell SPI driver");
MODULE_AUTHOR("Saeed Bishara <saeed@marvell.com>");
MODULE_LICENSE("GPL");
