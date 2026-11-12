#ifndef __VN_PLAT_H__
#define __VN_PLAT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ql_adc.h"
#include "ql_gpio.h"
#include "ql_iic.h"
#include "ql_rtos.h"
#include "ql_spi.h"
#include "ql_type.h"
#include "ql_uart.h"

#include "vn_plat_pin_def.h"

/*
 * Common return codes used by the Venus application layer.
 */
#define VN_RET_OK      0
#define VN_RET_ERROR   (-1)

/*
 * System tick in milliseconds.
 */
#define VN_RTOS_SYSTEM_MS   (ql_rtos_get_systicks())

/*
 * EasyFlash key names used by the application.
 */
#define VN_PARAM_KEY_WIFI_SSID       "2"
#define VN_PARAM_KEY_WIFI_PASSWORD   "3"
#define VN_PARAM_KEY_FACTORY_PARAMS  "5"
#define VN_PARAM_KEY_NET_TYPE        "6"

typedef enum
{
    VN_NET_TYPE_CELLULAR = 0,
    VN_NET_TYPE_WIFI = 1
} Vn_NetType_Typedef;

typedef enum
{
    VN_PROFILE_IDX_XIAOZHI_OTA = 1,
    VN_PROFILE_IDX_XIAOZHI_CHAT = 2,
    VN_PROFILE_IDX_XIAOZHI_HTTP = 3,
    VN_PROFILE_IDX_XIAOZHI_DNS = 4,
    VN_PROFILE_IDX_XIAOZHI_MAX = 5
} Vn_ProfileIdx_Typedef;

#endif /* __VN_PLAT_H__ */
