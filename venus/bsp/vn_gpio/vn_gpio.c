#include "vn_gpio.h"


/**
 * @brief  Initialize the status LED
 * @return  0: success, -1: fail
 */
int32_t Vn_LED_StatusInit(void)
{
    int32_t ret = 0;
    ret = ql_gpio_init(VN_PIN_STATUS,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}


/**
 * @brief  Turn on the status LED
 * @return  None
 */
void Vn_LED_StatusOn(void)
{
    ql_gpio_set_level(VN_PIN_STATUS, VN_LED_STATUS_ON);
}

/**
 * @brief  Turn off the status LED
 * @return  None
 */
void Vn_LED_StatusOff(void)
{
    ql_gpio_set_level(VN_PIN_STATUS, VN_LED_STATUS_OFF);
}

/**
 * @brief  Initialize the key touch
 * @return  0: success, -1: fail
 */
int32_t Vn_Key_TouchInit(void)
{
    int32_t ret = 0;
    ret = ql_gpio_init(VN_PIN_TOUCH,
                       PIN_DIRECTION_IN,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief  Get key touch status
 * @param  Status: key touch status
 * @return  0: success, -1: fail
 */
int32_t Vn_Key_GetTouchStatus(Vn_Key_TouchStatus_Typedef *Status)
{
    int32_t ret = 0;
    PIN_LEVEL_E pin_level = PIN_LEVEL_LOW;

    ret = ql_gpio_get_level(VN_PIN_TOUCH, &pin_level);

    *Status = (pin_level == PIN_LEVEL_LOW) ? VN_KEY_TOUCH_STATUS_VALID : VN_KEY_TOUCH_STATUS_INVALID;

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK); 
}

/**
 * @brief LDO init
 * @return  0: success, -1: fail
 */
int32_t Vn_LDO_Init(void)
{
    int32_t ret = 0;

    ret = ql_gpio_init(VN_PIN_WIFI_LDO_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_RADAR_LDO_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_WAKEUP_CHIP_LDO_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_OLED_LCD_LDO_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_CODEC_LDO_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);


    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief LDO control
 * @param LDO LDO number
 * @param Status LDO status
 * @return 0: success, -1: fail
 */
int32_t Vn_LDO_Control(Vn_LDO_Typedef LDO, Vn_LDO_Status_Typedef Status)
{
    int32_t ret = 0;
    uint32_t gpio_pin_num;

    if((Status != VN_LDO_STATUS_DISABLE) && (Status != VN_LDO_STATUS_ENABLE))
    {
        return VN_RET_ERROR;
    }

    switch(LDO)
    {
        case VN_LDO_WIFI_CHIP:
            gpio_pin_num = VN_PIN_WIFI_LDO_EN;
            break;
        case VN_LDO_RADAR:
            gpio_pin_num = VN_PIN_RADAR_LDO_EN;
            break;
        case VN_LDO_WAKEUP_CHIP:
            gpio_pin_num = VN_PIN_WAKEUP_CHIP_LDO_EN;
            break;
        case VN_LDO_OLED_LCD:
            gpio_pin_num = VN_PIN_OLED_LCD_LDO_EN;
            break;
        case VN_LDO_CODEC:
            gpio_pin_num = VN_PIN_CODEC_LDO_EN;
            break;
        default:
            return VN_RET_ERROR;
            break;
    }

    ret = ql_gpio_set_level(gpio_pin_num, Status);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}