#include "vn_cell_fota.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ql_fota.h"
#include "ql_gpio.h"
#include "ql_http_client.h"
#include "ql_power.h"
#include "ql_rtos.h"
#include "vn_celluar.h"
#include "vn_gpio.h"
#include "vn_log.h"
#include "vn_net.h"

#define VN_FOTA_DATACALL_TIMEOUT_S  10
#define VN_FOTA_CONTENT_LENGTH_SIZE 64U
#define VN_FOTA_LCD_NETWORK_CELLULAR 1U

extern void Vn_LCD_SetOTA(uint8_t Networktype, uint8_t progress);

static qlFotaImgProcCtxPtr ctx = NULL;
static QL_HTTP_CLIENT_T *fota_httpclient = NULL;
static int32_t fota_process = 0;
static qlFotaProgressCB_t fota_progress_cb = NULL;
static int32_t total_len = 0;
static int32_t recv_len = 0;
static uint32_t Vn_OTA_Time = 0;

static int Vn_FotaHttp_Resp_Callback(QL_HTTP_CLIENT_T *client,
                                     QL_HTTP_CLIENT_EVENT_E event,
                                     int status_code, char *data,
                                     int data_len, void *private_data)
{
    char para[VN_FOTA_CONTENT_LENGTH_SIZE] = {0};
    int ret;

    (void)client;
    (void)private_data;

    switch (event)
    {
        case QL_HTTP_CLIENT_EVENT_SEND_FAIL:
            VN_LOG_E("http send failed");
            fota_process = -3;
            break;

        case QL_HTTP_CLIENT_EVENT_SEND_SUCCESSED:
            VN_LOG_I("http send succeeded");
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL:
            VN_LOG_E("http parse response header failed");
            fota_process = -4;
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED:
            VN_LOG_I("http recv header status_code: %d, header_len: %d",
                     status_code, data_len);
            ret = ql_http_client_get_header(data, "Content-Length", 0,
                                            para, (int)sizeof(para));
            total_len = (ret > 0) ? atoi(para) : 0;
            if (ret > 0)
            {
                VN_LOG_I("Content-Length: %d", total_len);
            }
            if (status_code == 200)
            {
                VN_LOG_I("http recv response ok");
            }
            return 1;

        case QL_HTTP_CLIENT_EVENT_RECV_BODY:
            if ((ctx == NULL) || (data == NULL) || (data_len <= 0))
            {
                return 0;
            }
            VN_LOG_D("http recv body status_code: %d, recv_body_len: %d",
                     status_code, data_len);
            recv_len += data_len;
            ret = ql_fota_image_write(ctx, data, data_len);
            VN_LOG_D("write_ret: %d", ret);
            if (ret != 0)
            {
                VN_LOG_E("fota image write fail");
                return 0;
            }
            if (total_len > 0)
            {
                fota_process = (recv_len * 100) / total_len;
            }
            if (fota_progress_cb != NULL)
            {
                fota_progress_cb(QL_FOTA_INPROGRESS, fota_process);
            }
            return 1;

        case QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED:
            if (fota_progress_cb != NULL)
            {
                fota_progress_cb(QL_FOTA_SUCCEED, 100);
            }
            fota_process = 100;
            ret = ql_fota_image_flush(ctx);
            if (ret != 0)
            {
                VN_LOG_E("fota image flush fail");
            }
            ret = ql_fota_image_verify(ctx);
            if (ret != 0)
            {
                VN_LOG_E("fota image verify fail");
                return ret;
            }
            if (fota_progress_cb != NULL)
            {
                fota_progress_cb(QL_FOTA_SETFLAG, 100);
            }
            VN_LOG_I("http recv body finished");
            return ret;

        case QL_HTTP_CLIENT_EVENT_DISCONNECTED:
            VN_LOG_I("http closed by server");
            fota_process = -5;
            break;

        default:
            break;
    }
    return 0;
}

static void Vn_FotaProgressCB(qlFotaSta sta, int progress)
{
    static int32_t progress_step = -1;

    if (sta == QL_FOTA_INPROGRESS)
    {
        if (progress_step == -1)
        {
            (void)Vn_LDO_Control(VN_LDO_CODEC, VN_LDO_STATUS_DISABLE);
            progress_step = 0;
            Vn_LCD_SetOTA(VN_FOTA_LCD_NETWORK_CELLULAR, (uint8_t)progress);
        }
        if ((progress_step + 9) < progress)
        {
            progress_step = progress;
            Vn_LCD_SetOTA(VN_FOTA_LCD_NETWORK_CELLULAR, (uint8_t)progress);
        }
        VN_LOG_D("step %d, progress %d", progress_step, progress);
        VN_LOG_I("fota downloading (%d)%d ...", sta, progress);
    }
    else if (sta == QL_FOTA_SUCCEED)
    {
        progress_step = -1;
        Vn_LCD_SetOTA(VN_FOTA_LCD_NETWORK_CELLULAR, 100U);
        VN_LOG_I("fota downloading (%d)%d ...", sta, progress);
    }
    else if (sta == QL_FOTA_SETFLAG)
    {
        VN_LOG_I("fota update flag set, will restart to update");
        Vn_OTA_Time++;
        VN_LOG_I("http download success Vn_OTA_Time: %u",
                 (unsigned int)Vn_OTA_Time);
        ql_power_reset();
    }
    else if (sta == QL_FOTA_FAIL)
    {
        VN_LOG_E("fota download failed (%d)%d", sta, progress);
        (void)ql_gpio_set_level(GPIO_PIN_NO_24, PIN_LEVEL_HIGH);
    }
    else
    {
        VN_LOG_W("unknown fota state: %d", sta);
    }
}

int Vn_Http_Protocol_Type(char *url)
{
    size_t len;

    if (url == NULL)
    {
        return -1;
    }

    VN_LOG_I("vn_url: %s", url);
    len = strlen(url);
    if ((len >= 8U) &&
        ((strncmp(url, "https://", 8U) == 0) ||
         (strncmp(url, "HTTPS://", 8U) == 0)))
    {
        return 1;
    }
    if ((len >= 7U) &&
        ((strncmp(url, "http://", 7U) == 0) ||
         (strncmp(url, "HTTP://", 7U) == 0)))
    {
        return 0;
    }
    return -1;
}

int Vn_Fota_Firmware_Cellular(int profile_idx, char *address,
                              uint16_t port, char *rootCA_path)
{
    SSLConfig sslConfig;
    QL_HTTP_CLIENT_ERR_E request_ret;
    char *domain;
    int protocol_type;
    int ret = -1;

    VN_LOG_I("=========== fota test start ==========");
    if ((address == NULL) || (address[0] == '\0'))
    {
        VN_LOG_E("fota address is null");
        return -1;
    }

    domain = (char *)malloc(strlen(address) + 1U);
    if (domain == NULL)
    {
        VN_LOG_E("fota malloc fail");
        return -1;
    }
    memset(domain, 0, strlen(address) + 1U);
    (void)Vn_Extract_Domain_From_Url(address, domain);

    protocol_type = Vn_Http_Protocol_Type(address);
    if (protocol_type < 0)
    {
        VN_LOG_E("unsupported fota protocol");
        goto exit;
    }
    if (Vn_DataCall_Start(profile_idx, VN_FOTA_DATACALL_TIMEOUT_S) != 0)
    {
        VN_LOG_E("datacall start error");
        goto exit;
    }
    if ((protocol_type == 1) && (rootCA_path == NULL))
    {
        VN_LOG_E("HTTPS but rootCA is null");
        goto exit;
    }

    memset(&sslConfig, 0, sizeof(sslConfig));
    sslConfig.en = 1U;
    sslConfig.verify = SSL_VERIFY_MODE_OPTIONAL;
    sslConfig.profileIdx = (unsigned char)profile_idx;
    sslConfig.vsn = SSL_VSN_ALL;
    sslConfig.cipherList = "ALL";
    sslConfig.serverName = domain;
    sslConfig.serverPort = port;
    sslConfig.cert.from = SSL_CERT_FROM_BUF;
    sslConfig.cert.path.rootCA = rootCA_path;

    ctx = ql_fota_init();
    if (ctx == NULL)
    {
        VN_LOG_E("fota init fail");
        goto exit;
    }
    fota_process = 0;
    total_len = 0;
    recv_len = 0;
    fota_progress_cb = Vn_FotaProgressCB;

    fota_httpclient = ql_http_client_init();
    if (fota_httpclient == NULL)
    {
        VN_LOG_E("fota http init fail");
        goto exit;
    }
    (void)ql_http_client_setopt(fota_httpclient, QL_HTTP_CLIENT_OPT_ASYN, 0);
    (void)ql_http_client_setopt(fota_httpclient, QL_HTTP_CLIENT_OPT_PDP_CID,
                                profile_idx);
    (void)ql_http_client_setopt(fota_httpclient, QL_HTTP_CLIENT_OPT_PROTOCOL_VER,
                                QL_HTTP_CLIENT_HTTP1_1);
    if (protocol_type == 1)
    {
        (void)ql_http_client_setopt(fota_httpclient, QL_HTTP_CLIENT_OPT_SSL_CTX,
                                    &sslConfig);
    }

    VN_LOG_I("fota_client: %p, address: %s", (void *)fota_httpclient, address);
    request_ret = ql_http_client_request(
        fota_httpclient, address, QL_HTTP_CLIENT_REQUEST_GET,
        QL_HTTP_CLIENT_AUTH_TYPE_NONE, NULL, NULL, NULL, 0,
        Vn_FotaHttp_Resp_Callback, NULL);
    if (request_ret != QL_HTTP_CLIENT_ERR_SUCCESS)
    {
        VN_LOG_E("fota http request failed: %d", request_ret);
        goto exit;
    }
    ret = 0;
    VN_LOG_I("********* fota end ********");

exit:
    if (fota_httpclient != NULL)
    {
        ql_http_client_release(fota_httpclient);
        fota_httpclient = NULL;
    }
    if (ctx != NULL)
    {
        ql_fota_deinit(ctx);
        ctx = NULL;
    }
    free(domain);
    return ret;
}
