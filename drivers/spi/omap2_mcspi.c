/*
 * OMAP2 McSPI controller driver
 *
 * Copyright (C) 2005, 2006 Nokia Corporation
 * Author:	Samuel Ortiz <samuel.ortiz@nokia.com> and
 *		Juha Yrjl <juha.yrjola@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <linux/spi/spi.h>

#include <plat/dma.h>
#include <plat/clock.h>
#include <plat/mcspi.h>

#define OMAP2_MCSPI_MAX_FREQ		48000000
#define OMAP2_MCSPI_MAX_FIFODEPTH	64
/* OMAP2 has 3 SPI controllers, while OMAP3/4 has 4 */
#define OMAP2_MCSPI_MAX_CTRL 		4

#define OMAP2_MCSPI_SYSCONFIG		0x10
#define OMAP2_MCSPI_SYSSTATUS		0x14
#define OMAP2_MCSPI_IRQSTATUS		0x18
#define OMAP2_MCSPI_IRQENABLE		0x1c
#define OMAP2_MCSPI_WAKEUPENABLE	0x20
#define OMAP2_MCSPI_MODULCTRL		0x28
#define OMAP2_MCSPI_XFERLEVEL		0x7c

/* per-channel banks, 0x14 bytes each, first is: */
#define OMAP2_MCSPI_CHCONF0		0x2c
#define OMAP2_MCSPI_CHSTAT0		0x30
#define OMAP2_MCSPI_CHCTRL0		0x34
#define OMAP2_MCSPI_TX0			0x38
#define OMAP2_MCSPI_RX0			0x3c

/* per-register bitmasks: */

#define OMAP2_MCSPI_SYSCONFIG_SMARTIDLE	BIT(4)
#define OMAP2_MCSPI_SYSCONFIG_ENAWAKEUP	BIT(2)
#define OMAP2_MCSPI_SYSCONFIG_AUTOIDLE	BIT(0)
#define OMAP2_MCSPI_SYSCONFIG_SOFTRESET	BIT(1)

#define OMAP2_MCSPI_SYSSTATUS_RESETDONE	BIT(0)

#define OMAP2_MCSPI_MODULCTRL_SINGLE	BIT(0)
#define OMAP2_MCSPI_MODULCTRL_MS	BIT(2)
#define OMAP2_MCSPI_MODULCTRL_STEST	BIT(3)

#define OMAP2_MCSPI_CHCONF_PHA		BIT(0)
#define OMAP2_MCSPI_CHCONF_POL		BIT(1)
#define OMAP2_MCSPI_CHCONF_CLKD_MASK	(0x0f << 2)
#define OMAP2_MCSPI_CHCONF_EPOL		BIT(6)
#define OMAP2_MCSPI_CHCONF_WL_MASK	(0x1f << 7)
#define OMAP2_MCSPI_CHCONF_TRM_RX_ONLY	BIT(12)
#define OMAP2_MCSPI_CHCONF_TRM_TX_ONLY	BIT(13)
#define OMAP2_MCSPI_CHCONF_TRM_MASK	(0x03 << 12)
#define OMAP2_MCSPI_CHCONF_DMAW		BIT(14)
#define OMAP2_MCSPI_CHCONF_DMAR		BIT(15)
#define OMAP2_MCSPI_CHCONF_DPE0		BIT(16)
#define OMAP2_MCSPI_CHCONF_DPE1		BIT(17)
#define OMAP2_MCSPI_CHCONF_IS		BIT(18)
#define OMAP2_MCSPI_CHCONF_TURBO	BIT(19)
#define OMAP2_MCSPI_CHCONF_FORCE	BIT(20)
#define OMAP2_MCSPI_CHCONF_FFEW 	BIT(27)
#define OMAP2_MCSPI_CHCONF_FFER 	BIT(28)

#define OMAP2_MCSPI_CHSTAT_RXS		BIT(0)
#define OMAP2_MCSPI_CHSTAT_TXS		BIT(1)
#define OMAP2_MCSPI_CHSTAT_EOT		BIT(2)
#define OMAP2_MCSPI_IRQ_RX0_OVERFLOW	BIT(3)
#define OMAP2_MCSPI_IRQ_TX0_EMPTY	BIT(0)

#define OMAP2_MCSPI_CHCTRL_EN		BIT(0)

#define OMAP2_MCSPI_WAKEUPENABLE_WKEN	BIT(0)
#define OMAP2_MCSPI_MAX_SLAVE_OP_FREQ	24000000

static struct omap2_mcspi_regs omap2_mcspi_ctx[OMAP2_MCSPI_MAX_CTRL];

#ifdef CONFIG_SPI_DEBUG
struct reg_type {
	char name[40];
	int offset;
};

static struct reg_type reg_map[] = {
	{"MCSPI_REV", 0x0},
	{"MCSPI_SYSCONFIG", 0x10},
	{"MCSPI_SYSSTATUS", 0x14},
	{"MCSPI_IRQSTATUS", 0x18},
	{"MCSPI_IRQENABLE", 0x1C},
	{"MCSPI_WAKEUPENABLE", 0x20},
	{"MCSPI_SYST", 0x24},
	{"MCSPI_MODULCTRL", 0x28},
	{"MCSPI_XFERLEVEL", 0x7c},
	{"CH0", 0x2C},
	{"CH1", 0x40},
	{"CH2", 0x54},
	{"CH3", 0x68}
};

static struct reg_type ch_reg_type[] = {
	{"CONF", 0x00},
	{"STAT", 0x04},
	{"CTRL", 0x08},
	{"TX", 0x0C},
	{"RX", 0x10},
};
#endif

static struct workqueue_struct *omap2_mcspi_wq;

#define MOD_REG_BIT(val, mask, set) do { \
	if (set) \
		val |= mask; \
	else \
		val &= ~mask; \
} while (0)

static inline void mcspi_write_reg(struct spi_master *master,
		int idx, u32 val)
{
	struct omap2_mcspi *mcspi = spi_master_get_devdata(master);

	__raw_writel(val, mcspi->base + idx);
}

static inline u32 mcspi_read_reg(struct spi_master *master, int idx)
{
	struct omap2_mcspi *mcspi = spi_master_get_devdata(master);

	return __raw_readl(mcspi->base + idx);
}
static inline void mcspi_write_cs_reg(const struct spi_device *spi,
		int idx, u32 val)
{
	struct omap2_mcspi_cs	*cs = spi->controller_state;

	__raw_writel(val, cs->base + idx);
}

static inline u32 mcspi_read_cs_reg(const struct spi_device *spi, int idx)
{
	struct omap2_mcspi_cs	*cs = spi->controller_state;

	return __raw_readl(cs->base + idx);
}

static inline u32 mcspi_cached_chconf0(const struct spi_device *spi)
{
	struct omap2_mcspi_cs *cs = spi->controller_state;

	return cs->chconf0;
}

static inline void mcspi_write_chconf0(const struct spi_device *spi, u32 val)
{
	struct omap2_mcspi_cs *cs = spi->controller_state;

	cs->chconf0 = val;
	mcspi_write_cs_reg(spi, OMAP2_MCSPI_CHCONF0, val);
	mcspi_read_cs_reg(spi, OMAP2_MCSPI_CHCONF0);
}

static void omap2_mcspi_set_dma_req(const struct spi_device *spi,
		int is_read, int enable)
{
	u32 l, rw;

	l = mcspi_cached_chconf0(spi);

	if (is_read) /* 1 is read, 0 write */
		rw = OMAP2_MCSPI_CHCONF_DMAR;
	else
		rw = OMAP2_MCSPI_CHCONF_DMAW;

	MOD_REG_BIT(l, rw, enable);
	mcspi_write_chconf0(spi, l);
}

#ifdef CONFIG_SPI_DEBUG
static int
omap2_mcspi_dump_regs(struct spi_master *master)
{
	u32 spi_base;
	u32 reg;
	u32 channel;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(master);

	spi_base = (u32)mcspi->base;

	for (reg = 0; (reg < ARRAY_SIZE(reg_map)); reg++) {
		struct reg_type *reg_d = &reg_map[reg];
		u32 base1 = spi_base + reg_d->offset;
		if (reg_d->name[0] == 'C') {
			for (channel = 0; (channel < (ARRAY_SIZE(ch_reg_type)));
			    channel++) {
				struct reg_type *reg_c = &ch_reg_type[channel];
				u32 base2 = base1 + reg_c->offset;
				pr_debug("MCSPI_%s%s [0x%08X] = 0x%08X\n",
				       reg_d->name, reg_c->name, base2,
				       __raw_readl(base2));
			}
		} else {
			pr_debug("%s : [0x%08X] = 0x%08X\n",
				reg_d->name, base1, __raw_readl(base1));
		}

	}
	return 0;
}
#endif

static void omap2_mcspi_set_enable(const struct spi_device *spi, int enable)
{
	u32 l;

	l = enable ? OMAP2_MCSPI_CHCTRL_EN : 0;
	mcspi_write_cs_reg(spi, OMAP2_MCSPI_CHCTRL0, l);
	/* Flash post-writes */
	mcspi_read_cs_reg(spi, OMAP2_MCSPI_CHCTRL0);
}

static void omap2_mcspi_force_cs(struct spi_device *spi, int cs_active)
{
	u32 l;

	l = mcspi_cached_chconf0(spi);
	MOD_REG_BIT(l, OMAP2_MCSPI_CHCONF_FORCE, cs_active);
	mcspi_write_chconf0(spi, l);
}

static int omap2_mcspi_start_slave_dma(const struct spi_device *spi)
{
	unsigned long base, rx_reg, tx_reg;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[spi->chip_select];
	struct omap2_mcspi_cs	*cs = spi->controller_state;

	base = cs->phys;
	rx_reg = base + OMAP2_MCSPI_RX0;
	tx_reg = base + OMAP2_MCSPI_TX0;

	mcspi->slave_rx_buf = dma_alloc_coherent(NULL, OMAP2_MCSPI_DMA_BUF_SIZE,
		(dma_addr_t *)&(mcspi->slave_rx_buf_dma_phys), 0);
	mcspi->slave_tx_buf = dma_alloc_coherent(NULL, OMAP2_MCSPI_DMA_BUF_SIZE,
		(dma_addr_t *)&(mcspi->slave_tx_buf_dma_phys), 0);

	/*
	 * Link channel with itself so DMA doesn't need any
	 * reprogramming while looping the buffer
	 */
	omap_dma_link_lch(mcspi_dma->dma_rx_channel,
			mcspi_dma->dma_rx_channel);

	/* Configure Rx DMA */
	omap_set_dma_transfer_params(mcspi_dma->dma_rx_channel,
			OMAP_DMA_DATA_TYPE_S8, OMAP2_MCSPI_DMA_BUF_SIZE,
			1, OMAP_DMA_SYNC_ELEMENT,
			mcspi_dma->dma_rx_sync_dev, 1);	
	omap_set_dma_src_params(mcspi_dma->dma_rx_channel, 0,
			OMAP_DMA_AMODE_CONSTANT,
			rx_reg, 0, 0);
	omap_set_dma_dest_params(mcspi_dma->dma_rx_channel, 0,
			OMAP_DMA_AMODE_POST_INC,
			mcspi->slave_rx_buf_dma_phys, 0, 0);	
	mcspi->slave_prev_rx_dma_pos = mcspi->slave_rx_buf_dma_phys;
	omap2_mcspi_set_dma_req(spi, 1, 1);

	/* Configure Tx DMA */
	omap_set_dma_dest_params(mcspi_dma->dma_tx_channel, 0,
			OMAP_DMA_AMODE_CONSTANT,
			tx_reg, 0, 0);
	mcspi_write_cs_reg(spi, OMAP2_MCSPI_TX0, 0);
	omap2_mcspi_set_dma_req(spi, 0, 1);

	/*
	 * TBD: Should be done in omap_start_dma function
	 * Patch proposed to LO pending in LO.
	 */
	omap_writel(0, OMAP34XX_DMA4_BASE + OMAP_DMA4_CDAC(mcspi_dma->dma_rx_channel));
	omap_start_dma(mcspi_dma->dma_rx_channel);
	mod_timer(&mcspi->rx_dma_timer, jiffies + msecs_to_jiffies(OMAP2_MCSPI_DMA_RX_TIMEOUT_MS));
	return 0;
}

static int omap2_mcspi_stop_slave_dma(const struct spi_device *spi)
{
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	del_timer(&(mcspi->rx_dma_timer));
	omap_stop_dma(mcspi_dma->dma_rx_channel);
	omap2_mcspi_set_dma_req(spi, 1, 0);
	omap2_mcspi_set_dma_req(spi, 0, 0);
	dma_free_coherent(NULL,	OMAP2_MCSPI_DMA_BUF_SIZE, 
			mcspi->slave_rx_buf, mcspi->slave_rx_buf_dma_phys);
	dma_free_coherent(NULL,	OMAP2_MCSPI_DMA_BUF_SIZE, 
			mcspi->slave_tx_buf, mcspi->slave_tx_buf_dma_phys);
	return 0;
}

static void omap2_mcspi_set_master_mode(struct spi_master *master)
{
	u32 l;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(master);

	/* setup when switching from (reset default) slave mode
	 * to single-channel master mode based on config value
	 */
	l = mcspi_read_reg(master, OMAP2_MCSPI_MODULCTRL);
	MOD_REG_BIT(l, OMAP2_MCSPI_MODULCTRL_STEST, 0);
	MOD_REG_BIT(l, OMAP2_MCSPI_MODULCTRL_MS, 0);

	if (mcspi->force_cs_mode)
		MOD_REG_BIT(l, OMAP2_MCSPI_MODULCTRL_SINGLE, 1);

	mcspi_write_reg(master, OMAP2_MCSPI_MODULCTRL, l);

	/* Save context */
	omap2_mcspi_ctx[master->bus_num - 1].modulctrl = l;
}

static void omap2_mcspi_set_slave_mode(struct spi_master *master)
{
	u32 l;

	l = mcspi_read_reg(master, OMAP2_MCSPI_MODULCTRL);
	MOD_REG_BIT(l, OMAP2_MCSPI_MODULCTRL_STEST, 0);
	MOD_REG_BIT(l, OMAP2_MCSPI_MODULCTRL_MS, 1);
	MOD_REG_BIT(l, OMAP2_MCSPI_MODULCTRL_SINGLE, 0);
	mcspi_write_reg(master, OMAP2_MCSPI_MODULCTRL, l);

	/* Save context */
	omap2_mcspi_ctx[master->bus_num - 1].modulctrl = l;
}

static int mcspi_wait_for_reg_bit(void __iomem *reg, unsigned long bit)
{
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(1000);
	while (!(__raw_readl(reg) & bit)) {
		if (time_after(jiffies, timeout))
			return -1;
		cpu_relax();
	}
	return 0;
}

static void omap2_mcspi_restore_ctx(struct omap2_mcspi *mcspi)
{
	struct spi_master *spi_cntrl;
	struct omap2_mcspi_cs *cs;
	spi_cntrl = mcspi->master;

	/* McSPI: context restore */
	mcspi_write_reg(spi_cntrl, OMAP2_MCSPI_MODULCTRL,
			omap2_mcspi_ctx[spi_cntrl->bus_num - 1].modulctrl);

	mcspi_write_reg(spi_cntrl, OMAP2_MCSPI_WAKEUPENABLE,
			omap2_mcspi_ctx[spi_cntrl->bus_num - 1].wakeupenable);

	list_for_each_entry(cs, &omap2_mcspi_ctx[spi_cntrl->bus_num - 1].cs,
			node)
		__raw_writel(cs->chconf0, cs->base + OMAP2_MCSPI_CHCONF0);
}

static void omap2_mcspi_disable_clocks(struct omap2_mcspi *mcspi)
{
	if (!cpu_is_omap44xx())
		clk_disable(mcspi->ick);
	clk_disable(mcspi->fck);
}

static int omap2_mcspi_enable_clocks(struct omap2_mcspi *mcspi)
{
	if (!cpu_is_omap44xx()){
		if (clk_enable(mcspi->ick)) 
			return -ENODEV;
	}
	if (clk_enable(mcspi->fck))
		return -ENODEV;

	omap2_mcspi_restore_ctx(mcspi);

	return 0;
}

static unsigned
omap2_mcspi_txrx_dma(struct spi_device *spi, struct spi_transfer *xfer)
{
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_cs	*cs = spi->controller_state;
	struct omap2_mcspi_dma  *mcspi_dma;
	unsigned int		count, c, bytes_per_transfer;
	unsigned long		base, tx_reg, rx_reg;
	int			word_len, data_type, element_count, frame_count,
				sync_type, dma_fi;
	u8			*rx;
	const u8		*tx;

	mcspi = spi_master_get_devdata(spi->master);
	mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	count = xfer->len;
	c = count;
	word_len = cs->word_len;

	base = cs->phys;
	tx_reg = base + OMAP2_MCSPI_TX0;
	rx_reg = base + OMAP2_MCSPI_RX0;
	rx = xfer->rx_buf;
	tx = xfer->tx_buf;

	if (word_len <= 8) {
		data_type = OMAP_DMA_DATA_TYPE_S8;
		element_count = count;
		bytes_per_transfer = 1;
	} else if (word_len <= 16) {
		data_type = OMAP_DMA_DATA_TYPE_S16;
		element_count = count >> 1;
		bytes_per_transfer = 2;
	} else /* word_len <= 32 */ {
		data_type = OMAP_DMA_DATA_TYPE_S32;
		element_count = count >> 2;
		bytes_per_transfer = 4;
	}

	/* Always use element synchronised DMA */
	sync_type   = OMAP_DMA_SYNC_ELEMENT;
	frame_count = 1;
	dma_fi      = 0;
	dev_dbg(&spi->dev, "DMA ELEMENT\n");

	if (tx != NULL) {
		omap_set_dma_transfer_params(mcspi_dma->dma_tx_channel,
				data_type, element_count, frame_count,
				sync_type, mcspi_dma->dma_tx_sync_dev, 0);

		omap_set_dma_dest_params(mcspi_dma->dma_tx_channel, 0,
				OMAP_DMA_AMODE_CONSTANT,
				tx_reg, 0, dma_fi);

		omap_set_dma_src_params(mcspi_dma->dma_tx_channel, 0,
				OMAP_DMA_AMODE_POST_INC,
				xfer->tx_dma, 0, 0);

		omap2_mcspi_set_dma_req(spi, 0, 1);
	}
	if (rx != NULL) {
		omap_set_dma_transfer_params(mcspi_dma->dma_rx_channel,
				data_type, element_count,
				frame_count, sync_type,
				mcspi_dma->dma_rx_sync_dev, 1);

		omap_set_dma_src_params(mcspi_dma->dma_rx_channel, 0,
				OMAP_DMA_AMODE_CONSTANT,
				rx_reg, 0, dma_fi);

		omap_set_dma_dest_params(mcspi_dma->dma_rx_channel, 0,
				OMAP_DMA_AMODE_POST_INC,
				xfer->rx_dma, 0, 0);

		omap2_mcspi_set_dma_req(spi, 1, 1);

		/* Dummy write required for RX only mode */
		if (tx == NULL)
			mcspi_write_cs_reg(spi, OMAP2_MCSPI_TX0, 0);
	}

	if (tx != NULL) {
		omap_start_dma(mcspi_dma->dma_tx_channel);
		dev_dbg(&spi->dev, "DMA TX started\n");
	}

	if (rx != NULL) {
		omap_start_dma(mcspi_dma->dma_rx_channel);
		dev_dbg(&spi->dev, "DMA RX started\n");
	}

	if (tx != NULL) {
		wait_for_completion(&mcspi_dma->dma_tx_completion);
		dma_unmap_single(&spi->dev, xfer->tx_dma, count, DMA_TO_DEVICE);
		dev_dbg(&spi->dev, "DMA TX completed\n");
	}

	if (rx != NULL) {
		wait_for_completion(&mcspi_dma->dma_rx_completion);
		dma_unmap_single(&spi->dev, xfer->rx_dma, count, DMA_FROM_DEVICE);
		dev_dbg(&spi->dev, "DMA RX completed\n");
	}

	/* In packet synchronisation mode, the last words of the buffer  */
	/* have to be read manually if the frame size is not a multiple  */
	/* of 16: this is because in that case, the last DMA packet will */
	/* not trigger a DMAREQ since the SPI FIFO threshold will not be */
	/* reached.                                                      */
	if (sync_type == OMAP_DMA_SYNC_PACKET) {
		int i, j;

		/* Wait for SPI transfer's completion. */
		/* When the transfer is over, we must  */
		/* read the Rx FIFO.                   */
		while (mcspi_read_cs_reg(spi, OMAP2_MCSPI_XFERLEVEL) >> 16)
			cpu_relax();

		j = (element_count / 16) * 16;
		for (i = 0; i < element_count % 16; i++) {
			switch (bytes_per_transfer) {
			case 1:
				((u8 *)rx)[i+j] = (u8)mcspi_read_cs_reg(spi,
							OMAP2_MCSPI_RX0);
				break;
			case 2:
				((u16 *)rx)[i+j] = (u16)mcspi_read_cs_reg(spi,
							OMAP2_MCSPI_RX0);
				break;
			case 4:
				((u32 *)rx)[i+j] = (u32)mcspi_read_cs_reg(spi,
							OMAP2_MCSPI_RX0);
				break;
			}
		}
	}

	return count;
}

static unsigned
omap2_mcspi_txrx_pio(struct spi_device *spi, struct spi_transfer *xfer)
{
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_cs	*cs = spi->controller_state;
	unsigned int		count, c;
	u32			l;
	void __iomem		*base = cs->base;
	void __iomem		*tx_reg;
	void __iomem		*rx_reg;
	void __iomem		*chstat_reg;
	int			word_len;

	mcspi = spi_master_get_devdata(spi->master);
	count = xfer->len;
	c = count;
	word_len = cs->word_len;

	l = mcspi_cached_chconf0(spi);
	l &= ~OMAP2_MCSPI_CHCONF_TRM_MASK;

	/* We store the pre-calculated register addresses on stack to speed
	 * up the transfer loop. */
	tx_reg		= base + OMAP2_MCSPI_TX0;
	rx_reg		= base + OMAP2_MCSPI_RX0;
	chstat_reg	= base + OMAP2_MCSPI_CHSTAT0;

	if (c < (word_len>>3))
		return 0;

	if (word_len <= 8) {
		u8		*rx;
		const u8	*tx;

		rx = xfer->rx_buf;
		tx = xfer->tx_buf;

		do {
			c -= 1;
			if (tx != NULL) {
				if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_TXS) < 0) {
					dev_err(&spi->dev, "TXS timed out\n");
					goto out;
				}
				dev_vdbg(&spi->dev, "write-%d %02x '%c'\n",
						word_len, *tx, *tx);
				__raw_writel(*tx++, tx_reg);
			}
			if (rx != NULL) {
				if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_RXS) < 0) {
					dev_err(&spi->dev, "RXS timed out\n");
					goto out;
				}
				/* prevent last RX_ONLY read from triggering
				 * more word i/o: switch to rx+tx
				 */
				if (c == 0 && tx == NULL)
					omap2_mcspi_set_enable(spi,0);
				*rx++ = __raw_readl(rx_reg);
				dev_vdbg(&spi->dev, "read-%d %02x '%c'\n",
						word_len, *(rx - 1), *(rx - 1));
			}
		} while (c);
	} else if (word_len <= 16) {
		u16		*rx;
		const u16	*tx;

		rx = xfer->rx_buf;
		tx = xfer->tx_buf;
		do {
			c -= 2;
			if (tx != NULL) {
				if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_TXS) < 0) {
					dev_err(&spi->dev, "TXS timed out\n");
					goto out;
				}
				dev_vdbg(&spi->dev, "write-%d %04x\n",
						word_len, *tx);
				__raw_writel(*tx++, tx_reg);
			}
			if (rx != NULL) {
				if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_RXS) < 0) {
					dev_err(&spi->dev, "RXS timed out\n");
					goto out;
				}
				/* prevent last RX_ONLY read from triggering
				 * more word i/o: switch to rx+tx
				 */
				if (c == 0 && tx == NULL)
					omap2_mcspi_set_enable(spi,0);
				*rx++ = __raw_readl(rx_reg);
				dev_vdbg(&spi->dev, "read-%d %04x\n",
						word_len, *(rx - 1));
			}
		} while (c >= 2);
	} else if (word_len <= 32) {
		u32		*rx;
		const u32	*tx;

		rx = xfer->rx_buf;
		tx = xfer->tx_buf;
		do {
			c -= 4;
			if (tx != NULL) {
				if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_TXS) < 0) {
					dev_err(&spi->dev, "TXS timed out\n");
					goto out;
				}
				dev_vdbg(&spi->dev, "write-%d %08x\n",
						word_len, *tx);
				__raw_writel(*tx++, tx_reg);
			}
			if (rx != NULL) {
				if (mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_RXS) < 0) {
					dev_err(&spi->dev, "RXS timed out\n");
					goto out;
				}
				/* prevent last RX_ONLY read from triggering
				 * more word i/o: switch to rx+tx
				 */
				if (c == 0 && tx == NULL)
					omap2_mcspi_set_enable(spi,0);
				*rx++ = __raw_readl(rx_reg);
				dev_vdbg(&spi->dev, "read-%d %08x\n",
						word_len, *(rx - 1));
			}
		} while (c >= 4);
	}

	/* for TX_ONLY mode, be sure all words have shifted out */
	if (xfer->rx_buf == NULL) {
		if (mcspi_wait_for_reg_bit(chstat_reg,
				OMAP2_MCSPI_CHSTAT_TXS) < 0) {
			dev_err(&spi->dev, "TXS timed out\n");
		} else if (mcspi_wait_for_reg_bit(chstat_reg,
				OMAP2_MCSPI_CHSTAT_EOT) < 0)
			dev_err(&spi->dev, "EOT timed out\n");

		/* disable chan to purge rx datas received in TX_ONLY transfer,
		 * otherwise these rx datas will affect the direct following
		 * RX_ONLY transfer.
		 */
		omap2_mcspi_set_enable(spi, 0);
	}
out:
	omap2_mcspi_set_enable(spi, 1);
	return count - c;
}

static irqreturn_t
omap_mcspi_isr(int irq, void *mcspi_id)
{
	struct omap2_mcspi *mcspi = mcspi_id;
	u32 irqstatus = 0;

	irqstatus = mcspi_read_reg((struct spi_master *)mcspi->master,
					OMAP2_MCSPI_IRQSTATUS);

	/* clear the interrupt */
	mcspi_write_reg(mcspi->master, OMAP2_MCSPI_IRQSTATUS, irqstatus);
	if (irqstatus & OMAP2_MCSPI_IRQ_RX0_OVERFLOW)
		dev_warn(&(mcspi->master->dev), "RX overflow detected\n");

	return IRQ_HANDLED;
}

static u32 omap2_mcspi_calc_divisor(u32 speed_hz)
{
	u32 div;

	for (div = 0; div < 15; div++)
		if (speed_hz >= (OMAP2_MCSPI_MAX_FREQ >> div))
			return div;

	return 15;
}

/* called only when no transfer is active to this device */
static int omap2_mcspi_setup_transfer(struct spi_device *spi,
		struct spi_transfer *t)
{
	struct omap2_mcspi_cs *cs = spi->controller_state;
	struct omap2_mcspi_device_config *config = spi->controller_data;
	struct omap2_mcspi *mcspi;
	struct spi_master *spi_cntrl;
	u32 l = 0, div = 0;
	u8 word_len = spi->bits_per_word;
	u32 speed_hz = spi->max_speed_hz;

	/* Disable the channel */
	omap2_mcspi_set_enable(spi, 0);

	mcspi = spi_master_get_devdata(spi->master);
	spi_cntrl = mcspi->master;

	if (t != NULL && t->bits_per_word)
		word_len = t->bits_per_word;

	cs->word_len = word_len;

	if (t && t->speed_hz)
		speed_hz = t->speed_hz;

	speed_hz = min_t(u32, speed_hz, OMAP2_MCSPI_MAX_FREQ);
	div = omap2_mcspi_calc_divisor(speed_hz);

	l = mcspi_cached_chconf0(spi);

	/* standard 4-wire master mode:  SCK, MOSI/out, MISO/in, nCS
	 * REVISIT: this controller could support SPI_3WIRE mode.
	 */
	if (mcspi->mcspi_mode == OMAP2_MCSPI_MASTER) {
		l &= ~(OMAP2_MCSPI_CHCONF_IS|OMAP2_MCSPI_CHCONF_DPE1);
		l |= OMAP2_MCSPI_CHCONF_DPE0;
	} else {
		l |= OMAP2_MCSPI_CHCONF_IS;
		l |= OMAP2_MCSPI_CHCONF_DPE1;
		l &= ~OMAP2_MCSPI_CHCONF_DPE0;
	}

	/* wordlength */
	l &= ~OMAP2_MCSPI_CHCONF_WL_MASK;
	l |= (word_len - 1) << 7;

	/* set chipselect polarity; manage with FORCE */
	if (!(spi->mode & SPI_CS_HIGH))
		l |= OMAP2_MCSPI_CHCONF_EPOL;	/* active-low; normal */
	else
		l &= ~OMAP2_MCSPI_CHCONF_EPOL;

	if (mcspi->mcspi_mode == OMAP2_MCSPI_MASTER) {
		/* set clock divisor */
		l &= ~OMAP2_MCSPI_CHCONF_CLKD_MASK;
		l |= div << 2;
	}

	/* set SPI mode 0..3 */
	if (spi->mode & SPI_CPOL)
		l |= OMAP2_MCSPI_CHCONF_POL;
	else
		l &= ~OMAP2_MCSPI_CHCONF_POL;
	if (spi->mode & SPI_CPHA)
		l |= OMAP2_MCSPI_CHCONF_PHA;
	else
		l &= ~OMAP2_MCSPI_CHCONF_PHA;

	mcspi_write_chconf0(spi, l);

	if (mcspi->mcspi_mode == OMAP2_MCSPI_SLAVE) {
		omap2_mcspi_set_slave_mode(spi_cntrl);

		if (config->rx_dma_timeout_func) {
			mcspi->slave_buffered_device = spi;
			omap2_mcspi_start_slave_dma(spi);
		}
	}

	/* Re-enable the channel */
	omap2_mcspi_set_enable(spi, 1);

#ifdef CONFIG_SPI_DEBUG
	omap2_mcspi_dump_regs(spi_cntrl);
#endif

	return 0;
}

static void omap2_mcspi_dma_rx_callback(int lch, u16 ch_status, void *data)
{
	struct spi_device	*spi = data;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;

	mcspi = spi_master_get_devdata(spi->master);
	mcspi_dma = &(mcspi->dma_channels[spi->chip_select]);

	if (!mcspi->slave_buffered_device) {
		complete(&mcspi_dma->dma_rx_completion);
	
		/* We must disable the DMA RX request */
		omap2_mcspi_set_dma_req(spi, 1, 0);
	}
}

static void omap2_mcspi_dma_tx_callback(int lch, u16 ch_status, void *data)
{
	struct spi_device	*spi = data;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;

	mcspi = spi_master_get_devdata(spi->master);
	mcspi_dma = &(mcspi->dma_channels[spi->chip_select]);

	if (mcspi->tx_callback)
		mcspi->tx_callback(NULL);
	else {
		complete(&mcspi_dma->dma_tx_completion);
	
		/* We must disable the DMA TX request */
		omap2_mcspi_set_dma_req(spi, 0, 0);
	}
}

static int omap2_mcspi_request_dma(struct spi_device *spi)
{
	struct spi_master	*master = spi->master;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;

	mcspi = spi_master_get_devdata(master);
	mcspi_dma = mcspi->dma_channels + spi->chip_select;

	if (omap_request_dma(mcspi_dma->dma_rx_sync_dev, "McSPI RX",
			omap2_mcspi_dma_rx_callback, spi,
			&mcspi_dma->dma_rx_channel)) {
		dev_err(&spi->dev, "no RX DMA channel for McSPI\n");
		return -EAGAIN;
	}

	if (omap_request_dma(mcspi_dma->dma_tx_sync_dev, "McSPI TX",
			omap2_mcspi_dma_tx_callback, spi,
			&mcspi_dma->dma_tx_channel)) {
		omap_free_dma(mcspi_dma->dma_rx_channel);
		mcspi_dma->dma_rx_channel = -1;
		dev_err(&spi->dev, "no TX DMA channel for McSPI\n");
		return -EAGAIN;
	}

	init_completion(&mcspi_dma->dma_rx_completion);
	init_completion(&mcspi_dma->dma_tx_completion);

	return 0;
}

static int omap2_mcspi_setup(struct spi_device *spi)
{
	int			ret;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;
	struct omap2_mcspi_cs	*cs = spi->controller_state;
	struct omap2_mcspi_device_config *config = spi->controller_data;

	if (spi->bits_per_word < 4 || spi->bits_per_word > 32) {
		dev_dbg(&spi->dev, "setup: unsupported %d bit words\n",
			spi->bits_per_word);
		return -EINVAL;
	}

	mcspi = spi_master_get_devdata(spi->master);
	mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	/* At run-time, it is possible to change McSPI configuration master/slave mode */
	if (config->slave)
	{
		/* slave mode selected */
		if (mcspi->mcspi_mode != OMAP2_MCSPI_SLAVE) {
			/* Only enable the clocks if we are changing mode */
			if (omap2_mcspi_enable_clocks(mcspi))
				return -ENODEV;
			mcspi->mcspi_mode = OMAP2_MCSPI_SLAVE;
		}
		dev_dbg(&spi->dev, "setup: slave mode selected\n");

		/* in slave mode, we cannot achieve more than OMAP2_MCSPI_MAX_SLAVE_OP_FREQ MHz */
		if (spi->max_speed_hz > OMAP2_MCSPI_MAX_SLAVE_OP_FREQ)
		{
			dev_dbg(&spi->dev, "Max op. freq. in slave mode is %d. Reverting to this frequency\n", 
						OMAP2_MCSPI_MAX_SLAVE_OP_FREQ);
			spi->max_speed_hz = OMAP2_MCSPI_MAX_SLAVE_OP_FREQ;
		}

		mcspi->tx_callback = config->tx_callback;
		mcspi->rx_dma_timer.function = config->rx_dma_timeout_func;
		mcspi->rx_dma_timer.data = (unsigned long) mcspi;
	}

	if (!cs) {
		cs = kzalloc(sizeof *cs, GFP_KERNEL);
		if (!cs)
			return -ENOMEM;
		cs->base = mcspi->base + spi->chip_select * 0x14;
		cs->phys = mcspi->phys + spi->chip_select * 0x14;
		cs->chconf0 = 0;
		spi->controller_state = cs;
		/* Link this to context save list */
		list_add_tail(&cs->node,
			&omap2_mcspi_ctx[mcspi->master->bus_num - 1].cs);
	}

	if (mcspi_dma->dma_rx_channel == -1
			|| mcspi_dma->dma_tx_channel == -1) {
		ret = omap2_mcspi_request_dma(spi);
		if (ret < 0)
			return ret;
	}

	if (omap2_mcspi_enable_clocks(mcspi) < 0)
		return -ENODEV;

	ret = omap2_mcspi_setup_transfer(spi, NULL);
	omap2_mcspi_disable_clocks(mcspi);

	return ret;
}

static void omap2_mcspi_cleanup(struct spi_device *spi)
{
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;
	struct omap2_mcspi_cs	*cs;

	mcspi = spi_master_get_devdata(spi->master);

	if (spi->controller_state) {
		/* Unlink controller state from context save list */
		cs = spi->controller_state;
		list_del(&cs->node);

		kfree(spi->controller_state);
	}

	if (spi->chip_select < spi->master->num_chipselect) {
		mcspi_dma = &mcspi->dma_channels[spi->chip_select];

		if (mcspi_dma->dma_rx_channel != -1) {
			omap_free_dma(mcspi_dma->dma_rx_channel);
			mcspi_dma->dma_rx_channel = -1;
		}
		if (mcspi_dma->dma_tx_channel != -1) {
			omap_free_dma(mcspi_dma->dma_tx_channel);
			mcspi_dma->dma_tx_channel = -1;
		}
	}
}

static void omap2_mcspi_work(struct work_struct *work)
{
	struct omap2_mcspi	*mcspi;

	mcspi = container_of(work, struct omap2_mcspi, work);

	if (omap2_mcspi_enable_clocks(mcspi) < 0)
		return;

	spin_lock_irq(&mcspi->lock);

	/* We only enable one channel at a time -- the one whose message is
	 * at the head of the queue -- although this controller would gladly
	 * arbitrate among multiple channels.  This corresponds to "single
	 * channel" master mode.  As a side effect, we need to manage the
	 * chipselect with the FORCE bit ... CS != channel enable.
	 */
	while (!list_empty(&mcspi->msg_queue)) {
		struct spi_message		*m;
		struct spi_device		*spi;
		struct spi_transfer		*t = NULL;
		int				cs_active = 0;
		struct omap2_mcspi_cs		*cs;
		struct omap2_mcspi_device_config *cd;
		int				par_override = 0;
		int				status = 0;
		u32				chconf;

		m = container_of(mcspi->msg_queue.next, struct spi_message,
				 queue);

		list_del_init(&m->queue);
		spin_unlock_irq(&mcspi->lock);

		spi = m->spi;
		cs = spi->controller_state;
		cd = spi->controller_data;

		omap2_mcspi_set_enable(spi, 1);
		list_for_each_entry(t, &m->transfers, transfer_list) {
			if (t->tx_buf == NULL && t->rx_buf == NULL && t->len) {
				status = -EINVAL;
				break;
			}
			if (par_override || t->speed_hz || t->bits_per_word) {
				par_override = 1;
				status = omap2_mcspi_setup_transfer(spi, t);
				if (status < 0)
					break;
				if (!t->speed_hz && !t->bits_per_word)
					par_override = 0;
			}

			if ((!cs_active) && (mcspi->force_cs_mode) &&
				(mcspi->mcspi_mode ==
				OMAP2_MCSPI_MASTER)) {
				omap2_mcspi_force_cs(spi, 1);
				cs_active = 1;
			}

			chconf = mcspi_cached_chconf0(spi);
			chconf &= ~OMAP2_MCSPI_CHCONF_TRM_MASK;
			if (t->tx_buf == NULL)
				chconf |= OMAP2_MCSPI_CHCONF_TRM_RX_ONLY;
			else if (t->rx_buf == NULL)
				chconf |= OMAP2_MCSPI_CHCONF_TRM_TX_ONLY;
			mcspi_write_chconf0(spi, chconf);

			if (t->len) {
				unsigned	count;

				/* RX_ONLY mode needs dummy data in TX reg */
				if (t->tx_buf == NULL)
					__raw_writel(0, cs->base
							+ OMAP2_MCSPI_TX0);

				if (m->is_dma_mapped ||
					t->len >= DMA_MIN_BYTES ||
					mcspi->dma_mode)
					count = omap2_mcspi_txrx_dma(spi, t);
				else
					count = omap2_mcspi_txrx_pio(spi, t);
				m->actual_length += count;

				if (count != t->len) {
					status = -EIO;
					break;
				}
			}

			if (t->delay_usecs)
				udelay(t->delay_usecs);

			/* ignore the "leave it on after last xfer" hint */
			if ((t->cs_change) && (mcspi->force_cs_mode) &&
				(mcspi->mcspi_mode == OMAP2_MCSPI_MASTER)) {

				omap2_mcspi_force_cs(spi, 0);
				cs_active = 0;
			}
		}

		/* Restore defaults if they were overriden */
		if (par_override) {
			par_override = 0;
			status = omap2_mcspi_setup_transfer(spi, NULL);
		}

		if ((cs_active) && (mcspi->force_cs_mode) &&
			(mcspi->mcspi_mode == OMAP2_MCSPI_MASTER))
				omap2_mcspi_force_cs(spi, 0);

		omap2_mcspi_set_enable(spi, 0);

		m->status = status;
		m->complete(m->context);

		spin_lock_irq(&mcspi->lock);
	}

	spin_unlock_irq(&mcspi->lock);

	omap2_mcspi_disable_clocks(mcspi);
}

static int omap2_mcspi_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct omap2_mcspi	*mcspi;
	unsigned long		flags;
	struct spi_transfer	*t;

	m->actual_length = 0;
	m->status = 0;
	mcspi = spi_master_get_devdata(spi->master);

	/* reject invalid messages and transfers */
	if (list_empty(&m->transfers) || !m->complete)
		return -EINVAL;
	list_for_each_entry(t, &m->transfers, transfer_list) {
		const void	*tx_buf = t->tx_buf;
		void		*rx_buf = t->rx_buf;
		unsigned	len = t->len;

		if (t->speed_hz > OMAP2_MCSPI_MAX_FREQ
				|| (len && !(rx_buf || tx_buf))
				|| (t->bits_per_word &&
					(  t->bits_per_word < 4
					|| t->bits_per_word > 32))) {
			dev_dbg(&spi->dev, "transfer: %d Hz, %d %s%s, %d bpw\n",
					t->speed_hz,
					len,
					tx_buf ? "tx" : "",
					rx_buf ? "rx" : "",
					t->bits_per_word);
			return -EINVAL;
		}
		if (t->speed_hz && t->speed_hz < (OMAP2_MCSPI_MAX_FREQ >> 15)) {
			dev_dbg(&spi->dev, "speed_hz %d below minimum %d Hz\n",
				t->speed_hz,
				OMAP2_MCSPI_MAX_FREQ >> 15);
			return -EINVAL;
		}

		/* Ignore DMA_MIN_BYTES check if dma only mode is set */
		if (m->is_dma_mapped || ((len < DMA_MIN_BYTES) &&
						(!mcspi->dma_mode)))
			continue;

		/* Do DMA mapping "early" for better error reporting and
		 * dcache use.  Note that if dma_unmap_single() ever starts
		 * to do real work on ARM, we'd need to clean up mappings
		 * for previous transfers on *ALL* exits of this loop...
		 */
		if (tx_buf != NULL) {
			t->tx_dma = dma_map_single(&spi->dev, (void *) tx_buf,
					len, DMA_TO_DEVICE);
			if (dma_mapping_error(&spi->dev, t->tx_dma)) {
				dev_dbg(&spi->dev, "dma %cX %d bytes error\n",
						'T', len);
				return -EINVAL;
			}
		}
		if (rx_buf != NULL) {
			t->rx_dma = dma_map_single(&spi->dev, rx_buf, t->len,
					DMA_FROM_DEVICE);
			if (dma_mapping_error(&spi->dev, t->rx_dma)) {
				dev_dbg(&spi->dev, "dma %cX %d bytes error\n",
						'R', len);
				if (tx_buf != NULL)
					dma_unmap_single(&spi->dev, t->tx_dma,
							len, DMA_TO_DEVICE);
				return -EINVAL;
			}
		}
	}

	spin_lock_irqsave(&mcspi->lock, flags);
	list_add_tail(&m->queue, &mcspi->msg_queue);
	queue_work(omap2_mcspi_wq, &mcspi->work);
	spin_unlock_irqrestore(&mcspi->lock, flags);

	return 0;
}

static int __init omap2_mcspi_reset(struct omap2_mcspi *mcspi)
{
	struct spi_master	*master = mcspi->master;
	u32			tmp;

	if (omap2_mcspi_enable_clocks(mcspi) < 0)
		return -1;

	mcspi_write_reg(master, OMAP2_MCSPI_SYSCONFIG,
			OMAP2_MCSPI_SYSCONFIG_SOFTRESET);
	do {
		tmp = mcspi_read_reg(master, OMAP2_MCSPI_SYSSTATUS);
	} while (!(tmp & OMAP2_MCSPI_SYSSTATUS_RESETDONE));

	tmp = OMAP2_MCSPI_SYSCONFIG_AUTOIDLE |
		OMAP2_MCSPI_SYSCONFIG_ENAWAKEUP
		 | OMAP2_MCSPI_SYSCONFIG_SMARTIDLE;
	mcspi_write_reg(master, OMAP2_MCSPI_SYSCONFIG, tmp);
	omap2_mcspi_ctx[master->bus_num - 1].sysconfig = tmp;

	tmp = OMAP2_MCSPI_WAKEUPENABLE_WKEN;
	mcspi_write_reg(master, OMAP2_MCSPI_WAKEUPENABLE, tmp);
	omap2_mcspi_ctx[master->bus_num - 1].wakeupenable = tmp;

	if (mcspi->mcspi_mode == OMAP2_MCSPI_MASTER)
		omap2_mcspi_set_master_mode(master);
	else
		omap2_mcspi_set_slave_mode(master);

	if (mcspi->mcspi_mode == OMAP2_MCSPI_MASTER)
		omap2_mcspi_disable_clocks(mcspi);

	return 0;
}

static u8 __initdata spi1_rxdma_id [] = {
	OMAP24XX_DMA_SPI1_RX0,
	OMAP24XX_DMA_SPI1_RX1,
	OMAP24XX_DMA_SPI1_RX2,
	OMAP24XX_DMA_SPI1_RX3,
};

static u8 __initdata spi1_txdma_id [] = {
	OMAP24XX_DMA_SPI1_TX0,
	OMAP24XX_DMA_SPI1_TX1,
	OMAP24XX_DMA_SPI1_TX2,
	OMAP24XX_DMA_SPI1_TX3,
};

static u8 __initdata spi2_rxdma_id[] = {
	OMAP24XX_DMA_SPI2_RX0,
	OMAP24XX_DMA_SPI2_RX1,
};

static u8 __initdata spi2_txdma_id[] = {
	OMAP24XX_DMA_SPI2_TX0,
	OMAP24XX_DMA_SPI2_TX1,
};

#if defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP34XX) \
	|| defined(CONFIG_ARCH_OMAP4)
static u8 __initdata spi3_rxdma_id[] = {
	OMAP24XX_DMA_SPI3_RX0,
	OMAP24XX_DMA_SPI3_RX1,
};

static u8 __initdata spi3_txdma_id[] = {
	OMAP24XX_DMA_SPI3_TX0,
	OMAP24XX_DMA_SPI3_TX1,
};
#endif

#if defined(CONFIG_ARCH_OMAP3) || defined(CONFIG_ARCH_OMAP4)
static u8 __initdata spi4_rxdma_id[] = {
	OMAP34XX_DMA_SPI4_RX0,
};

static u8 __initdata spi4_txdma_id[] = {
	OMAP34XX_DMA_SPI4_TX0,
};
#endif

static int __init omap2_mcspi_probe(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct omap2_mcspi_platform_config *pdata =
		(struct omap2_mcspi_platform_config *)pdev->dev.platform_data;
	struct omap2_mcspi	*mcspi;
	struct resource		*r, *r_irq;
	int			status = 0, i;
	const u8		*rxdma_id, *txdma_id;
	unsigned		num_chipselect;

	dev_dbg(&pdev->dev, "omap2_mcspi_probe %d\n", pdev->id);

	switch (pdev->id) {
	case 1:
		rxdma_id = spi1_rxdma_id;
		txdma_id = spi1_txdma_id;
		num_chipselect = 4;
		break;
	case 2:
		rxdma_id = spi2_rxdma_id;
		txdma_id = spi2_txdma_id;
		num_chipselect = 2;
		break;
#if defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP3) \
	|| defined(CONFIG_ARCH_OMAP4)
	case 3:
		rxdma_id = spi3_rxdma_id;
		txdma_id = spi3_txdma_id;
		num_chipselect = 2;
		break;
#endif
#if defined(CONFIG_ARCH_OMAP3) || defined(CONFIG_ARCH_OMAP4)
	case 4:
		rxdma_id = spi4_rxdma_id;
		txdma_id = spi4_txdma_id;
		num_chipselect = 1;
		break;
#endif
	default:
		return -EINVAL;
	}

	master = spi_alloc_master(&pdev->dev, sizeof *mcspi);
	if (master == NULL) {
		dev_dbg(&pdev->dev, "master allocation failed\n");
		return -ENOMEM;
	}

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;

	if (pdev->id != -1)
		master->bus_num = pdev->id;

	master->setup = omap2_mcspi_setup;
	master->transfer = omap2_mcspi_transfer;
	master->cleanup = omap2_mcspi_cleanup;
	master->num_chipselect = num_chipselect;

	dev_set_drvdata(&pdev->dev, master);

	mcspi = spi_master_get_devdata(master);
	mcspi->master = master;
	mcspi->mcspi_mode = pdata->mode;
	mcspi->dma_mode = pdata->dma_mode;
	mcspi->force_cs_mode = pdata->force_cs_mode;

	if (pdata->fifo_depth <= OMAP2_MCSPI_MAX_FIFODEPTH)
		mcspi->fifo_depth = pdata->fifo_depth;
	else {
		mcspi->fifo_depth = 0;
		dev_dbg(&pdev->dev, "Invalid fifo depth specified\n");
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		status = -ENODEV;
		goto err1;
	}
	if (!request_mem_region(r->start, (r->end - r->start) + 1,
			dev_name(&pdev->dev))) {
		status = -EBUSY;
		goto err1;
	}

	mcspi->phys = r->start;
	mcspi->base = ioremap(r->start, r->end - r->start + 1);
	if (!mcspi->base) {
		dev_dbg(&pdev->dev, "can't ioremap MCSPI\n");
		status = -ENOMEM;
		goto err1aa;
	}

	INIT_WORK(&mcspi->work, omap2_mcspi_work);

	spin_lock_init(&mcspi->lock);
	INIT_LIST_HEAD(&mcspi->msg_queue);
	INIT_LIST_HEAD(&omap2_mcspi_ctx[master->bus_num - 1].cs);

	if (!cpu_is_omap44xx()) {
		mcspi->ick = clk_get(&pdev->dev, "ick");
		if (IS_ERR(mcspi->ick)) {
			dev_dbg(&pdev->dev, "can't get mcspi_ick\n");
			status = PTR_ERR(mcspi->ick);
			goto err1a;
		}
	}

	mcspi->fck = clk_get(&pdev->dev, "fck");
	if (IS_ERR(mcspi->fck)) {
		dev_dbg(&pdev->dev, "can't get mcspi_fck\n");
		status = PTR_ERR(mcspi->fck);
		goto err2;
	}


	r_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (r_irq == NULL) {
		dev_err(&pdev->dev, "can't get MCSPI %d IRQ\n", pdev->id);
		status = -ENODEV;
		goto err1;
	}
	dev_dbg(&pdev->dev, "Allocated IRQ %d\n", r_irq->start);


	/* request irq for the channel*/
	if (request_irq(r_irq->start, omap_mcspi_isr,
			 IRQF_DISABLED | IRQF_SAMPLE_RANDOM,
			"omap_mcspi_isr", mcspi)) {
		printk(KERN_ERR "MCSPI ERROR: couldnt allocate irq\n");
	}

	mcspi->dma_channels = kcalloc(master->num_chipselect,
			sizeof(struct omap2_mcspi_dma),
			GFP_KERNEL);

	if (mcspi->dma_channels == NULL)
		goto err3;

	for (i = 0; i < num_chipselect; i++) {
		mcspi->dma_channels[i].dma_rx_channel = -1;
		mcspi->dma_channels[i].dma_rx_sync_dev = rxdma_id[i];
		mcspi->dma_channels[i].dma_tx_channel = -1;
		mcspi->dma_channels[i].dma_tx_sync_dev = txdma_id[i];
	}
	init_timer(&(mcspi->rx_dma_timer));
	if (omap2_mcspi_reset(mcspi) < 0)
		goto err4;

	status = spi_register_master(master);
	if (status < 0)
		goto err4;

	return status;

err4:
	dev_err(&pdev->dev, "omap2_mcspi_probe err4(%d)\n", status);
	kfree(mcspi->dma_channels);
err3:
	dev_err(&pdev->dev, "omap2_mcspi_probe err3(%d)\n", status);
	clk_put(mcspi->fck);
err2:
	dev_err(&pdev->dev, "omap2_mcspi_probe err2(%d)\n", status);
	clk_put(mcspi->ick);
err1a:
	dev_err(&pdev->dev, "omap2_mcspi_probe err1a(%d)\n", status);
	iounmap(mcspi->base);
err1aa:
	dev_err(&pdev->dev, "omap2_mcspi_probe err1aa(%d)\n", status);
	release_mem_region(r->start, (r->end - r->start) + 1);
err1:
	dev_err(&pdev->dev, "omap2_mcspi_probe err1(%d)\n", status);
	spi_master_put(master);
	return status;
}

static int __exit omap2_mcspi_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*dma_channels;
	struct resource		*r;
	void __iomem *base;

	master = dev_get_drvdata(&pdev->dev);
	mcspi = spi_master_get_devdata(master);
	dma_channels = mcspi->dma_channels;

	if (mcspi->slave_buffered_device)
		omap2_mcspi_stop_slave_dma(mcspi->slave_buffered_device);	
	
	if (mcspi->mcspi_mode == OMAP2_MCSPI_SLAVE)
		omap2_mcspi_disable_clocks(mcspi);

	clk_put(mcspi->fck);
	clk_put(mcspi->ick);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!r)
		WARN_ON(1);
	else
		release_mem_region(r->start, (r->end - r->start) + 1);

	base = mcspi->base;
	spi_unregister_master(master);
	iounmap(base);
	kfree(dma_channels);

	return 0;
}

static int omap2_mcspi_suspend(struct platform_device *pdev, pm_message_t message)
{
	struct spi_master	*master;
	struct omap2_mcspi	*mcspi;

	master = dev_get_drvdata(&pdev->dev);
	mcspi = spi_master_get_devdata(master);

	if (mcspi->mcspi_mode == OMAP2_MCSPI_SLAVE) {
		/* Disable channel, interrupts and clocks */
		if (mcspi->slave_buffered_device)
			omap2_mcspi_stop_slave_dma(mcspi->slave_buffered_device);
		mcspi_write_reg(master, OMAP2_MCSPI_CHCTRL0, 0);
		omap2_mcspi_set_master_mode (master);
		omap2_mcspi_disable_clocks(mcspi);
	}

	return 0;
}

static int omap2_mcspi_resume(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct omap2_mcspi	*mcspi;

	master = dev_get_drvdata(&pdev->dev);
	mcspi = spi_master_get_devdata(master);

	if (mcspi->mcspi_mode == OMAP2_MCSPI_SLAVE) {
		u32 l;
		omap2_mcspi_enable_clocks(mcspi);

		// empty Receive FIFO before enabling SPI in slave mode.
		l = mcspi_read_reg(master, OMAP2_MCSPI_CHSTAT0);
		while (l & OMAP2_MCSPI_CHSTAT_RXS)
		{
			l = mcspi_read_reg(master, OMAP2_MCSPI_RX0);
			l = mcspi_read_reg(master, OMAP2_MCSPI_CHSTAT0);
		}

		/* Enable channel and interrupts */
		omap2_mcspi_set_slave_mode (master);
		mcspi_write_reg(master, OMAP2_MCSPI_CHCTRL0, OMAP2_MCSPI_CHCTRL_EN);
		if (mcspi->slave_buffered_device)
			omap2_mcspi_start_slave_dma(mcspi->slave_buffered_device);
	}

	return 0;
}

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:omap2_mcspi");

static struct platform_driver omap2_mcspi_driver = {
	.driver = {
		.name =		"omap2_mcspi",
		.owner =	THIS_MODULE,
	},
	.remove  =	__exit_p(omap2_mcspi_remove),
	.suspend = omap2_mcspi_suspend,
	.resume = omap2_mcspi_resume,
};

static int __init omap2_mcspi_init(void)
{
	omap2_mcspi_wq = create_singlethread_workqueue(
				omap2_mcspi_driver.driver.name);
	if (omap2_mcspi_wq == NULL)
		return -1;
	return platform_driver_probe(&omap2_mcspi_driver, omap2_mcspi_probe);
}
subsys_initcall(omap2_mcspi_init);

static void __exit omap2_mcspi_exit(void)
{
	platform_driver_unregister(&omap2_mcspi_driver);

	destroy_workqueue(omap2_mcspi_wq);
}
module_exit(omap2_mcspi_exit);

MODULE_LICENSE("GPL");
