#include "vn_websocket_wifi.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ql_rtos.h"
#include "vn_log.h"
#include "vn_utils.h"

#define VN_WS_WIFI_QUEUE_DEPTH 20
#define VN_WS_WIFI_QUEUE_ITEM_SIZE 8U
#define VN_WS_WIFI_RECV_TIMEOUT_MS 10U

typedef void (*Vn_Wifi_WsReadCallback_Typedef)(uint8_t *pData, uint32_t Size, uint8_t MsgType);

extern int32_t Vn_Wifi_ConfigWsServer(char *Url, char *Header,
                                      Vn_Wifi_WsReadCallback_Typedef WsReadCb);
extern int32_t Vn_Wifi_ConnectWsServer(void);
extern int32_t Vn_Wifi_DisConnectWsServer(void);
extern int32_t Vn_Wifi_SendToWsServer(char *pData, uint32_t DataLen, uint32_t DataType);

typedef struct
{
    uint8_t Type;
    uint32_t Len;
    uint8_t Data[1024];
} Vn_WsMsg_Typedef;

typedef struct
{
    uint8_t *MsgPtr;
    uint32_t Size;
} Vn_WebsocketWifiQueueMsg;

static ql_queue_t Vn_MsgQueue;
static bool Vn_IsConnected;
static Vn_Websocket_MsgHandlerFunc Vn_WebsocketWifi_UserMsgHandler;

static int32_t Vn_WebsocketWifi_QueueCreate(void)
{
    if (Vn_MsgQueue != NULL)
    {
        (void)ql_rtos_queue_delete(Vn_MsgQueue);
        Vn_MsgQueue = NULL;
    }
    return (ql_rtos_queue_create(&Vn_MsgQueue, VN_WS_WIFI_QUEUE_ITEM_SIZE,
                                 VN_WS_WIFI_QUEUE_DEPTH) == 0) ? 0 : -1;
}

static int32_t Vn_WebsocketWifi_QueueRelease(const void *data, uint32_t size, uint32_t timeout)
{
    Vn_WebsocketWifiQueueMsg msg;

    if ((Vn_MsgQueue == NULL) || (data == NULL) || (size == 0U))
    {
        return -1;
    }

    msg.MsgPtr = (uint8_t *)malloc(size);
    if (msg.MsgPtr == NULL)
    {
        return -1;
    }
    memcpy(msg.MsgPtr, data, size);
    msg.Size = size;

    if (ql_rtos_queue_release(Vn_MsgQueue, VN_WS_WIFI_QUEUE_ITEM_SIZE,
                              (u8 *)&msg, timeout) != 0)
    {
        free(msg.MsgPtr);
        return -1;
    }
    return 0;
}

static int32_t Vn_WebsocketWifi_QueueWait(void *data, int32_t max_len, uint32_t timeout)
{
    Vn_WebsocketWifiQueueMsg msg;
    uint32_t copy_len;

    if ((Vn_MsgQueue == NULL) || (data == NULL) || (max_len <= 0))
    {
        return -1;
    }
    if (ql_rtos_queue_wait(Vn_MsgQueue, (u8 *)&msg,
                           VN_WS_WIFI_QUEUE_ITEM_SIZE, timeout) != 0)
    {
        return -1;
    }

    copy_len = (msg.Size < (uint32_t)max_len) ? msg.Size : (uint32_t)max_len;
    if (copy_len != 0U)
    {
        memcpy(data, msg.MsgPtr, copy_len);
    }
    free(msg.MsgPtr);
    return (int32_t)copy_len;
}

void Vn_Wifi_WsReadCallback(uint8_t *pData, uint32_t Size, uint8_t MsgType)
{
    Vn_WsMsg_Typedef msg;

    memset(&msg, 0, sizeof(msg));
    if (MsgType == 0U)
    {
        msg.Type = VN_WEBSOCKET_MSG_TEXT;
        msg.Len = Size;
    }
    else if (MsgType == 1U)
    {
        if ((Size > UINT16_MAX) || !Vn_HexStringToBin((char *)pData, (uint16_t)Size, pData))
        {
            return;
        }
        msg.Len = Size >> 1;
        msg.Type = VN_WEBSOCKET_MSG_BINARY;
    }

    if (msg.Len > sizeof(msg.Data))
    {
        msg.Len = sizeof(msg.Data);
        VN_LOG_W("websocket data too long, truncate");
    }
    if (msg.Len != 0U)
    {
        memcpy(msg.Data, pData, msg.Len);
    }
    (void)Vn_WebsocketWifi_QueueRelease(&msg, sizeof(msg), VN_WS_WIFI_RECV_TIMEOUT_MS);
}

void Vn_WebsocketWifi_MsgHandler(uint8_t MsgType, uint8_t *pData, uint32_t Len)
{
    if (Vn_WebsocketWifi_UserMsgHandler != NULL)
    {
        Vn_WebsocketWifi_UserMsgHandler(MsgType, pData, Len);
    }
}

int32_t Vn_WebsocketWifi_Init(Vn_Websocket_MsgHandlerFunc Handler)
{
    Vn_WebsocketWifi_UserMsgHandler = Handler;
    if (Vn_WebsocketWifi_QueueCreate() != 0)
    {
        VN_LOG_E("websocket queue create failed");
        return -1;
    }
    return 0;
}

int32_t Vn_WebsocketWifi_DeInit(void)
{
    return 0;
}

int32_t Vn_WebsocketWifi_Connect(uint32_t LocalIP, uint16_t LocalPort,
                                 int8_t *Url, int8_t *Headers)
{
    uint8_t *header;
    size_t header_len;
    int32_t ret;

    (void)LocalIP;
    (void)LocalPort;
    if ((Url == NULL) || (Headers == NULL))
    {
        return -1;
    }

    header_len = strlen((const char *)Headers);
    header = (uint8_t *)malloc(header_len * 2U + 1U);
    if (header == NULL)
    {
        VN_LOG_E("websocket header malloc failed");
        return -1;
    }

    Vn_CrLfToString((const uint8_t *)Headers, (uint32_t)header_len, header);
    ret = Vn_Wifi_ConfigWsServer((char *)Url, (char *)header, Vn_Wifi_WsReadCallback);
    free(header);
    VN_LOG_D("Vn_Wifi_ConfigWsServer ret:%d", ret);

    (void)Vn_Wifi_DisConnectWsServer();
    ret = Vn_Wifi_ConnectWsServer();
    VN_LOG_D("Vn_Wifi_ConnectWsServer ret:%d", ret);
    Vn_IsConnected = (ret == 0);
    return (ret == 0) ? 0 : -1;
}

bool Vn_WebsocketWifi_IsConnected(void)
{
    return Vn_IsConnected;
}

int32_t Vn_WebsocketWifi_Disconnect(void)
{
    Vn_IsConnected = false;
    (void)Vn_Wifi_DisConnectWsServer();
    return 0;
}

int32_t Vn_WebsocketWifi_Send(uint8_t *pData, uint32_t Len, uint8_t MsgType)
{
    uint8_t *out;
    uint32_t out_len;
    uint32_t data_type;
    int32_t ret;

    if ((pData == NULL) && (Len != 0U))
    {
        return -1;
    }

    if (MsgType == VN_WEBSOCKET_MSG_TEXT)
    {
        VN_LOG_D("txt data, len %u", (unsigned int)Len);
        out = (uint8_t *)malloc((size_t)Len * 2U + 1U);
        if (out == NULL)
        {
            VN_LOG_E("websocket send malloc failed");
            return 0;
        }
        Vn_CrLfToString(pData, Len, out);
        out_len = (uint32_t)strlen((const char *)out);
        data_type = 0;
    }
    else
    {
        if (Len > UINT16_MAX)
        {
            return -1;
        }
        VN_LOG_D("send binary data, len %u", (unsigned int)Len);
        out_len = Len * 2U;
        out = (uint8_t *)malloc((size_t)out_len + 1U);
        if (out == NULL)
        {
            VN_LOG_E("websocket send malloc failed");
            return 0;
        }
        (void)Vn_BinToHexString(pData, (uint16_t)Len, (char *)out);
        data_type = 1;
    }

    ret = Vn_Wifi_SendToWsServer((char *)out, out_len, data_type);
    free(out);
    if (ret < -100)
    {
        Vn_IsConnected = false;
        VN_LOG_E("Vn_Wifi_SendToWsServer ret:%d", ret);
        return -1;
    }
    return 0;
}

int32_t Vn_WebsocketWifi_Poll(uint32_t Timeout)
{
    Vn_WsMsg_Typedef msg;

    (void)Timeout;
    if (Vn_WebsocketWifi_QueueWait(&msg, sizeof(msg), 0) > 0)
    {
        Vn_WebsocketWifi_MsgHandler(msg.Type, msg.Data, msg.Len);
    }
    return 0;
}
