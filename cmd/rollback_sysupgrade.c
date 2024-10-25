// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Baylibre
 * Author: Pavlo Yadvychuk <pyadvychuk@baylibre.com>
 */

#include <common.h>
#include <env.h>
#include <command.h>
#include <mtk_misc.h>

static int do_rollback_sysupgrade(struct cmd_tbl *cmdtp, int flag,
                int argc, char *const argv[])
{
    ulong bp, nbp, upd;
    char cmd[128];

    upd = env_get_ulong("upgrade_available", 10, 0);
    if (!upd)
        return CMD_RET_SUCCESS;

    if (!env_get("altboot_post_actions"))
        return CMD_RET_USAGE;

    bp = env_get_ulong("mender_boot_part", 10, MTK_MISC_BOOTPART_DEFLT);
    nbp = (MTK_MISC_BOOTPART_DEFLT == bp) ?
                MTK_MISC_BOOTPART_ALT_DEFLT : MTK_MISC_BOOTPART_DEFLT;

	sprintf(cmd, "setenv mender_boot_part  %lu", nbp);
	if (run_command_list(cmd, -1, 0) != CMD_RET_SUCCESS)
		return -1;

	sprintf(cmd, "setenv upgrade_available %d", 0);
	if (run_command_list(cmd, -1, 0) != CMD_RET_SUCCESS)
		return -1;

    (void)mtk_misc_savevars();

	sprintf(cmd, "run altboot_post_actions");
	if (run_command_list(cmd, -1, 0) != CMD_RET_SUCCESS)
		return -1;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	rollback_sysupgrade,	1,		1,	do_rollback_sysupgrade,
	"Roll back unsuccessful A/B update by switching back to"
    " the previous active boot partition. After switching partitions"
    " indexes mandatory subroutine \"altboot_post_actions\" will"
    " be executed",
	""
);
