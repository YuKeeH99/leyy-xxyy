#include "vn_radar.h"
#include "vn_xend101h.h"


#include "ql_rtos.h"
#include "ql_uart.h"
#include "ql_gpio.h"


#define VN_RADAR_UART_PORT_BAUD_RATE      QL_UART_BAUD_115200

static Vn_Radar_WakeupCallback Vn_WakeupUserCallback;

static ql_uart_config_t Vn_UartConfig = 
{
    .baudrate = VN_RADAR_UART_PORT_BAUD_RATE,
    .data_bit = QL_UART_DATABIT_8,
    .stop_bit = 1,
    .parity_bit = QL_UART_PARITY_NONE,
    .flow_ctrl = QL_FC_NONE
};

// Uart Interface
static int32_t Vn_Radar_UartInit(QL_UART_BAUD_E Baudrate)
{
    int32_t ret = 0;

    ret = ql_uart_open(VN_UART_PORT_RADAR, Baudrate, QL_FC_NONE);

    ret = ql_uart_set_dcbconfig(VN_UART_PORT_RADAR, &Vn_UartConfig);

     // register uart callback
    ql_uart_register_cb(VN_UART_PORT_RADAR, NULL);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

static int32_t Vn_Radar_UartRead(uint8_t *pData, uint32_t Length)
{
    if(NULL == pData)
    {
        return VN_RET_ERROR;
    }

    return ql_uart_read(VN_UART_PORT_RADAR, pData, Length);
}

static int32_t Vn_Radar_UartWrite(uint8_t *pData, uint32_t Length)
{
    if(NULL == pData)
    {
        return VN_RET_ERROR;
    }

    return ql_uart_write_sync(VN_UART_PORT_RADAR, pData, Length);
}

static uint32_t Vn_SystemTime(void)
{
    return VN_RTOS_SYSTEM_MS;
}

static void Vn_Radar_DelayMs(uint32_t Time)
{
    (void)ql_rtos_task_sleep_ms(Time);
}

/**
 * @brief  Radar init
 * @param  None
 * @return 0:success, others:fail
 */
int32_t Vn_Radar_Init(void)
{
    int32_t ret = 0;

    // radar out
    ret = ql_gpio_init(VN_PIN_RADAR_OUT,
                       PIN_DIRECTION_IN,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = Vn_Radar_UartInit(VN_RADAR_UART_PORT_BAUD_RATE);

    // init XenD101H
    
    ret = Vn_XenD101H_Init(Vn_Radar_UartWrite, Vn_Radar_UartRead, Vn_Radar_DelayMs, Vn_SystemTime, ql_rtos_task_yield);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}


static void Vn_Radar_TriggerCallback(void)
{
    VN_LOG_D("Radar trigger callback");
    if (Vn_WakeupUserCallback != NULL)
    {
        Vn_WakeupUserCallback();
    }

    // Re-enable
    ql_eint_enable_wakeup(VN_PIN_RADAR_OUT, PIN_RISING_EDGE);
}


/**
 * @brief Enable trigger radar module wake up
 * @param None
 * @return 0: success; -1: fail
 */
int32_t Vn_Radar_EnableTriggerWakeUp(Vn_Radar_WakeupCallback Callback)
{
    int32_t ret;

    ret = ql_eint_register(VN_PIN_RADAR_OUT, PIN_RISING_EDGE, PIN_PULL_PU, NULL, Vn_Radar_TriggerCallback);

    ret = ql_eint_enable_wakeup(VN_PIN_RADAR_OUT, PIN_RISING_EDGE);
    if (Callback != NULL)
    {
        Vn_WakeupUserCallback = Callback;
    }

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

/**
 * @brief Disable trigger radar module wake up
 * @param None
 * @return 0: success; -1: fail
 */
int32_t Vn_Radar_DisableTriggerWakeUp(void)
{
    int32_t ret;

    ret = ql_eint_disable_wakeup(VN_PIN_RADAR_OUT);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

/**
 * @brief Get target distance
 * @param[out] pDistance: target distance, util: 0.1m
 * @param[out] pHadTarget: whether had target
 * @return 0: success; -1: fail
 */
int32_t Vn_Radar_GetTragetDistance(int32_t *pDistance, bool *pHadTarget)
{
    int32_t ret;

    ret = Vn_XenD101H_GetTargetDistance(pDistance);
    
    if((0 == ret) && (*pDistance >= 0))
    {
        *pHadTarget = true;
    }
    else
    {
        *pHadTarget = false;
    }

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

/**
 * @brief Get trigger distance
 * @param[out] pDistance: trigger distance, util: 0.1m
 * @return 0: success; -1: fail
 */
int32_t Vn_Radar_GetTriggerDistance(uint32_t* pDistance)
{
    return Vn_XenD101H_GetRadarMaxDistance(pDistance);
}

/**
 * @brief Set trigger distance
 * @param Distance: trigger distance, util: 0.1m
 * @return 0: success; -1: fail
 */
int32_t Vn_Radar_SetTriggerDistance(uint32_t Distance)
{
    return Vn_XenD101H_SetRadarMaxDistance(Distance);
}

bool Vn_Radar_HadDetectedTarget(void)
{
    PIN_LEVEL_E level = PIN_LEVEL_LOW;
    return ql_gpio_get_level(VN_PIN_RADAR_OUT, &level) == 0 && level == PIN_LEVEL_HIGH;
}