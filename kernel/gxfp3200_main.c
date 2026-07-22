// SPDX-License-Identifier: GPL-2.0-only
#include <linux/acpi.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm.h>

#include "gxfp3200.h"

static int gxfp3200_probe(struct spi_device *spi)
{
	struct gxfp3200_device *gxfp;
	int ret;

	gxfp = devm_kzalloc(&spi->dev, sizeof(*gxfp), GFP_KERNEL);
	if (!gxfp)
		return -ENOMEM;

	gxfp->spi = spi;
	mutex_init(&gxfp->transfer_lock);
	mutex_init(&gxfp->research_lock);
	init_completion(&gxfp->irq_completion);
	atomic_set(&gxfp->irq_count, 0);
	spi_set_drvdata(spi, gxfp);

	ret = gxfp3200_gpio_init(gxfp);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
				     "failed to acquire GPIOs\n");

	ret = gxfp3200_power_on(gxfp);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
				     "failed to power device\n");

	ret = gxfp3200_hw_reset(gxfp);
	if (ret)
		goto err_power_off;

	ret = gxfp3200_irq_init(gxfp);
	if (ret)
		goto err_power_off;

	gxfp3200_debugfs_init(gxfp);
	dev_info(&spi->dev, "Goodix GXFP3200 sensor initialized\n");
	return 0;

err_power_off:
	gxfp3200_power_off(gxfp);
	return ret;
}

static void gxfp3200_remove(struct spi_device *spi)
{
	struct gxfp3200_device *gxfp = spi_get_drvdata(spi);

	gxfp3200_debugfs_remove(gxfp);
	gxfp3200_power_off(gxfp);
}

static int __maybe_unused gxfp3200_suspend(struct device *dev)
{
	struct gxfp3200_device *gxfp = spi_get_drvdata(to_spi_device(dev));

	mutex_lock(&gxfp->transfer_lock);
	gxfp3200_power_off(gxfp);
	mutex_unlock(&gxfp->transfer_lock);
	return 0;
}

static int __maybe_unused gxfp3200_resume(struct device *dev)
{
	struct gxfp3200_device *gxfp = spi_get_drvdata(to_spi_device(dev));
	int ret;

	mutex_lock(&gxfp->transfer_lock);
	ret = gxfp3200_power_on(gxfp);
	if (!ret)
		ret = gxfp3200_hw_reset(gxfp);
	mutex_unlock(&gxfp->transfer_lock);
	return ret;
}

static DEFINE_SIMPLE_DEV_PM_OPS(gxfp3200_pm_ops, gxfp3200_suspend,
			       gxfp3200_resume);

static const struct acpi_device_id gxfp3200_acpi_match[] = {
	{ "GXFP3200" },
	{ }
};
MODULE_DEVICE_TABLE(acpi, gxfp3200_acpi_match);

static const struct of_device_id gxfp3200_of_match[] = {
	{ .compatible = "goodix,gxfp3200" },
	{ }
};
MODULE_DEVICE_TABLE(of, gxfp3200_of_match);

static struct spi_driver gxfp3200_driver = {
	.driver = {
		.name = GXFP3200_DRIVER_NAME,
		.acpi_match_table = gxfp3200_acpi_match,
		.of_match_table = gxfp3200_of_match,
		.pm = pm_sleep_ptr(&gxfp3200_pm_ops),
	},
	.probe = gxfp3200_probe,
	.remove = gxfp3200_remove,
};
module_spi_driver(gxfp3200_driver);

MODULE_AUTHOR("GXFP3200 Linux contributors");
MODULE_DESCRIPTION("Goodix GXFP3200 SPI fingerprint sensor driver");
MODULE_LICENSE("GPL");
