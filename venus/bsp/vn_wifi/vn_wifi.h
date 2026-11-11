#ifndef __VN_WIFI_H__
#define __VN_WIFI_H__

#include "vn_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t Mode[8];
    uint8_t Ssid[32];
    uint8_t Password[64];
    uint8_t Channel;
    uint8_t Security[32];
    uint8_t Mac[18];
    uint8_t Ip[16];
} Vn_Wifi_StateInfo_Typedef;

typedef void (*Vn_Wifi_WebConfigCallback_Typedef)(void *pUserData);
typedef void (*Vn_Wifi_OtaCallback_Typedef)(void *pUserData);
typedef void (*Vn_Wifi_WsReadCallback_Typedef)(uint8_t *pData, uint32_t Size, uint8_t MsgType);

int32_t Vn_Wifi_Init(void);
void Vn_Wifi_Process(void);
int32_t Vn_Wifi_Enable(void);
int32_t Vn_Wifi_Disable(void);
int32_t Vn_Wifi_QueryVersion(char *pVersion);
int32_t Vn_Wifi_QuerySubVersion(char *pVersion);
int32_t Vn_Wifi_ConnectAP(char *Ssid, char *Pwd);
int32_t Vn_Wifi_GetRssi(int32_t *Rssi);
int32_t Vn_Wifi_GetStateInfo(Vn_Wifi_StateInfo_Typedef *StateInfo);
int32_t Vn_Wifi_IsConnectedAP(bool *IsConnected);
void Vn_Wifi_ResigterWebConfigCallback(Vn_Wifi_WebConfigCallback_Typedef Callback);
int32_t Vn_Wifi_StartOta(char *Url, uint16_t Port, Vn_Wifi_OtaCallback_Typedef Callback);
int32_t Vn_Wifi_OpenOwnAP(char *ApSsid);
int32_t Vn_Wifi_CloseOwnAP(void);
int32_t Vn_Wifi_HttpsClientPostSync(char *Host, uint16_t Port, char *Path,
                                    char *ContentType, char *ContentData,
                                    char *RspBuf, uint32_t RspBufSize);
int32_t Vn_Wifi_HttpsClientGetSync(char *Host, uint16_t Port, char *Path,
                                   char *ContentType, char *ContentData,
                                   char *RspBuf, uint32_t RspBufSize);
int32_t Vn_Wifi_ConfigWsServer(char *Url, char *Header,
                               Vn_Wifi_WsReadCallback_Typedef WsReadCb);
int32_t Vn_Wifi_ConnectWsServer(void);
int32_t Vn_Wifi_DisConnectWsServer(void);
int32_t Vn_Wifi_SendToWsServer(char *pData, uint32_t DataLen, uint32_t DataType);

#ifdef __cplusplus
}
#endif

#endif /* __VN_WIFI_H__ */
