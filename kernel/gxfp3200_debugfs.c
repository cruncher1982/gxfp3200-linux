// SPDX-License-Identifier: GPL-2.0-only
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/hex.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "gxfp3200.h"

#define GXFP3200_DEBUG_MAX_WRITE 4096

static int gxfp3200_status_show(struct seq_file *s, void *unused)
{
	struct gxfp3200_device *gxfp = s->private;

	seq_printf(s, "powered: %u\ninterrupts: %u\n", gxfp->powered,
		   atomic_read(&gxfp->irq_count));
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(gxfp3200_status);

static int gxfp3200_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t gxfp3200_reset_write(struct file *file, const char __user *buf,
				    size_t count, loff_t *ppos)
{
	struct gxfp3200_device *gxfp = file->private_data;
	int ret;

	ret = gxfp3200_hw_reset(gxfp);
	return ret ? ret : count;
}

static const struct file_operations gxfp3200_reset_fops = {
	.owner = THIS_MODULE,
	.open = gxfp3200_debugfs_open,
	.write = gxfp3200_reset_write,
	.llseek = no_llseek,
};

static ssize_t gxfp3200_send_write(struct file *file, const char __user *buf,
				   size_t count, loff_t *ppos)
{
	struct gxfp3200_device *gxfp = file->private_data;
	u8 *data;
	int len;
	int ret;

	if (!count || count > GXFP3200_DEBUG_MAX_WRITE || count % 2)
		return -EINVAL;

	data = memdup_user_nul(buf, count);
	if (IS_ERR(data))
		return PTR_ERR(data);

	len = hex2bin(data, (const char *)data, count / 2);
	if (len) {
		ret = -EINVAL;
		goto out_free;
	}

	ret = gxfp3200_spi_write(gxfp, data, count / 2);
	if (!ret)
		ret = count;
out_free:
	kfree(data);
	return ret;
}

static const struct file_operations gxfp3200_send_fops = {
	.owner = THIS_MODULE,
	.open = gxfp3200_debugfs_open,
	.write = gxfp3200_send_write,
	.llseek = no_llseek,
};

static ssize_t gxfp3200_receive_read(struct file *file, char __user *buf,
				     size_t count, loff_t *ppos)
{
	struct gxfp3200_device *gxfp = file->private_data;
	u8 data[GXFP3200_DEBUG_READ_SIZE];
	char output[GXFP3200_DEBUG_READ_SIZE * 3 + 1];
	unsigned int i;
	int ret;

	if (*ppos)
		return 0;

	ret = gxfp3200_spi_read(gxfp, data, sizeof(data));
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(data); i++)
		scprintf(output + i * 3, sizeof(output) - i * 3, "%02x%s",
			 data[i], i == ARRAY_SIZE(data) - 1 ? "\n" : " ");

	return simple_read_from_buffer(buf, count, ppos, output,
				       sizeof(output) - 1);
}

static const struct file_operations gxfp3200_receive_fops = {
	.owner = THIS_MODULE,
	.open = gxfp3200_debugfs_open,
	.read = gxfp3200_receive_read,
	.llseek = no_llseek,
};

void gxfp3200_debugfs_init(struct gxfp3200_device *gxfp)
{
	gxfp->debugfs_dir = debugfs_create_dir(dev_name(&gxfp->spi->dev), NULL);
	debugfs_create_file("status", 0400, gxfp->debugfs_dir, gxfp,
			    &gxfp3200_status_fops);
	debugfs_create_file("reset", 0200, gxfp->debugfs_dir, gxfp,
			    &gxfp3200_reset_fops);
	debugfs_create_file("send", 0200, gxfp->debugfs_dir, gxfp,
			    &gxfp3200_send_fops);
	debugfs_create_file("receive", 0400, gxfp->debugfs_dir, gxfp,
			    &gxfp3200_receive_fops);
}

void gxfp3200_debugfs_remove(struct gxfp3200_device *gxfp)
{
	debugfs_remove_recursive(gxfp->debugfs_dir);
}
