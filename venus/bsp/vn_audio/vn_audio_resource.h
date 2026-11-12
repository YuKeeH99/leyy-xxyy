#ifndef BSP_VN_AUDIO_VN_AUDIO_RESOURCE_H_
#define BSP_VN_AUDIO_VN_AUDIO_RESOURCE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef struct {
    unsigned int Length;
    unsigned char * Data;
} Vn_AudioResource_Typedef;

typedef enum {
    VN_AUDIO_SOUND_INDEX_POPUP = 0,
    VN_AUDIO_SOUND_INDEX_MAX = 1
} Vn_AudioSoundIndex_Typepef;

extern int32_t Vn_GetAudioResource(Vn_AudioSoundIndex_Typepef Index, Vn_AudioResource_Typedef * Resource);

#endif /* BSP_VN_AUDIO_VN_AUDIO_RESOURCE_H_ */
