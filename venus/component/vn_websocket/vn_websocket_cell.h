#ifndef COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_CELL_H_
#define COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_CELL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "vn_websocket_common.h"
#include "vn_net.h"

extern int32_t Vn_WebsocketCell_Init(Vn_Websocket_MsgHandlerFunc Handler);
extern int32_t Vn_WebsocketCell_DeInit(void);
extern int32_t Vn_WebsocketCell_Connect(uint32_t LocalIP, uint16_t LocalPort, int8_t * Url, int8_t * Headers);
extern _Bool Vn_WebsocketCell_IsConnected(void);
extern int32_t Vn_WebsocketCell_Disconnect(void);
extern int32_t Vn_WebsocketCell_Poll(uint32_t Timeout);
extern int32_t Vn_WebsocketCell_Send(uint8_t * pData, uint32_t Len, uint8_t MsgType);

#endif /* COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_CELL_H_ */
