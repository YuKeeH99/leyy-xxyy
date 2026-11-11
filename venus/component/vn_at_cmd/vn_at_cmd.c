#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ql_boot.h"
#include "ql_api_map_common.h"

#include "vn_at_cmd.h"
#include "vn_at_test_app.h"
#include "vn_audio_app.h"
#include "vn_factory.h"
#include "vn_plat.h"
#include "vn_wifi.h"
#include "vn_wifi_app.h"

#define VN_AT_RESPONSE_BUF_LEN  128
#define VN_AT_PARAM_STR_LEN     24
#define VN_AT_OTA_PARAM_STR_LEN 256

typedef void (*_api_quec_at_srv_set_app_shell_cmds_t)(utlAtCommand_T *shell_cmds,
                                                       int num);
typedef int (*_api_atRespStr_t)(UINT32 reqHandle, UINT8 resultCode,
                                UINT16 errCode, char *respString);
typedef BOOL (*_api_getExtString_t)(const utlAtParameterValue_P2c param_value_p,
                                    int index, CHAR *outString,
                                    INT16 maxStringLength,
                                    INT16 *outStringLength,
                                    CHAR *defaultString);

static _api_quec_at_srv_set_app_shell_cmds_t m_quec_at_srv_set_app_shell_cmds;
static _api_atRespStr_t m_atRespStr;
static _api_getExtString_t m_getExtString;

unsigned short gusXid = NUM_OF_TEL_ATP;

void ql_at_srv_set_app_shell_cmds(utlAtCommand_T *shell_cmds, int num)
{
    if (m_quec_at_srv_set_app_shell_cmds == NULL)
    {
        s32 ptr = m_get_api_ptr((char *)__func__);

        if (ptr == 0)
        {
            SDK_API_DEBUG_NOTSUP();
            return;
        }
        m_quec_at_srv_set_app_shell_cmds =
            (_api_quec_at_srv_set_app_shell_cmds_t)ptr;
    }

    m_quec_at_srv_set_app_shell_cmds(shell_cmds, num);
}

int atRespStr(UINT32 reqHandle, UINT8 resultCode, UINT16 errCode, char *respString)
{
    if (m_atRespStr == NULL)
    {
        s32 ptr = m_get_api_ptr((char *)__func__);

        if (ptr == 0)
        {
            SDK_API_DEBUG_NOTSUP();
            return 0;
        }
        m_atRespStr = (_api_atRespStr_t)ptr;
    }

    return m_atRespStr(reqHandle, resultCode, errCode, respString);
}

BOOL getExtString(const utlAtParameterValue_P2c param_value_p, int index,
                  CHAR *outString, INT16 maxStringLength,
                  INT16 *outStringLength, CHAR *defaultString)
{
    if (m_getExtString == NULL)
    {
        s32 ptr = m_get_api_ptr((char *)__func__);

        if (ptr == 0)
        {
            SDK_API_DEBUG_NOTSUP();
            return 0;
        }
        m_getExtString = (_api_getExtString_t)ptr;
    }

    return m_getExtString(param_value_p, index, outString, maxStringLength,
                          outStringLength, defaultString);
}

static utlAtParameter_T Vn_AT_OTA_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_READ_WRITE,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_READ_WRITE,
                                  utlAT_PARAMETER_PRESENCE_OPTIONAL),
};

static utlAtParameter_T Vn_AT_VOLUME_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_READ_WRITE,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
};

static utlAtParameter_T Vn_AT_AUDIO_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_READ_WRITE,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_READ_WRITE,
                                  utlAT_PARAMETER_PRESENCE_OPTIONAL),
};

static utlAtParameter_T Vn_AT_WifiSsidDel_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_OPTIONAL),
};

static utlAtParameter_T Vn_AT_WifiOta_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
};

static utlAtParameter_T Vn_AT_WifiEnable_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_REQUIRED),
};

static utlAtParameter_T Vn_AT_WifiVer_Test[] = {
    utlDEFINE_STRING_AT_PARAMETER(utlAT_PARAMETER_ACCESS_WRITE_ONLY,
                                  utlAT_PARAMETER_PRESENCE_OPTIONAL),
};

utlAtCommand_T Vn_At_Cmds[VN_AT_CMD_COUNT] = {
    utlDEFINE_EXTENDED_AT_COMMAND(0, "+VNOTATEST", Vn_AT_OTA_Test,
                                  "+VNOTATEST,OK", Vn_AT_OTA_Test_Cmd,
                                  Vn_AT_OTA_Test_Cmd),
    utlDEFINE_EXTENDED_AT_COMMAND(1, "+VOLUMETEST", Vn_AT_VOLUME_Test,
                                  "+VOLUMETEST,OK", Vn_AT_VOLUME_Test_Cmd,
                                  Vn_AT_VOLUME_Test_Cmd),
    utlDEFINE_EXTENDED_AT_COMMAND(2, "+AUDIOTEST", Vn_AT_AUDIO_Test,
                                  "+AUDIOTEST,OK", Vn_AT_AUDIO_Test_Cmd,
                                  Vn_AT_AUDIO_Test_Cmd),
    utlDEFINE_EXTENDED_AT_COMMAND(3, "+SSIDDELTEST", Vn_AT_WifiSsidDel_Test,
                                  "+SSIDDELTEST,OK", Vn_AT_WifiSsidDel_Cmd,
                                  Vn_AT_WifiSsidDel_Cmd),
    utlDEFINE_EXTENDED_AT_COMMAND(4, "+WIFIOTATEST", Vn_AT_WifiOta_Test,
                                  "+WIFIOTATEST,OK", Vn_AT_WifiOta_Cmd,
                                  Vn_AT_WifiOta_Cmd),
    utlDEFINE_EXTENDED_AT_COMMAND(5, "+WIFIENABLETEST", Vn_AT_WifiEnable_Test,
                                  "+WIFIENABLETEST,OK", Vn_AT_WifiEnable_Cmd,
                                  Vn_AT_WifiEnable_Cmd),
    utlDEFINE_EXTENDED_AT_COMMAND(6, "+WIFIVERTEST", Vn_AT_WifiVer_Test,
                                  "+WIFIVERTEST,OK", Vn_AT_WifiVer_Cmd,
                                  Vn_AT_WifiVer_Cmd),
};

static UINT32 Vn_AT_MakeHandle(unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = MAKE_AT_HANDLE(*(TelAtParserID *)arg_p);

    *xid_p = handle;
    return handle;
}

static utlReturnCode_T Vn_AT_Response(UINT32 handle, UINT8 result_code,
                                      UINT16 error_code, char *response)
{
    int ret = atRespStr(handle, result_code, error_code, response);

    return (ret == CIRC_SUCCESS) ? utlSUCCESS : utlFAILED;
}

utlReturnCode_T Vn_AT_OTA_Test_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_OTA_PARAM_STR_LEN] = {0};
    char param_str[VN_AT_OTA_PARAM_STR_LEN] = {0};
    INT16 param_str_len = 0;
    size_t response_len;

    (void)command_name_p;
    (void)num_parameters;
    (void)info_text_p;

    if (op == utlAT_PARAMETER_OP_GET)
    {
        (void)snprintf(buf, sizeof(buf), "+VNOTATEST:OK,%s,%d\r\n",
                       Vn_OTA_AT_Params.url, Vn_OTA_AT_Params.enable);
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
    }

    if (op == utlAT_PARAMETER_OP_SET)
    {
        if (getExtString(parameter_values_p, 0, param_str,
                         VN_AT_OTA_PARAM_STR_LEN - 1, &param_str_len, NULL))
        {
            (void)snprintf(buf, sizeof(buf), "+VNOTATEST:%s", param_str);
            (void)memcpy(Vn_OTA_AT_Params.url, param_str, (size_t)param_str_len);
            Vn_OTA_AT_Params.url[(size_t)param_str_len] = '\0';

            if (getExtString(parameter_values_p, 1, param_str, 1,
                             &param_str_len, "0"))
            {
                response_len = strlen(buf);
                (void)snprintf(buf + response_len, sizeof(buf) - response_len,
                               ",%s", param_str);
                Vn_OTA_AT_Params.enable = (char)atoi(param_str);
            }

            response_len = strlen(buf);
            (void)snprintf(buf + response_len, sizeof(buf) - response_len, "\r\n");
            return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
        }

        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                          CME_OPERATION_NOT_SUPPORTED, NULL);
}

utlReturnCode_T Vn_AT_VOLUME_Test_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_RESPONSE_BUF_LEN] = {0};
    char param_str[VN_AT_PARAM_STR_LEN] = {0};
    INT16 param_str_len = 0;
    size_t response_len;

    (void)command_name_p;
    (void)num_parameters;
    (void)info_text_p;

    if (op == utlAT_PARAMETER_OP_GET)
    {
        (void)snprintf(buf, sizeof(buf), "+VOLUMETEST:OK,%d\r\n",
                       Vn_Audio_Params.Volume);
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
    }

    if (op == utlAT_PARAMETER_OP_SET)
    {
        if (getExtString(parameter_values_p, 0, param_str,
                         VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
        {
            (void)snprintf(buf, sizeof(buf), "+VOLUMETEST:%s", param_str);
            Vn_Audio_Params.Volume = (uint8_t)atoi(param_str);
            response_len = strlen(buf);
            (void)snprintf(buf + response_len, sizeof(buf) - response_len, "\r\n");
            return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
        }

        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                          CME_OPERATION_NOT_SUPPORTED, NULL);
}

utlReturnCode_T Vn_AT_AUDIO_Test_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_RESPONSE_BUF_LEN] = {0};
    char param_str[VN_AT_PARAM_STR_LEN] = {0};
    INT16 param_str_len = 0;
    size_t response_len;

    (void)command_name_p;
    (void)num_parameters;
    (void)info_text_p;

    if (op == utlAT_PARAMETER_OP_GET)
    {
        (void)snprintf(buf, sizeof(buf), "+AUDIOTEST:OK,%d,%d\r\n",
                       Vn_Audio_Params.Enable, Vn_Audio_Params.Mode);
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
    }

    if (op == utlAT_PARAMETER_OP_SET)
    {
        if (getExtString(parameter_values_p, 0, param_str,
                         VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
        {
            (void)snprintf(buf, sizeof(buf), "+AUDIOTEST:%s", param_str);
            Vn_Audio_Params.Enable = (uint8_t)atoi(param_str);

            if (getExtString(parameter_values_p, 1, param_str,
                             VN_AT_PARAM_STR_LEN - 1, &param_str_len, "0"))
            {
                response_len = strlen(buf);
                (void)snprintf(buf + response_len, sizeof(buf) - response_len,
                               ",%s", param_str);
                Vn_Audio_Params.Mode = (uint8_t)atoi(param_str);
            }

            response_len = strlen(buf);
            (void)snprintf(buf + response_len, sizeof(buf) - response_len, "\r\n");
            return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
        }

        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                          CME_OPERATION_NOT_SUPPORTED, NULL);
}

utlReturnCode_T Vn_AT_WifiSsidDel_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_RESPONSE_BUF_LEN] = {0};

    (void)command_name_p;
    (void)parameter_values_p;
    (void)num_parameters;
    (void)info_text_p;

    if (op == utlAT_PARAMETER_OP_SET)
    {
        Vn_WifiApp_DeleteSsidAndPwd();
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                          CME_OPERATION_NOT_SUPPORTED, NULL);
}

utlReturnCode_T Vn_AT_WifiOta_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_RESPONSE_BUF_LEN] = {0};
    char param_str[VN_AT_PARAM_STR_LEN] = {0};
    int8_t ssid[64] = {0};
    int8_t password[64] = {0};
    int8_t ota_wifi_ip[128] = {0};
    INT16 param_str_len = 0;

    (void)command_name_p;
    (void)num_parameters;
    (void)info_text_p;

    if (op != utlAT_PARAMETER_OP_SET)
    {
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_OPERATION_NOT_SUPPORTED, NULL);
    }

    if (!getExtString(parameter_values_p, 0, param_str,
                      VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
    {
        VN_LOG_E("AT+WIFIOTATEST: ssid is null");
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }
    (void)snprintf((char *)ssid, sizeof(ssid), "%s", param_str);

    if (!getExtString(parameter_values_p, 1, param_str,
                      VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
    {
        VN_LOG_E("AT+WIFIOTATEST: pwd is null");
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }
    (void)snprintf((char *)password, sizeof(password), "%s", param_str);

    if (!getExtString(parameter_values_p, 2, param_str,
                      VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
    {
        VN_LOG_E("AT+WIFIOTATEST: ip is null");
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }
    (void)snprintf((char *)ota_wifi_ip, sizeof(ota_wifi_ip), "%s", param_str);

    if (!getExtString(parameter_values_p, 3, param_str,
                      VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
    {
        VN_LOG_E("AT+WIFIOTATEST: port is null");
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    if (Vn_Factory_SetOtaWifi(ssid, password, ota_wifi_ip,
                              (uint16_t)atoi(param_str)) != VN_RET_OK)
    {
        VN_LOG_E("AT+WIFIOTATEST: SetOtaWifi failed");
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
}

utlReturnCode_T Vn_AT_WifiEnable_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_RESPONSE_BUF_LEN] = {0};
    char param_str[VN_AT_PARAM_STR_LEN] = {0};
    INT16 param_str_len = 0;
    uint8_t enable;

    (void)command_name_p;
    (void)num_parameters;
    (void)info_text_p;

    if (op != utlAT_PARAMETER_OP_SET)
    {
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_OPERATION_NOT_SUPPORTED, NULL);
    }

    if (!getExtString(parameter_values_p, 0, param_str,
                      VN_AT_PARAM_STR_LEN - 1, &param_str_len, NULL))
    {
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    enable = (uint8_t)atoi(param_str);
    if (enable == 0U)
    {
        (void)Vn_Wifi_Disable();
    }
    else if (enable == 1U)
    {
        (void)Vn_Wifi_Enable();
    }
    else
    {
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                              CME_INVALID_PARAM, NULL);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
}

utlReturnCode_T Vn_AT_WifiVer_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p)
{
    UINT32 handle = Vn_AT_MakeHandle(xid_p, arg_p);
    char buf[VN_AT_RESPONSE_BUF_LEN] = {0};

    (void)command_name_p;
    (void)parameter_values_p;
    (void)num_parameters;
    (void)info_text_p;

    if ((op == utlAT_PARAMETER_OP_SET) ||
        (op == utlAT_PARAMETER_OP_ACTION) ||
        (op == utlAT_PARAMETER_OP_SYNTAX))
    {
        (void)snprintf(buf, sizeof(buf), "+WIFIVERTEST: %s\r\n",
                       Vn_WifiApp_GetVersion());
        return Vn_AT_Response(handle, ATCI_RESULT_CODE_OK, 0U, buf);
    }

    return Vn_AT_Response(handle, ATCI_RESULT_CODE_CME_ERROR,
                          CME_OPERATION_NOT_SUPPORTED, NULL);
}

static void Vn_Set_AT_Cmds(void *argv)
{
    (void)argv;
    ql_at_srv_set_app_shell_cmds(Vn_At_Cmds, (int)VN_AT_CMD_COUNT);
}

user_boot_init(Vn_Set_AT_Cmds, 1);
