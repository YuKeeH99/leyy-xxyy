#ifndef XIAOZHI_INC_VN_OTA_JSON_H_
#define XIAOZHI_INC_VN_OTA_JSON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef struct {
    char Version[16];
    char Elf_Sha256[256];
} VN_OTA_Json_TypeDef;

typedef struct {
    char Board_Wifi_Type[64];
    char Board_Wifi_Name[64];
    char Board_Ssid[64];
    int Board_Rssi;
    int Board_Channel;
    char Board_Ip[32];
    char Board_Mac[32];
} Vn_Wifi_Info_TypeDef;

typedef struct {
    char Board_Cell_Type[64];
    char Board_Cell_Name[64];
    char Board_Revision[32];
    char Board_Carrier[32];
    char Board_Csq[3];
    char Board_Imei[32];
    char Board_Iccid[32];
} Vn_Cell_Info_TypeDef;

typedef struct {
    char Host[128];
    char Language[16];
    char Content_Type[128];
    char User_Agent[128];
    char Device_ID[18];
    char Client_ID[38];
    VN_OTA_Json_TypeDef App_Info;
    Vn_Wifi_Info_TypeDef Wifi_Info;
    Vn_Cell_Info_TypeDef Cell_Info;
    uint8_t Register_Status;
} VN_OTA_HTTP_Information_TypeDef;


#endif /* XIAOZHI_INC_VN_OTA_JSON_H_ */
