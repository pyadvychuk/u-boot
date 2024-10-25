// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <env.h>
#include <mtk_misc.h>

void bootcount_store(ulong a)
{
    int upgrade_available = env_get_ulong("upgrade_available", 10, 0);

    if (upgrade_available) {
        env_set_ulong("bootcount", a);
        (void)mtk_misc_savevars();
    }
}

ulong bootcount_load(void)
{
    int upgrade_available = env_get_ulong("upgrade_available", 10, 0);

    return (upgrade_available) ?
                env_get_ulong("bootcount", 10, 0) : 0;
}
