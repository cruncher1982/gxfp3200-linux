// SPDX-License-Identifier: GPL-2.0-only
#include <linux/delay.h>
#include <linux/hex_dump.h>

#include "gxfp3200.h"

static int gxfp3200_spi_sync(struct gxfp3200_device *gxfp,
			     struct spi_transfer *xfers, unsigned int nxfers)
{
	struct spi_message message;
	int ret;
	int attempt;

	spi_message_init_with_transfers(&message, xfers, nxfers);
	for (attempt = 0; attempt < GXFP3200_SPI_RETRIES; attempt++) {
		ret = spi_sync(gxfp->spi, &message);
		if (!ret)
			return 0;

		dev_dbg(&gxfp->spi->dev, "SPI transfer attempt %d failed: %d\n",
			attempt + 1, ret);
		usleep_range(1000, 2000);
	}

	return ret;
}

int gxfp3200_spi_write(struct gxfp3200_device *gxfp, const void *buf,
		       size_t len)
{
	struct spi_transfer xfer = {.tx_buf = buf, .len = len};
	int ret;

	if (!len)
		return -EINVAL;

	print_hex_dump_debug("gxfp3200 TX: ", DUMP_PREFIX_OFFSET, 16, 1, buf,
			     len, false);
	mutex_lock(&gxfp->transfer_lock);
	ret = gxfp3200_spi_sync(gxfp, &xfer, 1);
	mutex_unlock(&gxfp->transfer_lock);
	return ret;
}

int gxfp3200_spi_read(struct gxfp3200_device *gxfp, void *buf, size_t len)
{
	struct spi_transfer xfer = {.rx_buf = buf, .len = len};
	int ret;

	if (!len)
		return -EINVAL;

	mutex_lock(&gxfp->transfer_lock);
	ret = gxfp3200_spi_sync(gxfp, &xfer, 1);
	mutex_unlock(&gxfp->transfer_lock);
	if (!ret)
		print_hex_dump_debug("gxfp3200 RX: ", DUMP_PREFIX_OFFSET, 16, 1,
				     buf, len, false);
	return ret;
}

int gxfp3200_spi_transaction(struct gxfp3200_device *gxfp, const void *tx_buf,
			     size_t tx_len, void *rx_buf, size_t rx_len)
{
	struct spi_transfer xfers[] = {
	    {.tx_buf = tx_buf, .len = tx_len},
	    {.rx_buf = rx_buf, .len = rx_len},
	};
	int ret;

	if (!tx_len || !rx_len)
		return -EINVAL;

	print_hex_dump_debug("gxfp3200 TX: ", DUMP_PREFIX_OFFSET, 16, 1, tx_buf,
			     tx_len, false);
	mutex_lock(&gxfp->transfer_lock);
	ret = gxfp3200_spi_sync(gxfp, xfers, ARRAY_SIZE(xfers));
	mutex_unlock(&gxfp->transfer_lock);
	if (!ret)
		print_hex_dump_debug("gxfp3200 RX: ", DUMP_PREFIX_OFFSET, 16, 1,
				     rx_buf, rx_len, false);
	return ret;
}
