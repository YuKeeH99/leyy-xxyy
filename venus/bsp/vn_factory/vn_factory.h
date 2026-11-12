#ifndef BSP_VN_FACTORY_VN_FACTORY_H_
#define BSP_VN_FACTORY_VN_FACTORY_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef struct {
    _Bool IsOtaWifiMode;
    int8_t Ssid[64];
    int8_t Password[64];
    int8_t OtaWifiIp[128];
    uint16_t OtaWifiPort;
} Vn_FactoryParams_Typedef;

extern _Bool Vn_Factory_IsOtaWifiMode(void);
extern void Vn_Factory_RunOtaWifi(void);
extern int32_t Vn_Factory_SetOtaWifi(const int8_t * Ssid, const int8_t * Password, const int8_t * OtaWifiIp, uint16_t OtaWifiPort);
extern int32_t Vn_Factory_ReadParams(Vn_FactoryParams_Typedef * Params);
extern int32_t Vn_Factory_WriteParams(const Vn_FactoryParams_Typedef * Params);
extern int32_t Vn_Factory_DeleteParams(void);
extern Vn_FactoryParams_Typedef Vn_FactoryParams;

#endif /* BSP_VN_FACTORY_VN_FACTORY_H_ */
