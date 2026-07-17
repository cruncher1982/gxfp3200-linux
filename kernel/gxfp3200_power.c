// SPDX-License-Identifier: GPL-2.0-only
#include "gxfp3200.h"

int gxfp3200_power_on(struct gxfp3200_device *gxfp)
{
	struct device *dev = &gxfp->spi->dev;
	int ret;

	if (!gxfp->supplies_acquired) {
		gxfp->vdd = devm_regulator_get_optional(dev, "vdd");
		if (IS_ERR(gxfp->vdd)) {
			if (PTR_ERR(gxfp->vdd) != -ENODEV)
				return PTR_ERR(gxfp->vdd);
			gxfp->vdd = NULL;
		}

		gxfp->vddio = devm_regulator_get_optional(dev, "vddio");
		if (IS_ERR(gxfp->vddio)) {
			if (PTR_ERR(gxfp->vddio) != -ENODEV)
				return PTR_ERR(gxfp->vddio);
			gxfp->vddio = NULL;
		}
		gxfp->supplies_acquired = true;
	}

	if (gxfp->powered)
		return 0;

	if (gxfp->vdd) {
		ret = regulator_enable(gxfp->vdd);
		if (ret)
			return ret;
	}
	if (gxfp->vddio) {
		ret = regulator_enable(gxfp->vddio);
		if (ret) {
			if (gxfp->vdd)
				regulator_disable(gxfp->vdd);
			return ret;
		}
	}

	gxfp->powered = true;
	return 0;
}

void gxfp3200_power_off(struct gxfp3200_device *gxfp)
{
	if (!gxfp->powered)
		return;

	if (gxfp->vddio)
		regulator_disable(gxfp->vddio);
	if (gxfp->vdd)
		regulator_disable(gxfp->vdd);
	gxfp->powered = false;
}
