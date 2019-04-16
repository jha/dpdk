# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2019 Paras Jha

include $(RTE_SDK)/mk/rte.vars.mk

#
# module name and path
#
MODULE = rte_kni

#
# CFLAGS
#
MODULE_CFLAGS += -I$(SRCDIR)
MODULE_CFLAGS += -I$(RTE_OUTPUT)/include
MODULE_CFLAGS += -Winline -Wall -Werror
MODULE_CFLAGS += -include $(RTE_OUTPUT)/include/rte_config.h

#
# all source are stored in SRCS-y
#
SRCS-y := kni.c kni_cdev.c kni_cdev.h kni_net.c kni_net.h

include $(RTE_SDK)/mk/rte.bsdmodule.mk
