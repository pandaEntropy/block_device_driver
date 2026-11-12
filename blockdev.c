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

static int serve_request(struct request *rq, unsigned int *nr_bytes)
{
    int ret = 0;
    struct bio_vec bvec;
    struct req_iterator iter;
    struct block_dev *dev = rq->q->queuedata;
    /*
     * blk_rq_pos() returns the sector number, but the driver needs the byte address to write/read.
     * Hence we multiply by 2^9 (shift left by SECTOR_SHIFT which is 9) and get the byte address.
     * dev->capacity is also measured in sectors.
     */
    loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT;
    loff_t dev_size = (loff_t)(dev->capacity << SECTOR_SHIFT);

    printk(KERN_WARNING "sblkdev: request start from sector %lld pos = %lld dev_size = %lld\n", blk_rq_pos(rq), pos, dev_size);

    rq_for_each_segment(bvec, rq, iter)
    {
        unsigned long b_len = bvec.bv_len;

        // bvec.bv_page is a pointer to a struct page
        void *b_buf = page_address(bvec.bv_page) + bvec.bv_offset;

        if ((pos + b_len) > dev_size) {
            b_len = (unsigned long)(dev_size - pos);
        }

        if (rq_data_dir(rq) == WRITE) {
            memcpy(dev->data + pos, b_buf, b_len);
        } else {
            memcpy(b_buf, dev->data + pos, b_len);
        }

        pos += b_len;
        *nr_bytes += b_len;
    }

    return ret;
}

static blk_status_t queue_rq(struct blk_mq_hw_ctx *hctx, struct blk_mq_queue_data *qd)
{
    unsigned int nr_bytes = 0;
    // NOTE: shouldnt this be a pointer??
    blk_status_t status = BLK_STS_OK;
    struct request *rq = qd->rq;

    blk_mq_start_request(rq);

    if (serve_request(rq, &nr_bytes) != 0) {
        status = BLK_STS_IOERR;
    }

    // NOTE: Look into this. A lot of why's and how's
    // Notify he kernel about the processed nr_bytes
    if (blk_update_request(rq, status, nr_bytes)) {
        BUG();
    }

    __blk_mq_end_request(rq, status);

    return status;
}

static struct blk_mq_ops mq_ops = {
    .queue_rq = queue_rq;
};
