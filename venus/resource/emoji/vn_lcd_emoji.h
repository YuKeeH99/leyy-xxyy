#ifndef RESOURCE_EMOJI_VN_LCD_EMOJI_H_
#define RESOURCE_EMOJI_VN_LCD_EMOJI_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "lvgl.h"

typedef struct {
    const char * Name;
    const lv_img_dsc_t * Img;
} Vn_Emoji_TypeDef;

extern uint32_t Vn_Emoji_GetNum(void);
extern const Vn_Emoji_TypeDef Vn_EmojiResource[];
extern const lv_img_dsc_t emoji_1F636_neutral;
extern const lv_img_dsc_t emoji_1F642_happy;
extern const lv_img_dsc_t emoji_1f606_laughing;
extern const lv_img_dsc_t emoji_1F602_funny;
extern const lv_img_dsc_t emoji_1F614_sad;
extern const lv_img_dsc_t emoji_1F620_angry;
extern const lv_img_dsc_t emoji_1F62D_crying;
extern const lv_img_dsc_t emoji_1F60D_loving;
extern const lv_img_dsc_t emoji_1F633_embarrassed;
extern const lv_img_dsc_t emoji_1F62F_surprised;
extern const lv_img_dsc_t emoji_1F631_shocked;
extern const lv_img_dsc_t emoji_1F914_thinking;
extern const lv_img_dsc_t emoji_1F609_winking;
extern const lv_img_dsc_t emoji_1F60E_cool;
extern const lv_img_dsc_t emoji_1F60C_relaxed;
extern const lv_img_dsc_t emoji_1F924_delicious;
extern const lv_img_dsc_t emoji_1F618_kissy;
extern const lv_img_dsc_t emoji_1F60F_confident;
extern const lv_img_dsc_t emoji_1F634_sleepy;
extern const lv_img_dsc_t emoji_1F61C_silly;
extern const lv_img_dsc_t emoji_1F644_confused;

#endif /* RESOURCE_EMOJI_VN_LCD_EMOJI_H_ */
