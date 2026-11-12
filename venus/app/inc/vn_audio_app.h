#ifndef APP_INC_VN_AUDIO_APP_H_
#define APP_INC_VN_AUDIO_APP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "vn_audio_resource.h"

typedef struct {
    uint8_t Volume;
    uint8_t Mode;
    uint8_t Enable;
} Vn_Audio_Params_TypeDef;

extern void Vn_AudioApp_SetVolume(uint8_t Volume);
extern void Vn_AudioApp_PlaySound(Vn_AudioSoundIndex_Typepef Index);
extern uint32_t Vn_Audio_GetAppState(void);
extern int32_t Vn_AudioApp_GetVolume(void);
extern void Vn_Audio_App(void * arg);
extern Vn_Audio_Params_TypeDef Vn_Audio_Params;

#endif /* APP_INC_VN_AUDIO_APP_H_ */
