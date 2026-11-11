#include <stdio.h>

#include "ql_rtos.h"

#include "vn_wifi_app.h"
#include "vn_wifi.h"
#include "vn_param.h"
#include "vn_utils.h"
#include "vn_xiaozhi_app.h"

/* vn_display.h 尚未恢复，以下声明与 A01 的显示接口保持一致。 */
typedef enum
{
    VN_DISPLAY_MSGTYPE_EMOJI = 0,
    VN_DISPLAY_MSGTYPE_CHAT_EMOJI,
    VN_DISPLAY_MSGTYPE_CHAT_TEXT,
    VN_DISPLAY_MSGTYPE_TITLE,
    VN_DISPLAY_MSGTYPE_OTA,
    VN_DISPLAY_MSGTYPE_INITIALIZING,
    VN_DISPLAY_MSGTYPE_SIGNAL,
    VN_DISPLAY_MSGTYPE_BATTERY,
    VN_DISPLAY_MSGTYPE_QRCODE,
    VN_DISPLAY_MSGTYPE_JPEG,
    VN_DISPLAY_MSGTYPE_SLEEP,
    VN_DISPLAY_MSGTYPE_OLED_CHAT_EMOJI,
    VN_DISPLAY_MSGTYPE_MsgBox
} __attribute__((__packed__)) Vn_DisplayMsgType_TypeDef;

extern void Vn_DisplayMsg_SendString(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData);

#define VN_WIFI_APP_TASK_STACK_SIZE  20480
#define VN_WIFI_APP_TASK_PRIORITY    106
#define VN_WIFI_APP_VERSION_SIZE     64

typedef struct
{
    int8_t ConnectedState;
    char Ssid[32];
    char Password[32];
} Vn_WifiAppInfo_Typedef;

ql_task_t Vn_Wifi_CmdTaskRef;

static Vn_WifiAppInfo_Typedef Vn_WifiAppInfo;
static uint8_t Vn_WifiVersion[VN_WIFI_APP_VERSION_SIZE];

void Vn_Wifi_CmdProgress(void *arg)
{
    (void)arg;

    VN_LOG_D("Vn_Wifi_CmdProgress....");
    while (1)
    {
        Vn_Wifi_Process();
        ql_rtos_task_sleep_ms(1);
    }
}

void Vn_Wifi_ApConnectCallback(void *pUserData)
{
    if (pUserData == NULL)
    {
        return;
    }

    if (sscanf((char *)pUserData, "+QAPNETURC:0,STA,%31[^,],%31s",
               Vn_WifiAppInfo.Ssid, Vn_WifiAppInfo.Password) == 2)
    {
        Vn_WifiAppInfo.ConnectedState = 1;
    }
    else if (strstr((char *)pUserData, "+QAPNETURC:1") != NULL)
    {
        Vn_WifiAppInfo.ConnectedState = -1;
    }
}

int32_t Vn_Wifi_AppInit(void)
{
    int32_t ret = Vn_Wifi_Init();

    VN_LOG_I("Vn_Wifi_Init ret:%d", ret);
    if (Vn_Wifi_CmdTaskRef == NULL)
    {
        ret = ql_rtos_task_create(&Vn_Wifi_CmdTaskRef,
                                  VN_WIFI_APP_TASK_STACK_SIZE,
                                  VN_WIFI_APP_TASK_PRIORITY,
                                  "Vn_Wifi_CmdProgress",
                                  Vn_Wifi_CmdProgress,
                                  NULL);
        if (ret != QUEC_SUCCESS)
        {
            VN_LOG_E("Vn_Wifi_CmdProgress task create failed:%d", ret);
        }
    }

    memset(Vn_WifiVersion, 0, sizeof(Vn_WifiVersion));
    ret = Vn_Wifi_QueryVersion((char *)Vn_WifiVersion);
    if (ret == VN_RET_OK)
    {
        VN_LOG_D("wifi ver: %s", Vn_WifiVersion);
    }
    else
    {
        VN_LOG_E("query wifi version failed:%d", ret);
    }

    return VN_RET_OK;
}

char *Vn_WifiApp_GetVersion(void)
{
    return (char *)Vn_WifiVersion;
}

void Vn_WifiApp_DeleteSsidAndPwd(void)
{
    int32_t ret = Vn_param_delete(VN_PARAM_KEY_WIFI_SSID);

    if (ret != VN_RET_OK)
    {
        VN_LOG_E("SSID param delete fail:%d", ret);
    }

    ret = Vn_param_delete(VN_PARAM_KEY_WIFI_PASSWORD);
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("password param delete fail:%d", ret);
    }
}

int32_t Vn_WifiApp_WriteSsidAndPwd(char *Ssid, char *Pwd)
{
    size_t ssid_len;
    size_t pwd_len;
    int32_t ret;

    if (Ssid == NULL || Pwd == NULL)
    {
        return VN_RET_ERROR;
    }

    ssid_len = strlen(Ssid);
    pwd_len = strlen(Pwd);
    if (ssid_len == 0 || ssid_len >= sizeof(Vn_WifiAppInfo.Ssid) ||
        pwd_len >= sizeof(Vn_WifiAppInfo.Password))
    {
        return VN_RET_ERROR;
    }

    VN_LOG_D("write SSID len:%u, password len:%u",
             (unsigned int)ssid_len, (unsigned int)pwd_len);
    Vn_WifiApp_DeleteSsidAndPwd();

    ret = Vn_Param_Write(VN_PARAM_KEY_WIFI_SSID, (const uint8_t *)Ssid,
                         (uint16_t)(ssid_len + 1));
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("write SSID failed:%d", ret);
        return VN_RET_ERROR;
    }

    ret = Vn_Param_Write(VN_PARAM_KEY_WIFI_PASSWORD, (const uint8_t *)Pwd,
                         (uint16_t)(pwd_len + 1));
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("write password failed:%d", ret);
        return VN_RET_ERROR;
    }

    return VN_RET_OK;
}

int32_t Vn_WifiApp_ReadSsidAndPwd(uint8_t *Ssid, uint8_t SsidBuffLen,
                                  uint8_t *Pwd, uint8_t PwdBuffLen)
{
    int32_t ssid_ret;
    int32_t pwd_ret;

    if (Ssid == NULL || Pwd == NULL || SsidBuffLen == 0 || PwdBuffLen == 0)
    {
        return VN_RET_ERROR;
    }

    Ssid[0] = '\0';
    Pwd[0] = '\0';
    ssid_ret = Vn_Param_Read(VN_PARAM_KEY_WIFI_SSID, Ssid, SsidBuffLen);
    pwd_ret = Vn_Param_Read(VN_PARAM_KEY_WIFI_PASSWORD, Pwd, PwdBuffLen);
    if (ssid_ret > 0 && pwd_ret > 0)
    {
        Ssid[(ssid_ret < SsidBuffLen) ? ssid_ret : (SsidBuffLen - 1)] = '\0';
        Pwd[(pwd_ret < PwdBuffLen) ? pwd_ret : (PwdBuffLen - 1)] = '\0';
        return VN_RET_OK;
    }

    VN_LOG_E("read SSID ret:%d or password ret:%d", ssid_ret, pwd_ret);
    return VN_RET_ERROR;
}

int32_t Vn_WifiAppTryConnectAp(uint32_t TimeOut)
{
    int32_t ret;
    int32_t rssi = 0;

    (void)TimeOut;

    ret = Vn_WifiApp_ReadSsidAndPwd((uint8_t *)Vn_WifiAppInfo.Ssid,
                                    sizeof(Vn_WifiAppInfo.Ssid),
                                    (uint8_t *)Vn_WifiAppInfo.Password,
                                    sizeof(Vn_WifiAppInfo.Password));
    if (ret != VN_RET_OK)
    {
        return -1;
    }

    VN_LOG_D("got SSID and password from flash, connecting AP");
    Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                             (uint8_t *)"等待连接AP");

    ret = Vn_Wifi_ConnectAP(Vn_WifiAppInfo.Ssid, Vn_WifiAppInfo.Password);
    if (ret != VN_RET_OK)
    {
        Vn_WifiAppInfo.ConnectedState = 0;
        VN_LOG_D("Connect AP Failed:%d", ret);
        return -2;
    }

    ret = Vn_Wifi_GetRssi(&rssi);
    if (ret != VN_RET_OK || rssi == 0)
    {
        Vn_WifiAppInfo.ConnectedState = 0;
        return -3;
    }

    Vn_WifiAppInfo.ConnectedState = 1;
    Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                             (uint8_t *)"连接AP成功");
    VN_LOG_D("Connect AP Success");
    return VN_RET_OK;
}

int32_t Vn_WifiAppConfigOwnAP(void)
{
    char app_ssid[32] = {0};
    uint8_t buff[64] = {0};
    Vn_Xiaozhi_Typedef *xiaozhi = Vn_Xiaozhi_GetInstance();
    int32_t ret;

    if (xiaozhi == NULL)
    {
        return VN_RET_ERROR;
    }

    snprintf(app_ssid, sizeof(app_ssid), "MJ-%c%c%c%c",
             xiaozhi->MacAddr[12], xiaozhi->MacAddr[13],
             xiaozhi->MacAddr[15], xiaozhi->MacAddr[16]);
    snprintf((char *)buff, sizeof(buff), "请连接%s, 配置WIFI", app_ssid);
    Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING, buff);
    VN_LOG_D("no SSID and password in flash, waiting for user configuration");

    Vn_Wifi_ResigterWebConfigCallback(Vn_Wifi_ApConnectCallback);
    ret = Vn_Wifi_CloseOwnAP();
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("close own AP failed:%d", ret);
        return VN_RET_ERROR;
    }

    ret = Vn_Wifi_OpenOwnAP(app_ssid);
    if (ret != VN_RET_OK)
    {
        Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                 (uint8_t *)"打开AP失败");
        ql_rtos_task_sleep_ms(2000);
        VN_LOG_E("open own AP failed:%d", ret);
        return VN_RET_ERROR;
    }

    Vn_WifiAppInfo.ConnectedState = 0;
    return VN_RET_OK;
}

int32_t Vn_WifiAppWaitOwnApConnect(uint32_t TimeOut)
{
    uint32_t start_time = VN_RTOS_SYSTEM_MS;

    while (1)
    {
        VN_LOG_D("Waiting for AP configure...");
        if (Vn_WifiAppInfo.ConnectedState == 1)
        {
            (void)Vn_WifiApp_WriteSsidAndPwd(Vn_WifiAppInfo.Ssid,
                                             Vn_WifiAppInfo.Password);
            return VN_RET_OK;
        }

        if (Vn_WifiAppInfo.ConnectedState < 0)
        {
            VN_LOG_E("AP configure failed");
            return -1;
        }

        ql_rtos_task_sleep_ms(1000);
        if (Vn_DiffTime(start_time, VN_RTOS_SYSTEM_MS) >= TimeOut)
        {
            return -2;
        }
    }
}
