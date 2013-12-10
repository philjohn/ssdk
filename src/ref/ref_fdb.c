/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#include "sw.h"
#include "ssdk_init.h"
#include "fal_init.h"
#include "fal_misc.h"
#include "fal_mib.h"
#include "fal_port_ctrl.h"
#include "fal_portvlan.h"
#include "fal_fdb.h"
#include "fal_stp.h"
#include "fal_igmp.h"
#include "fal_qos.h"
#include "hsl.h"
#include "hsl_dev.h"
#include "ssdk_init.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <asm/mach-types.h>
#include <linux/kconfig.h>
#include <generated/autoconf.h>
#include <net/switch.h>
#include <linux/delay.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/ar8216_platform.h>
#include "ssdk_plat.h"
#include "ref_vlan.h"


int
qca_ar8327_sw_atu_flush(struct qca_phy_priv *priv)
{
	/* 0: dynamic 1:dynamic, static */
	fal_fdb_del_all(0, 1);

	return 0;
}

int
qca_ar8327_sw_atu_dump(struct switch_dev *dev,
		       		const struct switch_attr *attr,
		       		struct switch_val *val)
{
	struct qca_phy_priv *priv = qca_phy_priv_get(dev);
	a_uint32_t rv;
	char *buf;
	a_uint32_t len = 0;
	int i;
	fal_fdb_op_t option;
	fal_fdb_entry_t entry;

	buf = (char*) priv->buf;
	memset(buf, 0, 2048);
	memset(&option, 0, sizeof(fal_fdb_op_t));
	memset(&entry, 0, sizeof(fal_fdb_entry_t));

	rv = fal_fdb_extend_first(0, &option, &entry);
	while (!rv)
    {
		len += snprintf(buf+len, 2048-len, "MAC: %02x:%02x:%02x:%02x:%02x:%02x PORTMAP: 0x%02x VID: 0x%x STATUS: 0x%x\n",
			entry.addr.uc[0],entry.addr.uc[1],entry.addr.uc[2],entry.addr.uc[3],entry.addr.uc[4],entry.addr.uc[5],
			entry.port.map,
			entry.fid,
			entry.static_en);

		if (2048-len < 74){
//			snprintf(buf+len, 2048-len, "Buffer not enough!\n");
			break;
		}

		rv = fal_fdb_extend_next(0, &option, &entry);
    }

	val->value.s = priv->buf;
	val->len = len;

	return 0;
}

