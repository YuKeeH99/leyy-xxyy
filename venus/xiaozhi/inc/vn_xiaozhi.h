#ifndef __VN_XIAOZHI_H__
#define __VN_XIAOZHI_H__

#include <stdbool.h>
#include <stdint.h>

#include "ql_decoder_cJSON.h"
#include "vn_xiaozhi_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    VN_XIAOZHI_ABORT_REASON_NONE = 0,
    VN_XIAOZHI_ABORT_REASON_WAKEWORD_DETECTED
} __attribute__((__packed__)) VN_Xiaozhi_AbortReason_Typedef;

typedef enum
{
    PROPERTY_BOOLEAN = 0,
    PROPERTY_INTEGER,
    PROPERTY_STRING
} __attribute__((__packed__)) PropertyType_Typedef;

typedef struct
{
    int8_t *Name;
    PropertyType_Typedef Type;
    int32_t MaxValue;
    int32_t MinValue;
    int32_t DefaultValue;
} ToolProperty_Typedef;

typedef enum
{
    VN_XIAOZHI_TTS_MSG_STATE_NONE = 0,
    VN_XIAOZHI_TTS_MSG_STATE_START,
    VN_XIAOZHI_TTS_MSG_STATE_STOP,
    VN_XIAOZHI_TTS_MSG_STATE_SENTENCE_START,
    VN_XIAOZHI_TTS_MSG_STATE_SENTENCE_STOP
} __attribute__((__packed__)) Vn_Xiaozhi_TtsMsgState_Typedef;

typedef enum
{
    VN_XIAOZHI_MSG_TYPE_NONE = 0,
    VN_XIAOZHI_MSG_TYPE_HELLO,
    VN_XIAOZHI_MSG_TYPE_TTS,
    VN_XIAOZHI_MSG_TYPE_LLM,
    VN_XIAOZHI_MSG_TYPE_MCP,
    VN_XIAOZHI_MSG_TYPE_STT,
    VN_XIAOZHI_MSG_TYPE_DEMO_MODE
} __attribute__((__packed__)) Vn_Xiaozhi_MsgType_Typedef;

typedef struct
{
    Vn_Xiaozhi_MsgType_Typedef Type;
    void *Msg;
} Vn_Xiaozhi_Msg_Typedef;

typedef struct
{
    uint8_t SessionID[64];
    Vn_Xiaozhi_AudioParams_Typedef AudioParams;
} Vn_Xiaozhi_HelloMsg_Typedef;

typedef struct
{
    int8_t Text[8];
    int8_t Emotion[32];
} Vn_Xiaozhi_LlmMsg_Typedef;

typedef struct
{
    Vn_Xiaozhi_TtsMsgState_Typedef State;
    int8_t Text[256];
    uint8_t QrCodeUrl[256];
} Vn_Xiaozhi_TtsMsg_Typedef;

typedef Vn_Xiaozhi_TtsMsg_Typedef Vn_Xiaozhi_SttMsg_Typedef;

typedef struct
{
    int8_t Action[32];
    int8_t ImgUrl[256];
    uint8_t Duration;
} Vn_Xiaozhi_DemoModeMsg_Typedef;

void Vn_McpServer_AddTool(cJSON *Tools, char *Name, char *Description,
                          ToolProperty_Typedef *Property);
void Vn_McpServer_ReplyError(int id, uint8_t *message);
void Vn_McpServer_ReplyResult(int id, uint8_t *result);
void Vn_McpServer_GetToolsList(int id, uint8_t *cursor);
void Vn_McpServer_DoToolCall(int id, uint8_t *ToolsName, cJSON *params);
void Vn_McpServer_ParseJosn(cJSON *McpJson);

void Vn_Xiaozhi_SendGreetingRequest(void *Inst, uint8_t *StrType);
void Vn_Xiaozhi_SendHello(void *Inst, bool IsMcpSuppprted);
void Vn_Xiaozhi_SendStartListening(void *Inst, VN_Xiaozhi_ListeningMode_Typedef Mode);
void Vn_Xiaozhi_SendWakeWordDetected(void *Inst, int8_t *WakeWord);
void Vn_Xiaozhi_SendAbortSpeaking(void *Inst, VN_Xiaozhi_AbortReason_Typedef Reason);
void Vn_Xiaozhi_SendHeartBeat(void *Inst);
int32_t Vn_Xiaozhi_DecodeJson(int8_t *JsonStr, Vn_Xiaozhi_Msg_Typedef *Msg);

#ifdef __cplusplus
}
#endif

#endif /* __VN_XIAOZHI_H__ */
