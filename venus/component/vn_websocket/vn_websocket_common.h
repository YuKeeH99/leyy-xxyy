#ifndef COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_COMMON_H_
#define COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_COMMON_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    VN_WEBSOCKET_MSG_TEXT = 1,
    VN_WEBSOCKET_MSG_BINARY = 2
} Vn_Websocket_MsgType_Typedef;

typedef void (*Vn_Websocket_MsgHandlerFunc)(uint8_t MsgType, uint8_t *Data, uint32_t Len);

#endif /* COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_COMMON_H_ */
