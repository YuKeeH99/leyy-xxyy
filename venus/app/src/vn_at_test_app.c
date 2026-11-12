#include "vn_at_test_app.h"

#include <string.h>

#include "ql_application.h"
#include "ql_func.h"
#include "ql_rtos.h"
#include "vn_cell_fota.h"
#include "vn_log.h"

Vn_OTA_AT_Params_TypeDef Vn_OTA_AT_Params;

void Vn_At_Cmd_App(void *arg)
{
    (void)arg;

    memset(&Vn_OTA_AT_Params, 0, sizeof(Vn_OTA_AT_Params));

    while (1)
    {
        if (Vn_OTA_AT_Params.url[0] == '\0')
        {
            ql_rtos_task_sleep_ms(1000);
            continue;
        }

        if (strlen(Vn_OTA_AT_Params.url) <= 8U)
        {
            VN_LOG_E("Please input correct firmware update url now is %s",
                     Vn_OTA_AT_Params.url);
        }
        else if (Vn_OTA_AT_Params.enable == 1)
        {
            VN_LOG_I("Start Cellura OTA");
            Vn_Fota_Firmware_Cellular(1, Vn_OTA_AT_Params.url, 0, NULL);
        }

        ql_rtos_task_sleep_ms(1000);
    }
}

application_init(Vn_At_Cmd_App, "Vn_At_Cmd_App", 10, 2);
