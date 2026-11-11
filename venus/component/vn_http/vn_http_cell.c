#include "vn_http_cell.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "ql_rtos.h"
#include "vn_net.h"
#include "vn_log.h"

#define VN_HTTP_CELL_PROFILE_IDX 3
#define VN_HTTP_CELL_REGISTER_TIMEOUT_S 10
#define VN_HTTP_CELL_RESPONSE_TIMEOUT_MS 5000U
#define VN_HTTP_CELL_HEADER_TEXT_SIZE 512U
#define VN_HTTP_CELL_HEADER_LINE_SIZE 64U

SSLConfig SslConfig = {
    .en = 1,
    .serverPort = 443,
    .dbgLevel = 1,
    .sessionReuseEn = 0,
    .vsn = SSL_VSN_ALL,
    .verify = SSL_VERIFY_MODE_OPTIONAL,
    .cert.from = SSL_CERT_FROM_BUF,
    .cipherList = "ALL"
};

static const char Vn_HttpCellRootCa[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDuzCCAqOgAwIBAgIDBETAMA0GCSqGSIb3DQEBBQUAMH4xCzAJBgNVBAYTAlBM\n"
    "MSIwIAYDVQQKExlVbml6ZXRvIFRlY2hub2xvZ2llcyBTLkEuMScwJQYDVQQLEx5D\n"
    "ZXJ0dW0gQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkxIjAgBgNVBAMTGUNlcnR1bSBU\n"
    "cnVzdGVkIE5ldHdvcmsgQ0EwHhcNMDgxMDIyMTIwNzM3WhcNMjkxMjMxMTIwNzM3\n"
    "WjB+MQswCQYDVQQGEwJQTDEiMCAGA1UEChMZVW5pemV0byBUZWNobm9sb2dpZXMg\n"
    "Uy5BLjEnMCUGA1UECxMeQ2VydHVtIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MSIw\n"
    "IAYDVQQDExlDZXJ0dW0gVHJ1c3RlZCBOZXR3b3JrIENBMIIBIjANBgkqhkiG9w0B\n"
    "AQEFAAOCAQ8AMIIBCgKCAQEA4/t9o3K6wvDJFIf1awFO4W5AB7ptJ11/91sts1rH\n"
    "UV+rpDKmYYe2bg+G0jACl/jXaVehGDldamR5xgFZrDwxSjh80gTSSyjoIF87B6LM\n"
    "TXPb865Px1bVWqeWifrzq2jUI4ZZJ88JJ7ysbnKDHDBy3+Ci6dLhdHUZvSqeexVU\n"
    "BBvXQzmtVSjF4hq79MDkrjhJM8x2hZ85RdKknvISjFH4fOQtf/WsX+sWn7Et0brM\n"
    "kUJ3TCXJkDhv2/DM+44el1k+1WBO5gUo7Ul5E0u6SNsv+XLTOcr+H9g0cvW0QM8x\n"
    "AcPs3hEtF10fuFDRXhmnad4HMyjKUJX5p1TLVIZQRan5SQIDAQABo0IwQDAPBgNV\n"
    "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBQIds3LB/8k9sXN7buQvOKEN0Z19zAOBgNV\n"
    "HQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQEFBQADggEBAKaorSLOAT2mo/9i0Eidi15y\n"
    "sHhE49wcrwn9I0j6vSrEuVUEtRCjjSfeC4Jj0O7eDDd5QVsisrCaQVymcODU0HfL\n"
    "I9MA4GxWL+FpDQ3Zqr8hgVDZBqWo/5U30Kr+4rP1mS1FhIrlQgnXdAIv94nYmem8\n"
    "J9RHjboNRhx3zxSkHLmkMcScKHQDNP8zGSal6Q10tz6XxnboJ5ajZt3hrvJBW8qY\n"
    "VoNzcOSGGtIxQbovvi0TWnZvTuhOgQ4/WwMioBK+ZlgRSssDxLQqKi2WF+A5VLxI\n"
    "03YnnZotBqbJ7DnSq9ufmgsnAjUpsUCV5/nonFWIGUbWtzT1fs45mtk48VH3Tyw=\n"
    "-----END CERTIFICATE-----\n";

static uint32_t Vn_HttpCell_NowMs(void)
{
    return ql_rtos_get_systicks() * 5U;
}

static int Vn_HttpCell_BuildHeaderList(const char *Header, QL_HTTP_CLIENT_LIST_T **OutList)
{
    char header_str[VN_HTTP_CELL_HEADER_TEXT_SIZE] = {0};
    char header_temp[VN_HTTP_CELL_HEADER_LINE_SIZE];
    QL_HTTP_CLIENT_LIST_T *list = NULL;
    QL_HTTP_CLIENT_LIST_T *new_list;
    char *cursor;
    char *end;
    char *crlf;
    size_t len;

    *OutList = NULL;
    if (Header == NULL)
    {
        return 0;
    }

    len = strlen(Header);
    if (len >= sizeof(header_str))
    {
        return -1;
    }
    memcpy(header_str, Header, len + 1U);
    VN_LOG_D("len: %u, all header: %s", (unsigned int)(len + 1U), header_str);

    cursor = header_str;
    end = header_str + len + 1U;
    while (cursor < end)
    {
        crlf = strstr(cursor, "\r\n");
        if (crlf != NULL)
        {
            *crlf = '\0';
        }

        memset(header_temp, 0, sizeof(header_temp));
        (void)snprintf(header_temp, sizeof(header_temp), "%s\r\n", cursor);
        new_list = ql_http_client_list_append(list, header_temp);
        if (new_list == NULL)
        {
            if (list != NULL)
            {
                ql_http_client_list_destroy(list);
            }
            return -1;
        }
        list = new_list;
        VN_LOG_D("decode header: %s", header_temp);
        if (crlf == NULL)
        {
            break;
        }
        cursor = crlf + 2;
    }

    *OutList = list;
    return 0;
}

int Vn_HttpCell_EventCallback(QL_HTTP_CLIENT_T *client, QL_HTTP_CLIENT_EVENT_E event,
                              int status_code, char *data, int data_len, void *private_data)
{
    Vn_HttpData_Typedef *response = (Vn_HttpData_Typedef *)private_data;
    uint32_t copy_len;

    (void)client;
    switch (event)
    {
        case QL_HTTP_CLIENT_EVENT_SEND_FAIL:
            VN_LOG_E("http send failed");
            break;

        case QL_HTTP_CLIENT_EVENT_SEND_SUCCESSED:
            VN_LOG_D("http send successed");
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_HEADER_FAIL:
            VN_LOG_E("http parse response header_list failed");
            break;

        case QL_HTTP_CLIENT_EVENT_RECV_HEADER_FINISHED:
            VN_LOG_D("----------http recv start--------");
            VN_LOG_D("status_code:%d, len %d, http recv header:%.*s",
                     status_code, data_len, data_len, data);
            copy_len = ((uint32_t)data_len < response->Size) ?
                       (uint32_t)data_len : response->Size;
            if (copy_len != 0U)
            {
                memcpy(response->Data, data, copy_len);
            }
            response->Offset = copy_len;
            return 1;

        case QL_HTTP_CLIENT_EVENT_RECV_BODY:
            VN_LOG_D("http recv body len:%d", data_len);
            copy_len = response->Size - response->Offset;
            if ((uint32_t)data_len < copy_len)
            {
                copy_len = (uint32_t)data_len;
            }
            if (copy_len != 0U)
            {
                memcpy(response->Data + response->Offset, data, copy_len);
            }
            response->Offset += copy_len;
            return 1;

        case QL_HTTP_CLIENT_EVENT_RECV_BODY_FINISHED:
            VN_LOG_D("http recv body finished");
            VN_LOG_D("----------http recv end--------");
            response->IsHadRecv = true;
            break;

        case QL_HTTP_CLIENT_EVENT_DISCONNECTED:
            VN_LOG_D("http be closed by server");
            break;

        default:
            break;
    }
    return 0;
}

static int32_t Vn_HttpCell_RequestSync(QL_HTTP_CLIENT_REQUEST_METHOD_E method,
                                       const char *Url, const char *Header, const char *Body,
                                       char *Response, int32_t ResponseBuffLen)
{
    Vn_HttpData_Typedef http_rsp_data;
    QL_HTTP_CLIENT_LIST_T *header_list = NULL;
    QL_HTTP_CLIENT_T *client = NULL;
    QL_HTTP_CLIENT_ERR_E request_ret;
    uint8_t domain[64] = {0};
    uint32_t start_time;
    const char *body = (Body != NULL) ? Body : "";

    if ((Url == NULL) || (ResponseBuffLen < 0) ||
        ((ResponseBuffLen > 0) && (Response == NULL)))
    {
        return -1;
    }

    http_rsp_data.Data = (uint8_t *)Response;
    http_rsp_data.Size = (uint32_t)ResponseBuffLen;
    http_rsp_data.Offset = 0;
    http_rsp_data.IsHadRecv = false;
    (void)Vn_Extract_Domain_From_Url(Url, (char *)domain);

    if (Vn_Cellular_StartDataCall(VN_HTTP_CELL_PROFILE_IDX,
                                  VN_HTTP_CELL_REGISTER_TIMEOUT_S) != 0)
    {
        VN_LOG_E("datacall start error");
        return -1;
    }
    if (Vn_HttpCell_BuildHeaderList(Header, &header_list) != 0)
    {
        VN_LOG_E("http header create failed");
        return -1;
    }

    client = ql_http_client_init();
    if (client == NULL)
    {
        if (header_list != NULL)
        {
            ql_http_client_list_destroy(header_list);
        }
        return -1;
    }

    VN_LOG_D("http client start");
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PDP_CID,
                                VN_HTTP_CELL_PROFILE_IDX);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_PROTOCOL_VER,
                                QL_HTTP_CLIENT_HTTP1_1);
    (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_ENABLE_COOKIE, 1);

    if (strstr(Url, "https") != NULL)
    {
        SslConfig.profileIdx = VN_HTTP_CELL_PROFILE_IDX;
        SslConfig.serverName = (char *)domain;
        SslConfig.cert.path.rootCA = (char *)Vn_HttpCellRootCa;
        (void)ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_SSL_CTX, &SslConfig);
    }

    request_ret = ql_http_client_setopt(client, QL_HTTP_CLIENT_OPT_HTTPHEADER, header_list);
    if (request_ret == QL_HTTP_CLIENT_ERR_LAST_REQUEST_NOT_FINISH)
    {
        VN_LOG_I("last request not finish, not to set");
    }

    request_ret = ql_http_client_request(client, (char *)Url, method,
                                         QL_HTTP_CLIENT_AUTH_TYPE_NONE,
                                         NULL, NULL, (char *)body, (int)strlen(body),
                                         Vn_HttpCell_EventCallback, &http_rsp_data);
    VN_LOG_I("ql_http_client_request ret=%d", request_ret);

    start_time = Vn_HttpCell_NowMs();
    for (;;)
    {
        if (((uint32_t)(Vn_HttpCell_NowMs() - start_time) >=
             VN_HTTP_CELL_RESPONSE_TIMEOUT_MS) || http_rsp_data.IsHadRecv)
        {
            break;
        }
        VN_LOG_I("waiting for response..., diff: %u",
                 (unsigned int)(Vn_HttpCell_NowMs() - start_time));
        ql_rtos_task_sleep_s(1);
    }
    VN_LOG_D("IsHadRecv: %d", http_rsp_data.IsHadRecv ? 1 : 0);

    if (header_list != NULL)
    {
        ql_http_client_list_destroy(header_list);
    }
    ql_http_client_release(client);

    if (!http_rsp_data.IsHadRecv)
    {
        VN_LOG_E("not receive response");
        return -1;
    }
    return 0;
}

int32_t Vn_HttpCell_Init(void)
{
    return 0;
}

int32_t Vn_HttpCell_DeInit(void)
{
    return 0;
}

int32_t Vn_HttpCell_ClientPostSync(const char *Url, const char *Header, const char *Body,
                                   char *Response, int32_t ResponseBuffLen)
{
    return Vn_HttpCell_RequestSync(QL_HTTP_CLIENT_REQUEST_POST, Url, Header, Body,
                                   Response, ResponseBuffLen);
}

int32_t Vn_HttpCell_ClientGetSync(const char *Url, const char *Header, const char *Body,
                                  char *Response, int32_t ResponseBuffLen)
{
    return Vn_HttpCell_RequestSync(QL_HTTP_CLIENT_REQUEST_GET, Url, Header, Body,
                                   Response, ResponseBuffLen);
}
