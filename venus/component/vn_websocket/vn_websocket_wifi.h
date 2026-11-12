#ifndef COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_WIFI_H_
#define COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_WIFI_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "vn_websocket_common.h"

extern void Vn_Wifi_WsReadCallback(uint8_t * pData, uint32_t Size, uint8_t MsgType);
extern void Vn_WebsocketWifi_MsgHandler(uint8_t MsgType, uint8_t * pData, uint32_t Len);
extern int32_t Vn_WebsocketWifi_Init(Vn_Websocket_MsgHandlerFunc Handler);
extern int32_t Vn_WebsocketWifi_DeInit(void);
extern int32_t Vn_WebsocketWifi_Connect(uint32_t LocalIP, uint16_t LocalPort, int8_t * Url, int8_t * Headers);
extern _Bool Vn_WebsocketWifi_IsConnected(void);
extern int32_t Vn_WebsocketWifi_Disconnect(void);
extern int32_t Vn_WebsocketWifi_Send(uint8_t * pData, uint32_t Len, uint8_t MsgType);
extern int32_t Vn_WebsocketWifi_Poll(uint32_t Timeout);

#endif /* COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_WIFI_H_ */
