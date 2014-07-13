#ifndef _OMAP2_MCSPI_H
#define _OMAP2_MCSPI_H

#define OMAP2_MCSPI_MASTER		0
#define OMAP2_MCSPI_SLAVE		1
#define OMAP2_MCSPI_DMA_BUF_SIZE (4096)
#define OMAP2_MCSPI_DMA_RX_TIMEOUT_MS (1)

struct omap2_mcspi_platform_config {
	unsigned short	num_cs;

	/* SPI is master or slave */
	unsigned short	mode;

	/* Use only DMA for data transfers */
	unsigned short	dma_mode;

	/* Force chip select mode */
	unsigned short	force_cs_mode;

	/* FIFO depth in bytes, max value 64 */
	unsigned short fifo_depth;

};

struct omap2_mcspi_device_config {
	unsigned turbo_mode:1;

	/* Do we want one channel enabled at the same time? */
	unsigned single_channel:1;

	/* SPI is master or slave : configurable at run-time */
	unsigned slave:1;

	/* Callback function for tx events */
	void (*tx_callback)(void *cb_data);

	/* Rx DMA timeout function */
	void (*rx_dma_timeout_func)(unsigned long data);
};

/* We have 2 DMA channels per CS, one for RX and one for TX */
struct omap2_mcspi_dma {
	int dma_tx_channel;
	int dma_rx_channel;

	int dma_tx_sync_dev;
	int dma_rx_sync_dev;

	struct completion dma_tx_completion;
	struct completion dma_rx_completion;
};

/* use PIO for small transfers, avoiding DMA setup/teardown overhead and
 * cache operations; better heuristics consider wordsize and bitrate.
 */
#define DMA_MIN_BYTES			32


struct omap2_mcspi {
	struct work_struct	work;
	struct work_struct	rx_cb_work;
	/* lock protects queue and registers */
	spinlock_t		lock;
	struct list_head	msg_queue;
	struct spi_master	*master;
	struct clk		*ick;
	struct clk		*fck;
	/* Virtual base address of the controller */
	void __iomem		*base;
	unsigned long		phys;
	/* SPI1 has 4 channels, while SPI2 has 2 */
	struct omap2_mcspi_dma	*dma_channels;
	unsigned short		mcspi_mode;
	unsigned short		dma_mode;
	unsigned short		force_cs_mode;
	unsigned short		fifo_depth;

	void 			(*tx_callback)(void *cb_data);
	int					slave_tx_dma_used;
	unsigned char*		slave_rx_buf;
	unsigned char*		slave_tx_buf;
	unsigned int 		slave_prev_rx_dma_pos;
	dma_addr_t 		slave_rx_buf_dma_phys;
	dma_addr_t 		slave_tx_buf_dma_phys;	
	struct timer_list	rx_dma_timer;
	struct spi_device	*slave_buffered_device;
};

struct omap2_mcspi_cs {
	void __iomem		*base;
	unsigned long		phys;
	int			word_len;
	struct list_head	node;
	/* Context save and restore shadow register */
	u32			chconf0;
};

/* used for context save and restore, structure members to be updated whenever
 * corresponding registers are modified.
 */
struct omap2_mcspi_regs {
	u32 sysconfig;
	u32 modulctrl;
	u32 wakeupenable;
	u32 xferlevel;
	struct list_head cs;
};

#endif
