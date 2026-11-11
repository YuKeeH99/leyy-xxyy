#ifndef __VN_XIAOZHI_OTA_H__
#define __VN_XIAOZHI_OTA_H__

#include <stdbool.h>
#include <stdint.h>

#include "vn_board_info.h"
#include "vn_net.h"
#include "vn_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t Version[16];
    uint8_t Url[126];
} Vn_OtaFirmwareInfo_Typedef;

typedef struct
{
    uint8_t Code[16];
    uint8_t Url[126];
} Vn_OtaRspActivate_Typedef;

typedef struct
{
    uint32_t ResultNum;
    uint8_t Message[16];
} Vn_OtaRspResigter_Typedef;

typedef struct
{
    Vn_OtaFirmwareInfo_Typedef Firmware;
    uint8_t ChatUrl[126];
    bool IsNendActivate;
    Vn_OtaRspActivate_Typedef Activate;
    Vn_OtaRspResigter_Typedef Resigter;
} Vn_OtaRspInfo_Typedef;

extern Vn_Queue_Typedef Vn_OtaWif_Queue;

void Vn_Wifi_OtaCallback(void *pUserData);
int32_t Vn_Ota_ParseOtaJson(char *JsonStr, Vn_OtaRspInfo_Typedef *OtaRspInfo);
int32_t Vn_Ota_ParseRegisterJson(char *JsonStr,
                                 Vn_OtaRspResigter_Typedef *OtaRspResigter);
int32_t Vn_Ota_CheckVersion(Vn_NetType_Typedef NetType, char *Url,
                            char *BoardVersion, char *OtaVersion);
int32_t Vn_Ota_ConnectServer(Vn_NetType_Typedef NetType, char *OtaUrl,
                             Vn_BoardInfo_Typedef *BoardInfo,
                             Vn_OtaRspInfo_Typedef *OtaRspInfo);

#ifdef __cplusplus
}
#endif

#endif /* __VN_XIAOZHI_OTA_H__ */
