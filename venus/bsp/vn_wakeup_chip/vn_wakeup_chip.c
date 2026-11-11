#include "vn_wakeup_chip.h"

#include "ql_uart.h"

#include "ql_gpio.h"

#define VN_WAKEUP_CHIP_UART_PORT_BAUD_RATE      QL_UART_BAUD_115200

static ql_uart_config_t Vn_UartConfig = 
{
    .baudrate = VN_WAKEUP_CHIP_UART_PORT_BAUD_RATE,
    .data_bit = QL_UART_DATABIT_8,
    .stop_bit = 1,
    .parity_bit = QL_UART_PARITY_NONE,
    .flow_ctrl = QL_FC_NONE
};

static Vn_WakeupChip_WakeupCallback Vn_WakeupUserCallback;
static uint8_t Vn_UartRxBuffer[256];
static uint16_t Vn_HadRecved = 0;
static const uint8_t Vn_WakeupChip_WakeUpFrame[] = {0x56, 0x4E, 0x06, 0x11, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x53};

void Vn_WakeupChip_UartCallback(QL_UART_PORT_NUMBER_E Port, void *Para)
{
    int recv_len = 0;
    char recv_data[256] = {0};

    recv_len = ql_uart_read(VN_UART_PORT_WAKEUP_CHIP, (uint8_t *)recv_data, sizeof(recv_data));

    if(recv_len > 0)
    {
        if(Vn_HadRecved == 0)
        {
            // find frame header 0x56, 0x4E
            for(int i = 0; i < recv_len - 1; i++)
            {
                if(recv_data[i] == 0x56 && recv_data[i + 1] == 0x4E)
                {
                    Vn_HadRecved = recv_len - i;
                    memcpy(Vn_UartRxBuffer, recv_data + i, recv_len - i);
                    break;
                }
            }
        }

        if(Vn_HadRecved >= sizeof(Vn_WakeupChip_WakeUpFrame))
        {
            if(memcmp(Vn_UartRxBuffer, Vn_WakeupChip_WakeUpFrame, sizeof(Vn_WakeupChip_WakeUpFrame)) == 0)
            {
                VN_LOG_I("Wakeup-chip wake up");
            }
            memset(Vn_UartRxBuffer, 0, Vn_HadRecved);
            Vn_HadRecved = 0;
        }
    }
    VN_LOG_D("Wake chip recv data: %.*s", recv_len, recv_data);
}

static int32_t Vn_WakeupChip_UartInit(QL_UART_BAUD_E Baudrate, uart_callback Callback)
{
    int32_t ret = 0;

    ret = ql_uart_open(VN_UART_PORT_WAKEUP_CHIP, Baudrate, QL_FC_NONE);

    ret = ql_uart_set_dcbconfig(VN_UART_PORT_WAKEUP_CHIP, &Vn_UartConfig);

     // register uart callback
    ql_uart_register_cb(VN_UART_PORT_WAKEUP_CHIP, Callback);

    return (ret != 0 ? VN_RET_ERROR : VN_RET_OK);
}

static int32_t Vn_WakeupChip_UartRead(uint8_t *pData, uint32_t Length)
{
    if(NULL == pData)
    {
        return VN_RET_ERROR;
    }

    return ql_uart_read(VN_UART_PORT_WAKEUP_CHIP, pData, Length);
}


/**
 * @brief Init wakeup chip
 * @param None
 * @return 0: success; -1: fail
 */
int32_t Vn_WakeupChip_Init(void)
{
    int32_t ret = 0;

    // radar out
    ret = ql_gpio_init(VN_PIN_WAKEUP_CHIP_INT,
                       PIN_DIRECTION_IN,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);
    
    ret = Vn_WakeupChip_UartInit(VN_WAKEUP_CHIP_UART_PORT_BAUD_RATE, Vn_WakeupChip_UartCallback);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

static void Vn_WakeupChip_TriggerCallback(void)
{
    VN_LOG_D("wakeup chip trigger callback");
    if (Vn_WakeupUserCallback != NULL)
    {
        Vn_WakeupUserCallback();
    }
    
    // Re-enable
    ql_eint_enable_wakeup(VN_PIN_WAKEUP_CHIP_INT, PIN_RISING_EDGE);
}

/**
 * @brief Enable trigger wake up
 * @param None
 * @return 0: success; -1: fail
 */
int32_t Vn_WakeupChip_EnableTrigger(Vn_WakeupChip_WakeupCallback Callback)
{
    int32_t ret;

    ret = ql_eint_register(VN_PIN_WAKEUP_CHIP_INT, PIN_RISING_EDGE, PIN_PULL_PU, NULL, Vn_WakeupChip_TriggerCallback);

    ret = ql_eint_enable_wakeup(VN_PIN_WAKEUP_CHIP_INT, PIN_RISING_EDGE);
    if (Callback != NULL)
    {
        Vn_WakeupUserCallback = Callback;
    }

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

/**
 * @brief Disable trigger wake up
 * @param None
 * @return 0: success; -1: fail
 */
int32_t Vn_WakeupChip_DisableTrigger(void)
{
    int32_t ret;

    ret = ql_eint_disable_wakeup(VN_PIN_WAKEUP_CHIP_INT);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}