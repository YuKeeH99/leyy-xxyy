#include "vn_websocket.h"
#include "vn_websocket_cell.h"
#include "vn_websocket_wifi.h"

int32_t Vn_Websocket_Instance(Vn_Websocket_Typedef *Instance, Vn_NetType_Typedef NetType)
{
    if (Instance == NULL)
    {
        return -1;
    }

    if (NetType == VN_NET_TYPE_WIFI)
    {
        Instance->Init = Vn_WebsocketWifi_Init;
        Instance->DeInit = Vn_WebsocketWifi_DeInit;
        Instance->Connect = Vn_WebsocketWifi_Connect;
        Instance->DisConnect = Vn_WebsocketWifi_Disconnect;
        Instance->Send = Vn_WebsocketWifi_Send;
        Instance->IsConnected = Vn_WebsocketWifi_IsConnected;
        Instance->Poll = Vn_WebsocketWifi_Poll;
        return 0;
    }

    if (NetType == VN_NET_TYPE_CELLULAR)
    {
        Instance->Init = Vn_WebsocketCell_Init;
        Instance->DeInit = Vn_WebsocketCell_DeInit;
        Instance->Connect = Vn_WebsocketCell_Connect;
        Instance->DisConnect = Vn_WebsocketCell_Disconnect;
        Instance->Send = Vn_WebsocketCell_Send;
        Instance->IsConnected = Vn_WebsocketCell_IsConnected;
        Instance->Poll = Vn_WebsocketCell_Poll;
        return 0;
    }
    return -1;
}
