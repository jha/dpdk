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

MALLOC_DEFINE(M_RTE_KNI, "rte_kni", "rte_kni allocations");

static int
kni_modevent(module_t mod, int type, void *arg)
{
	int error = 0;

	switch (type) {
	case MOD_LOAD:
		error = 0;
		break;
	case MOD_UNLOAD:
		error = 0;
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
