// SPDX-License-Identifier: GPL-2.0-only
#include <linux/delay.h>

#include "gxfp3200.h"

int gxfp3200_gpio_init(struct gxfp3200_device *gxfp)
{
	struct device *dev = &gxfp->spi->dev;

	gxfp->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(gxfp->reset_gpio))
		return PTR_ERR(gxfp->reset_gpio);

	gxfp->irq_gpio = devm_gpiod_get_optional(dev, "irq", GPIOD_IN);
	if (IS_ERR(gxfp->irq_gpio))
		return PTR_ERR(gxfp->irq_gpio);

	return 0;
}

int gxfp3200_hw_reset(struct gxfp3200_device *gxfp)
{
	if (!gxfp->reset_gpio) {
		dev_dbg(&gxfp->spi->dev, "no reset GPIO described\n");
		return 0;
	}

	gpiod_set_value_cansleep(gxfp->reset_gpio, 1);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(gxfp->reset_gpio, 0);
	usleep_range(10000, 12000);
	return 0;
}
