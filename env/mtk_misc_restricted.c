// SPDX-License-Identifier: GPL-2.0+
/*
 * (c) Copyright 2024 by BayLibre
 *
 * Author:
 *  Pavlo Yadvychuk <pyadvychuk@baylibre.com>
 *
 */

#include <common.h>
#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <asm/global_data.h>
#include <linux/stddef.h>
#include <u-boot/crc.h>
#include <search.h>
#include <errno.h>
#include <linux/compiler.h>	/* for BUG_ON */
#include <malloc.h>
#include <memalign.h>
#include <mmc.h>
#include <mtk_misc.h>

DECLARE_GLOBAL_DATA_PTR;

struct mtk_misc_data {
    ulong bootcount;
    u8 boot_part;
    u8 upgrade_available;
    u8 fwupdate_available;
    u8 reserved[5];
};

struct mtk_misc_env_var {
    char *name;
    unsigned int size;      //!< for future use
};

static const struct mtk_misc_env_var mtk_misc_env_vars[] = {
    { .name = "mender_boot_part", .size = 1 },
    { .name = "bootcount", .size = 1 },
    { .name = "upgrade_available", .size = 1 },
    { .name = "fwupdate_available", .size = 1 },
};

static void env_mtk_misc_var2env(struct mtk_misc_data *buf)
{
    const struct mtk_misc_env_var *var;
    int i;

    for (i = 0; i < ARRAY_SIZE(mtk_misc_env_vars); i++) {
        var = &mtk_misc_env_vars[i];

        if (!strncmp(var->name, "mender_boot_part", strlen(var->name)))
            env_set_ulong(var->name, buf->boot_part);
        else if (!strncmp(var->name, "upgrade_available", strlen(var->name)))
            env_set_ulong(var->name, buf->upgrade_available);
        else if (!strncmp(var->name, "fwupdate_available", strlen(var->name)))
            env_set_ulong(var->name, buf->fwupdate_available);
        else if (!strncmp(var->name, "bootcount", strlen(var->name)))
            env_set_ulong(var->name, buf->bootcount);
    }
    env_set("altbootcmd", "rollback_sysupgrade");
}

static void env_mtk_misc_env2var(struct mtk_misc_data *buf)
{
    ulong n_var;
    const struct mtk_misc_env_var *var;
    int i;

    for (i = 0; i < ARRAY_SIZE(mtk_misc_env_vars); i++) {
        var = &mtk_misc_env_vars[i];
        n_var = env_get_ulong(var->name, 10, 0);

        if (!strncmp(var->name, "mender_boot_part", strlen(var->name)))
            buf->boot_part = n_var;
        else if (!strncmp(var->name, "upgrade_available", strlen(var->name)))
            buf->upgrade_available = n_var;
        else if (!strncmp(var->name, "fwupdate_available", strlen(var->name)))
            buf->fwupdate_available = n_var;
        else if (!strncmp(var->name, "bootcount", strlen(var->name)))
            buf->bootcount = n_var;
    }
}

static int env_mtk_misc_erase(void)
{
    struct mtk_misc_data buf = {0};
    int ret = -1;

    buf.boot_part = MTK_MISC_BOOTPART_DEFLT;
    buf.bootcount = 0;
    buf.upgrade_available = 0;
    buf.fwupdate_available = 0;
    ret = mtk_misc_store(&buf, sizeof(struct mtk_misc_data));
    if (!ret)
        env_mtk_misc_var2env(&buf);
    else
        log_err("Erase request failed for mtk-misc location.\n");

    return ret;
}

static int env_mtk_misc_save(void)
{
    struct mtk_misc_data buf = {0};

    env_mtk_misc_env2var(&buf);

    return mtk_misc_store(&buf, sizeof(buf));
}

static int env_mtk_misc_load(void)
{
    struct mtk_misc_data buf = {0};
    int ret = 0;

    if (!(gd->flags & GD_FLG_ENV_READY)) {
        env_t ee = {0};
        env_import((const char*)&ee, 0, H_NOCLEAR);
    }

    ret = mtk_misc_load(&buf, sizeof(struct mtk_misc_data));
    if (!ret)
        env_mtk_misc_var2env(&buf);
    else {
        log_err("Load from mtk-misc failed, resetting to default...\n");
        ret = env_mtk_misc_erase();
    }

    return ret;
}

int mtk_misc_loadvars(void)
{
    return env_mtk_misc_load();
}

int mtk_misc_savevars(void)
{
    return env_mtk_misc_save();
}

U_BOOT_ENV_LOCATION(mtkmisc) = {
	.location	= ENVL_MTK_MISC,
	.load		= env_mtk_misc_load,
	.save		= ENV_SAVE_PTR(env_mtk_misc_save),
	.erase		= ENV_ERASE_PTR(env_mtk_misc_erase),
	.name       = "mtk-misc"
};
