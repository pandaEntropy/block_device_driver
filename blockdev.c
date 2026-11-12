#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/blk-mq.h>
#include <linux/hdreg.h>

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512
#endif

MODULE_LICENSE("GPL")

static int dev_major = 0;

struct block_dev {
    sector_t capacity;
    u8 *data; //buffer to emulate real storage
    struct blk_mq_tag_set tag_set;
    struct request_queue *queue;
    struct gendisk *gdisk;
};

static struct block_dev *block_device = NULL;

static int blockdev_open(struct block_device *dev, fmode_t mode)
{
    printk(">>> blockdev_open\n");
    return 0;
}

static void blockdev_release(struct gendisk *gdisk, fmode_t mode)
{
    printk(">>> blockdev released\n");
    return 0;
}

static int blockdev_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
    printk("ioctl cmd 0x%08x\n", cmd);
    return -ENOTTY;
}

//Set block device file io
static struct block_device_operations blockdev_ops = {
    .owner = THIS_MODULE,
    .open = blockdev_open,
    .release = blockdev_release,
    .ioctl = blockdev_ioctl
};
