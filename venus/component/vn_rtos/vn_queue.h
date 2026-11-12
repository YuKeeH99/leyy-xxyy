#ifndef COMPONENT_VN_RTOS_VN_QUEUE_H_
#define COMPONENT_VN_RTOS_VN_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "ql_rtos.h"

typedef struct {
    uint8_t * MsgPtr;
    uint32_t Size;
} Vn_Queue_Msg_Typedef;

typedef struct {
    ql_queue_t Queue;
    Vn_Queue_Msg_Typedef Msg;
} Vn_Queue_Typedef;

extern int32_t Vn_Queue_Create(Vn_Queue_Typedef * Queue, int MaxNumber);
extern void Vn_Queue_Delete(Vn_Queue_Typedef * Queue);
extern int32_t Vn_Queue_Wait(Vn_Queue_Typedef Queue, void * pData, int DataMaxLen, int Timeout);
extern int32_t Vn_Queue_Release(Vn_Queue_Typedef Queue, const void * pData, int Size, int Timeout);
extern void Vn_Queue_Clear(Vn_Queue_Typedef Queue);

#endif /* COMPONENT_VN_RTOS_VN_QUEUE_H_ */
