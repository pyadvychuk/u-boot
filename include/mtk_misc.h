/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Copyright (C) 2024 Baylibre
 * Author: Pavlo Yadvychuk <pyadvychuk@baylibre.com>
 */

#ifndef __MEDIATEK_MISC_BLOB_H
#define __MEDIATEK_MISC_BLOB_H

/**
 *  \brief Load data blob from 'mtk-misc' partition
 *  @param data            pointer to externally allocated buffer
 *  @param size            data buffer size
 *  @return 0 if OK, any other value means error
 */
int mtk_misc_load(void *data, size_t size);

/**
 *  \brief Store data blob to the 'mtk-misc' partition
 *  @param data            pointer to externally allocated buffer
 *  @param size            data buffer size
 *  @return 0 if OK, any other value means error
 */
int mtk_misc_store(void *data, size_t size);

#endif /* __MEDIATEK_MISC_BLOB_H */
