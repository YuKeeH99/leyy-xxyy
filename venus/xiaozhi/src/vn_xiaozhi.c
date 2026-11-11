#include "vn_xiaozhi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ql_rtos.h"
#include "vn_log.h"
#include "vn_utils.h"

#define VN_XIAOZHI_JSON_STR_SIZE       256U
#define VN_XIAOZHI_WS_TEXT_FRAME       1U
#define VN_MCP_ERROR_REPLY_SIZE        512U
#define VN_MCP_RESULT_REPLY_SIZE       2028U
#define VN_MCP_DEVICE_STATUS_SIZE      1024U

extern uint64_t ql_rtc_to_timestamp(void);
extern void Vn_XiaozhiFsmHandleEvent(Vn_XiaozhiEvent_Typedef Event);

static void Vn_Xiaozhi_SendMessage(void *Inst, uint8_t *Message)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;

    if ((xiaozhi != NULL) && (Message != NULL) && (xiaozhi->Ws.Send != NULL))
    {
        xiaozhi->Ws.Send(Message, (uint32_t)strlen((const char *)Message),
                         VN_XIAOZHI_WS_TEXT_FRAME);
    }
}

static void Vn_Xiaozhi_FreeMsg(Vn_Xiaozhi_Msg_Typedef *Msg)
{
    if (Msg->Msg != NULL)
    {
        free(Msg->Msg);
        Msg->Msg = NULL;
    }
}

static void generate_16_digit_string(char *str, const char *prefix)
{
    size_t prefix_len;
    int i;

    strcpy(str, prefix);
    prefix_len = strlen(prefix);
    for (i = 0; i < 16; ++i)
    {
        str[prefix_len + (size_t)i] = (char)('0' + (rand() % 10));
    }
    str[prefix_len + 16U] = '\0';
}

void Vn_McpServer_AddTool(cJSON *Tools, char *Name, char *Description,
                          ToolProperty_Typedef *Property)
{
    cJSON *tool = cJSON_CreateObject();
    cJSON *input_schema;
    cJSON *properties;

    cJSON_AddStringToObject(tool, "name", Name);
    cJSON_AddStringToObject(tool, "description", Description);

    input_schema = cJSON_CreateObject();
    cJSON_AddStringToObject(input_schema, "type", "object");
    properties = cJSON_CreateObject();

    if ((Property != NULL) && (Property->Type == PROPERTY_INTEGER))
    {
        cJSON *item_property = cJSON_CreateObject();

        cJSON_AddStringToObject(item_property, "type", "integer");
        cJSON_AddNumberToObject(item_property, "minimum", (double)Property->MinValue);
        cJSON_AddNumberToObject(item_property, "maximum", (double)Property->MaxValue);
        cJSON_AddNumberToObject(item_property, "default", (double)Property->DefaultValue);
        cJSON_AddItemToObject(properties, (const char *)Property->Name, item_property);
    }

    cJSON_AddItemToObject(input_schema, "properties", properties);
    cJSON_AddItemToObject(tool, "inputSchema", input_schema);
    cJSON_AddItemToArray(Tools, tool);
}

void Vn_McpServer_ReplyError(int id, uint8_t *message)
{
    uint8_t *json_str = (uint8_t *)malloc(VN_MCP_ERROR_REPLY_SIZE);

    if (json_str == NULL)
    {
        return;
    }

    snprintf((char *)json_str, VN_MCP_ERROR_REPLY_SIZE,
             "{\"type\": \"mcp\", \"payload\": {\"jsonrpc\":\"2.0\",\"id\":%d,\"error\":{\"message\":\"%s\"}}}",
             id, (const char *)message);
    VN_LOG_D("Reply error: %s", json_str);
    Vn_Xiaozhi_SendMessage(Vn_Xiaozhi_GetInstance(), json_str);
    free(json_str);
}

void Vn_McpServer_ReplyResult(int id, uint8_t *result)
{
    uint8_t *json_str = (uint8_t *)malloc(VN_MCP_RESULT_REPLY_SIZE);

    if (json_str == NULL)
    {
        return;
    }

    snprintf((char *)json_str, VN_MCP_RESULT_REPLY_SIZE,
             "{\"type\": \"mcp\", \"payload\": {\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}}",
             id, (const char *)result);
    VN_LOG_D("Reply result: %s", json_str);
    Vn_Xiaozhi_SendMessage(Vn_Xiaozhi_GetInstance(), json_str);
    free(json_str);
}

void Vn_McpServer_GetToolsList(int id, uint8_t *cursor)
{
    Vn_Xiaozhi_Typedef *xiaozhi = Vn_Xiaozhi_GetInstance();
    cJSON *object;
    cJSON *tools;
    uint8_t *result;
    ToolProperty_Typedef Property;

    VN_LOG_D("Get tools list");

    object = cJSON_CreateObject();
    tools = cJSON_CreateArray();

    if (strstr((const char *)cursor, "cursor.defult") != NULL)
    {
        Vn_McpServer_AddTool(
            tools, "self.get_device_status",
            "Provides the real-time information of the device, including the current status of the audio speaker, screen, battery, network, etc.\r\n"
            "Use this tool for: \r\n"
            "1. Answering questions about current condition (e.g. what is the current volume of the audio speaker?)\r\n"
            "2. As the first step to control the device (e.g. turn up / down the volume of the audio speaker, etc.)",
            NULL);

        memset(&Property, 0, sizeof(Property));
        Property.Name = (int8_t *)"volume";
        Property.Type = PROPERTY_INTEGER;
        Property.MaxValue = 100;
        Property.DefaultValue = xiaozhi->VolumeControl.GetVolume();
        Vn_McpServer_AddTool(
            tools, "self.audio_speaker.set_volume",
            "Set the volume of the audio speaker. If the current volume is unknown, you must call `self.get_device_status` tool first and then call this tool.",
            &Property);
        cJSON_AddStringToObject(object, "nextCursor", "self.chat.exit");
    }
    else if (strstr((const char *)cursor, "self.chat.exit") != NULL)
    {
        Vn_McpServer_AddTool(
            tools, "self.chat.exit",
            "退出当前会话并将设备返回到空闲状态。\r\n"
            "使用场景：\r\n"
            "1. 用户明确请求结束对话(例如'退出'、'再见'、'停止说话'、'重新聊天')\r\n"
            "2. 用户想停止当前交互\r\n"
            "3. 需要任何原因结束聊天会话\r\n"
            "这将停止目前聆听，中止正在进行的说话，关闭音频通道，并将设备设为空闲状态。",
            NULL);
    }

    cJSON_AddItemToObject(object, "tools", tools);
    result = (uint8_t *)cJSON_PrintUnformatted(object);
    if (result != NULL)
    {
        Vn_McpServer_ReplyResult(id, result);
        cJSON_free(result);
    }
    cJSON_Delete(object);
}

void Vn_McpServer_DoToolCall(int id, uint8_t *ToolsName, cJSON *params)
{
    char *params_str = cJSON_PrintUnformatted(params);

    VN_LOG_D("Do tool call: %s, params %s", ToolsName, params_str);
    if (params_str != NULL)
    {
        cJSON_free(params_str);
    }

    if (strcmp((const char *)ToolsName, "self.audio_speaker.set_volume") == 0)
    {
        cJSON *volume_int = cJSON_GetObjectItem(params, "volume");

        if ((volume_int == NULL) || !cJSON_IsNumber(volume_int))
        {
            Vn_McpServer_ReplyError(id, (uint8_t *)"Invalid volume");
            return;
        }

        VN_LOG_I("Set volume: %d", volume_int->valueint);
        Vn_Xiaozhi_GetInstance()->VolumeControl.SetVolume((uint8_t)volume_int->valueint);
        Vn_McpServer_ReplyResult(id, (uint8_t *)"{\"success\": true}");
    }
    else if (strcmp((const char *)ToolsName, "self.chat.exit") == 0)
    {
        VN_LOG_I("Exit chat");
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_ABORT);
        Vn_McpServer_ReplyResult(id, (uint8_t *)"聊天会话即将退出。设备将返回空闲状态。");
    }
    else if (strcmp((const char *)ToolsName, "self.get_device_status") == 0)
    {
        uint8_t *result;
        int32_t volume;

        VN_LOG_I("DeviceC status");
        result = (uint8_t *)malloc(VN_MCP_DEVICE_STATUS_SIZE);
        if (result == NULL)
        {
            return;
        }

        volume = Vn_Xiaozhi_GetInstance()->VolumeControl.GetVolume();
        snprintf((char *)result, VN_MCP_DEVICE_STATUS_SIZE,
                 "{\"type\": \"text\", \"text\": {\"audio_speaker\": {\"volume\": %d}}}",
                 volume);
        Vn_McpServer_ReplyResult(id, result);
        free(result);
    }
}

void Vn_McpServer_ParseJosn(cJSON *McpJson)
{
    cJSON *jsonrpc;
    cJSON *method;
    cJSON *params;
    cJSON *id_json;
    uint8_t message[VN_XIAOZHI_JSON_STR_SIZE] = {0};
    int id;

    if (McpJson == NULL)
    {
        VN_LOG_E("Missing MCP payload");
        return;
    }

    jsonrpc = cJSON_GetObjectItem(McpJson, "jsonrpc");
    if ((jsonrpc == NULL) || !cJSON_IsString(jsonrpc) ||
        (strcmp(jsonrpc->valuestring, "2.0") != 0))
    {
        VN_LOG_E("Invalid JSONRPC version: %s",
                 ((jsonrpc != NULL) && cJSON_IsString(jsonrpc)) ? jsonrpc->valuestring : "null");
        return;
    }

    method = cJSON_GetObjectItem(McpJson, "method");
    if ((method == NULL) || !cJSON_IsString(method))
    {
        VN_LOG_E("Missing method");
        return;
    }

    if (strstr(method->valuestring, "notifications") != NULL)
    {
        return;
    }

    params = cJSON_GetObjectItem(McpJson, "params");
    if ((params != NULL) && !cJSON_IsObject(params))
    {
        VN_LOG_E("Invalid params for method: %s", method->valuestring);
        return;
    }

    id_json = cJSON_GetObjectItem(McpJson, "id");
    if ((id_json == NULL) || !cJSON_IsNumber(id_json))
    {
        VN_LOG_E("Invalid id for method: %s", method->valuestring);
        return;
    }
    id = id_json->valueint;

    if (strcmp(method->valuestring, "initialize") == 0)
    {
        Vn_Xiaozhi_Typedef *xiaozhi = Vn_Xiaozhi_GetInstance();

        if ((params != NULL) && cJSON_IsObject(params))
        {
            (void)cJSON_GetObjectItem(params, "capabilities");
        }
        snprintf((char *)message, sizeof(message),
                 "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"%s\",\"version\":\"%s\"}}",
                 xiaozhi->DeviceInfo.Name, xiaozhi->DeviceInfo.Version);
        return;
    }

    if (strcmp(method->valuestring, "tools/list") == 0)
    {
        strcpy((char *)message, "cursor.defult");
        if (params != NULL)
        {
            cJSON *cursor = cJSON_GetObjectItem(params, "cursor");

            if ((cursor != NULL) && cJSON_IsString(cursor))
            {
                strncpy((char *)message, cursor->valuestring, 128U);
                message[128] = '\0';
            }
        }
        Vn_McpServer_GetToolsList(id, message);
        return;
    }

    if (strcmp(method->valuestring, "tools/call") == 0)
    {
        cJSON *name;
        cJSON *arguments;
        cJSON *stack_size;

        if ((params == NULL) || !cJSON_IsObject(params))
        {
            VN_LOG_E("tools/call: Missing params");
            Vn_McpServer_ReplyError(id, (uint8_t *)"Missing params");
            return;
        }

        name = cJSON_GetObjectItem(params, "name");
        if ((name == NULL) || !cJSON_IsString(name))
        {
            VN_LOG_E("tools/call: Missing name");
            Vn_McpServer_ReplyError(id, (uint8_t *)"Missing name");
            return;
        }

        arguments = cJSON_GetObjectItem(params, "arguments");
        if ((arguments != NULL) && !cJSON_IsObject(arguments))
        {
            VN_LOG_E("tools/call: Invalid arguments");
            Vn_McpServer_ReplyError(id, (uint8_t *)"Invalid arguments");
            return;
        }

        stack_size = cJSON_GetObjectItem(params, "stackSize");
        if ((stack_size != NULL) && !cJSON_IsNumber(stack_size))
        {
            VN_LOG_E("tools/call: Invalid stackSize");
            Vn_McpServer_ReplyError(id, (uint8_t *)"Invalid stackSize");
            return;
        }

        Vn_McpServer_DoToolCall(id, (uint8_t *)name->valuestring, arguments);
        return;
    }

    snprintf((char *)message, sizeof(message), "Method not implemented: %s",
             method->valuestring);
    VN_LOG_E("%s", message);
    Vn_McpServer_ReplyError(id, message);
}

void Vn_Xiaozhi_SendGreetingRequest(void *Inst, uint8_t *StrType)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;
    uint64_t timestamp = ql_rtc_to_timestamp();
    char trace_id[64];
    char request_id[64];
    cJSON *object;
    cJSON *payload;
    cJSON *params;
    cJSON *device_info;
    char *json_str;

    generate_16_digit_string(trace_id, "trace_id_");
    generate_16_digit_string(request_id, "request_id_");

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "session_id", (const char *)xiaozhi->SessionID);
    cJSON_AddStringToObject(object, "type", "cml_message_bridge");
    cJSON_AddStringToObject(object, "api_version", "v1.0");

    payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(payload, "id", 1.0);
    cJSON_AddStringToObject(payload, "destination", "cml_server");
    cJSON_AddStringToObject(payload, "method", "greeting_request");
    cJSON_AddStringToObject(payload, "trace_id", trace_id);
    cJSON_AddStringToObject(payload, "request_id", request_id);

    params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "type", (const char *)StrType);
    cJSON_AddItemToObject(payload, "params", params);

    device_info = cJSON_CreateObject();
    cJSON_AddStringToObject(device_info, "device_id", (const char *)xiaozhi->DeviceInfo.DeviceId);
    cJSON_AddStringToObject(device_info, "version", (const char *)xiaozhi->DeviceInfo.Version);
    cJSON_AddStringToObject(device_info, "name", (const char *)xiaozhi->DeviceInfo.Name);
    cJSON_AddItemToObject(payload, "device_info", device_info);
    cJSON_AddNumberToObject(payload, "timestamp", (double)(timestamp * 1000ULL));
    cJSON_AddItemToObject(object, "payload", payload);

    json_str = cJSON_PrintUnformatted(object);
    VN_LOG_D("Json Format: %s", json_str);
    if (json_str != NULL)
    {
        Vn_Xiaozhi_SendMessage(Inst, (uint8_t *)json_str);
        cJSON_free(json_str);
    }
    cJSON_Delete(object);
}

void Vn_Xiaozhi_SendHello(void *Inst, bool IsMcpSuppprted)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;
    int8_t json_str[VN_XIAOZHI_JSON_STR_SIZE] = {0};
    const char *format;

    if (IsMcpSuppprted)
    {
        format = "{\"type\": \"hello\", \"version\": 1, \"transport\": \"websocket\", \"features\": {\"mcp\": true}, \"audio_params\": {\"format\": \"opus\", \"sample_rate\": %d, \"channels\": %d, \"frame_duration\": %d}}";
    }
    else
    {
        format = "{\"type\": \"hello\", \"version\": 1, \"transport\": \"websocket\", \"audio_params\": {\"format\": \"opus\", \"sample_rate\": %d, \"channels\": %d, \"frame_duration\": %d}}";
    }

    sprintf((char *)json_str, format,
            xiaozhi->InAudio.Params.SampleRate,
            xiaozhi->InAudio.Params.Channels,
            xiaozhi->InAudio.Params.FrameDuration);
    VN_LOG_D("Json Format: %s", json_str);
    Vn_Xiaozhi_SendMessage(Inst, (uint8_t *)json_str);
}

void Vn_Xiaozhi_SendStartListening(void *Inst, VN_Xiaozhi_ListeningMode_Typedef Mode)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;
    int8_t json_str[VN_XIAOZHI_JSON_STR_SIZE] = {0};
    int8_t mode_str[32] = {0};
    const char *mode;

    if (Mode == VN_XIAOZHI_LISTENING_MODE_AUTO)
    {
        mode = "auto";
    }
    else if (Mode == VN_XIAOZHI_LISTENING_MODE_REALTIME)
    {
        mode = "realtime";
    }
    else
    {
        mode = "manual";
    }

    strcpy((char *)mode_str, mode);
    sprintf((char *)json_str,
            "{\"session_id\": \"%s\", \"type\": \"listen\", \"state\": \"start\", \"mode\": \"%s\"}",
            xiaozhi->SessionID, mode_str);
    VN_LOG_D("Json Format: %s", json_str);
    Vn_Xiaozhi_SendMessage(Inst, (uint8_t *)json_str);
}

void Vn_Xiaozhi_SendWakeWordDetected(void *Inst, int8_t *WakeWord)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;
    int8_t json_str[VN_XIAOZHI_JSON_STR_SIZE] = {0};

    sprintf((char *)json_str,
            "{\"session_id\": \"%s\", \"type\": \"listen\", \"state\": \"detect\", \"text\": \"%s\"}",
            xiaozhi->SessionID, WakeWord);
    VN_LOG_D("Json Format: %s", json_str);
    Vn_Xiaozhi_SendMessage(Inst, (uint8_t *)json_str);
}

void Vn_Xiaozhi_SendAbortSpeaking(void *Inst, VN_Xiaozhi_AbortReason_Typedef Reason)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;
    int8_t json_str[VN_XIAOZHI_JSON_STR_SIZE] = {0};

    if (Reason == VN_XIAOZHI_ABORT_REASON_WAKEWORD_DETECTED)
    {
        sprintf((char *)json_str,
                "{\"session_id\": \"%s\", \"type\": \"abort\", \"reason\": \"%s\"}",
                xiaozhi->SessionID, "wake_word_detected");
    }
    else
    {
        sprintf((char *)json_str,
                "{\"session_id\": \"%s\", \"type\": \"abort\"}",
                xiaozhi->SessionID);
    }
    VN_LOG_D("Json Format: %s", json_str);
    Vn_Xiaozhi_SendMessage(Inst, (uint8_t *)json_str);
}

void Vn_Xiaozhi_SendHeartBeat(void *Inst)
{
    Vn_Xiaozhi_Typedef *xiaozhi = (Vn_Xiaozhi_Typedef *)Inst;
    int8_t json_str[VN_XIAOZHI_JSON_STR_SIZE] = {0};
    uint64_t timestamp = ql_rtc_to_timestamp() * 1000ULL;

    sprintf((char *)json_str,
            "{\"session_id\": \"%s\", \"type\": \"heartbeat\", \"timestamp\": %llu }",
            xiaozhi->SessionID, (unsigned long long)timestamp);
    VN_LOG_D("Json Format: %s", json_str);
    VN_LOG_I("Send HeartBeat");
    Vn_Xiaozhi_SendMessage(Inst, (uint8_t *)json_str);
}

int32_t Vn_Xiaozhi_DecodeJson(int8_t *JsonStr, Vn_Xiaozhi_Msg_Typedef *Msg)
{
    cJSON *object;
    cJSON *item;
    int32_t ret = -1;

    if ((JsonStr == NULL) || (Msg == NULL))
    {
        VN_LOG_E("JsonStr or Msg is null");
        return -1;
    }

    Vn_Xiaozhi_FreeMsg(Msg);

    object = cJSON_Parse((const char *)JsonStr);
    if (object == NULL)
    {
        VN_LOG_E("Json prase error");
        return -1;
    }

    item = cJSON_GetObjectItem(object, "type");
    if ((item == NULL) || !cJSON_IsString(item))
    {
        VN_LOG_E("Json get type error");
        goto exit;
    }

    if (strcmp(item->valuestring, "llm") == 0)
    {
        Vn_Xiaozhi_LlmMsg_Typedef *llm_msg;

        Msg->Type = VN_XIAOZHI_MSG_TYPE_LLM;
        llm_msg = (Vn_Xiaozhi_LlmMsg_Typedef *)malloc(sizeof(*llm_msg));
        Msg->Msg = llm_msg;
        if (llm_msg == NULL)
        {
            VN_LOG_E("llm_msg malloc error");
            goto exit;
        }
        memset(llm_msg, 0, sizeof(*llm_msg));

        item = cJSON_GetObjectItem(object, "emotion");
        if ((item != NULL) && cJSON_IsString(item))
        {
            strncpy((char *)llm_msg->Emotion, item->valuestring,
                    sizeof(llm_msg->Emotion) - 1U);
        }
        item = cJSON_GetObjectItem(object, "text");
        if ((item != NULL) && cJSON_IsString(item))
        {
            strncpy((char *)llm_msg->Text, item->valuestring,
                    sizeof(llm_msg->Text) - 1U);
        }
        ret = 0;
    }
    else if (strcmp(item->valuestring, "tts") == 0)
    {
        Vn_Xiaozhi_TtsMsg_Typedef *tts_msg;

        Msg->Type = VN_XIAOZHI_MSG_TYPE_TTS;
        tts_msg = (Vn_Xiaozhi_TtsMsg_Typedef *)malloc(sizeof(*tts_msg));
        Msg->Msg = tts_msg;
        if (tts_msg == NULL)
        {
            VN_LOG_E("tts_msg malloc error");
            goto exit;
        }
        memset(tts_msg, 0, sizeof(*tts_msg));

        item = cJSON_GetObjectItem(object, "state");
        if ((item == NULL) || !cJSON_IsString(item))
        {
            VN_LOG_E("Json get state error");
            goto exit;
        }

        if (strcmp(item->valuestring, "start") == 0)
        {
            tts_msg->State = VN_XIAOZHI_TTS_MSG_STATE_START;
            ret = 0;
        }
        else if (strcmp(item->valuestring, "stop") == 0)
        {
            tts_msg->State = VN_XIAOZHI_TTS_MSG_STATE_STOP;
            ret = 0;
        }
        else if (strcmp(item->valuestring, "sentence_start") == 0)
        {
            cJSON *data;

            tts_msg->State = VN_XIAOZHI_TTS_MSG_STATE_SENTENCE_START;
            item = cJSON_GetObjectItem(object, "text");
            if ((item != NULL) && cJSON_IsString(item))
            {
                strncpy((char *)tts_msg->Text, item->valuestring,
                        sizeof(tts_msg->Text) - 1U);
            }

            data = cJSON_GetObjectItem(object, "data");
            if (data != NULL)
            {
                cJSON *qr_code_url = cJSON_GetObjectItem(data, "qr_code_url");

                if ((qr_code_url != NULL) && cJSON_IsString(qr_code_url))
                {
                    VN_LOG_D("QR Code URL: %s", qr_code_url->valuestring);
                    strncpy((char *)tts_msg->QrCodeUrl, qr_code_url->valuestring,
                            sizeof(tts_msg->QrCodeUrl) - 1U);
                }
            }
            ret = 0;
        }
        else if (strcmp(item->valuestring, "sentence_end") == 0)
        {
            tts_msg->State = VN_XIAOZHI_TTS_MSG_STATE_SENTENCE_STOP;
            ret = 0;
        }
        else
        {
            ret = 0;
        }
    }
    else if (strcmp(item->valuestring, "mcp") == 0)
    {
        Msg->Type = VN_XIAOZHI_MSG_TYPE_MCP;
        VN_LOG_D("type mcp");
        Vn_McpServer_ParseJosn(cJSON_GetObjectItem(object, "payload"));
        ret = 0;
    }
    else if (strcmp(item->valuestring, "hello") == 0)
    {
        Vn_Xiaozhi_HelloMsg_Typedef *hello_msg;
        cJSON *audio_params_json;
        cJSON *format;
        bool is_opus = false;

        hello_msg = (Vn_Xiaozhi_HelloMsg_Typedef *)malloc(sizeof(*hello_msg));
        if (hello_msg == NULL)
        {
            VN_LOG_E("hello_msg malloc error");
            goto exit;
        }
        memset(hello_msg, 0, sizeof(*hello_msg));
        Msg->Type = VN_XIAOZHI_MSG_TYPE_HELLO;
        Msg->Msg = hello_msg;

        audio_params_json = cJSON_GetObjectItem(object, "audio_params");
        if (audio_params_json == NULL)
        {
            goto exit;
        }
        format = cJSON_GetObjectItem(audio_params_json, "format");
        if ((format == NULL) || !cJSON_IsString(format))
        {
            goto exit;
        }

        is_opus = (strcmp(format->valuestring, "opus") == 0);
        hello_msg->AudioParams.Format = 1;

        item = cJSON_GetObjectItem(audio_params_json, "sample_rate");
        if (item == NULL)
        {
            goto exit;
        }
        hello_msg->AudioParams.SampleRate = (uint32_t)item->valueint;

        item = cJSON_GetObjectItem(audio_params_json, "channels");
        if (item == NULL)
        {
            goto exit;
        }
        hello_msg->AudioParams.Channels = (uint8_t)item->valueint;

        item = cJSON_GetObjectItem(audio_params_json, "frame_duration");
        if (item == NULL)
        {
            goto exit;
        }
        hello_msg->AudioParams.FrameDuration = (uint8_t)item->valueint;

        item = cJSON_GetObjectItem(object, "session_id");
        if ((item == NULL) || !cJSON_IsString(item))
        {
            goto exit;
        }
        strncpy((char *)hello_msg->SessionID, item->valuestring,
                sizeof(hello_msg->SessionID) - 1U);
        ret = is_opus ? 0 : -1;
    }
    else if (strcmp(item->valuestring, "stt") == 0)
    {
        Vn_Xiaozhi_SttMsg_Typedef *stt_msg;

        stt_msg = (Vn_Xiaozhi_SttMsg_Typedef *)malloc(sizeof(*stt_msg));
        Msg->Type = VN_XIAOZHI_MSG_TYPE_STT;
        Msg->Msg = stt_msg;
        if (stt_msg == NULL)
        {
            VN_LOG_E("stt_msg malloc error");
            goto exit;
        }
        memset(stt_msg, 0, sizeof(*stt_msg));

        item = cJSON_GetObjectItem(object, "text");
        if ((item != NULL) && cJSON_IsString(item))
        {
            strncpy((char *)stt_msg->Text, item->valuestring,
                    sizeof(stt_msg->Text) - 1U);
        }
        ret = 0;
    }
    else if (strcmp(item->valuestring, "demo_mode") == 0)
    {
        Vn_Xiaozhi_DemoModeMsg_Typedef *demo_mode_msg;

        Msg->Type = VN_XIAOZHI_MSG_TYPE_DEMO_MODE;
        demo_mode_msg = (Vn_Xiaozhi_DemoModeMsg_Typedef *)malloc(sizeof(*demo_mode_msg));
        Msg->Msg = demo_mode_msg;
        if (demo_mode_msg == NULL)
        {
            VN_LOG_E("demo_mode_msg malloc error");
            goto exit;
        }
        memset(demo_mode_msg, 0, sizeof(*demo_mode_msg));

        item = cJSON_GetObjectItem(object, "action");
        if ((item != NULL) && cJSON_IsString(item))
        {
            strncpy((char *)demo_mode_msg->Action, item->valuestring,
                    sizeof(demo_mode_msg->Action) - 1U);
        }

        item = cJSON_GetObjectItem(object, "img_url");
        if ((item != NULL) && cJSON_IsString(item))
        {
            strncpy((char *)demo_mode_msg->ImgUrl, item->valuestring,
                    sizeof(demo_mode_msg->ImgUrl) - 1U);
            VN_LOG_D("img_url: %s", demo_mode_msg->ImgUrl);
        }

        item = cJSON_GetObjectItem(object, "duration");
        if ((item != NULL) && cJSON_IsNumber(item))
        {
            demo_mode_msg->Duration = (uint8_t)item->valueint;
            VN_LOG_D("duration: %d", demo_mode_msg->Duration);
        }
        ret = 0;
    }

exit:
    cJSON_Delete(object);
    return ret;
}
