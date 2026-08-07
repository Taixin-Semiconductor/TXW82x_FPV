/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <k_api.h>

void k_err_proc(kstat_t err)
{
#if 1
    soc_err_proc(err);
#else
    if (g_err_proc != NULL) {
        g_err_proc(err);
    }
#endif
}

