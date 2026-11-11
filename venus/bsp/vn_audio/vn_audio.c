#include "vn_audio.h"

#include  "ql_rtos.h"
#include "ql_audio.h"
#include "ql_vad.h"
#include "ql_audio_denoise.h"
#include "ql_gpio.h"




static audio_track_handle Vn_TrackHandle = 0;
static audio_record_handle Vn_RecordHandle = 0;
static uint8_t *Vn_PcmBuffer = NULL;
static void *Vn_VadObj = NULL;

static bool Vn_IsRecording = false;
static bool Vn_IsDetectSpeech = false;


/**
 * @brief Wait for the audio system to be ready.
 * @param TimeoutMs: Timeout in milliseconds.
 * @return VN_RET_OK if success, otherwise VN_RET_ERROR.
 */
int32_t Vn_Audio_WaitReady(uint32_t TimeoutMs)
{
    uint32_t timestamp = 0;

    timestamp = ql_rtos_get_systicks();
    // Initialize the audio system
    while(!ql_get_audio_state())
    {
        ql_rtos_task_sleep_ms(5);

        if((ql_rtos_get_systicks() - timestamp) > TimeoutMs)
        {
            VN_LOG_E("Audio system init timeout!");
            return VN_RET_ERROR;
        }
    }

    return VN_RET_OK;
}

/**
 * @brief Set the volume of the audio.
 * @param Volume: Volume of the audio. (0 ~ 100, 0 is mute, 100 is max volume)
 */
void Vn_Audio_SetVolume(uint8_t Volume)
{
    if (Volume > 100)
    {
        Volume = 100;
    }

    ql_set_volume((AUDIOHAL_SPK_LEVEL_T)((Volume * 11) / 100));
}

int32_t Vn_Audio_GetVolume(void)
{
    return (ql_get_volume() * 100) / 11;
}

/**
 * @brief Initialize the PCM recorder.
 * @param SamleRate: Sample rate of the audio.
 * @param Channels: Number of channels.
 * @return VN_RET_OK if success, otherwise VN_RET_ERROR.
 */
int32_t Vn_Audio_PcmRecorderInit(uint16_t SamleRate, uint8_t Channels)
{
    int32_t ret;
    uint32_t pcm_buff_size = 0;
    char denoise_version[64] = {0};

    // init recording
    Vn_RecordHandle = ql_audio_record_open(SamleRate, Channels, 0, NULL);
    if(Vn_RecordHandle == 0)
    {
        VN_LOG_E("Failed to open audio record");
    }

#if 0
    // init denoise
    ql_set_denoise_flag(1);  // open denoise
    
    VN_LOG_I("get denoise switch: %d ...", ql_get_denoise_flag());
    ql_denoise_get_version(denoise_version);
    
    VN_LOG_I("denoise_version :%s", denoise_version);
    ret = ql_denoise_licenses_verify();

    if(ret != 0)
    {
        VN_LOG_E("ql_denoise_licenses_verify fail:%d",ret);
        ql_set_denoise_flag(0); // close denoise
        return VN_RET_ERROR;
    }
#endif

    return VN_RET_OK;
}

/**
 * @brief Read PCM data from the recorder.
 * @param pBuffer: Buffer to store the PCM data.
 * @param Len: Length of the PCM data.
 * @return Length of the PCM data. <= 0 means error.
 */
int32_t Vn_Audio_PcmRecorderRead(int8_t *pBuffer, uint32_t Len)
{
    int32_t ret;

    ret = ql_audio_record_read(Vn_RecordHandle, (char *)pBuffer, (unsigned int *)&Len);

    if(ret == 0)
    {
        return Len;
    }

    return VN_RET_ERROR;
}

void Vn_Audio_PcmRecorderSuspend(void)
{
    int32_t ret = ql_audio_record_ctrl(Vn_RecordHandle, AUDIO_RECORD_CTRL_SUSPEND, 0);
    VN_LOG_D("suspend recorder ret:%d", ret);
}

void Vn_Audio_PcmRecorderResume(void)
{
    int32_t ret = ql_audio_record_ctrl(Vn_RecordHandle, AUDIO_RECORD_CTRL_RESUME, 0);
    VN_LOG_D("resume recorder ret:%d", ret);
}

/**
 * @brief Deinitialize the PCM recorder.
 * @return None.
 */
void Vn_Audio_PcmRecorderDeinit(void)
{
    if (Vn_PcmBuffer) {
        free(Vn_PcmBuffer);
        Vn_PcmBuffer = NULL;
    }

    if(Vn_RecordHandle)
    {
        ql_audio_record_close(Vn_RecordHandle, 1);
    }
}

/**
 * @brief Initialize the PCM player.
 * @param SamleRate: Sample rate of the audio.
 * @param Channels: Number of channels.
 * @return VN_RET_OK if success, otherwise VN_RET_ERROR.
 */
int32_t Vn_Audio_PcmPlayerInit(uint16_t SamleRate, uint8_t Channels)
{
    int32_t ret = 0;

    ql_set_audio_path_receiver();
    ql_codec_choose(AUD_INTERNAL_CODEC, NULL);

    ql_set_volume(AUDIOHAL_SPK_VOL_6);

    if(Vn_TrackHandle != 0)
    {
        ret = ql_audio_track_close(Vn_TrackHandle, 1);

        if(ret != 0)
        {
            VN_LOG_E("reset pcm player failed");
            return VN_RET_ERROR;
        }
    }

    Vn_TrackHandle = ql_audio_track_open(SamleRate, Channels, 0x100, NULL);

    if(Vn_TrackHandle == 0)
    {
        VN_LOG_E("failed to opcm player track");
        return VN_RET_ERROR;
    }

    return VN_RET_OK;
}

/**
 * @brief Add PCM data to the player.
 * @param pBuffer: Buffer to store the PCM data.
 * @param Len: Length of the PCM data.
 * @return VN_RET_OK if success, otherwise VN_RET_ERROR.
 */
int32_t Vn_Audio_PcmPlayerAdd(uint8_t *pBuffer, uint32_t Len)
{
    if(Vn_TrackHandle == 0)
    {
        return VN_RET_ERROR;
    }

    int32_t ret = ql_audio_track_write(Vn_TrackHandle, (char *)pBuffer, Len);

    if(ret < 0)
    {
        VN_LOG_E("write pcm player failed");
        return VN_RET_ERROR;
    }

    return VN_RET_OK;
}

/**
 * @brief Deinitialize the PCM player.
 * @return None.
 */
void Vn_Audio_PcmPlayerDeinit(void)
{
    if(Vn_TrackHandle != 0)
    {
        ql_audio_track_close(Vn_TrackHandle, 1);
        Vn_TrackHandle = 0;
    }
}

/**
 * @brief Initialize the VAD.
 * @return VN_RET_OK if success, otherwise VN_RET_ERROR.
 */
int32_t Vn_Audio_VadInit(void)
{
    int32_t ret = 0;
    char vad_version[256] = {0};
    QlVadParam vad_param;

    if(Vn_VadObj != NULL)
    {
        VN_LOG_D("VAD has been initialized.");
        return VN_RET_OK;
    }

    // init vad
    VN_LOG_D("Init VAD...");
    Vn_VadObj = ql_vad_create();
    if(Vn_VadObj == NULL)
    {
        VN_LOG_E("Failed to create VAD object.");
        return VN_RET_ERROR;
    }

    vad_param.threshold_speech = 32768;     // Threshold for a single frame of speech; Q16.16 format, 0 - 65536
    vad_param.threshold_nonspeech = 24576;  // Threshold for a single frame of speech; Q16.16 format, 0 - 65536
    vad_param.ring_buf_len_st = 20;         /* The number of windows used to locate the starting point of the human voice.
                                               Each window represents one frame, which is 10 milliseconds; the default value is 20. */
    vad_param.ring_buf_len_ed = 40;         /*  Number of windows for detecting the end point of human voice: One window represents one frame,
                                                which is 10 milliseconds; The default value is 40. */
    vad_param.nframe_enter_speech = 15;     /* Controls the number of frames required to enter the voice state.
                                               The larger the value, the more difficult it is to enter. The range is: [1, ring_buf_len_st]; The default value is 15. */
    vad_param.nframe_enter_nonspeech = 35;  /*  Control the number of frames entering the non-speech state.
                                                The larger the value, the more difficult it is to exit. The range is: [1, ring_buf_len_ed]; The default value is 35. */
    VN_LOG_D("Set VAD parameters");
    ret = ql_vad_set_param(Vn_VadObj, vad_param);
    if(ret != 0)
    {
        VN_LOG_E("Failed to set VAD parameters: %d", ret);
    }
    ret = ql_vad_init(Vn_VadObj);
    if(ret != 0)
    {
        VN_LOG_E("Failed to initialize VAD: %d", ret);
    }
    ret = ql_vad_set_mode(Vn_VadObj, 3);
    if(ret != 0)
    {
        VN_LOG_E("Failed to set VAD mode: %d", ret);
    }

    ql_vad_get_version(vad_version);
    VN_LOG_I("VAD version: %s", vad_version);

    return VN_RET_OK;
}

/**
 * @brief Check whether the input buffer contains speech.
 * @param pBuffer: Buffer to store the PCM data.
 * @param Len: Length of the PCM data.
 * @return 0 if contains speech, otherwise -1.
 */
int32_t Vn_Audio_CheckSpeek(int16_t *pBuffer, uint32_t Len)
{

    if(Vn_VadObj == NULL)
    {
        VN_LOG_D("VAD is not initialized");
        return -1;
    }

    int vad_ret = ql_vad_process(Vn_VadObj, (short *)pBuffer);
    
    if(vad_ret > 0)
    {
        return 0;
    }

    return -1;
}

/**
 * @brief Deinitialize the VAD.
 * @return VN_RET_OK if success, otherwise VN_RET_ERROR.
 */
int32_t Vn_Audio_VadDeInit(void)
{
    int32_t ret = 0;
    if(Vn_VadObj != NULL)
    {
        ret = ql_vad_destroy(Vn_VadObj);

        if(ret == 0)
        {
            Vn_VadObj = NULL;
            return VN_RET_OK;
        }
    }

    return VN_RET_ERROR;
}

int32_t Vn_Audio_InitPA(void)
{
    int32_t ret = 0;

    // radar out
    ret = ql_gpio_init(VN_PIN_PA_EN,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    Vn_Audio_SetPAMode(VN_AUDIO_PA_MODE4);
    
    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

static void Vn_DelayUs(int us)
{
    volatile uint32_t i;
    for(i = 0; i < us * 100; i++)
    {
        i = i + 1;
    }
}

int32_t Vn_Audio_SetPAMode(uint8_t Mode)
{
    int32_t ret = 0;

    uint32_t flip_count = 0;

    if((VN_AUDIO_PA_MODE1 != Mode) && (VN_AUDIO_PA_MODE2 != Mode) &&
       (VN_AUDIO_PA_MODE3 != Mode) && (VN_AUDIO_PA_MODE4 != Mode))
    {
        VN_LOG_E("Invalid PA mode: %d", Mode);
        return VN_RET_ERROR;
    }

    if(VN_AUDIO_PA_MODE1 == Mode)
    {
        flip_count = 0;
    }
    else if(VN_AUDIO_PA_MODE2 == Mode)
    {
        flip_count = 1;
    }
    else if(VN_AUDIO_PA_MODE3 == Mode)
    {
        flip_count = 2;
    }
    else
    {
        flip_count = 3;
    }

    // shutdown PA
    ret = ql_gpio_set_level(VN_PIN_PA_EN, PIN_LEVEL_LOW);
    Vn_DelayUs(600);

    // enable PA
    for (int32_t i = 0; i < flip_count; i++)
    {
        ret = ql_gpio_set_level(VN_PIN_PA_EN, PIN_LEVEL_HIGH);
        Vn_DelayUs(10);
        ret = ql_gpio_set_level(VN_PIN_PA_EN, PIN_LEVEL_LOW);
        Vn_DelayUs(10);
    }
    ret = ql_gpio_set_level(VN_PIN_PA_EN, PIN_LEVEL_HIGH);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}
