#include "vn_lcd_emoji.h"
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

const Vn_Emoji_TypeDef Vn_EmojiResource[] = {
    {"neutral", &emoji_1F636_neutral},
    {"happy", &emoji_1F642_happy},
    {"laughing", &emoji_1f606_laughing},
    {"funny", &emoji_1F602_funny},
    {"sad", &emoji_1F614_sad},
    {"angry", &emoji_1F620_angry},
    {"crying", &emoji_1F62D_crying},
    {"loving", &emoji_1F60D_loving},
    {"embarrassed", &emoji_1F633_embarrassed},
    {"surprised", &emoji_1F62F_surprised},
    {"shocked", &emoji_1F631_shocked},
    {"thinking", &emoji_1F914_thinking},
    {"winking", &emoji_1F609_winking},
    {"cool", &emoji_1F60E_cool},
    {"relaxed", &emoji_1F60C_relaxed},
    {"delicious", &emoji_1F924_delicious},
    {"kissy", &emoji_1F618_kissy},
    {"confident", &emoji_1F60F_confident},
    {"sleepy", &emoji_1F634_sleepy},
    {"silly", &emoji_1F61C_silly},
    {"confused", &emoji_1F644_confused},
};
uint32_t Vn_Emoji_GetNum(void) { return sizeof(Vn_EmojiResource) / sizeof(Vn_EmojiResource[0]); }
