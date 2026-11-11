#ifndef __VN_OPUS_PCM_H__
#define __VN_OPUS_PCM_H__

#include "vn_plat.h"
#include "opus.h"

typedef enum
{
    SAMPLE_RATE_8000 = 0,
    SAMPLE_RATE_12000,
    SAMPLE_RATE_16000,
    SAMPLE_RATE_24000,
    SAMPLE_RATE_48000
} __attribute__((__packed__)) Vn_Opus_SampleRate_Typedef;

typedef enum
{
    OPUS_CHANNELS_1 = 1,
    OPUS_CHANNELS_2 = 2
} __attribute__((__packed__)) Vn_Opus_Channels_Typedef;

OpusEncoder *Vn_Opus_Encoder_Init(Vn_Opus_SampleRate_Typedef Sample_rate,
                                  Vn_Opus_Channels_Typedef chnls, int *error);
OpusDecoder *Vn_Opus_Decoder_Init(Vn_Opus_SampleRate_Typedef Sample_rate,
                                  Vn_Opus_Channels_Typedef chnls, int *error);

void Vn_Opus_Encoder_Destroy(OpusEncoder *enc);
void Vn_Opus_Decoder_Destroy(OpusDecoder *dec);

int32_t Vn_Opus_Encode(OpusEncoder *Encoder, int16_t *PcmFrameIn,
                       int32_t PcmFrameLen, uint8_t *OpusOut);
int32_t Vn_Opus_Decode(OpusDecoder *Decoder, uint8_t *OpusIn, int32_t OpusLen,
                       int16_t *PcmOut, uint16_t PcmFrameSize);

uint32_t Vn_Opus_GetPcmSize(uint32_t SampleRate, uint8_t Channels, uint16_t FrameTime);
Vn_Opus_SampleRate_Typedef Vn_Opus_GetOpusSampleRateEnum(uint32_t SampleRate);

#endif
