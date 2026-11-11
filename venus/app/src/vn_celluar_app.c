#include <stdio.h>
#include "ql_application.h"
#include "ql_rtos.h"


#include "vn_plat.h"

#include "ql_data_call.h"
#include "ql_sim.h"

#define VN_CELLULAR_APP_STATE_SIM_CHECK       0
#define VN_CELLULAR_APP_STATE_NETWORK_CHECK   1
#define VN_CELLULAR_APP_STATE_DATA_CALL_START 2
#define VN_CELLULAR_APP_STATE_DATA_CALL_CHECK 3
#define VN_CELLULAR_APP_STATE_IDLE            4

static struct ql_data_call_info info;

static uint32 Vn_CellularAppState = VN_CELLULAR_APP_STATE_SIM_CHECK;

uint32_t Vn_Celluar_GetAppState(void)
{
    return Vn_CellularAppState;
}

void Vn_Celluar_App(void *argv)
{
    int ret = 0;
    uint8_t imsi_buff[256] = {0};
    uint32_t sim_status = 0;
    // get sim status
    ret = ql_sim_get_cur_SimId();

    VN_LOG_I("current sim id: %d", ret);

    while(1)
    {
        switch (Vn_CellularAppState)
        {
            case VN_CELLULAR_APP_STATE_SIM_CHECK:
            {
                ret = ql_sim_get_card_status(&sim_status);
                VN_LOG_I("sim status: %d", sim_status);
                if(sim_status == QL_SIM_STATUS_READY)
                {
                    ret = ql_sim_get_imsi(imsi_buff, sizeof(imsi_buff));
                    VN_LOG_I("imsi: %s", imsi_buff);
                    Vn_CellularAppState = VN_CELLULAR_APP_STATE_NETWORK_CHECK;
                }
                else
                {
                    VN_LOG_E("sim not ready, ret: %d", ret);
                }
            }
            break;
            case VN_CELLULAR_APP_STATE_NETWORK_CHECK:
                ret = ql_network_register_wait(5);
                if (ret == 0)
                {
                    VN_LOG_I("network register ok");
                    Vn_CellularAppState = VN_CELLULAR_APP_STATE_DATA_CALL_START;
                }
                else
                {
                    VN_LOG_E("network not ready, ret: %d", ret);
                }
            break;
            case VN_CELLULAR_APP_STATE_DATA_CALL_START:
            {
                VN_LOG_I("start data call");
                ql_set_data_call_asyn_mode(0, NULL);
                ql_set_auto_connect(1, true);
                ret = ql_start_data_call(1, 0, NULL, NULL, NULL, 0);
                if(ret != 0)
                {
                    VN_LOG_E("ql_get_data_call_info failed");
                }
                else
                {
                    Vn_CellularAppState = VN_CELLULAR_APP_STATE_DATA_CALL_CHECK;
                }
            }
            break;
            case VN_CELLULAR_APP_STATE_DATA_CALL_CHECK:
            {
                VN_LOG_I("check data call status");
                if(ql_get_data_call_info(1, 0, &info) == 0)
                {
                    if(info.v4.state == 1)
                    {
                        VN_LOG_I("data call success, ip is %s", inet_ntoa(info.v4.addr.ip));
                        Vn_CellularAppState = VN_CELLULAR_APP_STATE_IDLE;
                    }
                }
            }
            break;
            case VN_CELLULAR_APP_STATE_IDLE:

            break;
        default:
            break;
        }

        ql_rtos_task_sleep_ms(1000);
    }
}
