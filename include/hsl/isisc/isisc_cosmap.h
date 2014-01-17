/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


/**
 * @defgroup isisc_cosmap ISISC_COSMAP
 * @{
 */
#ifndef _ISISC_COSMAP_H_
#define _ISISC_COSMAP_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "fal/fal_cosmap.h"

    sw_error_t isisc_cosmap_init(a_uint32_t dev_id);

#ifdef IN_COSMAP
#define ISISC_COSMAP_INIT(rv, dev_id) \
    { \
        rv = isisc_cosmap_init(dev_id); \
        SW_RTN_ON_ERROR(rv); \
    }
#else
#define ISISC_COSMAP_INIT(rv, dev_id)
#endif

#ifdef HSL_STANDALONG

    HSL_LOCAL sw_error_t
    isisc_cosmap_dscp_to_pri_set(a_uint32_t dev_id, a_uint32_t dscp,
                                a_uint32_t pri);

    HSL_LOCAL sw_error_t
    isisc_cosmap_dscp_to_pri_get(a_uint32_t dev_id, a_uint32_t dscp,
                                a_uint32_t * pri);

    HSL_LOCAL sw_error_t
    isisc_cosmap_dscp_to_dp_set(a_uint32_t dev_id, a_uint32_t dscp,
                               a_uint32_t dp);

    HSL_LOCAL sw_error_t
    isisc_cosmap_dscp_to_dp_get(a_uint32_t dev_id, a_uint32_t dscp,
                               a_uint32_t * dp);

    HSL_LOCAL sw_error_t
    isisc_cosmap_up_to_pri_set(a_uint32_t dev_id, a_uint32_t up,
                              a_uint32_t pri);

    HSL_LOCAL sw_error_t
    isisc_cosmap_up_to_pri_get(a_uint32_t dev_id, a_uint32_t up,
                              a_uint32_t * pri);

    HSL_LOCAL sw_error_t
    isisc_cosmap_up_to_dp_set(a_uint32_t dev_id, a_uint32_t up,
                             a_uint32_t dp);

    HSL_LOCAL sw_error_t
    isisc_cosmap_up_to_dp_get(a_uint32_t dev_id, a_uint32_t up,
                             a_uint32_t * dp);

    HSL_LOCAL sw_error_t
    isisc_cosmap_pri_to_queue_set(a_uint32_t dev_id, a_uint32_t pri,
                                 a_uint32_t queue);

    HSL_LOCAL sw_error_t
    isisc_cosmap_pri_to_queue_get(a_uint32_t dev_id, a_uint32_t pri,
                                 a_uint32_t * queue);

    HSL_LOCAL sw_error_t
    isisc_cosmap_pri_to_ehqueue_set(a_uint32_t dev_id, a_uint32_t pri,
                                   a_uint32_t queue);

    HSL_LOCAL sw_error_t
    isisc_cosmap_pri_to_ehqueue_get(a_uint32_t dev_id, a_uint32_t pri,
                                   a_uint32_t * queue);

    HSL_LOCAL sw_error_t
    isisc_cosmap_egress_remark_set(a_uint32_t dev_id, a_uint32_t tbl_id,
                                  fal_egress_remark_table_t * tbl);

    HSL_LOCAL sw_error_t
    isisc_cosmap_egress_remark_get(a_uint32_t dev_id, a_uint32_t tbl_id,
                                  fal_egress_remark_table_t * tbl);

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _ISISC_COSMAP_H_ */

/**
 * @}
 */

