#ifndef BSP_VN_RADAR_VN_RADAR_H_
#define BSP_VN_RADAR_VN_RADAR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef void (*Vn_Radar_WakeupCallback)(void);

extern int32_t Vn_Radar_Init(void);
extern int32_t Vn_Radar_EnableTriggerWakeUp(Vn_Radar_WakeupCallback Callback);
extern int32_t Vn_Radar_GetTragetDistance(int32_t * pDistance, _Bool * pHadTarget);
extern int32_t Vn_Radar_SetTriggerDistance(uint32_t Distance);
extern int32_t Vn_Radar_DisableTriggerWakeUp(void);
extern int32_t Vn_Radar_GetTriggerDistance(uint32_t * pDistance);
extern _Bool Vn_Radar_HadDetectedTarget(void);

#endif /* BSP_VN_RADAR_VN_RADAR_H_ */
