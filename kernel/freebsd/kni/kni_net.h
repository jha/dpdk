/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Paras Jha
 */

#ifndef _KNI_NET_H_
#define _KNI_NET_H_

#include <rte_kni_common.h>

int
kni_net_init(void);

int
kni_net_free(void);

int
kni_net_if_create(const struct rte_kni_device_info *dev_info);

int
kni_net_if_destroy(const struct rte_kni_device_info *dev_info);

#endif /* _KNI_NET_H_ */
