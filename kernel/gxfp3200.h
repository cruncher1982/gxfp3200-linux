/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __GXFP3200_H__
#define __GXFP3200_H__

#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/gpio/consumer.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>

#define GXFP3200_DRIVER_NAME "gxfp3200"
#define GXFP3200_SPI_RETRIES 3
#define GXFP3200_DEBUG_READ_SIZE 256

struct dentry;

struct gxfp3200_device {
	struct spi_device *spi;
	struct mutex transfer_lock;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *irq_gpio;
	struct regulator *vdd;
	struct regulator *vddio;
	struct dentry *debugfs_dir;
	struct completion irq_completion;
	atomic_t irq_count;
	bool supplies_acquired;
	bool powered;
};

int gxfp3200_spi_write(struct gxfp3200_device *gxfp, const void *buf,
		       size_t len);
int gxfp3200_spi_read(struct gxfp3200_device *gxfp, void *buf, size_t len);
int gxfp3200_spi_transaction(struct gxfp3200_device *gxfp, const void *tx_buf,
			     size_t tx_len, void *rx_buf, size_t rx_len);

int gxfp3200_gpio_init(struct gxfp3200_device *gxfp);
int gxfp3200_hw_reset(struct gxfp3200_device *gxfp);
int gxfp3200_power_on(struct gxfp3200_device *gxfp);
void gxfp3200_power_off(struct gxfp3200_device *gxfp);
int gxfp3200_irq_init(struct gxfp3200_device *gxfp);
void gxfp3200_debugfs_init(struct gxfp3200_device *gxfp);
void gxfp3200_debugfs_remove(struct gxfp3200_device *gxfp);

#endif /* __GXFP3200_H__ */
