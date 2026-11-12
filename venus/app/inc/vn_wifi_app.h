#ifndef APP_INC_VN_WIFI_APP_H_
#define APP_INC_VN_WIFI_APP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef enum {
    VN_WIFI_APP_STATE_INIT = 0,
    VN_WIFI_APP_STATE_CONNECT_AP = 1,
    VN_WIFI_APP_STATE_CONFIG_AP = 2,
    VN_WIFI_APP_STATE_CONNECT_NET = 3,
    VN_WIFI_APP_STATE_FAIL = 4
} Vn_Wifi_App_State;

extern int32_t Vn_Wifi_AppInit(void);
extern char * Vn_WifiApp_GetVersion(void);
extern void Vn_WifiApp_DeleteSsidAndPwd(void);
extern int32_t Vn_WifiAppTryConnectAp(uint32_t TimeOut);
extern int32_t Vn_WifiAppConfigOwnAP(void);
extern int32_t Vn_WifiAppWaitOwnApConnect(uint32_t TimeOut);
extern void Vn_Wifi_CmdProgress(void * arg);
extern void Vn_Wifi_ApConnectCallback(void * pUserData);
extern int32_t Vn_WifiApp_WriteSsidAndPwd(char * Ssid, char * Pwd);
extern int32_t Vn_WifiApp_ReadSsidAndPwd(uint8_t * Ssid, uint8_t SsidBuffLen, uint8_t * Pwd, uint8_t PwdBuffLen);
extern int32_t Vn_WifiAppConnectNet(uint32_t TimeOut);

#endif /* APP_INC_VN_WIFI_APP_H_ */
