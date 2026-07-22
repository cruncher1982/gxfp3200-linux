// SPDX-License-Identifier: GPL-2.0-only
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/hex.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "gxfp3200.h"

#define GXFP3200_DEBUG_MAX_WRITE 4096

static int gxfp3200_status_show(struct seq_file *s, void *unused)
{
	struct gxfp3200_device *gxfp = s->private;

	mutex_lock(&gxfp->research_lock);
	seq_printf(s, "powered: %u\ninterrupts: %u\nlast_exchange_rx: %zu\n",
		   gxfp->powered, atomic_read(&gxfp->irq_count),
		   gxfp->research_response_len);
	mutex_unlock(&gxfp->research_lock);
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

static ssize_t gxfp3200_exchange_write(struct file *file,
				       const char __user *buf, size_t count,
				       loff_t *ppos)
{
	struct gxfp3200_device *gxfp = file->private_data;
	char *input;
	char *hex;
	char *rx_token;
	u8 *tx;
	unsigned int rx_len;
	size_t hex_len;
	int ret;

	if (!count || count > GXFP3200_RESEARCH_MAX_XFER * 2 + 16)
		return -EINVAL;

	input = memdup_user_nul(buf, count);
	if (IS_ERR(input))
		return PTR_ERR(input);

	hex = strim(input);
	rx_token = strsep(&hex, " 	\n\r");
	if (!rx_token || !hex) {
		ret = -EINVAL;
		goto out_free_input;
	}

	ret = kstrtouint(rx_token, 0, &rx_len);
	if (ret)
		goto out_free_input;

	if (!rx_len || rx_len > GXFP3200_RESEARCH_MAX_XFER) {
		ret = -EMSGSIZE;
		goto out_free_input;
	}

	hex = strim(hex);
	hex_len = strlen(hex);
	if (!hex_len || hex_len > GXFP3200_RESEARCH_MAX_XFER * 2 ||
	    hex_len % 2) {
		ret = -EINVAL;
		goto out_free_input;
	}

	tx = kmalloc(hex_len / 2, GFP_KERNEL);
	if (!tx) {
		ret = -ENOMEM;
		goto out_free_input;
	}

	ret = hex2bin(tx, hex, hex_len / 2);
	if (ret)
		goto out_free_tx;

	mutex_lock(&gxfp->research_lock);
	gxfp->research_response_len = 0;
	ret = gxfp3200_protocol_exchange(gxfp, tx, hex_len / 2,
					   gxfp->research_response, rx_len);
	if (!ret)
		gxfp->research_response_len = rx_len;
	mutex_unlock(&gxfp->research_lock);

	if (!ret)
		ret = count;

out_free_tx:
	kfree(tx);
out_free_input:
	kfree(input);
	return ret;
}

static ssize_t gxfp3200_exchange_read(struct file *file, char __user *buf,
				      size_t count, loff_t *ppos)
{
	struct gxfp3200_device *gxfp = file->private_data;
	char *output;
	size_t output_len;
	unsigned int i;
	int ret;

	if (*ppos)
		return 0;

	mutex_lock(&gxfp->research_lock);
	if (!gxfp->research_response_len) {
		ret = -ENODATA;
		goto out_unlock;
	}

	output_len = gxfp->research_response_len * 3 + 1;
	output = kmalloc(output_len, GFP_KERNEL);
	if (!output) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	for (i = 0; i < gxfp->research_response_len; i++)
		scprintf(output + i * 3, output_len - i * 3, "%02x%s",
			 gxfp->research_response[i],
			 i == gxfp->research_response_len - 1 ? "\n" : " ");

	ret = simple_read_from_buffer(buf, count, ppos, output, output_len - 1);
	kfree(output);

out_unlock:
	mutex_unlock(&gxfp->research_lock);
	return ret;
}

static const struct file_operations gxfp3200_exchange_fops = {
	.owner = THIS_MODULE,
	.open = gxfp3200_debugfs_open,
	.write = gxfp3200_exchange_write,
	.read = gxfp3200_exchange_read,
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
	debugfs_create_file("exchange", 0600, gxfp->debugfs_dir, gxfp,
			    &gxfp3200_exchange_fops);
}

void gxfp3200_debugfs_remove(struct gxfp3200_device *gxfp)
{
	debugfs_remove_recursive(gxfp->debugfs_dir);
}
