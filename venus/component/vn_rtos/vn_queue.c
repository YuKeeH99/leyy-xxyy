#include "vn_queue.h"

#include <stdlib.h>
#include <string.h>

#include "vn_log.h"

void Vn_Queue_Clear(Vn_Queue_Typedef Queue)
{
    u32 queue_cnt = 0U;
    uint8_t discard;

    if ((Queue.Queue == NULL) ||
        (ql_rtos_queue_get_cnt(Queue.Queue, &queue_cnt) != QUEC_SUCCESS))
    {
        return;
    }

    while (queue_cnt-- > 0U)
    {
        if (Vn_Queue_Wait(Queue,
                          &discard,
                          (int)sizeof(discard),
                          QL_NO_WAIT) < 0)
        {
            break;
        }
    }
}

void Vn_Queue_Delete(Vn_Queue_Typedef *Queue)
{
    if ((Queue == NULL) || (Queue->Queue == NULL))
    {
        return;
    }

    Vn_Queue_Clear(*Queue);
    (void)ql_rtos_queue_delete(Queue->Queue);
    Queue->Queue = NULL;
    Queue->Msg.MsgPtr = NULL;
    Queue->Msg.Size = 0U;
}

int32_t Vn_Queue_Create(Vn_Queue_Typedef *Queue, int MaxNumber)
{
    QlOSStatus status;

    if ((Queue == NULL) || (MaxNumber <= 0))
    {
        return -1;
    }

    if (Queue->Queue != NULL)
    {
        Vn_Queue_Delete(Queue);
    }

    Queue->Msg.MsgPtr = NULL;
    Queue->Msg.Size = 0U;

    status = ql_rtos_queue_create(&Queue->Queue,
                                  (u32)sizeof(Vn_Queue_Msg_Typedef),
                                  (u32)MaxNumber);
    if (status != QUEC_SUCCESS)
    {
        Queue->Queue = NULL;
        VN_LOG_E("ql_rtos_queue_create failed: %d", status);
        return -1;
    }

    return 0;
}

int32_t Vn_Queue_Wait(Vn_Queue_Typedef Queue,
                      void *pData,
                      int DataMaxLen,
                      int Timeout)
{
    Vn_Queue_Msg_Typedef message = {NULL, 0U};
    int32_t copy_len;

    if ((Queue.Queue == NULL) || (pData == NULL) || (DataMaxLen <= 0))
    {
        return -1;
    }

    if (ql_rtos_queue_wait(Queue.Queue,
                           (u8 *)&message,
                           (u32)sizeof(message),
                           (u32)Timeout) != QUEC_SUCCESS)
    {
        return -1;
    }

    if ((message.MsgPtr == NULL) && (message.Size != 0U))
    {
        return -1;
    }

    copy_len = (message.Size < (uint32_t)DataMaxLen)
                   ? (int32_t)message.Size
                   : DataMaxLen;
    if (copy_len > 0)
    {
        memcpy(pData, message.MsgPtr, (size_t)copy_len);
    }

    free(message.MsgPtr);
    return copy_len;
}

int32_t Vn_Queue_Release(Vn_Queue_Typedef Queue,
                         const void *pData,
                         int Size,
                         int Timeout)
{
    Vn_Queue_Msg_Typedef message;

    if ((Queue.Queue == NULL) || (pData == NULL) || (Size <= 0))
    {
        return -1;
    }

    message.MsgPtr = (uint8_t *)malloc((size_t)Size);
    if (message.MsgPtr == NULL)
    {
        return -1;
    }

    memcpy(message.MsgPtr, pData, (size_t)Size);
    message.Size = (uint32_t)Size;

    if (ql_rtos_queue_release(Queue.Queue,
                              (u32)sizeof(message),
                              (u8 *)&message,
                              (u32)Timeout) != QUEC_SUCCESS)
    {
        free(message.MsgPtr);
        return -1;
    }

    return 0;
}
