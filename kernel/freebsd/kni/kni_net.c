/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Paras Jha
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/bio.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>

#include "kni_net.h"

int
kni_net_init(void)
{
    return 0;
}

int
kni_net_free(void)
{
    return 0;
}

int
kni_net_if_create(const struct rte_kni_device_info *dev_info)
{
    return ENOMEM;
}

int
kni_net_if_destroy(const struct rte_kni_device_info *dev_info)
{
    return 0;
}
