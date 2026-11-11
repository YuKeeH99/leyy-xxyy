#include "vn_gif.h"
extern const lv_img_dsc_t dongyidong;
extern const lv_img_dsc_t idle;
extern const lv_img_dsc_t duoheshui;
extern const lv_img_dsc_t happy;
extern const lv_img_dsc_t listening;
extern const lv_img_dsc_t newmsg;
extern const lv_img_dsc_t ok;
extern const lv_img_dsc_t sad;
extern const lv_img_dsc_t sleepy;
extern const lv_img_dsc_t speaking;
extern const lv_img_dsc_t speaking2;
extern const lv_img_dsc_t superhappy;
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

const Vn_Gif_TypeDef Vn_GifResource[] = {
    {0, &idle},
    {1, &dongyidong},
    {2, &duoheshui},
    {3, &happy},
    {4, &listening},
    {5, &newmsg},
    {6, &ok},
    {7, &sad},
    {8, &sleepy},
    {9, &speaking},
    {10, &speaking2},
    {11, &superhappy},
};
const Vn_GifEmoji_TypeDef Vn_OLEDEmojiResource[] = {
    {"neutral", &idle},
    {"happy", &happy},
    {"laughing", &happy},
    {"funny", &happy},
    {"sad", &sad},
    {"loving", &superhappy},
    {"relaxed", &sleepy},
    {"kissy", &superhappy},
    {"confident", &happy},
    {"sleepy", &sleepy},
    {"confused", &listening},
};
uint32_t Vn_Gif_GetNum(void) { return sizeof(Vn_GifResource) / sizeof(Vn_GifResource[0]); }
uint32_t Vn_OLEDEmoji_GetNum(void) { return sizeof(Vn_OLEDEmojiResource) / sizeof(Vn_OLEDEmojiResource[0]); }
