#ifndef BSP_VN_CHARGE_VN_CHARGE_H_
#define BSP_VN_CHARGE_VN_CHARGE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "ql_type.h"

typedef enum {
    VN_CHARGE_STATUS_ERROR = -1,
    VN_CHARGE_STATUS_START = 0,
    VN_CHARGE_STATUS_STOP = 1
} Vn_ChargeStatus_Typedef;

extern int32_t Vn_Charge_Init(void);
extern int32_t Vn_Charge_GetStatus(Vn_ChargeStatus_Typedef * Status);
extern int32_t Vn_Charge_GetBatteryVoltage(float * Voltage);
extern int32_t Vn_Charge_Enable(void);
extern int32_t Vn_Charge_Disable(void);
extern int32_t Vn_Charge_GetBatteryTemp(float * Temp);

#endif /* BSP_VN_CHARGE_VN_CHARGE_H_ */
