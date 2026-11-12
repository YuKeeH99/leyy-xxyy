#include "vn_http.h"

#include "vn_http_cell.h"
#include "vn_http_wifi.h"

int32_t Vn_Http_Instance(Vn_Http_Typedef *Instance, Vn_NetType_Typedef NetType)
{
    if (Instance == NULL)
    {
        return -1;
    }

    if (NetType == VN_NET_TYPE_WIFI)
    {
        Instance->Init = Vn_HttpWifi_Init;
        Instance->ClientPostSync = Vn_HttpWifi_ClientPostSync;
        Instance->ClientGetSync = Vn_HttpWifi_ClientGetSync;
        Instance->DeInit = Vn_HttpWifi_DeInit;
        return 0;
    }

    if (NetType == VN_NET_TYPE_CELLULAR)
    {
        Instance->Init = Vn_HttpCell_Init;
        Instance->ClientPostSync = Vn_HttpCell_ClientPostSync;
        Instance->ClientGetSync = Vn_HttpCell_ClientGetSync;
        Instance->DeInit = Vn_HttpCell_DeInit;
        return 0;
    }

    return -1;
}
