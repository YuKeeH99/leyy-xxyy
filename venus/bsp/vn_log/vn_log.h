#ifndef __VN_LOG_H__
#define __VN_LOG_H__

#include <stdarg.h>
#include <stdio.h>

#include "ql_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VN_LOG_LEVEL_DEBUG 4
#define VN_LOG_LEVEL_INFO  3
#define VN_LOG_LEVEL_WARN  2
#define VN_LOG_LEVEL_ERROR 1
#define VN_LOG_LEVEL_NONE  0

#ifndef VN_LOG_LEVEL
#define VN_LOG_LEVEL       VN_LOG_LEVEL_DEBUG
#endif

#define VN_LOG_HDR         "[VN] "
#define VN_LOG_TIME_MS()   ((unsigned int)(ql_rtos_get_systicks() * 5U))

void Vn_Log_Init(void);
void Vn_Log_Task(void *p);
void Vn_Log_Printf(const char *fmt, ...);

#define VN_LOG_RAW(fmt, ...) Vn_Log_Printf(fmt, ##__VA_ARGS__)

#if (VN_LOG_LEVEL >= VN_LOG_LEVEL_DEBUG)
#define VN_LOG_D(fmt, ...) \
    VN_LOG_RAW(VN_LOG_HDR "[D T:%u] %s:%d " fmt "\n", \
               VN_LOG_TIME_MS(), __func__, __LINE__, ##__VA_ARGS__)
#else
#define VN_LOG_D(fmt, ...) ((void)0)
#endif

#if (VN_LOG_LEVEL >= VN_LOG_LEVEL_INFO)
#define VN_LOG_I(fmt, ...) \
    VN_LOG_RAW(VN_LOG_HDR "[I T:%u] %s:%d " fmt "\n", \
               VN_LOG_TIME_MS(), __func__, __LINE__, ##__VA_ARGS__)
#else
#define VN_LOG_I(fmt, ...) ((void)0)
#endif

#if (VN_LOG_LEVEL >= VN_LOG_LEVEL_WARN)
#define VN_LOG_W(fmt, ...) \
    VN_LOG_RAW(VN_LOG_HDR "[W T:%u] %s:%d " fmt "\n", \
               VN_LOG_TIME_MS(), __func__, __LINE__, ##__VA_ARGS__)
#else
#define VN_LOG_W(fmt, ...) ((void)0)
#endif

#if (VN_LOG_LEVEL >= VN_LOG_LEVEL_ERROR)
#define VN_LOG_E(fmt, ...) \
    VN_LOG_RAW(VN_LOG_HDR "[E T:%u] %s:%d " fmt "\n", \
               VN_LOG_TIME_MS(), __func__, __LINE__, ##__VA_ARGS__)
#else
#define VN_LOG_E(fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VN_LOG_H__ */