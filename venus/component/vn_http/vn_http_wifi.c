#include "vn_http_wifi.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "vn_log.h"
#include "vn_utils.h"

#define VN_HTTP_WIFI_HTTPS_PORT 443U
#define VN_HTTP_WIFI_HEADER_SIZE 512U
#define VN_HTTP_WIFI_JPG_BUFFER_SIZE 20480U
#define VN_HTTP_WIFI_CONTENT_TYPE_PREFIX_SIZE 14U

extern int32_t Vn_Wifi_HttpsClientPostSync(char *Host, uint16_t Port, char *Path,
                                           char *ContentType, char *ContentData,
                                           char *RspBuf, uint32_t RspBufSize);
extern int32_t Vn_Wifi_HttpsClientGetSync(char *Host, uint16_t Port, char *Path,
                                          char *ContentType, char *ContentData,
                                          char *RspBuf, uint32_t RspBufSize);

static void Vn_HttpWifi_AppendHeader(char *out, size_t out_size,
                                     const char *data, size_t len)
{
    size_t used = strlen(out);
    size_t available;

    if (used >= out_size - 1U)
    {
        return;
    }
    available = out_size - 1U - used;
    if (len > available)
    {
        len = available;
    }
    memcpy(out + used, data, len);
    out[used + len] = '\0';
}

static char *Vn_HttpWifi_PrepareHeader(const char *Header, char *header_str)
{
    const char *cursor;
    const char *end;
    const char *crlf;
    static const char escaped_crlf[] = "\\r\\n";

    memset(header_str, 0, VN_HTTP_WIFI_HEADER_SIZE);
    if (Header == NULL)
    {
        return NULL;
    }

    cursor = Header;
    end = Header + strlen(Header);
    while (cursor < end)
    {
        crlf = strstr(cursor, "\r\n");
        if ((crlf == NULL) || ((size_t)(end - crlf) <= 2U))
        {
            Vn_HttpWifi_AppendHeader(header_str, VN_HTTP_WIFI_HEADER_SIZE,
                                     cursor, (size_t)(end - cursor));
            break;
        }

        Vn_HttpWifi_AppendHeader(header_str, VN_HTTP_WIFI_HEADER_SIZE,
                                 cursor, (size_t)(crlf - cursor));
        Vn_HttpWifi_AppendHeader(header_str, VN_HTTP_WIFI_HEADER_SIZE,
                                 escaped_crlf, sizeof(escaped_crlf) - 1U);
        cursor = crlf + 2;
    }

    VN_LOG_D("header_str: %s", header_str + VN_HTTP_WIFI_CONTENT_TYPE_PREFIX_SIZE);
    return header_str + VN_HTTP_WIFI_CONTENT_TYPE_PREFIX_SIZE;
}

int32_t Vn_HttpWifi_Init(void)
{
    return 0;
}

int32_t Vn_HttpWifi_DeInit(void)
{
    return 0;
}

int32_t Vn_HttpWifi_ClientPostSync(const char *Url, const char *Header, const char *Body,
                                   char *Response, int32_t ResponseBuffLen)
{
    char protocol[64] = {0};
    char host[64] = {0};
    char path[128] = {0};
    char header_str[VN_HTTP_WIFI_HEADER_SIZE];
    char *content_type;
    int32_t ret;

    if ((ResponseBuffLen < 0) || ((ResponseBuffLen > 0) && (Response == NULL)))
    {
        return -1;
    }
    Vn_ParseUrl(Url, protocol, host, path);
    content_type = Vn_HttpWifi_PrepareHeader(Header, header_str);
    if (strstr(protocol, "https") == NULL)
    {
        return -1;
    }

    ret = Vn_Wifi_HttpsClientPostSync(host, VN_HTTP_WIFI_HTTPS_PORT, path,
                                      content_type, (char *)Body,
                                      Response, (uint32_t)ResponseBuffLen);
    return (ret == 0) ? 0 : -1;
}

int32_t Vn_HttpWifi_ClientGetSync(const char *Url, const char *Header, const char *Body,
                                  char *Response, int32_t ResponseBuffLen)
{
    char protocol[64] = {0};
    char host[64] = {0};
    char path[128] = {0};
    char header_str[VN_HTTP_WIFI_HEADER_SIZE];
    char *content_type;
    char *raw = NULL;
    char *body;
    char *length;
    char *line_end;
    size_t header_len;
    int body_len;
    int32_t ret;

    if ((ResponseBuffLen < 0) || ((ResponseBuffLen > 0) && (Response == NULL)))
    {
        return -1;
    }
    Vn_ParseUrl(Url, protocol, host, path);
    content_type = Vn_HttpWifi_PrepareHeader(Header, header_str);
    if (strstr(protocol, "https") == NULL)
    {
        return -1;
    }

    if ((Url != NULL) && (strstr(Url, ".jpg") == NULL))
    {
        ret = Vn_Wifi_HttpsClientGetSync(host, VN_HTTP_WIFI_HTTPS_PORT, path,
                                         content_type, (char *)Body,
                                         Response, (uint32_t)ResponseBuffLen);
        return (ret == 0) ? 0 : -1;
    }

    raw = (char *)malloc(VN_HTTP_WIFI_JPG_BUFFER_SIZE);
    if (raw == NULL)
    {
        return -1;
    }
    memset(raw, 0, VN_HTTP_WIFI_JPG_BUFFER_SIZE);
    ret = Vn_Wifi_HttpsClientGetSync(host, VN_HTTP_WIFI_HTTPS_PORT, path,
                                     content_type, (char *)Body,
                                     raw, VN_HTTP_WIFI_JPG_BUFFER_SIZE);
    if (ret == 0)
    {
        body = strstr(raw, "\r\n\r\n");
        if (body == NULL)
        {
            VN_LOG_E("parse HTTP body error");
            goto cleanup;
        }
        body += 4;
        header_len = (size_t)(body - raw);
        if ((ResponseBuffLen < 0) || (header_len > (uint32_t)ResponseBuffLen))
        {
            VN_LOG_E("HTTP response buffer too small");
            goto cleanup;
        }
        memcpy(Response, raw, header_len);

        length = strstr(raw, "Content-Length: ");
        if (length == NULL)
        {
            VN_LOG_E("parse Content-Length error");
            goto cleanup;
        }
        line_end = strstr(length, "\r\n");
        if (line_end == NULL)
        {
            VN_LOG_E("parse Content-Length line error");
            goto cleanup;
        }

        body_len = atoi(length + 16);
        VN_LOG_D("body len %d", body_len);
        if ((body_len < 0) || ((uint32_t)body_len > UINT16_MAX / 2U) ||
            (strlen(body) < (size_t)body_len * 2U) ||
            ((size_t)body_len > (size_t)ResponseBuffLen - header_len))
        {
            VN_LOG_E("invalid HTTP body length");
            goto cleanup;
        }
        if (!Vn_HexStringToBin(body, (uint16_t)((uint32_t)body_len * 2U),
                               (uint8_t *)Response + header_len))
        {
            VN_LOG_E("decode HTTP body error");
        }
    }

cleanup:
    free(raw);
    return (ret == 0) ? 0 : -1;
}
