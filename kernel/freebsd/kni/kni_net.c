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
#include <sys/sockio.h>
#include <sys/proc.h>
#include <sys/sched.h>

#include <net/ethernet.h>
#include <net/bpf.h>

MALLOC_DEFINE(M_RTE_KNI_NET, "rte_kni_net", "rte_kni_net allocations");

#include "kni_net.h"
#include "kni_worker.h"

struct kni_net_softc_tailq kn_softc_tq_head;

int
kni_net_init(void)
{
    TAILQ_INIT(&kn_softc_tq_head);
    return 0;
}

static struct kni_net_softc *
kni_net_softc_by_name(const char *name)
{
    struct kni_net_softc *p;

    TAILQ_FOREACH(p, &kn_softc_tq_head, tailq) {
        if (strncmp(p->name, name, RTE_KNI_NAMESIZE) == 0) {
            return p;
        }
    }

    return NULL;
}

int
kni_net_free(void)
{
    int error;
    struct kni_net_softc *sc;
    struct rte_kni_device_info dev_info_tmp;

    /* This loop can cause O(n^2) because we're linearly searching each time
     * we call kni_net_if_destroy(). One solution would be to make an internal
     * destroy function that takes just a kni_net_softc and gets called by both
     * this and kni_net_if_destroy() */
    while (!TAILQ_EMPTY(&kn_softc_tq_head)) {
        sc = TAILQ_FIRST(&kn_softc_tq_head);
        strcpy(dev_info_tmp.name, sc->name);
        error = kni_net_if_destroy(&dev_info_tmp);
        if (error != 0)
            return error;
    }
    return 0;
}

static int
kni_net_if_mtu(struct ifnet *ifp, struct kni_net_softc *sc, struct ifreq *ifr)
{
    KNI_NET_LOCK(sc);

    if (ifr->ifr_mtu < 72 || ifr->ifr_mtu > 1500) {
        KNI_NET_UNLOCK(sc);
        return ENOTSUP;
    }
    else
        ifp->if_mtu = ifr->ifr_mtu;

    KNI_NET_UNLOCK(sc);
    return 0;
}

/*
 * if_init
 * Initialize and bring up the hardware, e.g., reset the chip and
 * enable the receiver unit. Should mark the interface running, but
 * not active (IFF_DRV_RUNNING, ~IFF_DRV_OACTIVE).
 */
static void
kni_net_if_init(void *if_softc_)
{
    struct kni_net_softc *sc = (struct kni_net_softc *)if_softc_;
    struct ifnet *ifp = sc->ifp;

    KNI_NET_LOCK(sc);
    if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
        KNI_NET_UNLOCK(sc);
        return;
    }

    ifp->if_drv_flags |= IFF_DRV_RUNNING;
    ifp->if_drv_flags &= ~(IFF_DRV_OACTIVE);

    if_link_state_change(ifp, LINK_STATE_UP);
    KNI_NET_UNLOCK(sc);

    printf("kni: interface %s link up\n", sc->name);
}

static void
kni_net_if_stop(void *if_softc_)
{
    struct kni_net_softc *sc = (struct kni_net_softc *)if_softc_;
    struct ifnet *ifp = sc->ifp;

    KNI_NET_LOCK(sc);

    ifp->if_drv_flags &= ~IFF_DRV_RUNNING;
    ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;

    if_link_state_change(ifp, LINK_STATE_DOWN);
    KNI_NET_UNLOCK(sc);

    printf("kni: interface %s link down\n", sc->name);
}

static int
kni_net_if_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
    struct kni_net_softc *sc = (struct kni_net_softc *)ifp->if_softc;
    struct ifreq *ifr = (struct ifreq *)data;
    int err = 0;

    switch (cmd) {
        case SIOCSIFMTU:
            err = kni_net_if_mtu(ifp, sc, ifr);
            break;
        case SIOCSIFFLAGS:
            if (ifp->if_flags & IFF_UP) {
                if ((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0)
                    kni_net_if_init(sc);
            } else {
                if (ifp->if_drv_flags & IFF_DRV_RUNNING)
                    kni_net_if_stop(sc);
            }
            break;
        case SIOCSIFCAP:
            break;
        case SIOCSIFMEDIA:
        case SIOCGIFMEDIA:
            err = ifmedia_ioctl(ifp, ifr, &sc->ifmedia, cmd);
            break;
        default:
            err = ether_ioctl(ifp, cmd, data);
            break;
    }

    return err;
}

static int
kni_net_ifmedia_change(struct ifnet *ifp __unused)
{
    return 0;
}

static void
kni_net_ifmedia_stat(struct ifnet *ifp __unused, struct ifmediareq *req)
{
    req->ifm_status = IFM_AVALID | IFM_ACTIVE;
    req->ifm_active = KNI_NET_MEDIATYPE;
}

int
kni_net_if_create(const struct rte_kni_device_info *dev_info)
{
    struct kni_net_softc *sc;
    struct ifnet *ifp;

    /* Validate that the interface doesn't already exist */
    if (kni_net_softc_by_name(dev_info->name) != NULL) {
        printf("kni: interface %s already exists\n", dev_info->name);
        return EINVAL;
    }

    /* Create new private data for this interface */
    sc = malloc(sizeof (struct kni_net_softc),
        M_RTE_KNI_NET, M_WAITOK | M_ZERO);
    if (sc == NULL)
        return ENOMEM;
    strncpy(sc->name, dev_info->name, RTE_KNI_NAMESIZE);
    KNI_NET_LOCK_INIT(sc);

    /* Create a new interface */
    ifp = if_alloc(IFT_ETHER);
    if (ifp == NULL) {
        printf("kni: unable to allocate interface %s\n", dev_info->name);
        free(sc, M_RTE_KNI_NET);
        return ENOMEM;
    }
    ifp->if_softc = sc;
    sc->ifp = ifp;

    /* Set up the interface on the system */
    if_initname(ifp, dev_info->name, IF_DUNIT_NONE);
    ifp->if_baudrate = IF_Gbps(10);
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_init = kni_net_if_init;
    ifp->if_ioctl = kni_net_if_ioctl;
    ifp->if_start = kni_worker_tx;
    IFQ_SET_MAXLEN(&ifp->if_snd, ifqmaxlen);
    ifp->if_snd.ifq_drv_maxlen = ifqmaxlen;
    IFQ_SET_READY(&ifp->if_snd);

    /* Map the physical addresses of the fifo queue into kernel space */
    sc->tx_q = dev_info->tx_phys;
    sc->rx_q = dev_info->rx_phys;
    sc->alloc_q = dev_info->alloc_phys;
    sc->free_q = dev_info->free_phys;
    sc->req_q = dev_info->req_phys;
    sc->resp_q = dev_info->resp_phys;
    sc->sync_va = dev_info->sync_va;
    sc->sync_kva = dev_info->sync_phys;

    sc->mbuf_size = dev_info->mbuf_size;
    if (dev_info->mtu) {
        struct ifreq ifr;

        ifr.ifr_mtu = dev_info->mtu;
        if (kni_net_if_mtu(ifp, sc, &ifr) == 0) {
            sc->mtu = dev_info->mtu;
        }
    }

    /* Set the media type to Ethernet/duplex/speed */
    ifmedia_init(&sc->ifmedia, IFM_IMASK,
        kni_net_ifmedia_change, kni_net_ifmedia_stat);
    ifmedia_add(&sc->ifmedia, KNI_NET_MEDIATYPE, 0, NULL);
    ifmedia_set(&sc->ifmedia, KNI_NET_MEDIATYPE);

    /* Create a kernel thread to deal with DPDK FIFI */
    sc->fifo_td_keep_running = 1;
    kthread_add(&kni_worker_multiple, sc, NULL, &sc->fifo_td, 0, 0,
        "kni_%s", sc->name);

    /* Generate a random hardware address and attach our Ethernet interface */
    memcpy(sc->hwaddr, dev_info->mac_addr, 6);
    sc->hwaddr[0] = 0x3A;
    sc->hwaddr[1] = 0xCC;
    sc->hwaddr[2] = 0x45;
    sc->hwaddr[3] = 0x0F;
    sc->hwaddr[4] = 0x28;
    sc->hwaddr[5] = 0xF0;
    ether_ifattach(ifp, sc->hwaddr);

    TAILQ_INSERT_TAIL(&kn_softc_tq_head, sc, tailq);

    printf("kni: created new interface %s\n", dev_info->name);
    return 0;
}

int
kni_net_if_destroy(const struct rte_kni_device_info *dev_info)
{
    struct kni_net_softc *sc;

    sc = kni_net_softc_by_name(dev_info->name);
    if (sc == NULL) {
        printf("kni: cannot delete nonexistent interface %s\n", dev_info->name);
        return 0;
    }

    ether_ifdetach(sc->ifp);
    sc->fifo_td_keep_running = 0;
    sched_relinquish(sc->fifo_td);
    /* XXX Needs to be guarded by a "join" or something similar to that to
     *     ensure that the thread is terminated before we free the underlying
     *     softc, otherwise we have a race condition on our hands */
    ifmedia_removeall(&sc->ifmedia);
    if_free(sc->ifp);

    TAILQ_REMOVE(&kn_softc_tq_head, sc, tailq);
    printf("kni: interface %s was removed\n", dev_info->name);
    KNI_NET_LOCK_FREE(sc);
    free(sc, M_RTE_KNI_NET);
    return 0;
}
