#include "vn_factory.h"

#include <stdio.h>
#include <string.h>

#include "ql_rtos.h"
#include "vn_display.h"
#include "vn_log.h"
#include "vn_param.h"
#include "vn_wifi.h"

#define VN_FACTORY_OTA_COUNTDOWN_S 60

Vn_FactoryParams_Typedef Vn_FactoryParams;

static int32_t Vn_Factory_CopyString(int8_t *Destination,
                                     size_t DestinationSize,
                                     const int8_t *Source,
                                     bool AllowEmpty)
{
    size_t length;

    if (Source == NULL)
    {
        return -1;
    }

    length = strlen((const char *)Source);
    if ((length >= DestinationSize) || (!AllowEmpty && (length == 0U)))
    {
        return -1;
    }

    memcpy(Destination, Source, length + 1U);
    return 0;
}

static void Vn_Factory_Show(const char *Text)
{
    Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING, (uint8_t *)Text);
}

int32_t Vn_Factory_ReadParams(Vn_FactoryParams_Typedef *Params)
{
    int32_t read_len;

    if (Params == NULL)
    {
        return -1;
    }

    read_len = Vn_Param_Read(VN_PARAM_KEY_FACTORY_PARAMS,
                             (uint8_t *)Params,
                             (uint16_t)sizeof(*Params));
    if (read_len != (int32_t)sizeof(*Params))
    {
        VN_LOG_E("Read factory params failed: %d", read_len);
        return -1;
    }

    return 0;
}

int32_t Vn_Factory_WriteParams(const Vn_FactoryParams_Typedef *Params)
{
    Vn_FactoryParams_Typedef saved_params;
    int32_t status;

    if (Params == NULL)
    {
        return -1;
    }

    status = Vn_Param_Write(VN_PARAM_KEY_FACTORY_PARAMS,
                            (const uint8_t *)Params,
                            (uint16_t)sizeof(*Params));
    if (status != 0)
    {
        return -1;
    }

    if ((Vn_Factory_ReadParams(&saved_params) != 0) ||
        (memcmp(&saved_params, Params, sizeof(*Params)) != 0))
    {
        VN_LOG_E("Verify factory params failed");
        return -1;
    }

    return 0;
}

int32_t Vn_Factory_DeleteParams(void)
{
    return Vn_param_delete(VN_PARAM_KEY_FACTORY_PARAMS);
}

bool Vn_Factory_IsOtaWifiMode(void)
{
    if (Vn_Factory_ReadParams(&Vn_FactoryParams) != 0)
    {
        return false;
    }

    VN_LOG_D("IsOtaWifiMode %d", (int)Vn_FactoryParams.IsOtaWifiMode);
    return Vn_FactoryParams.IsOtaWifiMode;
}

int32_t Vn_Factory_SetOtaWifi(const int8_t *Ssid,
                              const int8_t *Password,
                              const int8_t *OtaWifiIp,
                              uint16_t OtaWifiPort)
{
    Vn_FactoryParams_Typedef params = {0};
    const int8_t *password = (Password != NULL) ? Password : (const int8_t *)"";

    if ((OtaWifiPort == 0U) ||
        (Vn_Factory_CopyString(params.Ssid, sizeof(params.Ssid), Ssid, false) != 0) ||
        (Vn_Factory_CopyString(params.Password,
                               sizeof(params.Password),
                               password,
                               true) != 0) ||
        (Vn_Factory_CopyString(params.OtaWifiIp,
                               sizeof(params.OtaWifiIp),
                               OtaWifiIp,
                               false) != 0))
    {
        return -1;
    }

    params.IsOtaWifiMode = true;
    params.OtaWifiPort = OtaWifiPort;

    VN_LOG_D("Set OTA Wi-Fi, ssid: %s, url: %s, port: %u",
             (const char *)params.Ssid,
             (const char *)params.OtaWifiIp,
             (unsigned int)params.OtaWifiPort);

    if (Vn_Factory_WriteParams(&params) != 0)
    {
        return -1;
    }

    Vn_FactoryParams = params;
    return 0;
}

void Vn_Factory_RunOtaWifi(void)
{
    Vn_FactoryParams_Typedef params = Vn_FactoryParams;
    uint8_t show_buff[64];
    uint8_t version[128] = {0};
    int32_t status;
    int countdown;

    VN_LOG_D("Run OTA Wi-Fi, ssid: %s, url: %s, port: %u",
             (const char *)params.Ssid,
             (const char *)params.OtaWifiIp,
             (unsigned int)params.OtaWifiPort);

    if (Vn_Factory_DeleteParams() == 0)
    {
        Vn_FactoryParams.IsOtaWifiMode = false;
    }

    (void)snprintf((char *)show_buff,
                   sizeof(show_buff),
                   "等待连接AP: %s",
                   (const char *)params.Ssid);
    Vn_Factory_Show((const char *)show_buff);

    status = Vn_Wifi_ConnectAP((const char *)params.Ssid,
                               (params.Password[0] == '\0')
                                   ? NULL
                                   : (const char *)params.Password);
    if (status == 0)
    {
        Vn_Factory_Show("连接AP成功");
        VN_LOG_D("Connect AP success");
    }
    else
    {
        Vn_Factory_Show("连接AP失败, 重新配置OTA升级");
        VN_LOG_E("Connect AP failed: %d", status);
        return;
    }

    VN_LOG_D("Start OTA, url: %s, port: %u",
             (const char *)params.OtaWifiIp,
             (unsigned int)params.OtaWifiPort);
    status = Vn_Wifi_StartOta((char *)params.OtaWifiIp,
                              params.OtaWifiPort,
                              NULL);
    if (status != 0)
    {
        Vn_Factory_Show("OTA升级失败, 请重新配置OTA升级");
        VN_LOG_E("Start OTA failed: %d", status);
        return;
    }

    VN_LOG_D("Start upgrade");
    for (countdown = VN_FACTORY_OTA_COUNTDOWN_S; countdown > 0; --countdown)
    {
        (void)snprintf((char *)show_buff,
                       sizeof(show_buff),
                       "开始升级, 请等待 %d 秒",
                       countdown);
        Vn_Factory_Show((const char *)show_buff);
        ql_rtos_task_sleep_ms(1000U);
    }

    status = Vn_Wifi_QueryVersion((char *)version);
    version[sizeof(version) - 1U] = '\0';
    (void)snprintf((char *)show_buff,
                   sizeof(show_buff),
                   "升级完成, 当前版本: %s",
                   (const char *)version);
    Vn_Factory_Show((const char *)show_buff);
    VN_LOG_D("Version: %s, ret %d", (const char *)version, status);
}
