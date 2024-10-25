// SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
/*
 * Copyright (C) 2024 Baylibre
 * Author: Pavlo Yadvychuk <pyadvychuk@baylibre.com>
 */

#include <blk.h>
#include <scsi.h>
#include <log.h>
#include <part.h>
#include <malloc.h>
#include <common.h>
#include <memalign.h>
#include <u-boot/crc.h>
#include <linux/err.h>
#include <mtk_misc.h>

#define MTK_MISC_MAGIC          0x455F595D
#define MTK_MISC_VERSION        1
#define MTK_MISC_DEV            "mmc"
#define MTK_MISC_PART           "0#misc"
#define MTK_MISC_OFFSET         2

struct mtk_misc_blob {
    u32 checksum;
    u32 magic;
    u8 version;
    u8 size;
    u8 reserved[6];
    u8 data[];
};

#define MTK_MISC_BLOB_HEADER

static uint32_t mtk_misc_crc32(struct mtk_misc_blob *pt)
{
    return crc32(0, (u8*)pt+sizeof(u32),
                (sizeof(struct mtk_misc_blob) - sizeof(u32) + pt->size));
}

static int mtk_misc_blkdev_info(struct blk_desc **dev_desc,
                struct disk_partition *part_info)
{
    int ret = -1;

    if (CONFIG_IS_ENABLED(UFS_MEDIATEK)) {
        if (scsi_scan(false)) {
            log_err("Request failed.\n");
            return ret;
        }
    }

    ret = part_get_info_by_dev_and_name_or_num(MTK_MISC_DEV, MTK_MISC_PART,
                    dev_desc, part_info, false);
    if (ret < 0)
        log_err("Invalid 'mtk-misc' partition.\n");

    return ret;
}

static void mtk_misc_initialize(struct mtk_misc_blob *obj)
{
    memset(obj, 0, sizeof(struct mtk_misc_blob));
    obj->magic = MTK_MISC_MAGIC;
    obj->version = MTK_MISC_VERSION;
}

static int mtk_misc_store_data(struct blk_desc *dev_desc,
                struct disk_partition *part_info, void *data, size_t size)
{
    struct mtk_misc_blob *buf = NULL;
    struct mtk_misc_blob *buf_rb = NULL;
    u32 blocks;
    int ret = -1;

    do {
        blocks = DIV_ROUND_UP(sizeof(struct mtk_misc_blob) + size, part_info->blksz);
        buf = malloc_cache_aligned(blocks * part_info->blksz);
        if (!buf) {
            log_err("Memory allocation error.\n");
            break;
        }

        mtk_misc_initialize(buf);
        buf->size = size;
        memcpy(buf->data, data, size);
        buf->checksum = mtk_misc_crc32(buf);

        ret = blk_dwrite(dev_desc, (part_info->start + (part_info->blksz * MTK_MISC_OFFSET)),
                    blocks, buf);
        if (ret < 0) {
            log_err("Could not write 'mtk-misc' block.\n");
            break;
        }

        buf_rb = malloc_cache_aligned(blocks * part_info->blksz);
        if (!buf_rb) {
            log_err("Memory allocation error.\n");
            break;
        }

        ret = blk_dread(dev_desc, (part_info->start + (part_info->blksz * MTK_MISC_OFFSET)),
                    blocks, buf_rb);
        if (ret < 0 || memcmp(buf, buf_rb, sizeof(struct mtk_misc_blob)+size)) {
            log_err("Error while read back 'mtk-misc' block.\n");
            ret = -1;
            break;
        }
        ret = 0;
    } while (0);

    if (buf)
        free(buf);
    if (buf_rb)
        free(buf_rb);

    return ret;
}

static void mtk_misc_store_default(struct blk_desc *dev_desc,
    struct disk_partition *part_info)
{
    struct mtk_misc_blob buf;

    mtk_misc_initialize(&buf);
    mtk_misc_store_data(dev_desc, part_info, &buf, sizeof(buf));
}

static int mtk_misc_load_data(struct blk_desc *dev_desc,
                struct disk_partition *part_info, void *data, size_t size)
{
    struct mtk_misc_blob *buf = NULL;
    u32 crc32_buf, blocks;
    int ret = -1;

    do {
        blocks = DIV_ROUND_UP(sizeof(struct mtk_misc_blob)+size, part_info->blksz);
        buf = malloc_cache_aligned(blocks * part_info->blksz);
        if (!buf) {
            log_err("Memory allocation error.\n");
            break;
        }

        ret = blk_dread(dev_desc, (part_info->start + (part_info->blksz * MTK_MISC_OFFSET)),
                    blocks, buf);
        if (ret < 0) {
            log_err("Could not load data block.\n");
            break;
        }

        crc32_buf = crc32(0, (u8*)buf+sizeof(u32),
                        (sizeof(struct mtk_misc_blob) - sizeof(u32) + size));
        if (buf->magic != MTK_MISC_MAGIC || buf->checksum != crc32_buf ||
           buf->size != size) {
            log_err("Invalid crc for mtk-misc data.\n");
            ret = -1;
            break;
        }

        memcpy(data, buf->data, size);
        ret = 0;
    } while (0);

    if (buf)
        free(buf);

    return ret;
}

int mtk_misc_load(void *data, size_t size)
{
    struct blk_desc *dev_desc;
    struct disk_partition part_info;
    int ret = -1;

    do {
        ret = mtk_misc_blkdev_info(&dev_desc, &part_info);
        if (ret < 0) {
            log_err("Invalid 'mtk-misc' partition.\n");
            break;
        }

        ret = mtk_misc_load_data(dev_desc, &part_info, data, size);
        if (ret)
            log_err("Error while loading 'mtk-misc' data.\n");
    } while (0);

    return ret;
}

int mtk_misc_store(void *data, size_t size)
{
    struct blk_desc *dev_desc;
    struct disk_partition part_info;
    int ret = -1;

    do {
        ret = mtk_misc_blkdev_info(&dev_desc, &part_info);
        if (ret < 0) {
            log_err("Invalid 'mtk-misc' partition.\n");
            break;
        }

        ret = mtk_misc_store_data(dev_desc, &part_info, data, size);
        if (ret)
            log_err("Error while storing 'mtk-misc' data.\n");

    } while (0);

    return ret;
}
