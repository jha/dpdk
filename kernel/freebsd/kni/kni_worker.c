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
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/sched.h>

#include "kni_worker.h"
#include "kni_net.h"
#include "kni_fifo.h"

#include <net/bpf.h>

void
kni_worker_rx(struct ifnet *ifp)
{

}

void
kni_worker_tx(struct ifnet *ifp)
{
    struct kni_net_softc *sc = (struct kni_net_softc *)ifp->if_softc;
    struct mbuf *m;

    KNI_NET_LOCK(sc);

    if ((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0) {
        KNI_NET_UNLOCK(sc);
        return;
    }

    while (!IFQ_DRV_IS_EMPTY(&ifp->if_snd)) {
        IFQ_DRV_DEQUEUE(&ifp->if_snd, m);
        if (m == NULL)
            break;
        BPF_MTAP(ifp, m);
        m_freem(m);
    }

    KNI_NET_UNLOCK(sc);
}

void
kni_worker_single(void *arg0)
{
    int z;

    printf("kni: kthread for all interfaces started\n");

    while (1) {
        kthread_suspend_check();
        /* XXX Iterate over each interface and call rx on it */
        if (tsleep(&z, PCATCH, "ktwait", 5) != EWOULDBLOCK)
            break;
    }

    printf("kni: kthread for all interfaces stopped\n");
    kthread_exit();
    __builtin_unreachable();
}

void
kni_worker_multiple(void *arg0)
{
    struct kni_net_softc *sc;
    int z;

    sc = (struct kni_net_softc *)arg0;
    printf("kni: kthread for interface %s started\n", sc->name);

    while (sc->fifo_td_keep_running) {
        kthread_suspend_check();
        kni_worker_rx(sc->ifp);
        if (tsleep(&z, PCATCH, "ktwait", 5) != EWOULDBLOCK)
            break;
    }

    printf("kni: kthread for interface %s stopped\n", sc->name);
    kthread_exit();
    __builtin_unreachable();
}
