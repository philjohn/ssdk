/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifndef _ISISC_MISC_H_
#define _ISISC_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "fal/fal_misc.h"

    sw_error_t isisc_misc_init(a_uint32_t dev_id);

#ifdef IN_MISC
#define ISISC_MISC_INIT(rv, dev_id) \
    { \
        rv = isisc_misc_init(dev_id); \
        SW_RTN_ON_ERROR(rv); \
    }
#else
#define ISISC_MISC_INIT(rv, dev_id)
#endif

#ifdef HSL_STANDALONG


    HSL_LOCAL sw_error_t
    isisc_frame_max_size_set(a_uint32_t dev_id, a_uint32_t size);


    HSL_LOCAL sw_error_t
    isisc_frame_max_size_get(a_uint32_t dev_id, a_uint32_t * size);


    HSL_LOCAL sw_error_t
    isisc_port_unk_uc_filter_set(a_uint32_t dev_id, fal_port_t port_id,
                                a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_port_unk_uc_filter_get(a_uint32_t dev_id, fal_port_t port_id,
                                a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_port_unk_mc_filter_set(a_uint32_t dev_id, fal_port_t port_id,
                                a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_port_unk_mc_filter_get(a_uint32_t dev_id, fal_port_t port_id,
                                a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_port_bc_filter_set(a_uint32_t dev_id, fal_port_t port_id,
                            a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_port_bc_filter_get(a_uint32_t dev_id, fal_port_t port_id,
                            a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_cpu_port_status_set(a_uint32_t dev_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_cpu_port_status_get(a_uint32_t dev_id, a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_pppoe_cmd_set(a_uint32_t dev_id, fal_fwd_cmd_t cmd);


    HSL_LOCAL sw_error_t
    isisc_pppoe_cmd_get(a_uint32_t dev_id, fal_fwd_cmd_t * cmd);


    HSL_LOCAL sw_error_t
    isisc_pppoe_status_set(a_uint32_t dev_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_pppoe_status_get(a_uint32_t dev_id, a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_port_dhcp_set(a_uint32_t dev_id, fal_port_t port_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_port_dhcp_get(a_uint32_t dev_id, fal_port_t port_id, a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_arp_cmd_set(a_uint32_t dev_id, fal_fwd_cmd_t cmd);


    HSL_LOCAL sw_error_t
    isisc_arp_cmd_get(a_uint32_t dev_id, fal_fwd_cmd_t * cmd);


    HSL_LOCAL sw_error_t
    isisc_eapol_cmd_set(a_uint32_t dev_id, fal_fwd_cmd_t cmd);


    HSL_LOCAL sw_error_t
    isisc_eapol_cmd_get(a_uint32_t dev_id, fal_fwd_cmd_t * cmd);


    HSL_LOCAL sw_error_t
    isisc_pppoe_session_table_add(a_uint32_t dev_id, fal_pppoe_session_t * session_tbl);


    HSL_LOCAL sw_error_t
    isisc_pppoe_session_table_del(a_uint32_t dev_id, fal_pppoe_session_t * session_tbl);


    HSL_LOCAL sw_error_t
    isisc_pppoe_session_table_get(a_uint32_t dev_id, fal_pppoe_session_t * session_tbl);


    HSL_LOCAL sw_error_t
    isisc_pppoe_session_id_set(a_uint32_t dev_id, a_uint32_t index,
                              a_uint32_t id);


    HSL_LOCAL sw_error_t
    isisc_pppoe_session_id_get(a_uint32_t dev_id, a_uint32_t index,
                              a_uint32_t * id);


    HSL_LOCAL sw_error_t
    isisc_eapol_status_set(a_uint32_t dev_id, a_uint32_t port_id, a_bool_t enable);

    HSL_LOCAL sw_error_t
    isisc_eapol_status_get(a_uint32_t dev_id, a_uint32_t port_id, a_bool_t * enable);

    HSL_LOCAL sw_error_t
    isisc_ripv1_status_set(a_uint32_t dev_id, a_bool_t enable);

    HSL_LOCAL sw_error_t
    isisc_ripv1_status_get(a_uint32_t dev_id, a_bool_t * enable);


    HSL_LOCAL sw_error_t
    isisc_port_arp_req_status_set(a_uint32_t dev_id, fal_port_t port_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_port_arp_req_status_get(a_uint32_t dev_id, fal_port_t port_id, a_bool_t *enable);


    HSL_LOCAL sw_error_t
    isisc_port_arp_ack_status_set(a_uint32_t dev_id, fal_port_t port_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_port_arp_ack_status_get(a_uint32_t dev_id, fal_port_t port_id, a_bool_t *enable);


    HSL_LOCAL sw_error_t
    isisc_intr_mask_set(a_uint32_t dev_id, a_uint32_t intr_mask);


    HSL_LOCAL sw_error_t
    isisc_intr_mask_get(a_uint32_t dev_id, a_uint32_t * intr_mask);


    HSL_LOCAL sw_error_t
    isisc_intr_status_get(a_uint32_t dev_id, a_uint32_t * intr_status);


    HSL_LOCAL sw_error_t
    isisc_intr_status_clear(a_uint32_t dev_id, a_uint32_t intr_status);


    HSL_LOCAL sw_error_t
    isisc_intr_port_link_mask_set(a_uint32_t dev_id, a_uint32_t port_id, a_uint32_t intr_mask_flag);


    HSL_LOCAL sw_error_t
    isisc_intr_port_link_mask_get(a_uint32_t dev_id, a_uint32_t port_id, a_uint32_t * intr_mask_flag);


    HSL_LOCAL sw_error_t
    isisc_intr_port_link_status_get(a_uint32_t dev_id, a_uint32_t port_id, a_uint32_t * intr_mask_flag);

    HSL_LOCAL sw_error_t
    isisc_intr_mask_mac_linkchg_set(a_uint32_t dev_id, a_uint32_t port_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_intr_mask_mac_linkchg_get(a_uint32_t dev_id, a_uint32_t port_id, a_bool_t * enable);

    HSL_LOCAL sw_error_t
    isisc_intr_status_mac_linkchg_get(a_uint32_t dev_id, fal_pbmp_t *port_bitmap);

    HSL_LOCAL sw_error_t
    isisc_cpu_vid_en_set(a_uint32_t dev_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_cpu_vid_en_get(a_uint32_t dev_id, a_bool_t * enable);

    HSL_LOCAL sw_error_t
    isisc_rtd_pppoe_en_set(a_uint32_t dev_id, a_bool_t enable);


    HSL_LOCAL sw_error_t
    isisc_rtd_pppoe_en_get(a_uint32_t dev_id, a_bool_t * enable);

    HSL_LOCAL sw_error_t
    isisc_intr_status_mac_linkchg_clear(a_uint32_t dev_id);

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif

