#include "vn_xiaozhi_ota.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ql_decoder_cJSON.h"
#include "ql_rtos.h"
#include "vn_cell_fota.h"
#include "vn_log.h"
#include "vn_utils.h"
#include "vn_wifi.h"
#include "vn_xiaozhi_app.h"

#define VN_OTA_RESPONSE_SIZE          2048U
#define VN_OTA_HEADER_SIZE            512U
#define VN_OTA_REQUEST_SIZE           512U
#define VN_OTA_REGISTER_REQUEST_SIZE  812U
#define VN_OTA_WIFI_PORT              80U
#define VN_OTA_HTTPS_PORT             443U
#define VN_OTA_WIFI_QUEUE_DEPTH       2
#define VN_OTA_WIFI_WAIT_MS           100
#define VN_OTA_WIFI_TIMEOUT_MS        20000U
#define VN_OTA_DISPLAY_MSG_TYPE       4U
#define VN_OTA_DISPLAY_STATUS         2U
#define VN_OTA_INVALID_FIRMWARE_URL   "NOT_ACTIVATED_FIRMWARE_THIS_IS_A_INVALID_URL"
#define VN_OTA_REGISTER_URL           "https://databerrypi.com/cmlinfra/device-register"

extern uint64_t ql_rtc_to_timestamp(void);
extern void Vn_DisplayMsg_Send(uint8_t Type, uint8_t *pData, uint8_t Size);

Vn_Queue_Typedef Vn_OtaWif_Queue = {0};

static const char Vn_OtaRootCa[] =
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

void Vn_Wifi_OtaCallback(void *pUserData)
{
    int32_t ota_progress = 0;

    if (pUserData == NULL)
    {
        return;
    }

    VN_LOG_I("%s", (const char *)pUserData);
    if (sscanf((const char *)pUserData, "+QOTAURC:progress,%d", &ota_progress) != 1)
    {
        if (sscanf((const char *)pUserData, "+QOTAURC:upgrade,%d", &ota_progress) != 1)
        {
            return;
        }
        if (ota_progress == 0)
        {
            ota_progress = 200;
        }
    }

    (void)Vn_Queue_Release(Vn_OtaWif_Queue, &ota_progress,
                           (int)sizeof(ota_progress), QL_NO_WAIT);
}

int32_t Vn_Ota_ParseOtaJson(char *JsonStr, Vn_OtaRspInfo_Typedef *OtaRspInfo)
{
    cJSON *object;
    cJSON *activation;
    cJSON *message;
    cJSON *websocket;
    cJSON *url;
    cJSON *firmware;
    cJSON *version;
    int32_t ret = -1;

    if ((JsonStr == NULL) || (OtaRspInfo == NULL))
    {
        return -1;
    }

    object = cJSON_Parse(JsonStr);
    if (object == NULL)
    {
        VN_LOG_E("JSON parse err");
        return -1;
    }

    activation = cJSON_GetObjectItemCaseSensitive(object, "activation");
    if (activation == NULL)
    {
        OtaRspInfo->IsNendActivate = false;
        VN_LOG_W("Device had registered");
    }
    else
    {
        message = cJSON_GetObjectItem(activation, "message");
        if (cJSON_IsString(message) && (message->valuestring != NULL) &&
            (strstr(message->valuestring, "success") != NULL))
        {
            OtaRspInfo->IsNendActivate = false;
            VN_LOG_W("Device had registered");
        }
        else
        {
            OtaRspInfo->IsNendActivate = true;
            VN_LOG_W("Device is not registered, need to register");
        }
    }

    websocket = cJSON_GetObjectItemCaseSensitive(object, "websocket");
    if (!cJSON_IsObject(websocket))
    {
        VN_LOG_E("websocket config err");
        goto exit;
    }

    url = cJSON_GetObjectItemCaseSensitive(websocket, "url");
    if (!cJSON_IsString(url) || (url->valuestring == NULL))
    {
        VN_LOG_E("websocket url err");
        goto exit;
    }

    VN_LOG_I("Websocket URL: %s", url->valuestring);
    (void)snprintf((char *)OtaRspInfo->ChatUrl, sizeof(OtaRspInfo->ChatUrl),
                   "%s", url->valuestring);

    firmware = cJSON_GetObjectItemCaseSensitive(object, "firmware");
    if (cJSON_IsObject(firmware))
    {
        version = cJSON_GetObjectItemCaseSensitive(firmware, "version");
        url = cJSON_GetObjectItemCaseSensitive(firmware, "url");
        if (!cJSON_IsString(version) || (version->valuestring == NULL))
        {
            VN_LOG_E("firmware version err");
            goto exit;
        }
        if (!cJSON_IsString(url) || (url->valuestring == NULL))
        {
            VN_LOG_E("firmware url err");
            goto exit;
        }

        VN_LOG_I("firmware version: %s", version->valuestring);
        VN_LOG_I("firmware download URL: %s", url->valuestring);
        (void)snprintf((char *)OtaRspInfo->Firmware.Version,
                       sizeof(OtaRspInfo->Firmware.Version), "%s", version->valuestring);
        (void)snprintf((char *)OtaRspInfo->Firmware.Url,
                       sizeof(OtaRspInfo->Firmware.Url), "%s", url->valuestring);
    }

    firmware = cJSON_GetObjectItemCaseSensitive(object, "firmware_4g");
    if (cJSON_IsArray(firmware))
    {
        VN_LOG_I("firmware is array");
    }
    ret = 0;

exit:
    cJSON_Delete(object);
    return ret;
}

int32_t Vn_Ota_ParseRegisterJson(char *JsonStr,
                                 Vn_OtaRspResigter_Typedef *OtaRspResigter)
{
    cJSON *object;
    cJSON *item;

    if ((JsonStr == NULL) || (OtaRspResigter == NULL))
    {
        return -1;
    }

    object = cJSON_Parse(JsonStr);
    if (object == NULL)
    {
        return -1;
    }

    item = cJSON_GetObjectItem(object, "message");
    if (cJSON_IsString(item) && (item->valuestring != NULL))
    {
        (void)snprintf((char *)OtaRspResigter->Message,
                       sizeof(OtaRspResigter->Message), "%s", item->valuestring);
    }

    item = cJSON_GetObjectItem(object, "data");
    if (item == NULL)
    {
        VN_LOG_E("data message err");
        goto exit;
    }
    item = cJSON_GetObjectItem(item, "payload");
    if (item == NULL)
    {
        VN_LOG_E("payload message err");
        goto exit;
    }
    item = cJSON_GetObjectItem(item, "result");
    if (item == NULL)
    {
        VN_LOG_E("result message err");
        goto exit;
    }
    item = cJSON_GetObjectItem(item, "data");
    if (item == NULL)
    {
        VN_LOG_E("payload_result_data message err");
        goto exit;
    }
    item = cJSON_GetObjectItem(item, "num");
    if (item == NULL)
    {
        VN_LOG_E("payload_result_data_num message err");
        goto exit;
    }
    OtaRspResigter->ResultNum = (uint32_t)item->valueint;

exit:
    cJSON_Delete(object);
    return 0;
}

int32_t Vn_Ota_CheckVersion(Vn_NetType_Typedef NetType, char *Url,
                            char *BoardVersion, char *OtaVersion)
{
    uint32_t start_time;
    int32_t ret;
    int32_t ota_progress;
    uint8_t display_data[2];

    if ((Url == NULL) || (BoardVersion == NULL) || (OtaVersion == NULL))
    {
        return -1;
    }

    VN_LOG_D("version board: %s, ota: %s", BoardVersion, OtaVersion);
    if (memcmp(BoardVersion, OtaVersion, strlen(BoardVersion)) == 0)
    {
        VN_LOG_I("version is new, not update");
        return -1;
    }
    if (strstr(Url, VN_OTA_INVALID_FIRMWARE_URL) != NULL)
    {
        VN_LOG_W("invalid url");
        return -1;
    }

    VN_LOG_I("start update the version to %s", OtaVersion);
    if (NetType != VN_NET_TYPE_WIFI)
    {
        (void)Vn_Fota_Firmware_Cellular(1, Url, VN_OTA_HTTPS_PORT,
                                        (char *)Vn_OtaRootCa);
        return -1;
    }

    ret = Vn_Wifi_StartOta(Url, VN_OTA_WIFI_PORT, Vn_Wifi_OtaCallback);
    if (ret != 0)
    {
        return -1;
    }
    if ((Vn_OtaWif_Queue.Queue == NULL) &&
        (Vn_Queue_Create(&Vn_OtaWif_Queue, VN_OTA_WIFI_QUEUE_DEPTH) != 0))
    {
        VN_LOG_E("create queue failed");
        return -1;
    }

    start_time = ql_rtos_get_systicks() * 5U;
    do
    {
        ota_progress = 0;
        ret = Vn_Queue_Wait(Vn_OtaWif_Queue, &ota_progress,
                            (int)sizeof(ota_progress), VN_OTA_WIFI_WAIT_MS);
        if (ret > 0)
        {
            start_time = ql_rtos_get_systicks() * 5U;
            if (ota_progress > 0)
            {
                display_data[0] = VN_OTA_DISPLAY_STATUS;
                display_data[1] = (uint8_t)ota_progress;
                Vn_DisplayMsg_Send(VN_OTA_DISPLAY_MSG_TYPE, display_data,
                                   (uint8_t)sizeof(display_data));
            }
            if ((uint32_t)ota_progress > 99U)
            {
                return 0;
            }
        }
    } while (Vn_DiffTime(start_time, ql_rtos_get_systicks() * 5U) <
             VN_OTA_WIFI_TIMEOUT_MS);

    return -1;
}

static int32_t Vn_Ota_RegisterDevice(Vn_Http_Typedef *Http,
                                     const Vn_BoardInfo_Typedef *BoardInfo,
                                     Vn_OtaRspResigter_Typedef *OtaRspResigter)
{
    char context_type[VN_OTA_HEADER_SIZE] = {0};
    char context_data[VN_OTA_REGISTER_REQUEST_SIZE] = {0};
    char *response;
    char *json;
    uint64_t timestamp;
    int32_t ret;

    if ((Http == NULL) || (Http->ClientPostSync == NULL) ||
        (BoardInfo == NULL) || (OtaRspResigter == NULL))
    {
        VN_LOG_E("invalid param");
        return -1;
    }

    memset(OtaRspResigter, 0, sizeof(*OtaRspResigter));
    timestamp = ql_rtc_to_timestamp() * 1000ULL;
    (void)snprintf(context_type, sizeof(context_type),
                   "Content-Type: application/json\r\n"
                   "User-Agent: %s/1.0.1\r\n"
                   "Device-Id: %s\r\n"
                   "Client-Id: %s\r\n"
                   "Connection: keep-alive",
                   (const char *)BoardInfo->Name,
                   (const char *)BoardInfo->MacAddr,
                   (const char *)BoardInfo->UUID);
    (void)snprintf(context_data, sizeof(context_data),
                   "{\"session_id\": \"%llu\",\"type\": \"cml_message_bridge\","
                   "\"api_version\": \"v1.0\",\"payload\": {\"jsonrpc\": \"2.0\","
                   "\"id\": 1,\"destination\": \"cml_server\","
                   "\"method\": \"device_register\","
                   "\"trace_id\": \"trace_id_1234567890abcdef\","
                   "\"request_id\": \"request_id_abcdef1234567890\","
                   "\"params\": {\"type\": \"physical_device\"},"
                   "\"device_info\": {\"device_id\": \"%s\","
                   "\"iccid\": \"22222222222222222222\",\"version\": \"%s\","
                   "\"name\": \"%s\"},\"timestamp\": %llu}}",
                   (unsigned long long)timestamp, (const char *)BoardInfo->MacAddr,
                   (const char *)BoardInfo->Version, (const char *)BoardInfo->Name,
                   (unsigned long long)timestamp);

    response = (char *)malloc(VN_OTA_RESPONSE_SIZE);
    if (response == NULL)
    {
        VN_LOG_E("register response malloc failed");
        return -1;
    }
    memset(response, 0, VN_OTA_RESPONSE_SIZE);

    ret = Http->ClientPostSync(VN_OTA_REGISTER_URL, context_type, context_data,
                               response, (int32_t)VN_OTA_RESPONSE_SIZE);
    if (ret != 0)
    {
        VN_LOG_E("register https post failed, ret: %d", ret);
        free(response);
        return -1;
    }

    json = strstr(response, "\r\n\r\n");
    if (json == NULL)
    {
        VN_LOG_E("register response has no JSON body");
        free(response);
        return -1;
    }
    ret = Vn_Ota_ParseRegisterJson(json + 4, OtaRspResigter);
    free(response);
    return ret;
}

int32_t Vn_Ota_ConnectServer(Vn_NetType_Typedef NetType, char *OtaUrl,
                             Vn_BoardInfo_Typedef *BoardInfo,
                             Vn_OtaRspInfo_Typedef *OtaRspInfo)
{
    char context_type[VN_OTA_HEADER_SIZE] = {0};
    char context_data[VN_OTA_REQUEST_SIZE] = {0};
    uint8_t response[VN_OTA_RESPONSE_SIZE] = {0};
    Vn_Xiaozhi_Typedef *xiaozhi;
    char *json;
    int32_t ret;

    (void)NetType;
    if (OtaRspInfo == NULL)
    {
        return -1;
    }
    memset(OtaRspInfo, 0, sizeof(*OtaRspInfo));
    if (BoardInfo == NULL)
    {
        return -2;
    }
    if (OtaUrl == NULL)
    {
        return -1;
    }

    xiaozhi = Vn_Xiaozhi_GetInstance();
    if ((xiaozhi == NULL) || (xiaozhi->Http.ClientPostSync == NULL))
    {
        VN_LOG_E("http client is not initialized");
        return -1;
    }

    ret = Vn_Ota_RegisterDevice(&xiaozhi->Http, BoardInfo, &OtaRspInfo->Resigter);
    if ((ret != 0) &&
        (strstr((const char *)OtaRspInfo->Resigter.Message, "Success") == NULL))
    {
        VN_LOG_E("register device failed");
        return -2;
    }

    (void)snprintf(context_type, sizeof(context_type),
                   "Content-Type: application/json\r\n"
                   "User-Agent: %s/1.0.1\r\n"
                   "Device-Id: %s\r\n"
                   "Client-Id: %s\r\n"
                   "Connection: keep-alive",
                   (const char *)BoardInfo->Name,
                   (const char *)BoardInfo->MacAddr,
                   (const char *)BoardInfo->UUID);
    (void)snprintf(context_data, sizeof(context_data),
                   "{\"application\":{\"name\":\"xiaozhi\",\"version\":\"%s\"},"
                   "\"board\":{\"type\":\"%s\",\"mac\":\"%s\"}}",
                   (const char *)BoardInfo->Version,
                   (const char *)BoardInfo->Name,
                   (const char *)BoardInfo->MacAddr);

    ret = xiaozhi->Http.ClientPostSync(OtaUrl, context_type, context_data,
                                       (char *)response, (int32_t)sizeof(response));
    if (ret != 0)
    {
        VN_LOG_E("ota https post failed, ret: %d", ret);
        return -1;
    }

    json = strstr((const char *)response, "\r\n\r\n");
    if (json == NULL)
    {
        VN_LOG_E("ota response has no JSON body");
        return -1;
    }
    ret = Vn_Ota_ParseOtaJson(json + 4, OtaRspInfo);
    if (ret != 0)
    {
        VN_LOG_E("parse json failed");
        return -1;
    }

    VN_LOG_D("ota info: chat url: %s, {version: %s, url: %s}",
             (const char *)OtaRspInfo->ChatUrl,
             (const char *)OtaRspInfo->Firmware.Version,
             (const char *)OtaRspInfo->Firmware.Url);
    return 0;
}
