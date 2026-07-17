// SPDX-License-Identifier: GPL-2.0-only
#include <linux/interrupt.h>

#include "gxfp3200.h"

static irqreturn_t gxfp3200_irq_thread(int irq, void *data)
{
	struct gxfp3200_device *gxfp = data;

	atomic_inc(&gxfp->irq_count);
	complete(&gxfp->irq_completion);
	dev_dbg(&gxfp->spi->dev, "interrupt %u\n",
		atomic_read(&gxfp->irq_count));
	return IRQ_HANDLED;
}

int gxfp3200_irq_init(struct gxfp3200_device *gxfp)
{
	int irq;

	if (!gxfp->irq_gpio)
		return 0;

	irq = gpiod_to_irq(gxfp->irq_gpio);
	if (irq < 0)
		return irq;

	return devm_request_threaded_irq(&gxfp->spi->dev, irq, NULL,
					 gxfp3200_irq_thread,
					 IRQF_ONESHOT | IRQF_TRIGGER_RISING,
					 GXFP3200_DRIVER_NAME, gxfp);
}
