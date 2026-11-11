#include "vn_charge.h"

#include "ql_adc.h"

#define VN_PIN_CHARGE_EN_LEVEL_VALID     PIN_LEVEL_HIGH
#define VN_PIN_CHARGE_CHRG_LEVEL_VALID   PIN_LEVEL_LOW
#define VN_PIN_CHARGE_STDBY_LEVEL_VALID  PIN_LEVEL_LOW

#define VN_VOLTAGE_ADC_REF_VOL                3.564f
#define VN_TEMP_ADC_REF_VOL                   1.2f

/**
 * @brief  Charge module initialization
 * @param  None
 * @retval 0: Success, others: Failed
 */
int32_t Vn_Charge_Init(void)
{
    int32_t ret = 0;

    ret = ql_gpio_init(VN_PIN_CHARGE_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_CHARGE_CHRG,
                       PIN_DIRECTION_IN,
                       PIN_PULL_PU,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_CHARGE_STDBY,
                       PIN_DIRECTION_IN,
                       PIN_PULL_PU,
                       PIN_LEVEL_HIGH);
    
    Vn_Charge_Enable();

    // ADC Init
    ret = ql_adc_init();

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief  Charge enable
 * @param  None
 * @retval 0: Success, others: Failed
 */
int32_t Vn_Charge_Enable(void)
{
    int32_t ret = 0;
    
    ret = ql_gpio_set_level(VN_PIN_CHARGE_EN, VN_PIN_CHARGE_EN_LEVEL_VALID);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief  Charge disable
 * @param  None
 * @retval 0: Success, others: Failed
 */
int32_t Vn_Charge_Disable(void)
{
    int32_t ret = 0;
    int32_t pin_level = VN_PIN_CHARGE_EN_LEVEL_VALID != PIN_LEVEL_HIGH ? PIN_LEVEL_HIGH : PIN_LEVEL_LOW;
    
    ret = ql_gpio_set_level(VN_PIN_CHARGE_EN, pin_level);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief  Get charge status
 * @param  Status: Charge status
 * @retval 0: Success, others: Failed
 */
int32_t Vn_Charge_GetStatus(Vn_ChargeStatus_Typedef *Status)
{
    int32_t ret = 0;
    PIN_LEVEL_E chrg_level, stdby_level;

    /**
     * Charge status
     * START: CHRG-0, STDBY-1
     * STOP: CHRG-1, STDBY-0
     * ERROR: CHRG-1, STDBY-1
     */

    ret = ql_gpio_get_level(VN_PIN_CHARGE_CHRG, &chrg_level);
    ret = ql_gpio_get_level(VN_PIN_CHARGE_STDBY, &stdby_level);

    if((VN_PIN_CHARGE_CHRG_LEVEL_VALID == chrg_level) && (VN_PIN_CHARGE_STDBY_LEVEL_VALID != stdby_level))
    {
        *Status = VN_CHARGE_STATUS_START;
    }
    else if((VN_PIN_CHARGE_CHRG_LEVEL_VALID != chrg_level) && (VN_PIN_CHARGE_STDBY_LEVEL_VALID == stdby_level))
    {
        *Status = VN_CHARGE_STATUS_STOP;
    }
    else
    {
        *Status = VN_CHARGE_STATUS_ERROR;
    }

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief Get battery voltage
 * @param Voltage: battery voltage (V)
 * @retval 0: Success, others: Failed
 */
int32_t Vn_Charge_GetBatteryVoltage(float *Voltage)
{
    int32_t ret = 0;
    uint16_t adc_value = 0;

    ql_adc_onff(1); 
    ql_rtos_task_sleep_ms(5);
    ql_adc_onff(0);

    ret = ql_adc_read(VN_ADC_CHANNEL_BATTERY_VOLTAGE, &adc_value);

    *Voltage = adc_value * VN_VOLTAGE_ADC_REF_VOL / 1000.0f;

    return (ret != 1 ? VN_RET_ERROR : VN_RET_OK);
}

/**
 * @brief Get battery temperature
 * @param Temp: battery temperature
 * @retval 0: Success, others: Failed
 */
int32_t Vn_Charge_GetBatteryTemp(float *Temp)
{
    int32_t ret = 0;

    uint16_t adc_value = 0;

    ql_adc_onff(1);
    ql_rtos_task_sleep_ms(5);
    ql_adc_onff(0);

    ret = ql_adc_read(VN_ADC_CHANNEL_BATTERY_TEMP, &adc_value);

    *Temp = adc_value * VN_TEMP_ADC_REF_VOL;

    return (ret != 1 ? VN_RET_ERROR : VN_RET_OK);
}
