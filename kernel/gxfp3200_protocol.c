// SPDX-License-Identifier: GPL-2.0-only
#include <linux/device.h>

#include "gxfp3200.h"

/**
 * gxfp3200_protocol_exchange() - run one research protocol exchange
 * @gxfp: GXFP3200 device
 * @tx_buf: bytes to transmit before reading
 * @tx_len: number of bytes to transmit
 * @rx_buf: caller-provided receive buffer
 * @rx_len: number of bytes to read after transmitting
 *
 * The GXFP3200 wire protocol is not yet known, so this helper deliberately
 * models only the packet shape that has already been validated by the SPI
 * transport: a write phase followed by a read phase.  Higher-level command
 * IDs, checksums, firmware handling, and response parsing must be added only
 * after they are confirmed from hardware traces.
 */
int gxfp3200_protocol_exchange(struct gxfp3200_device *gxfp,
			       const void *tx_buf, size_t tx_len,
			       void *rx_buf, size_t rx_len)
{
	int ret;

	if (!tx_len || !rx_len)
		return -EINVAL;

	if (tx_len > GXFP3200_RESEARCH_MAX_XFER ||
	    rx_len > GXFP3200_RESEARCH_MAX_XFER)
		return -EMSGSIZE;

	ret = gxfp3200_spi_transaction(gxfp, tx_buf, tx_len, rx_buf, rx_len);
	if (ret)
		return ret;

	dev_dbg(&gxfp->spi->dev,
		"research exchange complete: tx=%zu rx=%zu\n", tx_len, rx_len);
	return 0;
}
