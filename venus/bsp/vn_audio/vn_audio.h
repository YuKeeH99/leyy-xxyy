#ifndef __VN_AUDIO_H__
#define __VN_AUDIO_H__ 

#include "vn_plat.h"

#define VN_PCM_RECORD_SIZE(RATE, CHANNELS, TIME) (RATE * CHANNELS * 2 / 1000 * TIME)

#define VN_AUDIO_PA_MODE1  1   // Gain = 12dB
#define VN_AUDIO_PA_MODE2  2   // Gain = 16dB
#define VN_AUDIO_PA_MODE3  3   // Gain = 24dB
#define VN_AUDIO_PA_MODE4  4   // Gain = 27.5dB

int32_t Vn_Audio_WaitReady(uint32_t TimeoutMs);

void Vn_Audio_SetVolume(uint8_t Volume);
int32_t Vn_Audio_GetVolume(void);

int32_t Vn_Audio_PcmRecorderInit(uint16_t SamleRate, uint8_t Channels);
int32_t Vn_Audio_PcmRecorderRead(int8_t *pBuffer, uint32_t Len);
void Vn_Audio_PcmRecorderSuspend(void);
void Vn_Audio_PcmRecorderResume(void);
void Vn_Audio_PcmRecorderDeinit(void);

int32_t Vn_Audio_PcmPlayerInit(uint16_t SamleRate, uint8_t Channels);
int32_t Vn_Audio_PcmPlayerAdd(uint8_t *pBuffer, uint32_t Len);

int32_t Vn_Audio_VadInit(void);
int32_t Vn_Audio_CheckSpeek(int16_t *pBuffer, uint32_t Len);
int32_t Vn_Audio_VadDeInit(void);

int32_t Vn_Audio_InitPA(void);
int32_t Vn_Audio_SetPAMode(uint8_t Mode);

#endif