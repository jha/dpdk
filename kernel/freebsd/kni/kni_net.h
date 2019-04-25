/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2019 Paras Jha
 */

#ifndef _KNI_NET_H_
#define _KNI_NET_H_

#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/kthread.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/if_media.h>

#include <rte_kni_common.h>

#define KNI_NET_MEDIATYPE (IFM_ETHER | IFM_10G_T | IFM_FDX)

TAILQ_HEAD(kni_net_softc_tailq, kni_net_softc);
struct kni_net_softc {
    struct mtx mtx;
    char name[RTE_KNI_NAMESIZE];
    struct ifmedia ifmedia;
    struct ifnet *ifp;
    struct thread *fifo_td;
    uintptr_t tx_q, rx_q;
    uintptr_t alloc_q, free_q;
    uintptr_t req_q, resp_q;
    void *sync_va;
    uintptr_t sync_kva;
    uint8_t hwaddr[6];
    unsigned int mbuf_size;
    unsigned int mtu;
    volatile int fifo_td_keep_running;
    TAILQ_ENTRY(kni_net_softc) tailq;
};

#define KNI_NET_LOCK_INIT(sc) mtx_init(&(sc)->mtx, (sc)->name,\
    NULL, MTX_DEF);
#define KNI_NET_LOCK_FREE(sc) mtx_destroy(&(sc)->mtx)

#define KNI_NET_LOCK(sc) mtx_lock(&(sc)->mtx)
#define KNI_NET_UNLOCK(sc) mtx_unlock(&(sc)->mtx)

#define KNI_NET_ASSERT_OWNED(sc) mtx_assert(&(sc)->mtx, MA_OWNED)
#define KNI_NET_ASSERT_NOTOWNED(sc) mtx_assert(&(sc)->mtx, MA_NOTOWNED)

int
kni_net_init(void);

int
kni_net_free(void);

int
kni_net_if_create(const struct rte_kni_device_info *dev_info);

int
kni_net_if_destroy(const struct rte_kni_device_info *dev_info);

#endif /* _KNI_NET_H_ */
