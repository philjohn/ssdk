/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#ifndef _AOS_MEM_H
#define _AOS_MEM_H

#include "sal/os/aos_types.h"
#ifdef KERNEL_MODULE
#include "sal/os/linux/aos_mem_pvt.h"
#else
#include "sal/os/linux_user/aos_mem_pvt.h"
#endif

/**
 * @g aos_mem mem
 * @{
 *
 * @ig shim_ext
 */

/**
 * @brief Allocate a memory buffer. Note it's a non-blocking call.
 * This call can block.
 *
 * @param[in] size    buffer size
 *
 * @return Buffer pointer or NULL if there's not enough memory.
 */
static inline void *
aos_mem_alloc(aos_size_t size)
{
    return __aos_mem_alloc(size);
}

/**
 * @brief Free malloc'ed buffer
 *
 * @param[in] buf     buffer pointer allocated by aos_alloc()
 * @param[in] size    buffer size
 */
static inline void
aos_mem_free(void *buf)
{
    __aos_mem_free(buf);
}

/**
 * @brief Move a memory buffer
 *
 * @param[in] dst     destination address
 * @param[in] src     source address
 * @param[in] size    buffer size
 */
static inline void
aos_mem_copy(void *dst, void *src, aos_size_t size)
{
    __aos_mem_copy(dst, src, size);
}

/**
 * @brief Fill a memory buffer
 *
 * @param[in] buf   buffer to be filled
 * @param[in] b     byte to fill
 * @param[in] size  buffer size
 */
static inline void
aos_mem_set(void *buf, a_uint8_t b, aos_size_t size)
{
    __aos_mem_set(buf, b, size);
}

/**
 * @brief Zero a memory buffer
 *
 * @param[in] buf   buffer to be zeroed
 * @param[in] size  buffer size
 */
static inline void
aos_mem_zero(void *buf, aos_size_t size)
{
    __aos_mem_zero(buf, size);
}

/**
 * @brief Compare two memory buffers
 *
 * @param[in] buf1  first buffer
 * @param[in] buf2  second buffer
 * @param[in] size  buffer size
 *
 * @retval    0     equal
 * @retval    1     not equal
 */
static inline int
aos_mem_cmp(void *buf1, void *buf2, aos_size_t size)
{
    return __aos_mem_cmp(buf1, buf2, size);
}

/**
 * @}
 */

#endif
