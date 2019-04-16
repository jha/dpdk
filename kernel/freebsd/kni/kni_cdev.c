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
#include <sys/types.h>
#include <sys/systm.h>

#include <rte_kni_common.h>

#include "kni_cdev.h"

MALLOC_DEFINE(M_RTE_KNI_CDEV, "rte_kni_cdev", "rte_kni_cdev allocations");

static struct kni_cdev *rte_kni_cdev = NULL;

static int
kni_cdev_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	if (!mtx_trylock(&rte_kni_cdev->mtx)) {
		printf("kni: only one instance is allowed access to KNI\n");
		return EBUSY;
	}

	printf("kni: dpdk application has been opened\n");
	return 0;
}

static int
kni_cdev_ioctl(struct cdev *dev,
	u_long cmd, caddr_t data, int fflag, struct thread *td)
{
	int ret = -EINVAL;
	struct rte_kni_device_info *dev_info;

	dev_info = (struct rte_kni_device_info *)data;

	/* This doesn't seem necessary */
	if (IOCPARM_LEN(cmd) > sizeof (struct rte_kni_device_info))
		return -EINVAL;

	/* Ensure that the name is null-terminated. Future checks might also
	 * include ensuring that the name is valid. However, the Linux version
	 * doesn't have any such checks and so as of right ignored */
	if (strnlen(dev_info->name, RTE_KNI_NAMESIZE) >= RTE_KNI_NAMESIZE)
		return -EINVAL;

	switch (cmd) {
	case RTE_KNI_IOCTL_CREATE:
		printf("kni: RTE_KNI_IOCTL_CREATE\n");
		ret = -ENOMEM;
		break;
	case RTE_KNI_IOCTL_RELEASE:
		printf("kni: RTE_KNI_IOCTL_RELEASE\n");
		ret = -ENOMEM;
		break;
	default:
		printf("kni: default ioctl handler\n");
		break;
	}

	return ret;
}

static int
kni_cdev_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	if (rte_kni_cdev == NULL)
		return EBUSY;

	mtx_unlock(&rte_kni_cdev->mtx);
	printf("kni: dpdk application has been closed\n");
	return 0;
}

static struct cdevsw kni_ops = {
	.d_name			= KNI_DEVICE,
	.d_version		= D_VERSION,
	.d_flags		= D_TRACKCLOSE,
	.d_open			= kni_cdev_open,
	.d_ioctl		= kni_cdev_ioctl,
	.d_close		= kni_cdev_close,
};

int
kni_cdev_init(void)
{
	/* Try to allocate zeroed memory for the global KNI object */
	rte_kni_cdev = malloc(sizeof (struct kni_cdev), M_RTE_KNI_CDEV,
		M_WAITOK | M_ZERO);
	if (rte_kni_cdev == NULL)
		return ENOMEM;

	/* We must initialize the mutex before we create the cdev, otherwise we
	 * could run into race condition issues */
	mtx_init(&rte_kni_cdev->mtx, "rte_kni_cdev", NULL, MTX_DEF);

	/* Try to create the character device driver /dev/kni. On failure, clean
	 * up the rte_kni_cdev object and free its mutex */
	rte_kni_cdev->cdev = make_dev(&kni_ops, 0,
		UID_ROOT, GID_WHEEL, 0600, KNI_DEVICE);
	if (rte_kni_cdev->cdev == NULL) {
		mtx_destroy(&rte_kni_cdev->mtx);
		free(rte_kni_cdev, M_RTE_KNI_CDEV);
		rte_kni_cdev = NULL;
		return ENOMEM;
	}

	printf("kni: cdev created\n");
	return 0;
}

int
kni_cdev_free(void)
{
	if (rte_kni_cdev == NULL)
		return 0;

	if (!mtx_trylock(&rte_kni_cdev->mtx)) {
		printf("kni: unable to unload driver because a KNI app is open\n");
		return EBUSY;
	}

	if (rte_kni_cdev->cdev != NULL)
		destroy_dev(rte_kni_cdev->cdev);

	mtx_destroy(&rte_kni_cdev->mtx);
	free(rte_kni_cdev, M_RTE_KNI_CDEV);

	printf("kni: cdev destroyed\n");
	return 0;
}
