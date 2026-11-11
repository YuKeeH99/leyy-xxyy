#ifndef __VN_GPIO_H__
#define __VN_GPIO_H__ 

#include "vn_plat.h"

#include "ql_gpio.h"

// LED Status
#define VN_LED_STATUS_ON             PIN_LEVEL_HIGH
#define VN_LED_STATUS_OFF            PIN_LEVEL_LOW

// Touch Status
#define VN_KEY_TOUCH_VALID           PIN_LEVEL_LOW

#define VN_LDO_ENABLE                PIN_LEVEL_HIGH
#define VN_LDO_DISABLE               PIN_LEVEL_LOW

typedef enum
{
    VN_KEY_TOUCH_STATUS_VALID = VN_KEY_TOUCH_VALID,
    VN_KEY_TOUCH_STATUS_INVALID
} Vn_Key_TouchStatus_Typedef;

typedef enum
{
    VN_LDO_WIFI_CHIP    = 0,    ///< Power supply for WiFi chip
    VN_LDO_RADAR        = 1,    ///< Power supply for radar module
    VN_LDO_WAKEUP_CHIP  = 2,    ///< Power supply for wakeup chip
    VN_LDO_OLED_LCD     = 3,    ///< Power supply for OLED/LCD display
    VN_LDO_CODEC        = 4,    ///< Power supply for audio codec
    VN_LDO_MAX          = 5     ///< Maximum number of LDO types
} Vn_LDO_Typedef;

typedef enum
{
    VN_LDO_STATUS_DISABLE = PIN_LEVEL_LOW,
    VN_LDO_STATUS_ENABLE = PIN_LEVEL_HIGH,
}Vn_LDO_Status_Typedef;


int32_t Vn_LED_StatusInit(void);
void  Vn_LED_StatusOn(void);
void  Vn_LED_StatusOff(void);

int32_t Vn_Key_TouchInit(void);
int32_t Vn_Key_GetTouchStatus(Vn_Key_TouchStatus_Typedef *Status);

int32_t Vn_LDO_Init(void);
int32_t Vn_LDO_Control(Vn_LDO_Typedef LDO, Vn_LDO_Status_Typedef Status);
int32_t Vn_LDO_Status(Vn_LDO_Status_Typedef *Status);
#endif