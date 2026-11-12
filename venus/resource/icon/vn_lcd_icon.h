#ifndef RESOURCE_ICON_VN_LCD_ICON_H_
#define RESOURCE_ICON_VN_LCD_ICON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    VN_ICON_NO_SINGAL = 0,
    VN_ICON_CELLULAR_SINGAL_SLASH = 1,
    VN_ICON_CELLULAR_SINGAL_WEAK = 2,
    VN_ICON_CELLULAR_SINGAL_MIDDLE = 3,
    VN_ICON_CELLULAR_SINGAL_GOOD = 4,
    VN_ICON_WIFI_SINGAL_SLASH = 5,
    VN_ICON_WIFI_SINGAL_WEAK = 6,
    VN_ICON_WIFI_SINGAL_MIDDLE = 7,
    VN_ICON_WIFI_SINGAL_GOOD = 8,
    VN_ICON_MAX = 9
} Vn_IconIndex_Typedef;

typedef struct
{
    uint32_t Index;
    const char *name;
} Vn_Icon_TypeDef;

extern const Vn_Icon_TypeDef Vn_IconResource[];
extern uint32_t Vn_Icon_GetNum(void);

#endif /* RESOURCE_ICON_VN_LCD_ICON_H_ */
