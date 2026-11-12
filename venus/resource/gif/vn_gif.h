#ifndef RESOURCE_GIF_VN_GIF_H_
#define RESOURCE_GIF_VN_GIF_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "lvgl.h"

typedef struct {
    const uint8_t Index;
    const lv_img_dsc_t * Img;
} Vn_Gif_TypeDef;

typedef struct {
    const char * Name;
    const lv_img_dsc_t * Img;
} Vn_GifEmoji_TypeDef;

typedef enum {
    VN_GIF_IDLE = 0,
    VN_GIF_DONGYIDONG = 1,
    VN_GIF_DUOHESHUI = 2,
    VN_GIF_HAPPY = 3,
    VN_GIF_LISTENING = 4,
    VN_GIF_NEWMSG = 5,
    VN_GIF_OK = 6,
    VN_GIF_SAD = 7,
    VN_GIF_SLEEPY = 8,
    VN_GIF_SPEAKING = 9,
    VN_GIF_SPEAKING2 = 10,
    VN_GIF_SUPERHAPPY = 11,
    VN_GIF_MAX = 12
} Vn_GifIndex;

extern uint32_t Vn_Gif_GetNum(void);
extern uint32_t Vn_OLEDEmoji_GetNum(void);
extern const Vn_Gif_TypeDef Vn_GifResource[];
extern const Vn_GifEmoji_TypeDef Vn_OLEDEmojiResource[];
extern const lv_img_dsc_t dongyidong;
extern const lv_img_dsc_t duoheshui;
extern const lv_img_dsc_t happy;
extern const lv_img_dsc_t idle;
extern const lv_img_dsc_t listening;
extern const lv_img_dsc_t newmsg;
extern const lv_img_dsc_t ok;
extern const lv_img_dsc_t sad;
extern const lv_img_dsc_t sleepy;
extern const lv_img_dsc_t speaking;
extern const lv_img_dsc_t speaking2;
extern const lv_img_dsc_t superhappy;

#endif /* RESOURCE_GIF_VN_GIF_H_ */
