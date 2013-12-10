/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifndef _ISISC_STP_H_
#define _ISISC_STP_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "fal/fal_stp.h"

    sw_error_t isisc_stp_init(a_uint32_t dev_id);

#ifdef IN_STP
#define ISISC_STP_INIT(rv, dev_id) \
    { \
        rv = isisc_stp_init(dev_id); \
        SW_RTN_ON_ERROR(rv); \
    }
#else
#define ISISC_STP_INIT(rv, dev_id)
#endif

#ifdef HSL_STANDALONG


    HSL_LOCAL sw_error_t
    isisc_stp_port_state_set(a_uint32_t dev_id, a_uint32_t st_id,
                            fal_port_t port_id, fal_stp_state_t state);


    HSL_LOCAL sw_error_t
    isisc_stp_port_state_get(a_uint32_t dev_id, a_uint32_t st_id,
                            fal_port_t port_id, fal_stp_state_t * state);

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _ISISC_STP_H_ */

