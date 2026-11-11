#include <stdlib.h>

#include "ql_application.h"
#include "ql_rtos.h"

#include "vn_audio_app.h"
#include "vn_audio.h"
#include "vn_opus_pcm.h"
#include "vn_xiaozhi_app.h"

#define VN_AUDIO_SAMPLE_RATE              16000U
#define VN_AUDIO_CHANNELS                 1U
#define VN_AUDIO_PLAY_TIME_MS             3000U
#define VN_AUDIO_PLAY_FRAME_TIME_MS       60U
#define VN_AUDIO_PLAY_PCM_BUFFER_SAMPLES  1536U
#define VN_AUDIO_OPUS_HEADER_SIZE         4U

#define VN_AUDIO_DEFAULT_VOLUME           70U
#define VN_AUDIO_APP_MODE_LOOPBACK        1U

#define VN_AUDIO_APP_STATE_RECORD_INIT    0U
#define VN_AUDIO_APP_STATE_RECORD_START   1U
#define VN_AUDIO_APP_STATE_RECORD_STOP    2U

/* A01 参数模块使用数值 4 作为音量键；原型来自 ELF DWARF。 */
#define VN_AUDIO_VOLUME_PARAM_KEY         ((const char *)4)
extern int32_t Vn_Param_Read(const char *Key_ID, uint8_t *Val_Buf, uint16_t Buf_Len);
extern int32_t Vn_Param_Write(const char *Key_ID, const uint8_t *Val, uint16_t Val_len);

Vn_Audio_Params_TypeDef Vn_Audio_Params;

static uint8_t Vn_Audio_App_State = VN_AUDIO_APP_STATE_RECORD_INIT;

uint32_t Vn_Audio_GetAppState(void)
{
    return Vn_Audio_App_State;
}

static int32_t Vn_AudioApp_ReadVolume(uint8_t *pVolume)
{
    int32_t ret = Vn_Param_Read(VN_AUDIO_VOLUME_PARAM_KEY, pVolume, sizeof(*pVolume));

    if (ret < 1)
    {
        VN_LOG_E("read volume failed %d", ret);
        return VN_RET_ERROR;
    }

    VN_LOG_D("read volume %d", *pVolume);
    return VN_RET_OK;
}

static int32_t Vn_AudioApp_WriteVolume(uint8_t Volume)
{
    VN_LOG_D("write volume %d", Volume);
    return Vn_Param_Write(VN_AUDIO_VOLUME_PARAM_KEY, &Volume, sizeof(Volume));
}

void Vn_AudioApp_SetVolume(uint8_t Volume)
{
    int32_t is_write_ok;
    uint8_t temp_volume = 0;

    Vn_Audio_SetVolume(Volume);
    is_write_ok = Vn_AudioApp_WriteVolume(Volume);
    if (is_write_ok == 0)
    {
        VN_LOG_E("write audio volume failed!");
    }
    else if ((Vn_AudioApp_ReadVolume(&temp_volume) != VN_RET_OK) || (temp_volume != Volume))
    {
        VN_LOG_E("read audio volume failed!");
    }

    VN_LOG_D("set volume %d", Volume);
}

int32_t Vn_AudioApp_GetVolume(void)
{
    int32_t volume = 50;

    if (Vn_AudioApp_ReadVolume((uint8_t *)&volume) != VN_RET_OK)
    {
        volume = Vn_Audio_GetVolume();
    }

    VN_LOG_D("read volume %d", volume);
    return volume;
}

void Vn_Audio_App(void *arg)
{
    int32_t ret;
    int16_t *pcm_buffer;
    uint8_t *play_buffer;
    uint32_t pcm_buffer_size;
    uint32_t play_buffer_size;
    uint8_t volume = 0;
    static uint8_t last_volume = 8;
    static bool is_checked_speek = false;
    static uint32_t had_cached_count = 0;

    (void)arg;

    VN_LOG_I("Vn_Audio_App start");

    VN_LOG_I("Vn_Audio_WaitReady");
    if (Vn_Audio_WaitReady(5000) != VN_RET_OK)
    {
        VN_LOG_E("Vn_Audio_WaitReady timeout");
        return;
    }

    VN_LOG_I("Vn_Audio_PcmRecorderInit");
    ret = Vn_Audio_PcmPlayerInit(VN_AUDIO_SAMPLE_RATE, VN_AUDIO_CHANNELS);
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("Vn_Audio_PcmPlayerInit failed");
        return;
    }
    Vn_Audio_PcmRecorderSuspend();

    VN_LOG_I("Vn_Audio_PcmPlayerInit");
    ret = Vn_Audio_PcmRecorderInit(VN_AUDIO_SAMPLE_RATE, VN_AUDIO_CHANNELS);
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("Vn_Audio_PcmRecorderInit failed");
        return;
    }

    VN_LOG_I("Vn_Audio_VadInit");
    ret = Vn_Audio_VadInit();
    if (ret != VN_RET_OK)
    {
        VN_LOG_E("Vn_Audio_VadInit failed");
    }

    pcm_buffer_size = VN_PCM_RECORD_SIZE(VN_AUDIO_SAMPLE_RATE, VN_AUDIO_CHANNELS, 10U);
    pcm_buffer = (int16_t *)malloc(pcm_buffer_size);
    if (pcm_buffer == NULL)
    {
        VN_LOG_E("malloc PCM buffer failed");
        return;
    }

    play_buffer_size = VN_PCM_RECORD_SIZE(VN_AUDIO_SAMPLE_RATE, VN_AUDIO_CHANNELS,
                                          VN_AUDIO_PLAY_TIME_MS);
    play_buffer = (uint8_t *)malloc(play_buffer_size);
    if (play_buffer == NULL)
    {
        VN_LOG_E("malloc play buffer failed");
        return;
    }

    Vn_Audio_InitPA();
    Vn_Audio_SetPAMode(VN_AUDIO_PA_MODE4);

    if (Vn_AudioApp_ReadVolume(&volume) != VN_RET_OK)
    {
        VN_LOG_I("read volume failed from flash, set to default %d", VN_AUDIO_DEFAULT_VOLUME);
        volume = VN_AUDIO_DEFAULT_VOLUME;
    }
    Vn_Audio_SetVolume(volume);
    Vn_Audio_Params.Volume = volume;
    Vn_Audio_Params.Enable = 0;
    Vn_Audio_Params.Mode = VN_AUDIO_APP_MODE_LOOPBACK;

    while (1)
    {
        int32_t read_ret;

        if (Vn_Audio_Params.Volume != last_volume)
        {
            VN_LOG_D("set volume to %d", Vn_Audio_Params.Volume);
            Vn_Audio_SetVolume(Vn_Audio_Params.Volume);
            last_volume = Vn_Audio_Params.Volume;
        }

        if (Vn_Audio_Params.Enable != 1U)
        {
            ql_rtos_task_sleep_ms(1000);
            continue;
        }

        if (Vn_Audio_Params.Mode == VN_AUDIO_APP_MODE_LOOPBACK)
        {
            switch (Vn_Audio_App_State)
            {
                case VN_AUDIO_APP_STATE_RECORD_INIT:
                    VN_LOG_I("pcm recorder start .. %ds", VN_AUDIO_PLAY_TIME_MS / 1000U);
                    Vn_Audio_App_State = VN_AUDIO_APP_STATE_RECORD_START;
                    break;

                case VN_AUDIO_APP_STATE_RECORD_START:
                    read_ret = Vn_Audio_PcmRecorderRead((int8_t *)pcm_buffer, pcm_buffer_size);
                    if (read_ret > 0)
                    {
                        ret = Vn_Audio_CheckSpeek(pcm_buffer, (uint32_t)read_ret);
                        if (ret == VN_RET_OK)
                        {
                            is_checked_speek = true;
                        }

                        if ((had_cached_count + (uint32_t)read_ret) < play_buffer_size)
                        {
                            memcpy(play_buffer + had_cached_count, pcm_buffer, (uint32_t)read_ret);
                            had_cached_count += (uint32_t)read_ret;
                        }
                        else
                        {
                            VN_LOG_I("vad checked speek result: %s", is_checked_speek ? "yes" : "no");
                            is_checked_speek = false;
                            VN_LOG_I("pcm recorder, pcm player start .. %ds",
                                     VN_AUDIO_PLAY_TIME_MS / 1000U);
                            Vn_Audio_App_State = VN_AUDIO_APP_STATE_RECORD_STOP;
                        }
                    }
                    break;

                case VN_AUDIO_APP_STATE_RECORD_STOP:
                {
                    uint32_t had_send_count = 0;

                    while (had_cached_count > 0)
                    {
                        uint32_t send_count = had_cached_count > pcm_buffer_size
                                                  ? pcm_buffer_size
                                                  : had_cached_count;
                        int32_t write_ret = Vn_Audio_PcmPlayerAdd(play_buffer + had_send_count,
                                                                  send_count);
                        if (write_ret < 0)
                        {
                            VN_LOG_E("Audio track write error: %d", write_ret);
                        }
                        had_send_count += send_count;
                        had_cached_count -= send_count;
                        ql_rtos_task_sleep_ms(10);
                    }
                    VN_LOG_I("pcm player play done");
                    Vn_Audio_App_State = VN_AUDIO_APP_STATE_RECORD_INIT;
                    break;
                }

                default:
                    Vn_Audio_App_State = VN_AUDIO_APP_STATE_RECORD_INIT;
                    break;
            }
        }

        ql_rtos_task_sleep_ms(5);
    }
}

void Vn_AudioApp_PlaySound(Vn_AudioSoundIndex_Typepef Index)
{
    Vn_Xiaozhi_Typedef *xiaozhi_inst = Vn_Xiaozhi_GetInstance();
    Vn_AudioResource_Typedef audio_resource;
    int16_t pcm_buffer[VN_AUDIO_PLAY_PCM_BUFFER_SAMPLES];
    uint32_t pcm_frame_size;
    uint32_t offset = 0;

    if (Vn_GetAudioResource(Index, &audio_resource) != VN_RET_OK)
    {
        VN_LOG_E("get audio resource failed");
        return;
    }

    pcm_frame_size = Vn_Opus_GetPcmSize(VN_AUDIO_SAMPLE_RATE, VN_AUDIO_CHANNELS,
                                        VN_AUDIO_PLAY_FRAME_TIME_MS);
    while (offset < audio_resource.Length)
    {
        uint16_t opus_len = (uint16_t)(((uint16_t)audio_resource.Data[offset + 2U] << 8) |
                                       (uint16_t)audio_resource.Data[offset + 3U]);
        int32_t ret = Vn_Opus_Decode(xiaozhi_inst->OutAudio.OpusDecoder,
                                     audio_resource.Data + offset + VN_AUDIO_OPUS_HEADER_SIZE,
                                     opus_len, pcm_buffer, (uint16_t)pcm_frame_size);

        if (ret > 0)
        {
            Vn_Audio_PcmPlayerAdd((uint8_t *)pcm_buffer,
                                  pcm_frame_size * sizeof(pcm_buffer[0]));
        }
        offset += VN_AUDIO_OPUS_HEADER_SIZE + opus_len;
    }
}
