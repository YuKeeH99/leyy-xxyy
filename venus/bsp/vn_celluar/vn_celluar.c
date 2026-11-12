#include "vn_celluar.h"

#include "ql_data_call.h"
#include "ql_rtos.h"
#include "vn_log.h"

void Vn_DataCall_Status_Callback(int ProfileIdx, int Status)
{
    VN_LOG_I("profile(%d) status: %d", ProfileIdx, Status);
}

int Vn_DataCall_Start(int Pdp_id, int32_t TimeOut)
{
    VN_LOG_I("doing network activing ...");

    if (ql_network_register_wait((u32)TimeOut) != 0)
    {
        VN_LOG_E("network register timeout");
        return -1;
    }

    VN_LOG_I("wait for network register done");

    ql_wan_start(Vn_DataCall_Status_Callback);
    ql_set_auto_connect(Pdp_id, true);

    return ql_start_data_call(Pdp_id, 0, NULL, NULL, NULL, 0);
}
