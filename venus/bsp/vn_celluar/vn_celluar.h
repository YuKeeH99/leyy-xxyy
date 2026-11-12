#ifndef BSP_VN_CELLUAR_VN_CELLUAR_H_
#define BSP_VN_CELLUAR_VN_CELLUAR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

extern int Vn_DataCall_Start(int Pdp_id, int32_t TimeOut);
extern void Vn_DataCall_Status_Callback(int profile_idx, int status);
extern int Vn_DataCall_Stop(int Pdp_id);

#endif /* BSP_VN_CELLUAR_VN_CELLUAR_H_ */
