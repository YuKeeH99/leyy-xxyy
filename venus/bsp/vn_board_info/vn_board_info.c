#include "vn_board_info.h"

#include <stdio.h>
#include <string.h>

#include "ql_dev.h"
#include "vn_log.h"
#include "vn_wifi.h"

#define VN_BOARD_CELLULAR_NAME     "EC800MCN_LE"
#define VN_BOARD_CELLULAR_FIRMWARE "EC800MCNNR01A01SV08_DTN"
#define VN_BOARD_WIFI_NAME         "FGM840R"
#define VN_BOARD_MAC_TEXT_LEN      17U

/* This interface is provided by the Wi-Fi BSP. */
extern int32_t Vn_Wifi_QuerySubVersion(char *pVersion);

int Vn_Get_IMEI(char *imei, unsigned int imei_len)
{
    QL_DEV_ERROR_CODE status;

    if ((imei == NULL) || (imei_len == 0U))
    {
        return -1;
    }

    status = ql_dev_get_imei(imei, imei_len);
    if (status != QL_DEV_SUCCESS)
    {
        VN_LOG_E("Get IMEI failed: %d", status);
        return -1;
    }

    imei[imei_len - 1U] = '\0';
    VN_LOG_I("IMEI: %s", imei);
    return 0;
}

int Vn_Get_Cellura_MAC(uint8_t *mac_val)
{
    int status;

    if (mac_val == NULL)
    {
        return -1;
    }

    status = ql_dev_get_mac(mac_val, VN_BOARD_MAC_TEXT_LEN);
    if (status != 0)
    {
        VN_LOG_E("Get MAC failed: %d", status);
        return -1;
    }

    return 0;
}

static int32_t Vn_BoardInfo_GetCellularVersion(Vn_BoardInfo_Typedef *BoardInfo)
{
    int major = 0;
    int minor = 0;
    int patch = 0;

    (void)snprintf((char *)BoardInfo->Name,
                   sizeof(BoardInfo->Name),
                   "%s",
                   VN_BOARD_CELLULAR_NAME);
    (void)snprintf((char *)BoardInfo->Firmware,
                   sizeof(BoardInfo->Firmware),
                   "%s",
                   VN_BOARD_CELLULAR_FIRMWARE);

    if (sscanf((const char *)BoardInfo->Firmware,
               "EC800MCNNR%02dA%2dSV%02d",
               &major,
               &minor,
               &patch) != 3)
    {
        return -1;
    }

    if (minor > 0)
    {
        --minor;
    }

    (void)snprintf((char *)BoardInfo->Version,
                   sizeof(BoardInfo->Version),
                   "%d.%d.%d",
                   major,
                   minor,
                   patch);
    return 0;
}

static int32_t Vn_BoardInfo_GetWifiVersion(Vn_BoardInfo_Typedef *BoardInfo)
{
    char firmware[128] = {0};
    char sub_version[128] = {0};
    int major = 0;
    int minor = 0;
    int patch = 0;
    int32_t status;

    (void)snprintf((char *)BoardInfo->Name,
                   sizeof(BoardInfo->Name),
                   "%s",
                   VN_BOARD_WIFI_NAME);

    status = Vn_Wifi_QueryVersion(firmware);
    if (status < 0)
    {
        VN_LOG_E("Get Wi-Fi version failed: %d", status);
        return -1;
    }
    firmware[sizeof(firmware) - 1U] = '\0';

    if (strlen(firmware) >= sizeof(BoardInfo->Firmware))
    {
        VN_LOG_E("Wi-Fi firmware version is too long");
        return -1;
    }
    memcpy(BoardInfo->Firmware, firmware, strlen(firmware) + 1U);

    if (sscanf(firmware, "FGM840RWR%02dA%02dS", &major, &minor) != 2)
    {
        VN_LOG_E("Parse Wi-Fi firmware version failed: %s", firmware);
        return -1;
    }
    if (minor > 0)
    {
        --minor;
    }

    status = Vn_Wifi_QuerySubVersion(sub_version);
    if ((status < 0) || (sscanf(sub_version, "V%d", &patch) != 1))
    {
        VN_LOG_E("Get Wi-Fi sub version failed: %d", status);
        return -1;
    }

    (void)snprintf((char *)BoardInfo->Version,
                   sizeof(BoardInfo->Version),
                   "%d.%d.%d",
                   major,
                   minor,
                   patch);
    return 0;
}

int32_t Vn_GetBoardInfo(Vn_BoardInfo_Typedef *BoardInfo, Vn_NetType_Typedef NetType)
{
    int32_t result = 0;

    if (BoardInfo == NULL)
    {
        return -1;
    }

    memset(BoardInfo, 0, sizeof(*BoardInfo));

    if (Vn_Get_Cellura_MAC(BoardInfo->MacAddr) != 0)
    {
        result = -1;
    }
    if (Vn_Get_IMEI((char *)BoardInfo->UUID, sizeof(BoardInfo->UUID)) != 0)
    {
        result = -1;
    }

    if (NetType == VN_NET_TYPE_CELLULAR)
    {
        if (Vn_BoardInfo_GetCellularVersion(BoardInfo) != 0)
        {
            result = -1;
        }
    }
    else if (NetType == VN_NET_TYPE_WIFI)
    {
        if (Vn_BoardInfo_GetWifiVersion(BoardInfo) != 0)
        {
            result = -1;
        }
    }
    else
    {
        VN_LOG_E("Invalid network type: %d", (int)NetType);
        return -1;
    }

    VN_LOG_D("Firmware: %s, version: %s",
             (const char *)BoardInfo->Firmware,
             (const char *)BoardInfo->Version);
    return result;
}
