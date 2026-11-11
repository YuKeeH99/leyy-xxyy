#include "vn_log.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ql_log.h"
#include "vn_queue.h"

#define VN_LOG_BUFFER_SIZE     1024U
#define VN_LOG_QUEUE_DEPTH     20
#define VN_LOG_TASK_STACK_SIZE 10240U
#define VN_LOG_TASK_PRIORITY   123U
#define VN_LOG_WAIT_TIMEOUT_MS 100

static Vn_Queue_Typedef Vn_LogQueue;
static ql_task_t Vn_LogTaskRef;
static bool Vn_LogReady;

void Vn_Log_Task(void *p)
{
    uint8_t log_buff[VN_LOG_BUFFER_SIZE];

    (void)p;

    for (;;)
    {
        int32_t length = Vn_Queue_Wait(Vn_LogQueue,
                                       log_buff,
                                       (int)sizeof(log_buff),
                                       VN_LOG_WAIT_TIMEOUT_MS);

        if (length > 0)
        {
            printf("%.*s", (int)length, (const char *)log_buff);
            _ql_app_log("%.*s", (int)length, (const char *)log_buff);
        }

        ql_rtos_task_sleep_ms(1U);
    }
}

void Vn_Log_Init(void)
{
    QlOSStatus status;

    if (Vn_LogReady)
    {
        return;
    }

    ql_log_mask_set(QL_LOG_APP_MASK, QL_LOG_PORT_USB);

    if (Vn_Queue_Create(&Vn_LogQueue, VN_LOG_QUEUE_DEPTH) != 0)
    {
        printf("Vn_Log_Init Failed\n");
        return;
    }

    status = ql_rtos_task_create(&Vn_LogTaskRef,
                                 VN_LOG_TASK_STACK_SIZE,
                                 VN_LOG_TASK_PRIORITY,
                                 "Log_Task",
                                 Vn_Log_Task,
                                 NULL);
    if (status != QUEC_SUCCESS)
    {
        Vn_Queue_Delete(&Vn_LogQueue);
        printf("Vn_Log_Init Failed\n");
        return;
    }

    Vn_LogReady = true;
}

void Vn_Log_Printf(const char *fmt, ...)
{
    char *buffer;
    va_list args;
    int length;

    if ((fmt == NULL) || !Vn_LogReady)
    {
        return;
    }

    buffer = (char *)malloc(VN_LOG_BUFFER_SIZE);
    if (buffer == NULL)
    {
        return;
    }

    va_start(args, fmt);
    length = vsnprintf(buffer, VN_LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    if (length < 0)
    {
        free(buffer);
        return;
    }

    if ((size_t)length >= VN_LOG_BUFFER_SIZE)
    {
        length = (int)VN_LOG_BUFFER_SIZE - 1;
        buffer[length] = '\0';
    }

    if (length > 0)
    {
        (void)Vn_Queue_Release(Vn_LogQueue, buffer, length, QL_NO_WAIT);
    }

    free(buffer);
}
