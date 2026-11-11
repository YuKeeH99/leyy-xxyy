#ifndef __VN_XIAOZHI_APP_H__
#define __VN_XIAOZHI_APP_H__

#include "vn_plat.h"
#include "opus.h"
#include "ql_rtos.h"
#include "vn_http.h"
#include "vn_websocket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    VN_XIAOZHI_STATE_NONE = 0,
    VN_XIAOZHI_STATE_STARTUP,
    VN_XIAOZHI_STATE_CONNECTING,
    VN_XIAOZHI_STATE_KEEP_CONNECTED,
    VN_XIAOZHI_STATE_IDLE,
    VN_XIAOZHI_STATE_SLEEP,
    VN_XIAOZHI_STATE_LISTENING,
    VN_XIAOZHI_STATE_SPEAKING,
    VN_XIAOZHI_STATE_MAX
} __attribute__((__packed__)) Vn_XiaozhiState_Typedef;

typedef enum
{
    VN_XIAOZHI_EVENT_STARTUP = 0,
    VN_XIAOZHI_EVENT_WS_CONNECTING,
    VN_XIAOZHI_EVENT_WS_CONNECTED,
    VN_XIAOZHI_EVENT_TIMEOUT_NO_OP,
    VN_XIAOZHI_EVENT_RADAR_DETECTED,
    VN_XIAOZHI_EVENT_WAKEUP_WORD,
    VN_XIAOZHI_EVENT_TTS_START,
    VN_XIAOZHI_EVENT_TTS_STOP_AUTO_LISTEN,
    VN_XIAOZHI_EVENT_TTS_STOP_NO_AUTO,
    VN_XIAOZHI_EVENT_ABORT,
    VN_XIAOZHI_EVENT_TIMEOUT_NO_VOICE,
    VN_XIAOZHI_EVENT_TOUCH,
    VN_XIAOZHI_EVENT_WS_ABORT,
    VN_XIAOZHI_EVENT_NET_ERROR,
    VN_XIAOZHI_EVENT_FIRST_ENTRY,
    VN_XIAOZHI_EVENT_NET_TYPE_CHANGE,
    VN_XIAOZHI_EVENT_MAX
} __attribute__((__packed__)) Vn_XiaozhiEvent_Typedef;

typedef enum
{
    VN_XIAOZHI_INT_STATE_START = 0,
    VN_XIAOZHI_INT_STATE_CONNECT_CELL,
    VN_XIAOZHI_INT_STATE_CONNECT_WIFI_AP,
    VN_XIAOZHI_INT_STATE_CONFIG_WIFI_AP,
    VN_XIAOZHI_INT_STATE_CONNECT_SERVER,
    VN_XIAOZHI_INT_STATE_CHECK_VERSION,
    VN_XIAOZHI_INT_STATE_ENTER_CHAT,
    VN_XIAOZHI_INT_STATE_ERROR
} __attribute__((__packed__)) Vn_Xiaozhi_IntState_Typedef;

typedef enum
{
    VN_XIAOZHI_MSG_STATE_NONE = 0,
    VN_XIAOZHI_MSG_STATE_WAIT,
    VN_XIAOZHI_MSG_STATE_START,
    VN_XIAOZHI_MSG_STATE_STOP
} __attribute__((__packed__)) Vn_Xiaozhi_MsgState_Typedef;

typedef enum
{
    VN_XIAOZHI_LISTENING_MODE_AUTO = 0,
    VN_XIAOZHI_LISTENING_MODE_MANUAL,
    VN_XIAOZHI_LISTENING_MODE_REALTIME
} __attribute__((__packed__)) VN_Xiaozhi_ListeningMode_Typedef;

typedef struct
{
    uint8_t DeviceId[64];
    uint8_t Version[64];
    uint8_t Name[64];
} Vn_Xiaozhi_DeviceInfo_Typedef;

typedef struct
{
    uint8_t Format;
    uint8_t Channels;
    uint32_t SampleRate;
    uint8_t FrameDuration;
} Vn_Xiaozhi_AudioParams_Typedef;

typedef struct
{
    bool IsPlaying;
    OpusDecoder *OpusDecoder;
    int16_t *PcmFrameData;
    uint32_t PcmFrameSize;
    Vn_Xiaozhi_AudioParams_Typedef Params;
} Vn_Xiaozhi_OutAudio_Typedef;

typedef struct
{
    bool IsEnabled;
    OpusEncoder *OpusEncoder;
    int16_t *PcmFrameData;
    uint32_t PcmFrameSize;
    bool IsCheckedSpeaking;
    Vn_Xiaozhi_AudioParams_Typedef Params;
} Vn_Xiaozhi_InAudio_Typedef;

typedef struct
{
    int32_t (*SetVolume)(uint8_t Volume);
    int32_t (*GetVolume)(void);
} Vn_Xiaozhi_VolmueControl_Typedef;

typedef struct
{
    uint8_t NetType;
    uint32_t LocalIP;
    uint16_t LocalPort;
    uint32_t RemoteIP;
    uint16_t RemotePort;
    uint8_t ChatUrl[256];
    uint8_t OtaUrl[256];
    bool IsNetConnected;
    uint8_t MacAddr[18];
    uint8_t UUID[64];
    uint8_t SessionID[64];
    uint32_t CurTaskState;
    Vn_Xiaozhi_MsgState_Typedef MsgState;
    Vn_Xiaozhi_DeviceInfo_Typedef DeviceInfo;
    VN_Xiaozhi_ListeningMode_Typedef ListeningMode;
    Vn_Xiaozhi_OutAudio_Typedef OutAudio;
    Vn_Xiaozhi_InAudio_Typedef InAudio;
    Vn_Http_Typedef Http;
    Vn_Websocket_Typedef Ws;
    Vn_Xiaozhi_VolmueControl_Typedef VolumeControl;
} Vn_Xiaozhi_Typedef;

typedef struct
{
    int8_t ImageUrl[256];
    uint8_t Duration;
    int8_t Type;
} Vn_ImageNetInfo_Typedef;

typedef struct FsmContext FsmContext;
typedef void (*ActionFunc)(FsmContext *Context);
typedef void (*FsmStateRunFunc)(void);

typedef struct
{
    ActionFunc Action;
    Vn_XiaozhiState_Typedef NextState;
} FsmTransition;

struct FsmContext
{
    Vn_XiaozhiState_Typedef CurrentState;
    Vn_XiaozhiState_Typedef PrevState;
    ql_queue_t EventQueue;
    Vn_XiaozhiEvent_Typedef Event;
    bool IsFirstInFunc;
    void *UserData;
};

extern Vn_Xiaozhi_Typedef Vn_XiaozhiAgent;
extern const char *Vn_XiaozhiStateNames[VN_XIAOZHI_STATE_MAX];
extern const char *Vn_XiaozhiEventNames[VN_XIAOZHI_EVENT_MAX];
extern FsmTransition FsmTable[VN_XIAOZHI_STATE_MAX][VN_XIAOZHI_EVENT_MAX];
extern FsmStateRunFunc FsmStateRunTable[VN_XIAOZHI_STATE_MAX];

Vn_Xiaozhi_Typedef *Vn_Xiaozhi_GetInstance(void);
void Vn_Xiaozhi_App(void *arg);
void Vn_XiaozhiFsmHandleEvent(Vn_XiaozhiEvent_Typedef Event);
void Vn_XiaozhiFsmStartup(void);

void Vn_XiazhiFsmActionNetError(FsmContext *Context);
void Vn_XiazhiFsmActionWakeup(FsmContext *Context);
void Vn_XiazhiFsmActionWsConnected(FsmContext *Context);
void Vn_XiazhiFsmActionWakeupFromSleep(FsmContext *Context);
void Vn_XiazhiFsmActionTouchAbortSpeeking(FsmContext *Context);
void Vn_XiazhiFsmActionWakeupAbortSpeeking(FsmContext *Context);
void Vn_XiazhiFsmActionNetChange(FsmContext *Context);

void Vn_Xiaozhi_WsPollTask(void);
void Vn_Xiaozhi_OnAudioOutputTask(void);
void Vn_Xiaozhi_OnAudioOutputProgress(void);
bool Vn_Xiaozhi_OnAudioInput(void);
void Vn_Xiaozhi_OnAudioOutput(uint8_t *pData, uint32_t Len);
void Vn_Websocket_MsgHandler(uint8_t MsgType, uint8_t *pData, uint32_t Len);

void Vn_Xiaozhi_DownloadQrCodeInit(void);
void Vn_DownloadQrcodeTask(void *arg);

int32_t Vn_ReadNetTypeFormFlash(int32_t *pNetType);
int32_t Vn_WriteNetTypeToFlash(int32_t NetType);

#ifdef __cplusplus
}
#endif

#endif
