#include "vn_wifi.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ql_rtos.h"
#include "ql_gpio.h"
#include "ql_uart.h"

#include "at_chat.h"

#define VN_WIFI_BAUDRATE                 460800
#define VN_WIFI_URC_BUF_SIZE             4096
#define VN_WIFI_RECV_BUF_SIZE            8192
#define VN_WIFI_MUTEX_WAIT_FOREVER       0xFFFFFFFFu
#define VN_WIFI_WS_CONTEXT_ID            0
#define VN_WIFI_HTTP_METHOD_GET          2
#define VN_WIFI_HTTP_METHOD_POST         3
#define VN_WIFI_HTTP_DATA_FORMAT         2
#define VN_PIN_WIFI_CHIP_EN_LEVEL_VALID  PIN_LEVEL_LOW

static ql_mutex_t Vn_Wifi_Mutex = NULL;
static at_obj_t *Vn_At_Obj = NULL;
static Vn_Wifi_WebConfigCallback_Typedef Vn_Wifi_WebConfigUserCallback = NULL;
static Vn_Wifi_OtaCallback_Typedef Vn_Wifi_OtaUserCallback = NULL;
static Vn_Wifi_WsReadCallback_Typedef Vn_Wifi_WsReadUserCallback = NULL;

static uint8_t *HttpRespBuffPtr = NULL;
static uint32_t HttpRespBuffSize = 0;
static uint32_t HttpRespBuffHadRecvLen = 0;
static uint32_t Vn_Wifi_HttpBodyLength = 0;
static bool HttpRespBuffOverflow = false;

static uint8_t Vn_Wifi_ConId = 0;
static int8_t Vn_Wifi_RecvConIdState = 0;

static ql_uart_config_t Vn_UartConfig =
{
    .baudrate = VN_WIFI_BAUDRATE,
    .data_bit = QL_UART_DATABIT_8,
    .stop_bit = 1,
    .parity_bit = QL_UART_PARITY_NONE,
    .flow_ctrl = QL_FC_NONE
};

static void Vn_Wifi_UartInit(void)
{
    (void)ql_uart_open(VN_UART_PORT_WIFI, VN_WIFI_BAUDRATE, QL_FC_NONE);
    (void)ql_uart_set_dcbconfig(VN_UART_PORT_WIFI, &Vn_UartConfig);
    (void)ql_uart_register_cb(VN_UART_PORT_WIFI, NULL);
}

static unsigned int Vn_Wifi_UartSend(const void *Data, unsigned int Length)
{
    int ret = ql_uart_write_sync(VN_UART_PORT_WIFI, (uint8_t *)Data, (int)Length);

    return (ret < 0) ? 0u : (unsigned int)ret;
}

static unsigned int Vn_Wifi_UartRecv(void *Data, unsigned int Length)
{
    int ret = ql_uart_read(VN_UART_PORT_WIFI, (uint8_t *)Data, (int)Length);

    return (ret < 0) ? 0u : (unsigned int)ret;
}

static void Vn_Wifi_MutexLock(void)
{
    (void)ql_rtos_mutex_lock(Vn_Wifi_Mutex, VN_WIFI_MUTEX_WAIT_FOREVER);
}

static void Vn_Wifi_MutexUnlock(void)
{
    (void)ql_rtos_mutex_unlock(Vn_Wifi_Mutex);
}

void Vn_Wifi_AtDebug(const char *fmt, ...)
{
    (void)fmt;
}

static at_resp_code Vn_Wifi_WaitAtResult(at_context_t *ctx)
{
    uint32_t poll_count = 0;

    while (!at_work_is_finish(ctx))
    {
        if (poll_count++ >= 200)
        {
            (void)at_work_get_state(ctx);
            poll_count = 0;
        }
        ql_rtos_task_sleep_ms(10);
    }

    return at_work_get_result(ctx);
}

static at_resp_code Vn_Wifi_ExecuteAt(char *respbuf, int bufsize,
                                      at_attr_t *attr, const char *cmd,
                                      va_list args)
{
    char command[AT_MAX_CMD_LEN];
    at_context_t ctx;
    int command_len;

    if (Vn_At_Obj == NULL || cmd == NULL || respbuf == NULL || bufsize <= 0)
    {
        return AT_RESP_ERROR;
    }

    command_len = vsnprintf(command, sizeof(command), cmd, args);
    if (command_len < 0 || command_len >= (int)sizeof(command))
    {
        VN_LOG_E("AT command is too long:%d", command_len);
        return AT_RESP_ERROR;
    }

    respbuf[0] = '\0';
    at_context_init(&ctx, respbuf, (unsigned int)bufsize);
    at_context_attach(attr, &ctx);
    if (!at_exec_cmd(Vn_At_Obj, attr, "%s", command))
    {
        return AT_RESP_ERROR;
    }

    return Vn_Wifi_WaitAtResult(&ctx);
}

static at_resp_code at_send_cmd_sync(char *respbuf, int bufsize, int timeout,
                                     const char *cmd, ...)
{
    at_attr_t attr;
    at_resp_code code;
    va_list args;

    at_attr_deinit(&attr);
    attr.timeout = (unsigned short)timeout;
    attr.retry = 1;

    va_start(args, cmd);
    code = Vn_Wifi_ExecuteAt(respbuf, bufsize, &attr, cmd, args);
    va_end(args);
    return code;
}

static at_resp_code at_send_cmd_sync_by_attr(char *respbuf, int bufsize,
                                             const at_attr_t *input_attr,
                                             const char *cmd, ...)
{
    at_attr_t attr = *input_attr;
    at_resp_code code;
    va_list args;

    va_start(args, cmd);
    code = Vn_Wifi_ExecuteAt(respbuf, bufsize, &attr, cmd, args);
    va_end(args);
    return code;
}

static int32_t Vn_Wifi_AtResult(at_resp_code code)
{
    if (code == AT_RESP_OK)
    {
        return VN_RET_OK;
    }
    if (code == AT_RESP_TIMEOUT)
    {
        return -1;
    }
    return -2;
}

int32_t Vn_Wifi_WebConfigUrcHandler(at_urc_info_t *ctx)
{
    VN_LOG_D("web config urc received:%d bytes", ctx->urclen);
    if (Vn_Wifi_WebConfigUserCallback != NULL)
    {
        Vn_Wifi_WebConfigUserCallback(ctx->urcbuf);
    }
    return 0;
}

int32_t Vn_Wifi_OtaUrcHandler(at_urc_info_t *ctx)
{
    VN_LOG_D("ota urc: %.*s", ctx->urclen, ctx->urcbuf);
    if (Vn_Wifi_OtaUserCallback != NULL)
    {
        Vn_Wifi_OtaUserCallback(ctx->urcbuf);
    }
    return 0;
}

int32_t Vn_Wifi_WsClosetUrcHandler(at_urc_info_t *ctx)
{
    VN_LOG_D("ws close urc: %.*s", ctx->urclen, ctx->urcbuf);
    return 0;
}

int32_t Vn_Wifi_ClientConnectOkUrcHandler(at_urc_info_t *ctx)
{
    char *ptr = strstr(ctx->urcbuf, "con_id=");

    VN_LOG_D("client connect urc: %.*s", ctx->urclen, ctx->urcbuf);
    if (ptr == NULL)
    {
        Vn_Wifi_RecvConIdState = -1;
        return 0;
    }

    Vn_Wifi_ConId = (uint8_t)atoi(ptr + strlen("con_id="));
    Vn_Wifi_RecvConIdState = 1;
    VN_LOG_D("ConId:%u", Vn_Wifi_ConId);
    return 0;
}

static uint32_t Vn_Wifi_SkipCommaFields(const char *data, uint32_t count)
{
    const char *cursor = data;

    while (count-- > 0)
    {
        cursor = strchr(cursor, ',');
        if (cursor == NULL)
        {
            return 0;
        }
        cursor++;
    }

    return (uint32_t)(cursor - data);
}

int32_t Vn_Wifi_WsReadHandler(at_urc_info_t *ctx)
{
    int32_t type = 0;
    int32_t len = 0;
    uint32_t offset;
    uint32_t available;
    uint32_t payload_len;

    if (sscanf(ctx->urcbuf, "+QWSREAD:0,%d,%d", &type, &len) != 2 ||
        Vn_Wifi_WsReadUserCallback == NULL || len <= 2)
    {
        return 0;
    }

    offset = Vn_Wifi_SkipCommaFields(ctx->urcbuf, 3);
    if (offset == 0 || offset >= (uint32_t)ctx->urclen)
    {
        return 0;
    }

    available = (uint32_t)ctx->urclen - offset;
    payload_len = (uint32_t)(len - 2);
    if (available < payload_len)
    {
        return (int32_t)(payload_len - available);
    }

    Vn_Wifi_WsReadUserCallback((uint8_t *)ctx->urcbuf + offset,
                               payload_len, (uint8_t)type);
    return 0;
}

static void Vn_Wifi_HttpRespFinish(void)
{
    if (HttpRespBuffPtr != NULL && HttpRespBuffSize > 0)
    {
        uint32_t index = HttpRespBuffHadRecvLen;

        if (index >= HttpRespBuffSize)
        {
            index = HttpRespBuffSize - 1;
        }
        HttpRespBuffPtr[index] = '\0';
    }

    HttpRespBuffPtr = NULL;
    HttpRespBuffSize = 0;
    HttpRespBuffHadRecvLen = 0;
}

static void Vn_Wifi_HttpRespAppend(const uint8_t *data, uint32_t len)
{
    uint32_t room;

    if (HttpRespBuffPtr == NULL || HttpRespBuffSize == 0 || len == 0)
    {
        return;
    }

    if (HttpRespBuffHadRecvLen >= HttpRespBuffSize - 1)
    {
        HttpRespBuffOverflow = true;
        return;
    }

    room = HttpRespBuffSize - 1 - HttpRespBuffHadRecvLen;
    if (len > room)
    {
        len = room;
        HttpRespBuffOverflow = true;
    }

    memcpy(HttpRespBuffPtr + HttpRespBuffHadRecvLen, data, len);
    HttpRespBuffHadRecvLen += len;
}

int32_t Vn_Wifi_HttpUrcHandler(at_urc_info_t *ctx)
{
    unsigned int expected = 0;
    uint32_t offset = 0;
    uint32_t available = 0;
    bool is_body = false;

    if (sscanf(ctx->urcbuf, "+QHTTPURC,header,%u", &expected) == 1)
    {
        offset = Vn_Wifi_SkipCommaFields(ctx->urcbuf, 3);
        if (offset == 0 || offset > (uint32_t)ctx->urclen)
        {
            return 0;
        }
        available = (uint32_t)ctx->urclen - offset;
        Vn_Wifi_HttpBodyLength = 0;
        VN_LOG_D("HTTP header len:%u, received:%u",
                 expected, (unsigned int)available);
    }
    else if (sscanf(ctx->urcbuf, "+QHTTPURC,body,%*d,%u", &expected) == 1)
    {
        offset = Vn_Wifi_SkipCommaFields(ctx->urcbuf, 5);
        if (offset == 0 || offset > (uint32_t)ctx->urclen)
        {
            return 0;
        }
        available = ((uint32_t)ctx->urclen >= offset + 2) ?
                    ((uint32_t)ctx->urclen - offset - 2) : 0;
        is_body = true;
        VN_LOG_D("HTTP body len:%u, received:%u",
                 expected, (unsigned int)available);
    }
    else
    {
        if (strstr(ctx->urcbuf, "OK") != NULL)
        {
            VN_LOG_D("HTTP body received:%u bytes",
                     (unsigned int)Vn_Wifi_HttpBodyLength);
            Vn_Wifi_HttpRespFinish();
        }
        return 0;
    }

    if (available < expected)
    {
        return (int32_t)(expected - available);
    }

    if (expected > 0)
    {
        Vn_Wifi_HttpRespAppend((const uint8_t *)ctx->urcbuf + offset, expected);
        if (is_body)
        {
            Vn_Wifi_HttpBodyLength += expected;
        }
    }

    return 0;
}

static const at_adapter_t at_adapter =
{
    .lock = Vn_Wifi_MutexLock,
    .unlock = Vn_Wifi_MutexUnlock,
    .write = Vn_Wifi_UartSend,
    .read = Vn_Wifi_UartRecv,
    .error = NULL,
    .debug = Vn_Wifi_AtDebug,
#if AT_URC_WARCH_EN
    .urc_bufsize = VN_WIFI_URC_BUF_SIZE,
#endif
    .recv_bufsize = VN_WIFI_RECV_BUF_SIZE
};

static const urc_item_t urc_table[] =
{
    {.prefix = "+SKTCLIENT:OK", .endmark = '#',  .handler = Vn_Wifi_ClientConnectOkUrcHandler},
    {.prefix = "+QAPNETURC:",   .endmark = '\n', .handler = Vn_Wifi_WebConfigUrcHandler},
    {.prefix = "+QOTAURC:",     .endmark = '\n', .handler = Vn_Wifi_OtaUrcHandler},
    {.prefix = "+QWSREAD:",     .endmark = '\n', .handler = Vn_Wifi_WsReadHandler},
    {.prefix = "+QHTTPURC",     .endmark = '\n', .handler = Vn_Wifi_HttpUrcHandler},
    {.prefix = "+QWSCLOSEURC:", .endmark = '\n', .handler = Vn_Wifi_WsClosetUrcHandler}
};

void Vn_Wifi_Process(void)
{
    if (Vn_At_Obj != NULL)
    {
        at_obj_process(Vn_At_Obj);
    }
}

int32_t Vn_Wifi_Init(void)
{
    int32_t ret;

    ret = ql_rtos_mutex_create(&Vn_Wifi_Mutex);
    if (ret != QUEC_SUCCESS)
    {
        VN_LOG_E("%s, mutex create ret:%d", __func__, ret);
        return VN_RET_ERROR;
    }

    (void)ql_gpio_init(VN_PIN_WIFI_CHIP_EN, PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE, VN_PIN_WIFI_CHIP_EN_LEVEL_VALID);
    Vn_Wifi_UartInit();
    ret = Vn_Wifi_Enable();

    Vn_At_Obj = at_obj_create(&at_adapter);
    if (Vn_At_Obj == NULL)
    {
        VN_LOG_E("AT object create failed");
        return VN_RET_ERROR;
    }
    at_obj_set_urc(Vn_At_Obj, urc_table,
                   sizeof(urc_table) / sizeof(urc_table[0]));

    return (ret == VN_RET_OK) ? VN_RET_OK : VN_RET_ERROR;
}

int32_t Vn_Wifi_Enable(void)
{
    int32_t ret = ql_gpio_set_level(VN_PIN_WIFI_CHIP_EN,
                                    VN_PIN_WIFI_CHIP_EN_LEVEL_VALID);

    return (ret == 0) ? VN_RET_OK : VN_RET_ERROR;
}

int32_t Vn_Wifi_Disable(void)
{
    int32_t level = (VN_PIN_WIFI_CHIP_EN_LEVEL_VALID == PIN_LEVEL_HIGH) ?
                    PIN_LEVEL_LOW : PIN_LEVEL_HIGH;
    int32_t ret = ql_gpio_set_level(VN_PIN_WIFI_CHIP_EN, level);

    return (ret == 0) ? VN_RET_OK : VN_RET_ERROR;
}

static int32_t Vn_Wifi_QueryVersionAt(char *pVersion, const char *command,
                                     int timeout, const char *log_name)
{
    char respbuf[128] = {0};
    at_resp_code code;
    int32_t result;

    if (pVersion == NULL)
    {
        return VN_RET_ERROR;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), timeout, command);
    if (code == AT_RESP_OK)
    {
        if (sscanf(respbuf, "%63s", pVersion) == 1)
        {
            VN_LOG_I("Decode %s:%s", log_name, pVersion);
            return VN_RET_OK;
        }
        return -3;
    }

    result = Vn_Wifi_AtResult(code);
    VN_LOG_E("%s failed:%d, rsp:%s", command, result, respbuf);
    return result;
}

int32_t Vn_Wifi_QueryVersion(char *pVersion)
{
    return Vn_Wifi_QueryVersionAt(pVersion, "AT+QGMR", 500, "version");
}

int32_t Vn_Wifi_QuerySubVersion(char *pVersion)
{
    return Vn_Wifi_QueryVersionAt(pVersion, "AT+CSUB", 2000, "sub version");
}

int32_t Vn_Wifi_ConnectAP(char *Ssid, char *Pwd)
{
    char respbuf[128] = {0};
    char cmdbuf[128];
    at_resp_code code;
    int len;

    if (Ssid == NULL || Ssid[0] == '\0')
    {
        return VN_RET_ERROR;
    }

    if (Pwd == NULL)
    {
        len = snprintf(cmdbuf, sizeof(cmdbuf), "AT+WLCONN=\"%s\"", Ssid);
    }
    else
    {
        len = snprintf(cmdbuf, sizeof(cmdbuf), "AT+WLCONN=\"%s\",\"%s\"", Ssid, Pwd);
    }
    if (len < 0 || len >= (int)sizeof(cmdbuf))
    {
        return VN_RET_ERROR;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 10000, "%s", cmdbuf);
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+WLCONN failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
    }
    return Vn_Wifi_AtResult(code);
}

int32_t Vn_Wifi_GetRssi(int32_t *Rssi)
{
    char respbuf[128] = {0};
    at_resp_code code;

    if (Rssi == NULL)
    {
        return VN_RET_ERROR;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 10000, "AT+WLRSSI");
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+WLRSSI failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
        return Vn_Wifi_AtResult(code);
    }

    if (sscanf(respbuf, "RSSI = %d", Rssi) != 1)
    {
        char *rssi = strstr(respbuf, "RSSI");
        if (rssi == NULL || sscanf(rssi, "RSSI = %d", Rssi) != 1)
        {
            return -3;
        }
    }

    return VN_RET_OK;
}

int32_t Vn_Wifi_GetStateInfo(Vn_Wifi_StateInfo_Typedef *StateInfo)
{
    char respbuf[256] = {0};
    at_resp_code code;
    char *state;
    int channel = 0;
    int decoded;

    if (StateInfo == NULL)
    {
        return VN_RET_ERROR;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 10000, "AT+WLSTATE");
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+WLSTATE failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
        return Vn_Wifi_AtResult(code);
    }

    state = strstr(respbuf, "STA");
    if (state == NULL)
    {
        return -3;
    }

    memset(StateInfo, 0, sizeof(*StateInfo));
    decoded = sscanf(state, "%7[^,],%31[^,],%d,%31[^,],%63[^,],%17[^,],%15[^,]",
                     StateInfo->Mode, StateInfo->Ssid, &channel,
                     StateInfo->Security, StateInfo->Password,
                     StateInfo->Mac, StateInfo->Ip);
    VN_LOG_D("AT+WLSTATE decode_num:%d", decoded);
    if (decoded != 7 || channel < 0 || channel > 255)
    {
        return -3;
    }

    StateInfo->Channel = (uint8_t)channel;
    return VN_RET_OK;
}

int32_t Vn_Wifi_IsConnectedAP(bool *IsConnected)
{
    int32_t rssi = 0;
    int32_t ret;
    Vn_Wifi_StateInfo_Typedef state_info;

    if (IsConnected == NULL)
    {
        return VN_RET_ERROR;
    }
    *IsConnected = false;

    ret = Vn_Wifi_GetRssi(&rssi);
    if (ret != VN_RET_OK || rssi == 0)
    {
        return VN_RET_OK;
    }

    memset(&state_info, 0, sizeof(state_info));
    ret = Vn_Wifi_GetStateInfo(&state_info);
    if (ret != VN_RET_OK)
    {
        return ret;
    }

    if (strcmp((char *)state_info.Mode, "STA") != 0)
    {
        VN_LOG_D("WIFI: Not in STA mode");
        return VN_RET_OK;
    }

    *IsConnected = true;
    return VN_RET_OK;
}

void Vn_Wifi_ResigterWebConfigCallback(Vn_Wifi_WebConfigCallback_Typedef Callback)
{
    Vn_Wifi_WebConfigUserCallback = Callback;
}

int32_t Vn_Wifi_StartOta(char *Url, uint16_t Port, Vn_Wifi_OtaCallback_Typedef Callback)
{
    char respbuf[256] = {0};
    at_resp_code code;

    if (Url == NULL || Url[0] == '\0')
    {
        return VN_RET_ERROR;
    }

    Vn_Wifi_OtaUserCallback = Callback;
    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 10000,
                            "AT+OTA=%s,%d", Url, (unsigned int)Port);
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+OTA failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
    }
    return Vn_Wifi_AtResult(code);
}

int32_t Vn_Wifi_OpenOwnAP(char *ApSsid)
{
    char respbuf[128] = {0};
    at_resp_code code;

    if (ApSsid == NULL || ApSsid[0] == '\0')
    {
        return VN_RET_ERROR;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 10000,
                            "AT+QAPNETOPEN=%s", ApSsid);
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+QAPNETOPEN failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
    }
    return Vn_Wifi_AtResult(code);
}

int32_t Vn_Wifi_CloseOwnAP(void)
{
    char respbuf[128] = {0};
    at_resp_code code = at_send_cmd_sync(respbuf, sizeof(respbuf), 10000,
                                        "AT+QAPNETCLOSE");

    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+QAPNETCLOSE failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
    }
    return Vn_Wifi_AtResult(code);
}

static int32_t Vn_Wifi_HttpsClientSync(unsigned int method,
                                       char *Host, uint16_t Port, char *Path,
                                       char *ContentType, char *ContentData,
                                       char *RspBuf, uint32_t RspBufSize)
{
    char respbuf[128] = {0};
    at_attr_t attr;
    at_resp_code code;

    if (Host == NULL || Host[0] == '\0' || Path == NULL ||
        RspBuf == NULL || RspBufSize == 0)
    {
        return VN_RET_ERROR;
    }

    if (ContentType == NULL)
    {
        ContentType = "0";
    }
    if (ContentData == NULL)
    {
        ContentData = "0";
    }

    at_attr_deinit(&attr);
    attr.timeout = 10000;
    attr.retry = 0;
    attr.suffix = "+HTTPCLIENT:OK";

    HttpRespBuffPtr = (uint8_t *)RspBuf;
    HttpRespBuffSize = RspBufSize;
    HttpRespBuffHadRecvLen = 0;
    HttpRespBuffOverflow = false;
    Vn_Wifi_HttpBodyLength = 0;
    RspBuf[0] = '\0';

    code = at_send_cmd_sync_by_attr(respbuf, sizeof(respbuf), &attr,
                                    "AT+HTTPCLIENT=2,%s,%d,%d,%s,%d,%s,%s",
                                    Host, (unsigned int)Port, method, Path,
                                    VN_WIFI_HTTP_DATA_FORMAT,
                                    ContentType, ContentData);

    if (HttpRespBuffPtr != NULL)
    {
        ql_rtos_task_sleep_ms(200);
        Vn_Wifi_HttpRespFinish();
    }

    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+HTTPCLIENT failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
        return Vn_Wifi_AtResult(code);
    }
    if (HttpRespBuffOverflow)
    {
        VN_LOG_E("HTTP response buffer is too small");
        return -3;
    }

    return VN_RET_OK;
}

int32_t Vn_Wifi_HttpsClientPostSync(char *Host, uint16_t Port, char *Path,
                                    char *ContentType, char *ContentData,
                                    char *RspBuf, uint32_t RspBufSize)
{
    return Vn_Wifi_HttpsClientSync(VN_WIFI_HTTP_METHOD_POST,
                                   Host, Port, Path, ContentType, ContentData,
                                   RspBuf, RspBufSize);
}

int32_t Vn_Wifi_HttpsClientGetSync(char *Host, uint16_t Port, char *Path,
                                   char *ContentType, char *ContentData,
                                   char *RspBuf, uint32_t RspBufSize)
{
    return Vn_Wifi_HttpsClientSync(VN_WIFI_HTTP_METHOD_GET,
                                   Host, Port, Path, ContentType, ContentData,
                                   RspBuf, RspBufSize);
}

int32_t Vn_Wifi_ConfigWsServer(char *Url, char *Header,
                               Vn_Wifi_WsReadCallback_Typedef WsReadCb)
{
    char respbuf[256] = {0};
    at_resp_code code;

    if (Url == NULL || Url[0] == '\0')
    {
        VN_LOG_E("WebSocket URL is NULL");
        return -4;
    }
    if (Header == NULL)
    {
        Header = "0";
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 2000,
                            "AT+QWSCFG=websocket/header,%d,%s",
                            VN_WIFI_WS_CONTEXT_ID, Header);
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+QWSCFG=websocket/header failed:%d", code);
        return -2;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 2000,
                            "AT+QWSCFG=conn/urc,%d,%s",
                            VN_WIFI_WS_CONTEXT_ID, Url);
    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+QWSCFG=conn/urc failed:%d", code);
        return -2;
    }

    Vn_Wifi_WsReadUserCallback = WsReadCb;
    return VN_RET_OK;
}

int32_t Vn_Wifi_ConnectWsServer(void)
{
    char respbuf[128] = {0};
    at_resp_code code = at_send_cmd_sync(respbuf, sizeof(respbuf), 5000,
                                        "AT+QWSOPEN=%d", VN_WIFI_WS_CONTEXT_ID);

    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+QWSOPEN failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
    }
    return Vn_Wifi_AtResult(code);
}

int32_t Vn_Wifi_DisConnectWsServer(void)
{
    char respbuf[128] = {0};
    at_resp_code code = at_send_cmd_sync(respbuf, sizeof(respbuf), 5000,
                                        "AT+QWSCLOSE=%d", VN_WIFI_WS_CONTEXT_ID);

    if (code != AT_RESP_OK)
    {
        VN_LOG_E("AT+QWSCLOSE failed:%d, rsp:%s", Vn_Wifi_AtResult(code), respbuf);
    }
    return Vn_Wifi_AtResult(code);
}

int32_t Vn_Wifi_SendToWsServer(char *pData, uint32_t DataLen, uint32_t DataType)
{
    char respbuf[128] = {0};
    at_resp_code code;
    char *error;
    int error_code = 0;

    if (pData == NULL && DataLen > 0)
    {
        return VN_RET_ERROR;
    }

    code = at_send_cmd_sync(respbuf, sizeof(respbuf), 1000,
                            "AT+QWSWRITE=%d,%d,%d,%s",
                            VN_WIFI_WS_CONTEXT_ID, DataType, DataLen,
                            (pData != NULL) ? pData : "");
    if (code == AT_RESP_OK)
    {
        return VN_RET_OK;
    }
    if (code == AT_RESP_TIMEOUT)
    {
        VN_LOG_E("AT+QWSWRITE timeout");
        return -1;
    }

    error = strstr(respbuf, "ERROR");
    if (error != NULL && sscanf(error, "ERROR,%*d,%d", &error_code) == 1)
    {
        return -100 - error_code;
    }

    VN_LOG_E("AT+QWSWRITE failed, rsp:%s", respbuf);
    return -2;
}
