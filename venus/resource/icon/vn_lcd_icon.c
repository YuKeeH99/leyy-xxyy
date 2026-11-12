#include "vn_lcd_icon.h"

const Vn_Icon_TypeDef Vn_IconResource[] =
{
    {VN_ICON_NO_SINGAL, "\xEE\x98\x9D"},
    {VN_ICON_CELLULAR_SINGAL_SLASH, "\xEE\xA8\x99"},
    {VN_ICON_CELLULAR_SINGAL_WEAK, "\xEE\x98\x95"},
    {VN_ICON_CELLULAR_SINGAL_MIDDLE, "\xEE\x98\x96"},
    {VN_ICON_CELLULAR_SINGAL_GOOD, "\xEE\x98\x97"},
    {VN_ICON_WIFI_SINGAL_SLASH, "\xEE\x98\x8E"},
    {VN_ICON_WIFI_SINGAL_WEAK, "\xEE\x99\xB3"},
    {VN_ICON_WIFI_SINGAL_MIDDLE, "\xEE\x99\xB2"},
    {VN_ICON_WIFI_SINGAL_GOOD, "\xEE\x99\xB1"},
};

uint32_t Vn_Icon_GetNum(void)
{
    return VN_ICON_MAX;
}
