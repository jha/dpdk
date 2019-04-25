/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Paras Jha
 */

#ifndef _KNI_WORKER_H_
#define _KNI_WORKER_H_

#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/if_media.h>

void
kni_worker_rx(struct ifnet *ifp);

void
kni_worker_tx(struct ifnet *ifp);

void
kni_worker_single(void *arg0);

void
kni_worker_multiple(void *arg0);

#endif /* _KNI_WORKER_H_ */
