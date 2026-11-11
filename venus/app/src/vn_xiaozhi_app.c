#include "vn_xiaozhi_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ql_nw.h"
#include "ql_rtos.h"
#include "sockets.h"

#include "vn_audio.h"
#include "vn_audio_app.h"
#include "vn_board_info.h"
#include "vn_gpio.h"
#include "vn_opus_pcm.h"
#include "vn_param.h"
#include "vn_queue.h"
#include "vn_utils.h"
#include "vn_wifi.h"
#include "vn_wifi_app.h"
#include "vn_xiaozhi.h"

#define VN_XIAOZHI_TASK_STACK_SIZE          10240U
#define VN_XIAOZHI_WS_POLL_PRIORITY         105U
#define VN_XIAOZHI_AUDIO_OUT_PRIORITY       106U
#define VN_XIAOZHI_QR_DOWNLOAD_PRIORITY     109U
#define VN_XIAOZHI_AUDIO_QUEUE_DEPTH        40
#define VN_XIAOZHI_MSG_QUEUE_DEPTH          10
#define VN_XIAOZHI_AUDIO_FRAME_SIZE         512U
#define VN_XIAOZHI_OPUS_FRAME_SIZE          1536U
#define VN_XIAOZHI_HTTP_RESPONSE_SIZE       20480U
#define VN_XIAOZHI_AUDIO_PLAY_TIMEOUT_MS    20000U
#define VN_XIAOZHI_AUDIO_STOP_TIMEOUT_MS    100U
#define VN_XIAOZHI_NO_VOICE_TIMEOUT_MS      20000U
#define VN_XIAOZHI_NO_OP_TIMEOUT_MS         120000U
#define VN_XIAOZHI_IDLE_TIMEOUT_MS          30000U
#define VN_XIAOZHI_SITTING_TIMEOUT_MS       3600000U
#define VN_XIAOZHI_OTA_URL                  "https://databerrypi.com/xiaozhi/ota"

/* vn_display.h 尚未恢复，以下声明与 A01 显示接口一致。 */
typedef enum
{
    VN_DISPLAY_MSGTYPE_EMOJI = 0,
    VN_DISPLAY_MSGTYPE_CHAT_EMOJI,
    VN_DISPLAY_MSGTYPE_CHAT_TEXT,
    VN_DISPLAY_MSGTYPE_TITLE,
    VN_DISPLAY_MSGTYPE_OTA,
    VN_DISPLAY_MSGTYPE_INITIALIZING,
    VN_DISPLAY_MSGTYPE_SIGNAL,
    VN_DISPLAY_MSGTYPE_BATTERY,
    VN_DISPLAY_MSGTYPE_QRCODE,
    VN_DISPLAY_MSGTYPE_JPEG,
    VN_DISPLAY_MSGTYPE_SLEEP,
    VN_DISPLAY_MSGTYPE_OLED_CHAT_EMOJI,
    VN_DISPLAY_MSGTYPE_MsgBox
} __attribute__((__packed__)) Vn_DisplayMsgType_TypeDef;

extern void Vn_DisplayMsg_Send(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData, uint8_t Size);
extern void Vn_DisplayMsg_SendString(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData);
extern void Vn_DisplayMsg_SendOledEmoji(uint8_t Index);
extern void Vn_DisplayMsg_SendOledEmojiByTxt(uint8_t *Name, uint8_t Size);
extern void Vn_DisplayMsg_SendQrCode(uint8_t *pData, uint16_t Size);
extern void Vn_DisplayMsg_SendJPEG(uint8_t *pData, uint16_t Size, uint8_t duration);
extern void Vn_DisplayMsg_SendSleepMode(uint8_t Index);
extern void Vn_DisplayMsg_SendMsgBox(uint8_t *text, uint8_t time);

/* 以下外部接口在 A01 中存在，但当前对应模块头文件尚未完整恢复。 */
extern int32_t Vn_Wifi_IsConnectedAP(bool *IsConnected);
extern bool Vn_Radar_HadDetectedTarget(void);
extern struct tm *ql_localtime(const time_t *t);
extern void *memccpy(void *dest, const void *src, int c, size_t n);

typedef struct
{
    uint8_t Version[16];
    uint8_t Url[126];
} Vn_OtaFirmwareInfo_Typedef;

typedef struct
{
    uint8_t Code[16];
    uint8_t Url[126];
} Vn_OtaRspActivate_Typedef;

typedef struct
{
    uint32_t ResultNum;
    uint8_t Message[16];
} Vn_OtaRspResigter_Typedef;

typedef struct
{
    Vn_OtaFirmwareInfo_Typedef Firmware;
    uint8_t ChatUrl[126];
    bool IsNendActivate;
    Vn_OtaRspActivate_Typedef Activate;
    Vn_OtaRspResigter_Typedef Resigter;
} Vn_OtaRspInfo_Typedef;

extern int32_t Vn_Ota_ConnectServer(Vn_NetType_Typedef NetType, char *OtaUrl,
                                    Vn_BoardInfo_Typedef *BoardInfo,
                                    Vn_OtaRspInfo_Typedef *OtaRspInfo);
extern int32_t Vn_Ota_CheckVersion(Vn_NetType_Typedef NetType, char *Url,
                                   char *BoardVersion, char *OtaVersion);

typedef enum
{
    VN_IMAGE_TYPE_QRCODE = 1,
    VN_IMAGE_TYPE_IMAGE = 2
} Vn_ImageType_Typedef;

static void Vn_XiaozhiFsmSleep(void);
static void Vn_XiaozhiFsmSpeaking(void);
static void Vn_XiaozhiFsmListening(void);
static void Vn_XiaozhiFsmIdle(void);
static void Vn_XiaozhiFsmKeepConnected(void);
static void Vn_XiaozhiFsmConnecting(void);
static void Vn_Xiaozhi_TextMsgHandler(uint8_t *pData, uint32_t Len);
static int32_t Vn_Xiaozhi_SetVolume(uint8_t Volume);

Vn_Xiaozhi_Typedef Vn_XiaozhiAgent = {
    .InAudio = {
        .Params = {
            .Format = 1U,
            .Channels = 1U,
            .SampleRate = 16000U,
            .FrameDuration = 60U
        }
    }
};

static FsmContext FsmCtx;
static Vn_Queue_Typedef Vn_Xiaozhi_NetAudioOutputQueue;
static Vn_Queue_Typedef Vn_Xiaozhi_NetRecvMsgQueue;
static Vn_Queue_Typedef Vn_Xiaozhi_NetAudioOutStateQueue;
static Vn_Queue_Typedef Vn_Xiaozhi_PictureQueue;
static ql_task_t Vn_Xiaozhi_WsPollTaskRef;
static ql_task_t Vn_Xiaozhi_AudioOutTaskRef;
static ql_task_t Vn_DownQrCodeTaskRef;
static Vn_Xiaozhi_Msg_Typedef Vn_Xiaozhi_Msg;
static uint32_t opus_binary_count;

const char *Vn_XiaozhiEventNames[VN_XIAOZHI_EVENT_MAX] = {
    "VN_XIAOZHI_EVENT_STARTUP",
    "VN_XIAOZHI_EVENT_WS_CONNECTING",
    "VN_XIAOZHI_EVENT_WS_CONNECTED",
    "VN_XIAOZHI_EVENT_TIMEOUT_NO_OP",
    "VN_XIAOZHI_EVENT_RADAR_DETECTED",
    "VN_XIAOZHI_EVENT_WAKEUP_WORD",
    "VN_XIAOZHI_EVENT_TTS_START",
    "VN_XIAOZHI_EVENT_TTS_STOP_AUTO_LISTEN",
    "VN_XIAOZHI_EVENT_TTS_STOP_NO_AUTO",
    "VN_XIAOZHI_EVENT_ABORT",
    "VN_XIAOZHI_EVENT_TIMEOUT_NO_VOICE",
    "VN_XIAOZHI_EVENT_TOUCH",
    "VN_XIAOZHI_EVENT_WS_ABORT",
    "VN_XIAOZHI_EVENT_NET_ERROR",
    "VN_XIAOZHI_EVENT_FIRST_ENTRY",
    "VN_XIAOZHI_EVENT_NET_TYPE_CHANGE"
};

const char *Vn_XiaozhiStateNames[VN_XIAOZHI_STATE_MAX] = {
    "NONE",
    "Start up",
    "Connecting",
    "KeepConnected",
    "IDLE",
    "Sleep",
    "Listening",
    "Speaking"
};

FsmTransition FsmTable[VN_XIAOZHI_STATE_MAX][VN_XIAOZHI_EVENT_MAX] = {
    [VN_XIAOZHI_STATE_STARTUP] = {
        [VN_XIAOZHI_EVENT_WS_CONNECTING] = {NULL, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    },
    [VN_XIAOZHI_STATE_CONNECTING] = {
        [VN_XIAOZHI_EVENT_WS_CONNECTED] = {Vn_XiazhiFsmActionWsConnected, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_NET_ERROR] = {Vn_XiazhiFsmActionNetError, VN_XIAOZHI_STATE_STARTUP},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    },
    [VN_XIAOZHI_STATE_KEEP_CONNECTED] = {
        [VN_XIAOZHI_EVENT_TIMEOUT_NO_OP] = {NULL, VN_XIAOZHI_STATE_IDLE},
        [VN_XIAOZHI_EVENT_WAKEUP_WORD] = {Vn_XiazhiFsmActionWakeup, VN_XIAOZHI_STATE_LISTENING},
        [VN_XIAOZHI_EVENT_TTS_START] = {NULL, VN_XIAOZHI_STATE_SPEAKING},
        [VN_XIAOZHI_EVENT_TTS_STOP_AUTO_LISTEN] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_TOUCH] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_WS_ABORT] = {Vn_XiazhiFsmActionNetError, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_FIRST_ENTRY] = {NULL, VN_XIAOZHI_STATE_LISTENING},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    },
    [VN_XIAOZHI_STATE_IDLE] = {
        [VN_XIAOZHI_EVENT_TIMEOUT_NO_OP] = {NULL, VN_XIAOZHI_STATE_SLEEP},
        [VN_XIAOZHI_EVENT_RADAR_DETECTED] = {NULL, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_WAKEUP_WORD] = {NULL, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    },
    [VN_XIAOZHI_STATE_SLEEP] = {
        [VN_XIAOZHI_EVENT_RADAR_DETECTED] = {Vn_XiazhiFsmActionWakeupFromSleep, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_WAKEUP_WORD] = {Vn_XiazhiFsmActionWakeupFromSleep, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    },
    [VN_XIAOZHI_STATE_LISTENING] = {
        [VN_XIAOZHI_EVENT_TTS_START] = {NULL, VN_XIAOZHI_STATE_SPEAKING},
        [VN_XIAOZHI_EVENT_ABORT] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_TIMEOUT_NO_VOICE] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_TOUCH] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_WS_ABORT] = {Vn_XiazhiFsmActionNetError, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    },
    [VN_XIAOZHI_STATE_SPEAKING] = {
        [VN_XIAOZHI_EVENT_WAKEUP_WORD] = {Vn_XiazhiFsmActionWakeupAbortSpeeking, VN_XIAOZHI_STATE_LISTENING},
        [VN_XIAOZHI_EVENT_TTS_STOP_AUTO_LISTEN] = {NULL, VN_XIAOZHI_STATE_LISTENING},
        [VN_XIAOZHI_EVENT_TTS_STOP_NO_AUTO] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_ABORT] = {NULL, VN_XIAOZHI_STATE_KEEP_CONNECTED},
        [VN_XIAOZHI_EVENT_TOUCH] = {Vn_XiazhiFsmActionTouchAbortSpeeking, VN_XIAOZHI_STATE_LISTENING},
        [VN_XIAOZHI_EVENT_WS_ABORT] = {Vn_XiazhiFsmActionNetError, VN_XIAOZHI_STATE_CONNECTING},
        [VN_XIAOZHI_EVENT_NET_TYPE_CHANGE] = {Vn_XiazhiFsmActionNetChange, VN_XIAOZHI_STATE_STARTUP}
    }
};

FsmStateRunFunc FsmStateRunTable[VN_XIAOZHI_STATE_MAX] = {
    NULL,
    Vn_XiaozhiFsmStartup,
    Vn_XiaozhiFsmConnecting,
    Vn_XiaozhiFsmKeepConnected,
    Vn_XiaozhiFsmIdle,
    Vn_XiaozhiFsmSleep,
    Vn_XiaozhiFsmListening,
    Vn_XiaozhiFsmSpeaking
};

static int32_t Vn_Xiaozhi_SetVolume(uint8_t Volume)
{
    Vn_AudioApp_SetVolume(Volume);
    return VN_RET_OK;
}

Vn_Xiaozhi_Typedef *Vn_Xiaozhi_GetInstance(void)
{
    return &Vn_XiaozhiAgent;
}

void Vn_XiazhiFsmActionNetError(FsmContext *Context)
{
    (void)Context;
    Vn_DisplayMsg_SendMsgBox((uint8_t *)"连接异常，正在重新连接", 5U);
}

void Vn_Xiaozhi_WsPollTask(void)
{
    while (1)
    {
        if (Vn_XiaozhiAgent.Ws.IsConnected != NULL)
        {
            Vn_XiaozhiAgent.Ws.Poll(100U);
        }
        ql_rtos_task_sleep_ms(5U);
    }
}

void Vn_XiazhiFsmActionWakeup(FsmContext *Context)
{
    (void)Context;
    Vn_Xiaozhi_SendWakeWordDetected(&Vn_XiaozhiAgent, (int8_t *)"你好，小新");
    Vn_Xiaozhi_SendStartListening(&Vn_XiaozhiAgent, Vn_XiaozhiAgent.ListeningMode);
}

void Vn_XiazhiFsmActionWsConnected(FsmContext *Context)
{
    static bool IsFirstBoot = true;
    int ret_err = 0;
    uint32_t pcm_frame_size;

    (void)Context;

    VN_LOG_I("opus decode init");
    if (Vn_XiaozhiAgent.OutAudio.OpusDecoder != NULL)
    {
        Vn_Opus_Decoder_Destroy(Vn_XiaozhiAgent.OutAudio.OpusDecoder);
        Vn_XiaozhiAgent.OutAudio.OpusDecoder = NULL;
    }
    Vn_XiaozhiAgent.OutAudio.OpusDecoder = Vn_Opus_Decoder_Init(
        Vn_Opus_GetOpusSampleRateEnum(Vn_XiaozhiAgent.OutAudio.Params.SampleRate),
        Vn_XiaozhiAgent.OutAudio.Params.Channels, &ret_err);
    if (ret_err != 0)
    {
        VN_LOG_E("opus decode init failed ret_err: %d", ret_err);
    }

    pcm_frame_size = Vn_Opus_GetPcmSize(Vn_XiaozhiAgent.OutAudio.Params.SampleRate,
                                        Vn_XiaozhiAgent.OutAudio.Params.Channels,
                                        Vn_XiaozhiAgent.OutAudio.Params.FrameDuration);
    if ((Vn_XiaozhiAgent.OutAudio.PcmFrameData == NULL) ||
        (Vn_XiaozhiAgent.OutAudio.PcmFrameSize != pcm_frame_size))
    {
        if (Vn_XiaozhiAgent.OutAudio.PcmFrameData != NULL)
        {
            free(Vn_XiaozhiAgent.OutAudio.PcmFrameData);
        }
        Vn_XiaozhiAgent.OutAudio.PcmFrameData =
            (int16_t *)malloc((size_t)pcm_frame_size * sizeof(int16_t));
        Vn_XiaozhiAgent.OutAudio.PcmFrameSize = pcm_frame_size;
    }

    VN_LOG_I("opus encode init");
    if (Vn_XiaozhiAgent.InAudio.OpusEncoder != NULL)
    {
        Vn_Opus_Encoder_Destroy(Vn_XiaozhiAgent.InAudio.OpusEncoder);
        Vn_XiaozhiAgent.InAudio.OpusEncoder = NULL;
    }
    Vn_XiaozhiAgent.InAudio.OpusEncoder = Vn_Opus_Encoder_Init(
        Vn_Opus_GetOpusSampleRateEnum(Vn_XiaozhiAgent.InAudio.Params.SampleRate),
        Vn_XiaozhiAgent.InAudio.Params.Channels, &ret_err);
    if (ret_err != 0)
    {
        VN_LOG_E("opus encode init failed ret_err: %d", ret_err);
    }

    pcm_frame_size = Vn_Opus_GetPcmSize(Vn_XiaozhiAgent.InAudio.Params.SampleRate,
                                        Vn_XiaozhiAgent.InAudio.Params.Channels,
                                        Vn_XiaozhiAgent.InAudio.Params.FrameDuration);
    if ((Vn_XiaozhiAgent.InAudio.PcmFrameData == NULL) ||
        (Vn_XiaozhiAgent.InAudio.PcmFrameSize != pcm_frame_size))
    {
        if (Vn_XiaozhiAgent.InAudio.PcmFrameData != NULL)
        {
            free(Vn_XiaozhiAgent.InAudio.PcmFrameData);
        }
        Vn_XiaozhiAgent.InAudio.PcmFrameData =
            (int16_t *)malloc((size_t)pcm_frame_size * sizeof(int16_t));
        Vn_XiaozhiAgent.InAudio.PcmFrameSize = pcm_frame_size;
    }

    if (IsFirstBoot)
    {
        IsFirstBoot = false;
        ql_rtos_task_sleep_ms(1000U);
        Vn_Xiaozhi_SendGreetingRequest(&Vn_XiaozhiAgent, (uint8_t *)"regular_boot");
    }
}

void Vn_XiazhiFsmActionWakeupFromSleep(FsmContext *Context)
{
    (void)Context;
    ql_rtos_task_sleep_ms(3000U);
    VN_LOG_I("Mode index: %d", 1);
    Vn_DisplayMsg_SendSleepMode(1U);
}

static void Vn_XiazhiFsm_StopAudioPlayback(void)
{
    uint8_t discard[VN_XIAOZHI_AUDIO_FRAME_SIZE];

    /* 清空待播放的 TTS 数据，否则打断后扬声器仍会把队列里的音频播完 */
    while (Vn_Queue_Wait(Vn_Xiaozhi_NetAudioOutputQueue, discard,
                         (int)sizeof(discard), QL_NO_WAIT) > 0)
    {
    }
    Vn_Queue_Clear(Vn_Xiaozhi_NetAudioOutStateQueue);
    Vn_XiaozhiAgent.OutAudio.IsPlaying = false;
}

void Vn_XiazhiFsmActionTouchAbortSpeeking(FsmContext *Context)
{
    (void)Context;
    Vn_Xiaozhi_SendAbortSpeaking(&Vn_XiaozhiAgent, VN_XIAOZHI_ABORT_REASON_NONE);
    Vn_XiazhiFsm_StopAudioPlayback();
}

void Vn_XiazhiFsmActionWakeupAbortSpeeking(FsmContext *Context)
{
    (void)Context;
    Vn_Xiaozhi_SendAbortSpeaking(&Vn_XiaozhiAgent, VN_XIAOZHI_ABORT_REASON_WAKEWORD_DETECTED);
    Vn_XiazhiFsm_StopAudioPlayback();
    /* 打断后要重新开启聆听，服务器收到 listen start 才会接收上行音频 */
    Vn_Xiaozhi_SendStartListening(&Vn_XiaozhiAgent, Vn_XiaozhiAgent.ListeningMode);
}

void Vn_DownloadQrcodeTask(void *arg)
{
    Vn_ImageNetInfo_Typedef img_info;
    Vn_ImageNetInfo_Typedef last_img_info = {0};
    char *respbuff = NULL;
    char *header;
    char *body;
    int32_t body_length;

    (void)arg;

    while (1)
    {
        if (Vn_Queue_Wait(Vn_Xiaozhi_PictureQueue, &img_info,
                          (int)sizeof(img_info), QL_NO_WAIT) > 0)
        {
            VN_LOG_D("duration %d, download url: %s", img_info.Duration, img_info.ImageUrl);

            if (respbuff == NULL)
            {
                respbuff = (char *)malloc(VN_XIAOZHI_HTTP_RESPONSE_SIZE);
                if (respbuff == NULL)
                {
                    VN_LOG_E("malloc respbuff error");
                    ql_rtos_task_sleep_ms(100U);
                    continue;
                }
            }

            if ((img_info.Duration == last_img_info.Duration) &&
                (strstr(img_info.ImageUrl, last_img_info.ImageUrl) != NULL))
            {
                VN_LOG_D("skip download image");
            }
            else
            {
                VN_LOG_D("download image");
                memset(respbuff, 0, VN_XIAOZHI_HTTP_RESPONSE_SIZE);
                Vn_XiaozhiAgent.Http.ClientGetSync(img_info.ImageUrl, NULL, NULL,
                                                   respbuff, VN_XIAOZHI_HTTP_RESPONSE_SIZE);
            }

            header = strstr(respbuff, "Content-Length:");
            if (header != NULL)
            {
                body_length = 0;
                sscanf(header, "Content-Length: %d", &body_length);
                VN_LOG_D("decode body length : %d", body_length);
                body = strstr(respbuff, "\r\n\r\n");
                VN_LOG_D("get body, to show qr code");
                body += 4;

                if (img_info.Type == VN_IMAGE_TYPE_QRCODE)
                {
                    Vn_DisplayMsg_SendQrCode((uint8_t *)body, (uint16_t)body_length);
                }
                if (img_info.Type == VN_IMAGE_TYPE_IMAGE)
                {
                    Vn_DisplayMsg_SendJPEG((uint8_t *)body, (uint16_t)body_length,
                                           img_info.Duration);
                }
                memcpy(&last_img_info, &img_info, sizeof(last_img_info));
            }
        }
        ql_rtos_task_sleep_ms(100U);
    }
}

void Vn_Xiaozhi_DownloadQrCodeInit(void)
{
    if (Vn_DownQrCodeTaskRef == NULL)
    {
        Vn_Queue_Create(&Vn_Xiaozhi_PictureQueue, 1);
        VN_LOG_D("task ref is null, create new task");
        ql_rtos_task_create(&Vn_DownQrCodeTaskRef, VN_XIAOZHI_TASK_STACK_SIZE,
                            VN_XIAOZHI_QR_DOWNLOAD_PRIORITY, "DownloadQrCode",
                            Vn_DownloadQrcodeTask, NULL);
    }
}

static void Vn_XiaozhiFsmSleep(void)
{
    if (FsmCtx.IsFirstInFunc)
    {
        Vn_DisplayMsg_SendSleepMode(0U);
        Vn_DisplayMsg_SendOledEmoji(8U);
    }
}

void Vn_XiaozhiFsmHandleEvent(Vn_XiaozhiEvent_Typedef Event)
{
    QlOSStatus status;

    if ((Event >= VN_XIAOZHI_EVENT_MAX) || (FsmCtx.CurrentState >= VN_XIAOZHI_STATE_MAX))
    {
        VN_LOG_E("Invalid event: %d or CurrentState %d", Event, FsmCtx.CurrentState);
        return;
    }
    if (FsmCtx.EventQueue == NULL)
    {
        VN_LOG_E("Event queue is null");
        return;
    }
    if (FsmTable[FsmCtx.CurrentState][Event].NextState == VN_XIAOZHI_STATE_NONE)
    {
        VN_LOG_E("Event: %s, not support in state: %s",
                 Vn_XiaozhiEventNames[Event], Vn_XiaozhiStateNames[FsmCtx.CurrentState]);
        return;
    }

    VN_LOG_I("Event: %s", Vn_XiaozhiEventNames[Event]);
    status = ql_rtos_queue_release(FsmCtx.EventQueue, sizeof(Event),
                                   (u8 *)&Event, 5U);
    if (status != QUEC_SUCCESS)
    {
        VN_LOG_E("Event queue release failed");
    }
}

static void Vn_XizaozhiFsmInit(void)
{
    QlOSStatus status;

    FsmCtx.CurrentState = VN_XIAOZHI_STATE_STARTUP;
    FsmCtx.PrevState = VN_XIAOZHI_STATE_STARTUP;
    FsmCtx.UserData = NULL;
    Vn_XiaozhiAgent.VolumeControl.SetVolume = Vn_Xiaozhi_SetVolume;
    Vn_XiaozhiAgent.VolumeControl.GetVolume = Vn_AudioApp_GetVolume;

    status = ql_rtos_queue_create(&FsmCtx.EventQueue, sizeof(Vn_XiaozhiEvent_Typedef), 5U);
    if (status != QUEC_SUCCESS)
    {
        VN_LOG_E("Event queue create failed, ret: %d", status);
    }
}

static void Vn_XizaozhiFsmRun(void)
{
    static uint32_t LastRunTime;
    Vn_XiaozhiEvent_Typedef event;
    FsmTransition *transition;
    ActionFunc action;

    if ((FsmCtx.CurrentState >= VN_XIAOZHI_STATE_MAX) || (FsmCtx.EventQueue == NULL))
    {
        ql_rtos_task_sleep_ms(5U);
        return;
    }

    if (ql_rtos_queue_wait(FsmCtx.EventQueue, (u8 *)&event,
                           sizeof(event), QL_NO_WAIT) == QUEC_SUCCESS)
    {
        if (event > VN_XIAOZHI_EVENT_NET_TYPE_CHANGE)
        {
            VN_LOG_E("Invalid event: %d", event);
            ql_rtos_task_sleep_ms(5U);
            return;
        }

        FsmCtx.Event = event;
        VN_LOG_I("handling event %s", Vn_XiaozhiEventNames[event]);
        transition = &FsmTable[FsmCtx.CurrentState][event];
        if (transition->NextState == VN_XIAOZHI_STATE_NONE)
        {
            VN_LOG_E("Invalid transition: %s -> %s",
                     Vn_XiaozhiStateNames[FsmCtx.CurrentState],
                     Vn_XiaozhiStateNames[transition->NextState]);
        }
        else
        {
            action = transition->Action;
            FsmCtx.PrevState = FsmCtx.CurrentState;
            FsmCtx.CurrentState = transition->NextState;
            if (action != NULL)
            {
                action(&FsmCtx);
            }
            FsmCtx.IsFirstInFunc = true;
            VN_LOG_I("Transition: %s -> %s",
                     Vn_XiaozhiStateNames[FsmCtx.PrevState],
                     Vn_XiaozhiStateNames[FsmCtx.CurrentState]);
        }
    }

    Vn_XiaozhiAgent.CurTaskState = FsmCtx.CurrentState;
    if (FsmStateRunTable[FsmCtx.CurrentState] != NULL)
    {
        if (Vn_DiffTime(LastRunTime, VN_RTOS_SYSTEM_MS) > 3000U)
        {
            LastRunTime = VN_RTOS_SYSTEM_MS;
            VN_LOG_D("State running: %s", Vn_XiaozhiStateNames[FsmCtx.CurrentState]);
        }
        FsmStateRunTable[FsmCtx.CurrentState]();
    }

    if (FsmCtx.IsFirstInFunc)
    {
        FsmCtx.IsFirstInFunc = false;
    }
    if (FsmCtx.Event != VN_XIAOZHI_EVENT_MAX)
    {
        FsmCtx.Event = VN_XIAOZHI_EVENT_MAX;
    }
    ql_rtos_task_sleep_ms(5U);
}

void Vn_Xiaozhi_App(void *arg)
{
    (void)arg;
    VN_LOG_I("vn xiaozhi app start");

    Vn_XizaozhiFsmInit();
    ql_rtos_task_create(&Vn_Xiaozhi_WsPollTaskRef, VN_XIAOZHI_TASK_STACK_SIZE,
                        VN_XIAOZHI_WS_POLL_PRIORITY, "Ws_Poll",
                        (void (*)(void *))Vn_Xiaozhi_WsPollTask, NULL);
    ql_rtos_task_create(&Vn_Xiaozhi_AudioOutTaskRef, VN_XIAOZHI_TASK_STACK_SIZE,
                        VN_XIAOZHI_AUDIO_OUT_PRIORITY, "Audio_Out",
                        (void (*)(void *))Vn_Xiaozhi_OnAudioOutputTask, NULL);

    while (1)
    {
        Vn_XizaozhiFsmRun();
    }
}

void Vn_Xiaozhi_OnAudioOutput(uint8_t *pData, uint32_t Len)
{
    int32_t decode_len;
    int32_t write_ret;

    if (Vn_XiaozhiAgent.OutAudio.OpusDecoder == NULL)
    {
        VN_LOG_E("opus decoder not init");
        return;
    }

    decode_len = Vn_Opus_Decode(Vn_XiaozhiAgent.OutAudio.OpusDecoder, pData, (int32_t)Len,
                                Vn_XiaozhiAgent.OutAudio.PcmFrameData,
                                (uint16_t)Vn_XiaozhiAgent.OutAudio.PcmFrameSize);
    if (decode_len > 0)
    {
        write_ret = Vn_Audio_PcmPlayerAdd((uint8_t *)Vn_XiaozhiAgent.OutAudio.PcmFrameData,
                                          (uint32_t)decode_len * sizeof(int16_t));
        if (write_ret < 0)
        {
            VN_LOG_E("Audio track write error: %d", write_ret);
        }
        return;
    }
    VN_LOG_E("opus decode error");
}

bool Vn_Xiaozhi_OnAudioInput(void)
{
    static uint32_t ReadTimeStamp;
    int32_t read_ret;

    if (Vn_XiaozhiAgent.InAudio.OpusEncoder == NULL)
    {
        VN_LOG_E("opus encoder not init");
        return false;
    }

    if (Vn_DiffTime(ReadTimeStamp, VN_RTOS_SYSTEM_MS) <
        Vn_XiaozhiAgent.InAudio.Params.FrameDuration)
    {
        return false;
    }

    ReadTimeStamp = VN_RTOS_SYSTEM_MS;
    read_ret = Vn_Audio_PcmRecorderRead((int8_t *)Vn_XiaozhiAgent.InAudio.PcmFrameData,
                                        Vn_XiaozhiAgent.InAudio.PcmFrameSize * sizeof(int16_t));
    if (read_ret >= 0)
    {
        return true;
    }

    VN_LOG_E("Audio track read error: %d", read_ret);
    return false;
}

static void Vn_Xiaozhi_TextMsgHandler(uint8_t *pData, uint32_t Len)
{
    Vn_Xiaozhi_MsgState_Typedef msg_state;
    Vn_ImageNetInfo_Typedef img_info;
    Vn_Xiaozhi_HelloMsg_Typedef *hello_msg;
    Vn_Xiaozhi_TtsMsg_Typedef *tts_msg;
    Vn_Xiaozhi_SttMsg_Typedef *stt_msg;
    Vn_Xiaozhi_LlmMsg_Typedef *llm_msg;
    Vn_Xiaozhi_DemoModeMsg_Typedef *demo_msg;

    VN_LOG_I("get text data len=%d, data=%.*s\r\n", Len, (int)Len, pData);
    if (Vn_Xiaozhi_DecodeJson((int8_t *)pData, &Vn_Xiaozhi_Msg) != 0)
    {
        VN_LOG_E("decode json failed");
        return;
    }

    Vn_Queue_Release(Vn_Xiaozhi_NetRecvMsgQueue, &Vn_Xiaozhi_Msg.Type,
                     sizeof(Vn_Xiaozhi_Msg.Type), QL_NO_WAIT);

    switch (Vn_Xiaozhi_Msg.Type)
    {
        case VN_XIAOZHI_MSG_TYPE_HELLO:
            hello_msg = (Vn_Xiaozhi_HelloMsg_Typedef *)Vn_Xiaozhi_Msg.Msg;
            strncpy((char *)Vn_XiaozhiAgent.SessionID, (char *)hello_msg->SessionID,
                    strlen((char *)hello_msg->SessionID) + 1U);
            Vn_XiaozhiAgent.OutAudio.Params = hello_msg->AudioParams;
            VN_LOG_I("audio param: channels=%d, format=%d, sample_rate=%d, frame_duration=%d",
                     Vn_XiaozhiAgent.OutAudio.Params.Channels,
                     Vn_XiaozhiAgent.OutAudio.Params.Format,
                     Vn_XiaozhiAgent.OutAudio.Params.SampleRate,
                     Vn_XiaozhiAgent.OutAudio.Params.FrameDuration);
            Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_WS_CONNECTED);
            break;

        case VN_XIAOZHI_MSG_TYPE_TTS:
            tts_msg = (Vn_Xiaozhi_TtsMsg_Typedef *)Vn_Xiaozhi_Msg.Msg;
            if (tts_msg->State == VN_XIAOZHI_TTS_MSG_STATE_START)
            {
                Vn_XiaozhiAgent.MsgState = VN_XIAOZHI_MSG_STATE_START;
                VN_LOG_D("tts start");
                msg_state = VN_XIAOZHI_MSG_STATE_START;
                Vn_Queue_Release(Vn_Xiaozhi_NetAudioOutStateQueue, &msg_state,
                                 sizeof(msg_state), QL_NO_WAIT);
                Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TTS_START);
            }
            else if (tts_msg->State == VN_XIAOZHI_TTS_MSG_STATE_STOP)
            {
                Vn_XiaozhiAgent.MsgState = VN_XIAOZHI_MSG_STATE_STOP;
                VN_LOG_D("tts stop");
                msg_state = VN_XIAOZHI_MSG_STATE_STOP;
                Vn_Queue_Release(Vn_Xiaozhi_NetAudioOutStateQueue, &msg_state,
                                 sizeof(msg_state), QL_NO_WAIT);
            }
            else if (tts_msg->State == VN_XIAOZHI_TTS_MSG_STATE_SENTENCE_START)
            {
                VN_LOG_I("tts new sentence start");
                if (Vn_XiaozhiAgent.MsgState == VN_XIAOZHI_MSG_STATE_START)
                {
                    if (strstr((char *)tts_msg->QrCodeUrl, "http") != NULL)
                    {
                        VN_LOG_D("send qr code url queue");
                        memset(&img_info, 0, sizeof(img_info));
                        img_info.Duration = 20U;
                        img_info.Type = VN_IMAGE_TYPE_QRCODE;
                        strncpy(img_info.ImageUrl, (char *)tts_msg->QrCodeUrl,
                                strlen((char *)tts_msg->QrCodeUrl) + 1U);
                        Vn_Queue_Release(Vn_Xiaozhi_PictureQueue, &img_info,
                                         sizeof(img_info), QL_NO_WAIT);
                    }
                    if (tts_msg->Text[0] != '\0')
                    {
                        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_TEXT,
                                           (uint8_t *)tts_msg->Text,
                                           (uint8_t)strlen(tts_msg->Text));
                    }
                }
            }
            break;

        case VN_XIAOZHI_MSG_TYPE_STT:
            stt_msg = (Vn_Xiaozhi_SttMsg_Typedef *)Vn_Xiaozhi_Msg.Msg;
            Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_TEXT, (uint8_t *)stt_msg->Text,
                               (uint8_t)strlen(stt_msg->Text));
            break;

        case VN_XIAOZHI_MSG_TYPE_LLM:
            if (Vn_XiaozhiAgent.MsgState == VN_XIAOZHI_MSG_STATE_START)
            {
                llm_msg = (Vn_Xiaozhi_LlmMsg_Typedef *)Vn_Xiaozhi_Msg.Msg;
                Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_EMOJI, (uint8_t *)llm_msg->Emotion,
                                   (uint8_t)strlen(llm_msg->Emotion));
                Vn_DisplayMsg_SendOledEmojiByTxt((uint8_t *)llm_msg->Emotion,
                                                 (uint8_t)strlen(llm_msg->Emotion));
            }
            break;

        case VN_XIAOZHI_MSG_TYPE_DEMO_MODE:
            demo_msg = (Vn_Xiaozhi_DemoModeMsg_Typedef *)Vn_Xiaozhi_Msg.Msg;
            if (strstr(demo_msg->Action, "image") != NULL)
            {
                memset(&img_info, 0, sizeof(img_info));
                img_info.Type = VN_IMAGE_TYPE_IMAGE;
                img_info.Duration = demo_msg->Duration;
                strncpy(img_info.ImageUrl, demo_msg->ImgUrl, strlen(demo_msg->ImgUrl) + 1U);
                VN_LOG_D("send image queue, type=VN_IMAGE_TYPE_IMAGE");
                Vn_Queue_Release(Vn_Xiaozhi_PictureQueue, &img_info,
                                 sizeof(img_info), QL_NO_WAIT);
            }
            break;

        default:
            break;
    }

    VN_LOG_I("decode json type=%d, ret=%d, ", Vn_Xiaozhi_Msg.Type, 0);
}

void Vn_Websocket_MsgHandler(uint8_t MsgType, uint8_t *pData, uint32_t Len)
{
    if (MsgType == VN_WEBSOCKET_MSG_TEXT)
    {
        Vn_Xiaozhi_TextMsgHandler(pData, Len);
        return;
    }

    if (MsgType == VN_WEBSOCKET_MSG_BINARY)
    {
        opus_binary_count += Len;
        VN_LOG_I("websocket binary data len=%d", Len);
        if (Vn_XiaozhiAgent.MsgState == VN_XIAOZHI_MSG_STATE_START)
        {
            Vn_Queue_Release(Vn_Xiaozhi_NetAudioOutputQueue, pData, (int)Len, QL_NO_WAIT);
        }
    }
}

void Vn_Xiaozhi_OnAudioOutputProgress(void)
{
    static uint32_t AudioPlayTimestamp;
    static uint32_t Timeout = VN_XIAOZHI_AUDIO_PLAY_TIMEOUT_MS;
    Vn_Xiaozhi_MsgType_Typedef msg_type = VN_XIAOZHI_MSG_TYPE_NONE;
    Vn_Xiaozhi_MsgState_Typedef msg_state = VN_XIAOZHI_MSG_STATE_NONE;
    uint8_t audio_data[VN_XIAOZHI_AUDIO_FRAME_SIZE];
    int32_t audio_len;
    uint32_t diff_time;

    Vn_Queue_Wait(Vn_Xiaozhi_NetRecvMsgQueue, &msg_type, sizeof(msg_type), QL_NO_WAIT);
    Vn_Queue_Wait(Vn_Xiaozhi_NetAudioOutStateQueue, &msg_state, sizeof(msg_state), QL_NO_WAIT);

    if (msg_state == VN_XIAOZHI_MSG_STATE_START)
    {
        Timeout = VN_XIAOZHI_AUDIO_PLAY_TIMEOUT_MS;
        Vn_XiaozhiAgent.OutAudio.IsPlaying = true;
        AudioPlayTimestamp = VN_RTOS_SYSTEM_MS;
        VN_LOG_D("Start Play, wait tiomeout: %d ms", Timeout);
    }

    if (!Vn_XiaozhiAgent.OutAudio.IsPlaying)
    {
        return;
    }

    if (msg_type != VN_XIAOZHI_MSG_TYPE_NONE)
    {
        VN_LOG_D("update play timestamp %d", VN_RTOS_SYSTEM_MS);
        AudioPlayTimestamp = VN_RTOS_SYSTEM_MS;
    }

    if (msg_state == VN_XIAOZHI_MSG_STATE_STOP)
    {
        Timeout = VN_XIAOZHI_AUDIO_STOP_TIMEOUT_MS;
        VN_LOG_D("wait stop tiomeout: %d ms", Timeout);
    }

    if (!Vn_XiaozhiAgent.OutAudio.IsPlaying)
    {
        return;
    }

    memset(audio_data, 0, sizeof(audio_data));
    audio_len = Vn_Queue_Wait(Vn_Xiaozhi_NetAudioOutputQueue, audio_data,
                              sizeof(audio_data), QL_NO_WAIT);
    if ((int16_t)audio_len > 0)
    {
        AudioPlayTimestamp = VN_RTOS_SYSTEM_MS;
        Vn_Xiaozhi_OnAudioOutput(audio_data, (uint32_t)(int16_t)audio_len);
    }

    diff_time = Vn_DiffTime(AudioPlayTimestamp, VN_RTOS_SYSTEM_MS);
    if (diff_time > Timeout)
    {
        VN_LOG_I("audio play timeout %d, stop play, timestamp %d",
                 Timeout, VN_RTOS_SYSTEM_MS);
        Vn_XiaozhiAgent.OutAudio.IsPlaying = false;
        if (((uint8_t)Vn_XiaozhiAgent.ListeningMode &
             (uint8_t)~VN_XIAOZHI_LISTENING_MODE_REALTIME) == VN_XIAOZHI_LISTENING_MODE_AUTO)
        {
            Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TTS_STOP_AUTO_LISTEN);
        }
        else
        {
            Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TTS_STOP_NO_AUTO);
        }
    }
}

void Vn_Xiaozhi_OnAudioOutputTask(void)
{
    while (1)
    {
        Vn_Xiaozhi_OnAudioOutputProgress();
        ql_rtos_task_sleep_ms(5U);
    }
}

static void Vn_XiaozhiFsmSpeaking(void)
{
    if (FsmCtx.IsFirstInFunc)
    {
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"讲话中",
                           (uint8_t)(sizeof("讲话中") - 1U));
        Vn_DisplayMsg_SendOledEmoji(10U);
        Vn_Audio_PcmRecorderSuspend();
    }

    if (!Vn_XiaozhiAgent.Ws.IsConnected())
    {
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_WS_ABORT);
    }
}

static void Vn_XiaozhiFsmListening(void)
{
    static bool IsUpdatedTitle;
    static uint32_t AnimationTimeStamp;
    static uint32_t CheckedVoiceTimestamp;
    static uint32_t LastLogTimeStamp;
    static uint8_t opus_frame_data[VN_XIAOZHI_OPUS_FRAME_SIZE];
    int32_t opus_len;
    int32_t send_ret = 0;
    uint32_t no_voice_diff;

    if (FsmCtx.IsFirstInFunc)
    {
        IsUpdatedTitle = false;
        AnimationTimeStamp = VN_RTOS_SYSTEM_MS;
        CheckedVoiceTimestamp = VN_RTOS_SYSTEM_MS;
        Vn_Audio_PcmRecorderResume();
    }

    if (!IsUpdatedTitle &&
        (Vn_DiffTime(AnimationTimeStamp, VN_RTOS_SYSTEM_MS) > 500U))
    {
        IsUpdatedTitle = true;
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"聆听中",
                           (uint8_t)(sizeof("聆听中") - 1U));
        Vn_DisplayMsg_SendOledEmoji(4U);
    }

    if (Vn_Xiaozhi_OnAudioInput())
    {
        opus_len = Vn_Opus_Encode(Vn_XiaozhiAgent.InAudio.OpusEncoder,
                                  Vn_XiaozhiAgent.InAudio.PcmFrameData,
                                  (int32_t)Vn_XiaozhiAgent.InAudio.PcmFrameSize,
                                  opus_frame_data);
        if (opus_len > 0)
        {
            send_ret = Vn_XiaozhiAgent.Ws.Send(opus_frame_data, (uint32_t)opus_len,
                                               VN_WEBSOCKET_MSG_BINARY);
            VN_LOG_I("opus data len: %d, ret %d", opus_len, send_ret);
        }

        if (Vn_Audio_CheckSpeek(Vn_XiaozhiAgent.InAudio.PcmFrameData,
                                Vn_XiaozhiAgent.InAudio.PcmFrameSize) == 0)
        {
            VN_LOG_D("had checked voice");
            CheckedVoiceTimestamp = VN_RTOS_SYSTEM_MS;
        }

        no_voice_diff = Vn_DiffTime(CheckedVoiceTimestamp, VN_RTOS_SYSTEM_MS);
        if (no_voice_diff > VN_XIAOZHI_NO_VOICE_TIMEOUT_MS)
        {
            Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TIMEOUT_NO_VOICE);
        }
        if ((no_voice_diff - LastLogTimeStamp) >= 1000U)
        {
            VN_LOG_D("no voice diff time: %d s, timeout: %d s",
                     no_voice_diff / 1000U, VN_XIAOZHI_NO_VOICE_TIMEOUT_MS / 1000U);
            LastLogTimeStamp = no_voice_diff;
        }
    }

    if (!Vn_XiaozhiAgent.Ws.IsConnected())
    {
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_WS_ABORT);
    }
}

static void Vn_XiaozhiFsmIdle(void)
{
    static uint32_t CheckTargetTimeStamp;
    static uint32_t NoOpTimeStamp;
    uint32_t no_op_diff;

    if (FsmCtx.IsFirstInFunc)
    {
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"休息中",
                           (uint8_t)(sizeof("休息中") - 1U));
        Vn_DisplayMsg_SendOledEmoji(0U);
    }

    if (Vn_DiffTime(CheckTargetTimeStamp, VN_RTOS_SYSTEM_MS) > 1000U)
    {
        CheckTargetTimeStamp = VN_RTOS_SYSTEM_MS;
        if (Vn_Radar_HadDetectedTarget())
        {
            VN_LOG_I("had detected target");
            Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_RADAR_DETECTED);
        }
        else
        {
            VN_LOG_D("waiting for sleep, no op time %d s, timeout %d s",
                     Vn_DiffTime(NoOpTimeStamp, VN_RTOS_SYSTEM_MS) / 1000U,
                     VN_XIAOZHI_IDLE_TIMEOUT_MS / 1000U);
        }
    }

    no_op_diff = Vn_DiffTime(NoOpTimeStamp, VN_RTOS_SYSTEM_MS);
    if (no_op_diff > VN_XIAOZHI_IDLE_TIMEOUT_MS)
    {
        NoOpTimeStamp = VN_RTOS_SYSTEM_MS;
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TIMEOUT_NO_OP);
    }
}

static void Vn_XiaozhiFsmKeepConnected(void)
{
    static bool IsFirstInEntry = true;
    static uint32_t NoOpTimeStamp;
    static uint32_t CheckTargetTimeStamp;
    static uint32_t KeepAliveTimeStamp;
    static uint32_t UpdateDTimeStamp;
    static uint32_t CheckSittingTimeStamp;
    static uint32_t CheckSittingCount;
    static bool CanShowTime;
    static const uint8_t sitting_emoji[] = {2U, 1U};
    static const uint8_t touch_emoji[] = {3U, 6U, 2U, 7U, 8U, 11U, 1U};
    uint8_t buffer[32];
    time_t now;
    struct tm *local_time;

    if (FsmCtx.IsFirstInFunc)
    {
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"待命中",
                           (uint8_t)(sizeof("待命中") - 1U));
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_TEXT, (uint8_t *)"", 0U);
        Vn_DisplayMsg_SendOledEmoji(6U);
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_EMOJI, (uint8_t *)"neutral",
                           (uint8_t)(sizeof("neutral") - 1U));
        NoOpTimeStamp = VN_RTOS_SYSTEM_MS;
        CheckTargetTimeStamp = VN_RTOS_SYSTEM_MS;
        UpdateDTimeStamp = VN_RTOS_SYSTEM_MS;
        KeepAliveTimeStamp = VN_RTOS_SYSTEM_MS;
    }

    if (Vn_DiffTime(CheckTargetTimeStamp, VN_RTOS_SYSTEM_MS) > 1000U)
    {
        CheckTargetTimeStamp = VN_RTOS_SYSTEM_MS;
        if (Vn_Radar_HadDetectedTarget())
        {
            VN_LOG_I("had detected target");
            CheckSittingCount = 0U;
            NoOpTimeStamp = VN_RTOS_SYSTEM_MS;
        }
        else
        {
            if (CheckSittingCount <= 10U)
            {
                CheckSittingCount++;
            }
            else
            {
                CheckSittingCount = 0U;
                CheckSittingTimeStamp = VN_RTOS_SYSTEM_MS;
            }
            VN_LOG_D("waiting for idle, no op time %d s, timeout %d s",
                     Vn_DiffTime(NoOpTimeStamp, VN_RTOS_SYSTEM_MS) / 1000U,
                     VN_XIAOZHI_NO_OP_TIMEOUT_MS / 1000U);
        }

        VN_LOG_D("check sitting time %d s, timeout %d s",
                 Vn_DiffTime(CheckSittingTimeStamp, VN_RTOS_SYSTEM_MS) / 1000U,
                 VN_XIAOZHI_SITTING_TIMEOUT_MS / 1000U);
        if (Vn_DiffTime(CheckSittingTimeStamp, VN_RTOS_SYSTEM_MS) >
            VN_XIAOZHI_SITTING_TIMEOUT_MS)
        {
            CheckSittingTimeStamp = VN_RTOS_SYSTEM_MS;
            VN_LOG_I("========check sitting timeout, to do something==========");
            srand((unsigned int)time(NULL));
            Vn_DisplayMsg_SendOledEmoji(sitting_emoji[rand() & 1]);
            Vn_AudioApp_PlaySound(VN_AUDIO_SOUND_INDEX_POPUP);
        }
    }

    if (Vn_DiffTime(NoOpTimeStamp, VN_RTOS_SYSTEM_MS) > VN_XIAOZHI_NO_OP_TIMEOUT_MS)
    {
        Vn_XiaozhiAgent.Ws.DisConnect();
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_TIMEOUT_NO_OP);
    }

    if (FsmCtx.Event == VN_XIAOZHI_EVENT_TOUCH)
    {
        NoOpTimeStamp = VN_RTOS_SYSTEM_MS;
        VN_LOG_I("touch, do something");
        srand((unsigned int)time(NULL));
        Vn_DisplayMsg_SendOledEmoji(touch_emoji[rand() % (int)sizeof(touch_emoji)]);
        Vn_AudioApp_PlaySound(VN_AUDIO_SOUND_INDEX_POPUP);
        ql_rtos_task_sleep_ms(1000U);
    }

    if (!Vn_XiaozhiAgent.Ws.IsConnected())
    {
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_WS_ABORT);
    }

    if (Vn_DiffTime(KeepAliveTimeStamp, VN_RTOS_SYSTEM_MS) > 30000U)
    {
        KeepAliveTimeStamp = VN_RTOS_SYSTEM_MS;
        Vn_Xiaozhi_SendHeartBeat(&Vn_XiaozhiAgent);
    }

    if (Vn_DiffTime(UpdateDTimeStamp, VN_RTOS_SYSTEM_MS) > 10000U)
    {
        UpdateDTimeStamp = VN_RTOS_SYSTEM_MS;
        if (!CanShowTime)
        {
            CanShowTime = true;
            Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"待命中",
                               (uint8_t)(sizeof("待命中") - 1U));
        }
        else
        {
            memset(buffer, 0, sizeof(buffer));
            now = time(NULL);
            local_time = ql_localtime(&now);
            if (local_time != NULL)
            {
                snprintf((char *)buffer, sizeof(buffer), "%02d:%02d",
                         local_time->tm_hour, local_time->tm_min);
                Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, buffer,
                                   (uint8_t)strlen((char *)buffer));
            }
            CanShowTime = false;
        }
    }

    if (IsFirstInEntry && (FsmCtx.Event == VN_XIAOZHI_EVENT_TTS_STOP_AUTO_LISTEN))
    {
        IsFirstInEntry = false;
        Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_FIRST_ENTRY);
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_TEXT, (uint8_t *)"", 0U);
    }
}

static void Vn_XiaozhiFsmConnecting(void)
{
    static bool ConnectTrigger;
    static bool HadDataCall;
    static uint32_t ConnectTimestamp;
    uint8_t headers[512];
    bool is_connected = false;
    int csq = 0;
    int32_t connect_ret;

    if (FsmCtx.IsFirstInFunc)
    {
        ConnectTrigger = false;
        HadDataCall = false;
        ConnectTimestamp = VN_RTOS_SYSTEM_MS;
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"连接中",
                           (uint8_t)(sizeof("连接中") - 1U));
        Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_CHAT_TEXT, (uint8_t *)"正在连接中..",
                           (uint8_t)(sizeof("正在连接中..") - 1U));
    }

    if (!HadDataCall)
    {
        if (Vn_XiaozhiAgent.NetType == VN_NET_TYPE_WIFI)
        {
            Vn_Wifi_IsConnectedAP(&is_connected);
            if (is_connected)
            {
                HadDataCall = true;
                ConnectTrigger = true;
            }
            else
            {
                VN_LOG_W(" not connect ap");
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"网络异常，重新连接");
                Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_NET_ERROR);
                return;
            }
        }
        else
        {
            if ((ql_nw_get_csq(&csq) == QL_NW_SUCCESS) && (csq != 99))
            {
                VN_LOG_D("csq %d", csq);
                HadDataCall = true;
                ConnectTrigger = true;
            }
            else
            {
                VN_LOG_E("get csq fail");
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"网络异常，重新连接");
                Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_NET_ERROR);
                return;
            }
        }
    }

    if (ConnectTrigger)
    {
        memset(headers, 0, sizeof(headers));
        snprintf((char *)headers, sizeof(headers),
                 "Authorization: Bearer <token>\r\n"
                 "Protocol-Version: 1\r\n"
                 "Device-Id: %s\r\n"
                 "Client-Id: %s\r\n",
                 Vn_XiaozhiAgent.MacAddr, Vn_XiaozhiAgent.UUID);
        VN_LOG_I("websocket connect");
        Vn_XiaozhiAgent.LocalPort = ql_soc_generate_port();
        connect_ret = Vn_XiaozhiAgent.Ws.Connect(Vn_XiaozhiAgent.LocalIP,
                                                 Vn_XiaozhiAgent.LocalPort,
                                                 (int8_t *)Vn_XiaozhiAgent.ChatUrl,
                                                 (int8_t *)headers);
        if (connect_ret == 0)
        {
            VN_LOG_I("websocket connect success");
            Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_CHAT_TEXT,
                                     (uint8_t *)"连接成功，等待对话");
            Vn_DisplayMsg_SendOledEmoji(11U);
            Vn_Xiaozhi_SendHello(&Vn_XiaozhiAgent, true);
        }
        else
        {
            VN_LOG_E("websocket connect failed");
            HadDataCall = false;
            Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_CHAT_TEXT,
                                     (uint8_t *)"连接失败，请重新启动");
            Vn_DisplayMsg_SendOledEmoji(7U);
        }
        ConnectTimestamp = VN_RTOS_SYSTEM_MS;
        ConnectTrigger = false;
    }

    if (Vn_DiffTime(ConnectTimestamp, VN_RTOS_SYSTEM_MS) > 10000U)
    {
        VN_LOG_E("websocket connect timeout");
        HadDataCall = false;
    }
}

int32_t Vn_ReadNetTypeFormFlash(int32_t *pNetType)
{
    int32_t ret = Vn_Param_Read(VN_PARAM_KEY_NET_TYPE, (uint8_t *)pNetType,
                                sizeof(*pNetType));
    return (ret == 0) ? VN_RET_ERROR : VN_RET_OK;
}

int32_t Vn_WriteNetTypeToFlash(int32_t NetType)
{
    int32_t ret = Vn_Param_Write(VN_PARAM_KEY_NET_TYPE, (uint8_t *)&NetType,
                                 sizeof(NetType));
    return (ret == 0) ? VN_RET_ERROR : VN_RET_OK;
}

void Vn_XiaozhiFsmStartup(void)
{
    static bool IsFirstBoot = true;
    static Vn_Xiaozhi_IntState_Typedef IntState;
    static Vn_BoardInfo_Typedef BoardInfo;
    static Vn_OtaRspInfo_Typedef OtaRspInfo;
    static uint8_t WifiFailCount;
    static bool IsInitOwnAp;
    int32_t net_type = 0;
    int32_t ret;
    int profile_idx;
    uint8_t display_buffer[128];
    bool is_connected = false;

    if (FsmCtx.IsFirstInFunc)
    {
        IsFirstBoot = true;
        IntState = VN_XIAOZHI_INT_STATE_START;
        WifiFailCount = 0U;
    }

    switch (IntState)
    {
        case VN_XIAOZHI_INT_STATE_START:
            Vn_XiaozhiAgent.IsNetConnected = false;
            if (Vn_ReadNetTypeFormFlash(&net_type) == VN_RET_OK)
            {
                Vn_XiaozhiAgent.NetType = (uint8_t)net_type;
            }
            VN_LOG_I("net type: %s",
                     Vn_XiaozhiAgent.NetType == VN_NET_TYPE_WIFI ? "wifi" : "cellular");
            Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"初始化中",
                               (uint8_t)(sizeof("初始化中") - 1U));

            if (Vn_XiaozhiAgent.NetType == VN_NET_TYPE_WIFI)
            {
                for (profile_idx = 1; profile_idx <= 5; ++profile_idx)
                {
                    ql_stop_data_call(profile_idx, 0);
                }
                Vn_LDO_Control(VN_LDO_WIFI_CHIP, VN_LDO_STATUS_ENABLE);
                ql_rtos_task_sleep_ms(200U);
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"等待连接WIFI");
                IntState = VN_XIAOZHI_INT_STATE_CONNECT_WIFI_AP;
            }
            else
            {
                Vn_LDO_Control(VN_LDO_WIFI_CHIP, VN_LDO_STATUS_DISABLE);
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"等待连接4G网络");
                IntState = VN_XIAOZHI_INT_STATE_CONNECT_CELL;
            }

            Vn_Http_Instance(&Vn_XiaozhiAgent.Http, Vn_XiaozhiAgent.NetType);
            Vn_Websocket_Instance(&Vn_XiaozhiAgent.Ws, Vn_XiaozhiAgent.NetType);
            Vn_GetBoardInfo(&BoardInfo, Vn_XiaozhiAgent.NetType);
            memccpy(Vn_XiaozhiAgent.MacAddr, BoardInfo.MacAddr, 0,
                    sizeof(Vn_XiaozhiAgent.MacAddr));
            memccpy(Vn_XiaozhiAgent.UUID, BoardInfo.UUID, 0,
                    sizeof(Vn_XiaozhiAgent.UUID));
            break;

        case VN_XIAOZHI_INT_STATE_CONNECT_CELL:
            Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                     (uint8_t *)"等待连接基站");
            VN_LOG_D("watting for network...");
            ret = Vn_Cellular_RegisterNetwork(10);
            if (ret == 0)
            {
                IntState = VN_XIAOZHI_INT_STATE_CONNECT_SERVER;
                Vn_XiaozhiAgent.IsNetConnected = true;
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"等待连接服务器");
                VN_LOG_E("network register success");
            }
            else
            {
                VN_LOG_E("Network register failed");
            }
            break;

        case VN_XIAOZHI_INT_STATE_CONNECT_WIFI_AP:
            ret = Vn_WifiAppTryConnectAp(5000U);
            if (ret == 0)
            {
                Vn_XiaozhiAgent.IsNetConnected = true;
                IntState = VN_XIAOZHI_INT_STATE_CONNECT_SERVER;
            }
            else
            {
                if (ret == -2)
                {
                    WifiFailCount++;
                    if (WifiFailCount > 2U)
                    {
                        WifiFailCount = 0U;
                        IntState = VN_XIAOZHI_INT_STATE_CONFIG_WIFI_AP;
                    }
                }
                else
                {
                    IntState = VN_XIAOZHI_INT_STATE_CONFIG_WIFI_AP;
                }
                Vn_DisplayMsg_SendMsgBox((uint8_t *)"警告，连接AP错误", 3U);
            }
            break;

        case VN_XIAOZHI_INT_STATE_CONFIG_WIFI_AP:
            if (!IsInitOwnAp)
            {
                ret = Vn_WifiAppConfigOwnAP();
                if (ret == 0)
                {
                    IsInitOwnAp = true;
                }
                else
                {
                    Vn_DisplayMsg_SendMsgBox((uint8_t *)"配置WIFI的AP失败，重试中", 3U);
                }
                break;
            }

            ret = Vn_WifiAppWaitOwnApConnect(5000U);
            if (ret == 0)
            {
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"配置AP成功");
                Vn_XiaozhiAgent.IsNetConnected = true;
                IntState = VN_XIAOZHI_INT_STATE_CONNECT_SERVER;
            }
            else if (ret == -1)
            {
                IsInitOwnAp = false;
                Vn_DisplayMsg_SendMsgBox((uint8_t *)"警告，配置ap失败", 3U);
            }
            else
            {
                WifiFailCount++;
                if (WifiFailCount > 12U)
                {
                    WifiFailCount = 0U;
                    Vn_DisplayMsg_SendMsgBox((uint8_t *)"请及时配置WIFI，若长触摸切换4G", 5U);
                }
            }
            break;

        case VN_XIAOZHI_INT_STATE_CONNECT_SERVER:
            ret = Vn_Ota_ConnectServer(Vn_XiaozhiAgent.NetType, (char *)VN_XIAOZHI_OTA_URL,
                                       &BoardInfo, &OtaRspInfo);
            if (ret == 0)
            {
                memccpy(Vn_XiaozhiAgent.ChatUrl, OtaRspInfo.ChatUrl, 0,
                        sizeof(OtaRspInfo.ChatUrl));
                memset(display_buffer, 0, sizeof(display_buffer));
                snprintf((char *)display_buffer, sizeof(display_buffer),
                         "检查新版本，\n本机版本： %s,\n 编号: %d",
                         BoardInfo.Version, OtaRspInfo.Resigter.ResultNum);
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING, display_buffer);
                IntState = VN_XIAOZHI_INT_STATE_CHECK_VERSION;
            }
            else if (ret == -2)
            {
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"设备注册失败，重试中");
            }
            else
            {
                Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                         (uint8_t *)"连接服务器失败,重试");
            }
            break;

        case VN_XIAOZHI_INT_STATE_CHECK_VERSION:
            IntState = VN_XIAOZHI_INT_STATE_ENTER_CHAT;
            if (IsFirstBoot)
            {
                IsFirstBoot = false;
                ret = Vn_Ota_CheckVersion(Vn_XiaozhiAgent.NetType,
                                          (char *)OtaRspInfo.Firmware.Url,
                                          (char *)BoardInfo.Version,
                                          (char *)OtaRspInfo.Firmware.Version);
                if ((ret == 0) && (Vn_XiaozhiAgent.NetType == VN_NET_TYPE_WIFI))
                {
                    Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                             (uint8_t *)"WIFI版本升级成功");
                    IntState = VN_XIAOZHI_INT_STATE_START;
                }
            }
            break;

        case VN_XIAOZHI_INT_STATE_ENTER_CHAT:
            Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                     (uint8_t *)"等待连接");
            IntState = VN_XIAOZHI_INT_STATE_START;
            Vn_XiaozhiAgent.Ws.Init(Vn_Websocket_MsgHandler);
            strncpy((char *)Vn_XiaozhiAgent.DeviceInfo.DeviceId,
                    (char *)Vn_XiaozhiAgent.MacAddr,
                    strlen((char *)Vn_XiaozhiAgent.MacAddr) + 1U);
            strncpy((char *)Vn_XiaozhiAgent.DeviceInfo.Name, (char *)BoardInfo.Name,
                    strlen((char *)BoardInfo.Name) + 1U);
            strncpy((char *)Vn_XiaozhiAgent.DeviceInfo.Version, (char *)BoardInfo.Version,
                    strlen((char *)BoardInfo.Version) + 1U);
            VN_LOG_D("vn xiaozhi net audio output queue init");
            Vn_Queue_Create(&Vn_Xiaozhi_NetAudioOutputQueue, VN_XIAOZHI_AUDIO_QUEUE_DEPTH);
            Vn_Queue_Create(&Vn_Xiaozhi_NetRecvMsgQueue, VN_XIAOZHI_MSG_QUEUE_DEPTH);
            Vn_Queue_Create(&Vn_Xiaozhi_NetAudioOutStateQueue, VN_XIAOZHI_MSG_QUEUE_DEPTH);
            Vn_Xiaozhi_DownloadQrCodeInit();
            Vn_XiaozhiFsmHandleEvent(VN_XIAOZHI_EVENT_WS_CONNECTING);
            break;

        case VN_XIAOZHI_INT_STATE_ERROR:
            break;

        default:
            IntState = VN_XIAOZHI_INT_STATE_START;
            break;
    }

    if ((Vn_XiaozhiAgent.NetType == VN_NET_TYPE_WIFI) &&
        ((uint8_t)(IntState - VN_XIAOZHI_INT_STATE_CONNECT_SERVER) < 3U))
    {
        Vn_Wifi_IsConnectedAP(&is_connected);
        if (!is_connected)
        {
            IntState = VN_XIAOZHI_INT_STATE_CONNECT_WIFI_AP;
        }
    }
    ql_rtos_task_sleep_ms(2000U);
}

void Vn_XiazhiFsmActionNetChange(FsmContext *Context)
{
    (void)Context;

    if (FsmCtx.PrevState == VN_XIAOZHI_STATE_LISTENING)
    {
        Vn_XiazhiFsmActionTouchAbortSpeeking(&FsmCtx);
    }
    Vn_AudioApp_PlaySound(VN_AUDIO_SOUND_INDEX_POPUP);
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_TITLE, (uint8_t *)"", 0U);

    if (Vn_XiaozhiAgent.NetType == VN_NET_TYPE_WIFI)
    {
        Vn_WriteNetTypeToFlash(VN_NET_TYPE_CELLULAR);
        Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                 (uint8_t *)"正在切换网络，由WIFI切到4G");
    }
    else if (Vn_XiaozhiAgent.NetType == VN_NET_TYPE_CELLULAR)
    {
        Vn_WriteNetTypeToFlash(VN_NET_TYPE_WIFI);
        Vn_DisplayMsg_SendString(VN_DISPLAY_MSGTYPE_INITIALIZING,
                                 (uint8_t *)"正在切换网络，由4G切到WIFI");
    }
    ql_rtos_task_sleep_ms(5000U);
}
