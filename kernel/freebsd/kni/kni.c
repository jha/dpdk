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
#include <sys/module.h>

#include "kni_cdev.h"

static int
kni_mod_load(void)
{
	int error;

	error = kni_cdev_init();
	if (error != 0)
		return error;

	return 0;
}

static int
kni_mod_unload(void)
{
	int error;

	error = kni_cdev_free();
	if (error != 0)
		return error;

	return 0;
}

static int
kni_modevent(module_t mod, int type, void *arg)
{
	int error = 0;

	switch (type) {
	case MOD_LOAD:
		error = kni_mod_load();
		break;
	case MOD_UNLOAD:
		error = kni_mod_unload();
		break;
	default:
		break;
	}

	return error;
}

moduledata_t kni_mod = {
	"rte_kni",
	(modeventhand_t)kni_modevent,
	0
};

DECLARE_MODULE(rte_kni, kni_mod, SI_SUB_DRIVERS, SI_ORDER_ANY);
MODULE_VERSION(rte_kni, 1);
