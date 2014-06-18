/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SW_SHELL_H
#define _SW_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sw.h"
#include "sw_api.h"
#include "ssdk_init.h"

    extern a_uint32_t *ioctl_buf;
    extern ssdk_init_cfg init_cfg;

#define IOCTL_BUF_SIZE 2048
#define CMDSTR_BUF_SIZE 1024
#define CMDSTR_ARGS_MAX 128
#define dprintf cmd_print
    extern sw_error_t cmd_exec_api(a_uint32_t *arg_val);
    extern void cmd_print(char *fmt, ...);
    void cmd_print_error(sw_error_t rtn);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* _SW_SHELL_H */
