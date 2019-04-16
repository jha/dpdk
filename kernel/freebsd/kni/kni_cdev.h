/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Paras Jha
 */

#ifndef _KNI_CDEV_H_
#define _KNI_CDEV_H_

#include <sys/conf.h>
#include <sys/mutex.h>

struct kni_cdev {
    struct mtx mtx;
    struct cdev *cdev;
};

int
kni_cdev_init(void);

int
kni_cdev_free(void);

#endif /* _KNI_CDEV_H_ */
