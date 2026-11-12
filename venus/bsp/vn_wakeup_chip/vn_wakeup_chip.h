#ifndef BSP_VN_WAKEUP_CHIP_VN_WAKEUP_CHIP_H_
#define BSP_VN_WAKEUP_CHIP_VN_WAKEUP_CHIP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "ql_uart.h"

typedef void (*Vn_WakeupChip_WakeupCallback)(void);

extern int32_t Vn_WakeupChip_Init(void);
extern int32_t Vn_WakeupChip_EnableTrigger(Vn_WakeupChip_WakeupCallback Callback);
extern int32_t Vn_WakeupChip_DisableTrigger(void);
extern void Vn_WakeupChip_UartCallback(QL_UART_PORT_NUMBER_E Port, void * Para);

#endif /* BSP_VN_WAKEUP_CHIP_VN_WAKEUP_CHIP_H_ */
