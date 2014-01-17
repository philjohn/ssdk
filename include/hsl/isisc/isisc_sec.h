/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifndef _ISISC_SEC_H_
#define _ISISC_SEC_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "fal/fal_sec.h"

    sw_error_t isisc_sec_init(a_uint32_t dev_id);

#ifdef IN_SEC
#define ISISC_SEC_INIT(rv, dev_id) \
    { \
        rv = isisc_sec_init(dev_id); \
        SW_RTN_ON_ERROR(rv); \
    }
#else
#define ISISC_SEC_INIT(rv, dev_id)
#endif

#ifdef HSL_STANDALONG

    HSL_LOCAL sw_error_t
    isisc_sec_norm_item_set(a_uint32_t dev_id, fal_norm_item_t item,
                           void *value);

    HSL_LOCAL sw_error_t
    isisc_sec_norm_item_get(a_uint32_t dev_id, fal_norm_item_t item,
                           void *value);

#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _ISISC_SEC_H_ */

