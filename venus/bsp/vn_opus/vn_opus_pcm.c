#include "vn_opus_pcm.h"

#define VN_OPUS_BITRATE          64000
#define VN_OPUS_MAX_PACKET_SIZE  4068

static const uint16_t opus_sample_table[] = {
    8000, 12000, 16000, 24000, 48000
};

OpusEncoder *Vn_Opus_Encoder_Init(Vn_Opus_SampleRate_Typedef Sample_rate,
                                  Vn_Opus_Channels_Typedef chnls, int *error)
{
    OpusEncoder *st;

    if ((Sample_rate > SAMPLE_RATE_48000) || (chnls > OPUS_CHANNELS_2))
    {
        return NULL;
    }

    st = opus_encoder_create(opus_sample_table[Sample_rate], chnls,
                             OPUS_APPLICATION_VOIP, error);
    if (*error != OPUS_OK)
    {
        return NULL;
    }

    opus_encoder_ctl(st, OPUS_SET_VBR(0));
    opus_encoder_ctl(st, OPUS_SET_BITRATE(VN_OPUS_BITRATE));
    VN_LOG_I("opus bitrate:%d", VN_OPUS_BITRATE);
    return st;
}

OpusDecoder *Vn_Opus_Decoder_Init(Vn_Opus_SampleRate_Typedef Sample_rate,
                                  Vn_Opus_Channels_Typedef chnls, int *error)
{
    OpusDecoder *st;

    if ((Sample_rate > SAMPLE_RATE_48000) || (chnls > OPUS_CHANNELS_2))
    {
        return NULL;
    }

    st = opus_decoder_create(opus_sample_table[Sample_rate], chnls, error);
    if (*error != OPUS_OK)
    {
        return NULL;
    }
    return st;
}

void Vn_Opus_Encoder_Destroy(OpusEncoder *enc)
{
    if (enc != NULL)
    {
        opus_encoder_destroy(enc);
    }
}

void Vn_Opus_Decoder_Destroy(OpusDecoder *dec)
{
    if (dec != NULL)
    {
        opus_decoder_destroy(dec);
    }
}

int32_t Vn_Opus_Encode(OpusEncoder *Encoder, int16_t *PcmFrameIn,
                       int32_t PcmFrameLen, uint8_t *OpusOut)
{
    int32_t ret;

    if ((Encoder == NULL) || (PcmFrameIn == NULL) || (PcmFrameLen <= 0))
    {
        return VN_RET_ERROR;
    }

    ret = opus_encode(Encoder, PcmFrameIn, PcmFrameLen, OpusOut, VN_OPUS_MAX_PACKET_SIZE);
    if (ret < 0)
    {
        VN_LOG_E("PCM data encode to Opus fail!!!");
    }
    return ret;
}

int32_t Vn_Opus_Decode(OpusDecoder *Decoder, uint8_t *OpusIn, int32_t OpusLen,
                       int16_t *PcmOut, uint16_t PcmFrameSize)
{
    int32_t ret;

    if ((Decoder == NULL) || (OpusIn == NULL) || (OpusLen <= 0))
    {
        return VN_RET_ERROR;
    }

    ret = opus_decode(Decoder, OpusIn, OpusLen, PcmOut, PcmFrameSize, 0);
    if (ret < 0)
    {
        VN_LOG_E("Opus decoder to PCM fail:%d", ret);
        return ret;
    }

    /* A01 调用方按固定帧容量计算 PCM 字节数，成功时保持该返回约定。 */
    return PcmFrameSize;
}

uint32_t Vn_Opus_GetPcmSize(uint32_t SampleRate, uint8_t Channels, uint16_t FrameTime)
{
    return ((uint32_t)FrameTime * SampleRate * Channels) / 1000U;
}

Vn_Opus_SampleRate_Typedef Vn_Opus_GetOpusSampleRateEnum(uint32_t SampleRate)
{
    switch (SampleRate)
    {
        case 8000:
            return SAMPLE_RATE_8000;
        case 12000:
            return SAMPLE_RATE_12000;
        case 24000:
            return SAMPLE_RATE_24000;
        case 48000:
            return SAMPLE_RATE_48000;
        case 16000:
        default:
            return SAMPLE_RATE_16000;
    }
}
