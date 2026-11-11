#include "vn_websocket_cell.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "def.h"
#include "inet.h"
#include "mongoose.h"
#include "ql_rtos.h"
#include "sockets.h"
#include "vn_log.h"

typedef struct
{
    struct mg_mgr Mgr;
    struct mg_connection *Conn;
    bool IsConnected;
    Vn_Websocket_MsgHandlerFunc MsgHandler;
} Vn_WebsocketObj_Typedef;

static Vn_WebsocketObj_Typedef Vn_WebsocketObj;

extern int inet_aton(const char *cp, struct in_addr *inp);

static void Vn_WebsocketCell_EventHandler(struct mg_connection *c, int ev, void *ev_data)
{
    struct mg_ws_message *wm;
    uint8_t msg_type;

    if (ev == MG_EV_OPEN)
    {
        VN_LOG_D("websocket open");
        c->is_hexdumping = false;
        return;
    }

    if (ev == MG_EV_ERROR)
    {
        MG_LOG(MG_LL_ERROR, ("%lu %s", c->id, (char *)ev_data));
        VN_LOG_E("websocket error");
        if (c->fn_data != NULL)
        {
            *(bool *)c->fn_data = true;
        }
    }
    else if (ev == MG_EV_WS_OPEN)
    {
        VN_LOG_I("websocket connect success");
        Vn_WebsocketObj.IsConnected = true;
        if (c->fn_data != NULL)
        {
            *(bool *)c->fn_data = true;
        }
        return;
    }
    else if (ev == MG_EV_WS_MSG)
    {
        wm = (struct mg_ws_message *)ev_data;
        if (wm->flags == 0x82U)
        {
            msg_type = VN_WEBSOCKET_MSG_BINARY;
        }
        else if (wm->flags == 0x81U)
        {
            msg_type = VN_WEBSOCKET_MSG_TEXT;
        }
        else
        {
            return;
        }

        if ((Vn_WebsocketObj.MsgHandler != NULL) && (wm->data.len != 0U))
        {
            Vn_WebsocketObj.MsgHandler(msg_type, (uint8_t *)wm->data.buf,
                                       (uint32_t)wm->data.len);
        }
        return;
    }
    else if (ev != MG_EV_CLOSE)
    {
        return;
    }

    if (c->fn_data != NULL)
    {
        *(bool *)c->fn_data = true;
    }
    Vn_WebsocketObj.IsConnected = false;
    VN_LOG_I("websocket state %s", (ev == MG_EV_CLOSE) ? "closed" : "error");
    Vn_WebsocketObj.Conn = NULL;
}

static uint32_t Vn_WebsocketCell_ByteSwap32(uint32_t value)
{
    return ((value & 0x000000ffU) << 24) |
           ((value & 0x0000ff00U) << 8) |
           ((value & 0x00ff0000U) >> 8) |
           ((value & 0xff000000U) >> 24);
}

int32_t Vn_WebsocketCell_Init(Vn_Websocket_MsgHandlerFunc Handler)
{
    memset(&Vn_WebsocketObj, 0, sizeof(Vn_WebsocketObj));
    mg_mgr_init(&Vn_WebsocketObj.Mgr);
    mg_log_level = MG_LL_ERROR;
    Vn_WebsocketObj.MsgHandler = Handler;
    return 0;
}

int32_t Vn_WebsocketCell_DeInit(void)
{
    mg_mgr_free(&Vn_WebsocketObj.Mgr);
    Vn_WebsocketObj.Conn = NULL;
    Vn_WebsocketObj.IsConnected = false;
    return 0;
}

int32_t Vn_WebsocketCell_Connect(uint32_t LocalIP, uint16_t LocalPort,
                                 int8_t *Url, int8_t *Headers)
{
    static char ip[16];
    Vn_DataCallInfo_t info;
    struct mg_mgr *mgr = &Vn_WebsocketObj.Mgr;
    uint32_t local_ip;
    uint16_t local_port;
    uint8_t domain[128] = {0};
    bool done = false;
    char *port;

    (void)LocalIP;
    (void)LocalPort;

    if (Url == NULL)
    {
        return -1;
    }
    if (Headers == NULL)
    {
        Headers = (int8_t *)"";
    }

    VN_LOG_D("url: %s", Url);
    if (Vn_Extract_Domain_From_Url((const char *)Url, (char *)domain) == 0U)
    {
        return -1;
    }

    port = strchr((char *)domain, ':');
    if (port != NULL)
    {
        *port = '\0';
    }

    if (Vn_Parse_DNS(ip, domain) != 0)
    {
        if (strchr(ip, '.') == NULL)
        {
            VN_LOG_E("websocket dns parse error");
            return -1;
        }
        VN_LOG_W("websocket use previous dns ip %s", ip);
    }
    VN_LOG_D("websocket ip %s", ip);

    if (Vn_Cellular_StartDataCall(2, 5) != 0)
    {
        VN_LOG_E("start data call error");
        return -1;
    }
    if ((Vn_Cellular_GetDataCallInfo(2, &info) != 0) || (info.State != 1))
    {
        VN_LOG_E("get data call info error");
        return -1;
    }

    local_port = ql_soc_generate_port();
    local_ip = Vn_WebsocketCell_ByteSwap32(info.InIpAddrv4.s_addr);
    memcpy(mgr->local_addr.ip, &local_ip, sizeof(local_ip));
    mgr->local_addr.port = htons(local_port);
    mgr->local_addr.is_ip6 = false;
    (void)inet_aton(ip, (struct in_addr *)&mgr->rem_ip4);
    VN_LOG_D("websocket ip %u, port %u ntohs(port) %u",
             (unsigned int)local_ip, (unsigned int)local_port,
             (unsigned int)ntohs(mgr->local_addr.port));

    if (Vn_WebsocketObj.Conn != NULL)
    {
        VN_LOG_D("WebSocket connection re-release");
        mg_close_conn(Vn_WebsocketObj.Conn);
        Vn_WebsocketObj.Conn = NULL;
    }

    Vn_WebsocketObj.IsConnected = false;
    VN_LOG_D("websocket headers: %s", Headers);
    Vn_WebsocketObj.Conn = mg_ws_connect(mgr, (const char *)Url,
                                         Vn_WebsocketCell_EventHandler, &done,
                                         "%s", (char *)Headers);
    if (Vn_WebsocketObj.Conn == NULL)
    {
        return -1;
    }

    while (!done)
    {
        if (Vn_WebsocketObj.Conn != NULL)
        {
            mg_mgr_poll(mgr, 1000);
        }
        ql_rtos_task_sleep_ms(100);
    }
    return 0;
}

bool Vn_WebsocketCell_IsConnected(void)
{
    return Vn_WebsocketObj.IsConnected;
}

int32_t Vn_WebsocketCell_Disconnect(void)
{
    if (Vn_WebsocketObj.Conn != NULL)
    {
        mg_close_conn(Vn_WebsocketObj.Conn);
        Vn_WebsocketObj.Conn = NULL;
    }
    Vn_WebsocketObj.IsConnected = false;
    return 0;
}

int32_t Vn_WebsocketCell_Poll(uint32_t Timeout)
{
    if ((Vn_WebsocketObj.Conn == NULL) || !Vn_WebsocketObj.IsConnected)
    {
        return 0;
    }
    mg_mgr_poll(&Vn_WebsocketObj.Mgr, (int)Timeout);
    return 0;
}

int32_t Vn_WebsocketCell_Send(uint8_t *pData, uint32_t Len, uint8_t MsgType)
{
    if (Vn_WebsocketObj.Conn == NULL)
    {
        VN_LOG_E("websocket not connected");
        return -1;
    }
    if ((MsgType != WEBSOCKET_OP_TEXT) && (MsgType != WEBSOCKET_OP_BINARY))
    {
        VN_LOG_E("websocket send message type error");
        return -1;
    }
    return (int32_t)mg_ws_send(Vn_WebsocketObj.Conn, pData, Len, MsgType);
}
