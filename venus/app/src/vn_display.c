#include "vn_display.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "lvgl.h"
#include "ql_rtos.h"
#include "ql_time.h"
#include "vn_lcd.h"
#include "vn_oled.h"
#include "vn_plat.h"
#include "vn_utils.h"

#include "../../resource/emoji/vn_lcd_emoji.h"
#include "../../resource/gif/vn_gif.h"
#include "../../resource/icon/vn_lcd_icon.h"
#include "../../resource/vn_jpg_decode.h"
#include "../../third_party/lvgl/porting/lv_port_disp.h"

#define VN_DISPLAY_QUEUE_LENGTH       10U
#define VN_DISPLAY_QUEUE_TIMEOUT_MS   10U
#define VN_DISPLAY_TASK_STACK_SIZE    (10U * 1024U)
#define VN_DISPLAY_TASK_PRIORITY      111U
#define VN_DISPLAY_TASK_PERIOD_MS     10U
#define VN_DISPLAY_AUX_PERIOD_MS      20U
#define VN_DISPLAY_SCROLL_DELAY_MS    1600U
#define VN_DISPLAY_QR_TIMEOUT_MS      20000U
#define VN_LCD_CONTENT_WIDTH          128
#define VN_LCD_CONTENT_HEIGHT         104
#define VN_LCD_TEXT_HEIGHT            30
#define VN_LCD_IMAGE_ZOOM             110

extern const lv_font_t vn_font14;
extern const lv_font_t vn_iconfont14;
extern const lv_img_dsc_t idle;
extern const lv_img_dsc_t emoji_1F636_neutral;

uint8_t *Vn_LCD_JPG_BUF;
uint32_t Vn_LCD_JPG_LENGTH;

static ql_queue_t Vn_Queue_Display;
static ql_task_t Vn_DisplayTaskRef;
static ql_task_t Vn_DisplayTask2Ref;
static ql_timer_t Vn_LV_Timer;
static ql_timer_t Vn_LV_QRTimer;

static lv_obj_t *Vn_LV_MainScreen;
static lv_obj_t *Vn_LV_SleepScreen;
static lv_obj_t *Vn_LV_OLED_Screen;
static lv_obj_t *Vn_LV_OLED_EmojiGif;
static lv_obj_t *Vn_LV_Time_Label;
static lv_obj_t *Vn_LV_Date_Label;

static lv_obj_t *Vn_LV_Content_Container;
static lv_obj_t *Vn_LV_LCD_Status_Bar;
static lv_obj_t *Vn_LV_LCD_Signal_Icon;
static lv_obj_t *Vn_LV_LCD_Status_Label;
static lv_obj_t *Vn_LV_LCD_Battery_Icon;

static lv_obj_t *Vn_LV_LCD_Screen;
static lv_obj_t *Vn_LV_LCD_Emoji_Img;
static lv_obj_t *Vn_LV_LCD_Qr_Code;
static lv_obj_t *Vn_LV_LCD_Text_Container;
static lv_obj_t *Vn_LV_LCD_Text_Display;

static lv_obj_t *Vn_LV_LCD_InitScreen;
static lv_obj_t *Vn_LV_LCD_Init_Icon;
static lv_obj_t *Vn_LV_LCD_Init_Text;

static lv_obj_t *Vn_LV_LCD_OTAScreen;
static lv_obj_t *Vn_LV_LCD_OTA_Bar;
static lv_obj_t *Vn_LV_LCD_OTA_Bar_Label;
static lv_obj_t *Vn_LV_LCD_OTA_Text;

static lv_obj_t *Vn_LV_LCD_MBox;
static lv_obj_t *Vn_MsgBox_Title;
static lv_obj_t *Vn_MsgBox_Text;
static bool Vn_MsgBox_Show;
static uint32_t Vn_MsgBox_CurTime;
static uint32_t Vn_MsgBox_Timeout_Ms;

static bool Vn_Scroll_Start;
static lv_coord_t Vn_Scroll_Step = VN_LCD_TEXT_HEIGHT / 2;
static lv_coord_t Vn_Max_Scroll_Y;
static lv_coord_t Vn_Total_Text_Height;
static uint32_t Vn_Scroll_Timeout;

static lv_color_t Vn_ColorRGB565(uint16_t Value)
{
    lv_color_t Color;
    Color.full = Value;
    return Color;
}

static void Vn_Obj_SetHidden(lv_obj_t *obj, bool Hidden)
{
    if(Hidden)
    {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void Vn_LV_TimerCB(u32 Argv)
{
    (void)Argv;
    lv_tick_inc(5);
}

void Vn_LCD_SetOTABar(uint8_t progress)
{
    uint8_t value = progress > 100U ? 100U : progress;
    char buf[10];

    lv_bar_set_value(Vn_LV_LCD_OTA_Bar, value, LV_ANIM_OFF);
    snprintf(buf, sizeof(buf), "%u%%", (unsigned int)value);
    lv_label_set_text(Vn_LV_LCD_OTA_Bar_Label, buf);
}

static void Vn_DisplayTask2(void *arg)
{
    uint32_t clock_counter = 0;

    (void)arg;
    for(;;)
    {
        ++clock_counter;
        if(clock_counter >= 50U)
        {
            time_t now = time(NULL);
            struct tm *tm_ptr = ql_localtime(&now);
            char time_str[12];
            char date_str[20];

            if(tm_ptr != NULL)
            {
                snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                         tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec);
                snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d",
                         tm_ptr->tm_year + 1900, tm_ptr->tm_mon, tm_ptr->tm_mday);
                lv_label_set_text(Vn_LV_Time_Label, time_str);
                lv_label_set_text(Vn_LV_Date_Label, date_str);
            }
            clock_counter = 0;
        }

        if(Vn_Scroll_Start &&
           Vn_DiffTime(Vn_Scroll_Timeout, VN_RTOS_SYSTEM_MS) > VN_DISPLAY_SCROLL_DELAY_MS)
        {
            lv_coord_t current_y = lv_obj_get_scroll_y(Vn_LV_LCD_Text_Display);
            Vn_Scroll_Timeout = VN_RTOS_SYSTEM_MS;
            if(current_y < Vn_Max_Scroll_Y)
            {
                lv_coord_t next_y = current_y + Vn_Scroll_Step;
                if(next_y > Vn_Max_Scroll_Y)
                {
                    next_y = Vn_Max_Scroll_Y;
                }
                lv_obj_scroll_to_y(Vn_LV_LCD_Text_Display, next_y, LV_ANIM_ON);
            }
            else
            {
                Vn_Scroll_Start = false;
            }
        }

        if(Vn_MsgBox_Show &&
           Vn_DiffTime(Vn_MsgBox_CurTime, VN_RTOS_SYSTEM_MS) > Vn_MsgBox_Timeout_Ms)
        {
            Vn_MsgBox_Show = false;
            Vn_Obj_SetHidden(Vn_LV_LCD_MBox, true);
        }

        ql_rtos_task_sleep_ms(VN_DISPLAY_AUX_PERIOD_MS);
    }
}

static void Vn_LV_QRTimerCB(u32 Argv)
{
    (void)Argv;
    Vn_Obj_SetHidden(Vn_LV_LCD_Qr_Code, true);
    Vn_Obj_SetHidden(Vn_LV_LCD_Emoji_Img, false);
    if(Vn_LV_QRTimer != NULL)
    {
        (void)ql_rtos_timer_stop(Vn_LV_QRTimer);
    }
}

void Vn_LCD_SetOTAType(uint8_t OTAType)
{
    if(OTAType == 1U)
    {
        lv_label_set_text(Vn_LV_LCD_OTA_Text, "正在下载更新固件,请勿断电!");
    }
    else if(OTAType == 2U)
    {
        lv_label_set_text(Vn_LV_LCD_OTA_Text, "正在升级WIFI固件,请勿断电!");
    }
}

void Vn_LCD_MsgBox_Init(void)
{
    Vn_LV_LCD_MBox = lv_msgbox_create(Vn_LV_Content_Container, "test", "test", NULL, false);
    lv_obj_set_size(Vn_LV_LCD_MBox, 120, 32);
    lv_obj_align(Vn_LV_LCD_MBox, LV_ALIGN_BOTTOM_MID, 0, 0);
    Vn_Obj_SetHidden(Vn_LV_LCD_MBox, true);
    lv_obj_set_style_bg_color(Vn_LV_LCD_MBox, Vn_ColorRGB565(0x1084), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Vn_LV_LCD_MBox, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(Vn_LV_LCD_MBox, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_LCD_MBox, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(Vn_LV_LCD_MBox, 0, LV_PART_MAIN);

    Vn_MsgBox_Title = lv_msgbox_get_title(Vn_LV_LCD_MBox);
    Vn_MsgBox_Text = lv_msgbox_get_text(Vn_LV_LCD_MBox);

    Vn_Obj_SetHidden(Vn_MsgBox_Title, true);
    lv_obj_set_style_text_font(Vn_MsgBox_Title, &vn_font14, LV_PART_MAIN);
    lv_obj_set_style_text_color(Vn_MsgBox_Title, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_align(Vn_MsgBox_Title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_height(Vn_MsgBox_Title, 0);
    lv_obj_set_style_pad_all(Vn_MsgBox_Title, 0, LV_PART_MAIN);

    lv_obj_set_style_text_font(Vn_MsgBox_Text, &vn_font14, LV_PART_MAIN);
    lv_obj_set_style_text_color(Vn_MsgBox_Text, lv_color_black(), LV_PART_MAIN);
    lv_label_set_long_mode(Vn_MsgBox_Text, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(Vn_MsgBox_Text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_height(Vn_MsgBox_Text, 32);
    lv_obj_set_style_pad_all(Vn_MsgBox_Text, 0, LV_PART_MAIN);
    lv_label_set_text(Vn_MsgBox_Title, "");
    lv_label_set_text(Vn_MsgBox_Text, "");
}

void Vn_DisplayMsg_Send(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData, uint8_t Size)
{
    Vn_DisplayMsg_TypeDef disp_msg;

    if(Vn_Queue_Display == NULL || (Size > 0U && pData == NULL))
    {
        return;
    }

    disp_msg.Type = Type;
    memset(disp_msg.Data, 0, sizeof(disp_msg.Data));
    if(Size > VN_DISPLAY_MSG_DATA_SIZE)
    {
        Size = VN_DISPLAY_MSG_DATA_SIZE;
    }
    if(Size > 0U)
    {
        memcpy(disp_msg.Data, pData, Size);
    }

    if(ql_rtos_queue_release(Vn_Queue_Display, sizeof(disp_msg),
                             (uint8_t *)&disp_msg, VN_DISPLAY_QUEUE_TIMEOUT_MS) != 0)
    {
        VN_LOG_E("display queue send error");
    }
}

void Vn_DisplayMsg_SendString(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData)
{
    size_t length;

    if(Vn_Queue_Display == NULL || pData == NULL)
    {
        return;
    }

    length = strlen((const char *)pData);
    if(length >= VN_DISPLAY_MSG_DATA_SIZE)
    {
        length = VN_DISPLAY_MSG_DATA_SIZE - 1U;
    }
    Vn_DisplayMsg_Send(Type, pData, (uint8_t)length);
}

void Vn_DisplayMsg_SendMsgBattery(uint8_t Status, uint8_t Percent)
{
    uint8_t data[2] = {Status, Percent};
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_BATTERY, data, sizeof(data));
}

void Vn_DisplayMsg_SendOledEmoji(uint8_t Index)
{
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_EMOJI, &Index, 1U);
}

void Vn_DisplayMsg_SendLcdSignal(uint8_t Index)
{
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_SIGNAL, &Index, 1U);
}

void Vn_DisplayMsg_SendQrCode(uint8_t *pData, uint16_t Size)
{
    uint8_t trigger = 0;
    Vn_LCD_JPG_LENGTH = Size;
    Vn_LCD_JPG_BUF = pData;
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_QRCODE, &trigger, 1U);
}

void Vn_DisplayMsg_SendJPEG(uint8_t *pData, uint16_t Size, uint8_t duration)
{
    Vn_LCD_JPG_LENGTH = Size;
    Vn_LCD_JPG_BUF = pData;
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_JPEG, &duration, 1U);
}

void Vn_DisplayMsg_SendSleepMode(uint8_t Index)
{
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_SLEEP, &Index, 1U);
}

void Vn_OLED_SetEmoji(uint8_t EmojiIndex)
{
    const lv_img_dsc_t *src = &idle;
    uint32_t count = Vn_Gif_GetNum();
    uint32_t i;

    if(count != 0U && EmojiIndex <= 11U)
    {
        for(i = 0; i < count; ++i)
        {
            if(Vn_GifResource[i].Index == EmojiIndex)
            {
                src = Vn_GifResource[i].Img;
                break;
            }
        }
    }

    lv_gif_set_src(Vn_LV_OLED_EmojiGif, src);
    lv_obj_align(Vn_LV_OLED_EmojiGif, LV_ALIGN_TOP_LEFT, 0, 0);
}

void Vn_Display_LCDUI(void)
{
    Vn_LV_LCD_Screen = lv_obj_create(Vn_LV_Content_Container);
    lv_obj_set_size(Vn_LV_LCD_Screen, VN_LCD_CONTENT_WIDTH, VN_LCD_CONTENT_HEIGHT);
    lv_obj_set_style_pad_all(Vn_LV_LCD_Screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_LCD_Screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(Vn_LV_LCD_Screen, 0, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Screen, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(Vn_LV_LCD_Screen, Vn_ColorRGB565(0x9EF7), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Vn_LV_LCD_Screen, LV_OPA_TRANSP,
                            LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(Vn_LV_LCD_Screen, LV_OPA_TRANSP,
                            LV_PART_SCROLLBAR | LV_STATE_SCROLLED);

    Vn_LV_LCD_Emoji_Img = lv_img_create(Vn_LV_LCD_Screen);
    lv_img_set_src(Vn_LV_LCD_Emoji_Img, &emoji_1F636_neutral);
    lv_obj_align(Vn_LV_LCD_Emoji_Img, LV_ALIGN_CENTER, 0, -15);
    Vn_Obj_SetHidden(Vn_LV_LCD_Emoji_Img, false);

    Vn_LV_LCD_Qr_Code = lv_img_create(Vn_LV_LCD_Screen);
    lv_img_set_zoom(Vn_LV_LCD_Qr_Code, VN_LCD_IMAGE_ZOOM);
    lv_obj_align(Vn_LV_LCD_Qr_Code, LV_ALIGN_CENTER, 0, -15);
    Vn_Obj_SetHidden(Vn_LV_LCD_Qr_Code, true);

    Vn_LV_LCD_Text_Container = lv_obj_create(Vn_LV_LCD_Screen);
    lv_obj_set_style_bg_color(Vn_LV_LCD_Text_Container, Vn_ColorRGB565(0x9EF7), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Vn_LV_LCD_Text_Container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_width(Vn_LV_LCD_Text_Container, 120);
    lv_obj_set_height(Vn_LV_LCD_Text_Container, VN_LCD_TEXT_HEIGHT);
    lv_obj_align(Vn_LV_LCD_Text_Container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_width(Vn_LV_LCD_Text_Container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(Vn_LV_LCD_Text_Container, 0, LV_PART_MAIN);

    Vn_LV_LCD_Text_Display = lv_label_create(Vn_LV_LCD_Text_Container);
    lv_label_set_text(Vn_LV_LCD_Text_Display, "");
    lv_obj_set_style_text_font(Vn_LV_LCD_Text_Display, &vn_font14, LV_PART_MAIN);
    lv_label_set_long_mode(Vn_LV_LCD_Text_Display, LV_LABEL_LONG_WRAP);
    lv_obj_set_scroll_dir(Vn_LV_LCD_Text_Display, LV_DIR_VER);
    lv_obj_set_style_clip_corner(Vn_LV_LCD_Text_Display, true, LV_PART_MAIN);
    lv_obj_set_width(Vn_LV_LCD_Text_Display, 120);
    lv_obj_set_height(Vn_LV_LCD_Text_Display, VN_LCD_TEXT_HEIGHT);
    lv_obj_set_style_text_align(Vn_LV_LCD_Text_Display, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Text_Display, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_border_width(Vn_LV_LCD_Text_Display, 0, LV_PART_MAIN);
}

void Vn_LCD_SetEmoji(char *Name)
{
    const lv_img_dsc_t *src = &emoji_1F636_neutral;
    uint32_t count = Vn_Emoji_GetNum();
    uint32_t i;

    if(Name != NULL)
    {
        for(i = 0; i < count; ++i)
        {
            if(strcmp(Vn_EmojiResource[i].Name, Name) == 0)
            {
                src = Vn_EmojiResource[i].Img;
                break;
            }
        }
    }
    lv_img_set_src(Vn_LV_LCD_Emoji_Img, src);
}

void Vn_Display_SleepScreen_Init(void)
{
    Vn_LV_SleepScreen = lv_obj_create(NULL);
    lv_obj_set_size(Vn_LV_SleepScreen, VN_LCD_WIDTH, VN_LCD_HEIGHT);
    lv_obj_set_style_pad_all(Vn_LV_SleepScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(Vn_LV_SleepScreen, lv_color_black(), LV_PART_MAIN);

    Vn_LV_Time_Label = lv_label_create(Vn_LV_SleepScreen);
    lv_label_set_text(Vn_LV_Time_Label, "09:05:05");
    lv_obj_set_style_text_font(Vn_LV_Time_Label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(Vn_LV_Time_Label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(Vn_LV_Time_Label, LV_ALIGN_CENTER, 0, -15);
    lv_obj_set_style_text_align(Vn_LV_Time_Label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    Vn_LV_Date_Label = lv_label_create(Vn_LV_SleepScreen);
    lv_label_set_text(Vn_LV_Date_Label, "2025-10-01");
    lv_obj_set_style_text_font(Vn_LV_Date_Label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(Vn_LV_Date_Label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(Vn_LV_Date_Label, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_text_align(Vn_LV_Date_Label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

void Vn_Display_LCD_InitUI(void)
{
    Vn_LV_LCD_InitScreen = lv_obj_create(Vn_LV_Content_Container);
    lv_obj_set_size(Vn_LV_LCD_InitScreen, VN_LCD_CONTENT_WIDTH, VN_LCD_CONTENT_HEIGHT);
    lv_obj_set_style_pad_all(Vn_LV_LCD_InitScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_LCD_InitScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(Vn_LV_LCD_InitScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(Vn_LV_LCD_InitScreen, Vn_ColorRGB565(0x9EF7), LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_InitScreen, LV_ALIGN_TOP_LEFT, 0, 0);

    Vn_LV_LCD_Init_Icon = lv_label_create(Vn_LV_LCD_InitScreen);
    lv_label_set_text(Vn_LV_LCD_Init_Icon, LV_SYMBOL_REFRESH);
    lv_obj_align(Vn_LV_LCD_Init_Icon, LV_ALIGN_CENTER, 0, -20);

    Vn_LV_LCD_Init_Text = lv_label_create(Vn_LV_LCD_InitScreen);
    lv_label_set_text(Vn_LV_LCD_Init_Text, "系统初始化");
    lv_obj_set_style_text_font(Vn_LV_LCD_Init_Text, &vn_font14, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Init_Text, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_align(Vn_LV_LCD_Init_Text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(Vn_LV_LCD_Init_Text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(Vn_LV_LCD_Init_Text, 120);
}

void Vn_Display_LCD_OTAUI(void)
{
    Vn_LV_LCD_OTAScreen = lv_obj_create(Vn_LV_Content_Container);
    lv_obj_set_style_bg_color(Vn_LV_LCD_OTAScreen, Vn_ColorRGB565(0x9EF7), LV_PART_MAIN);
    lv_obj_set_size(Vn_LV_LCD_OTAScreen, VN_LCD_CONTENT_WIDTH, VN_LCD_CONTENT_HEIGHT);
    lv_obj_set_style_pad_all(Vn_LV_LCD_OTAScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_LCD_OTAScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(Vn_LV_LCD_OTAScreen, 0, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_OTAScreen, LV_ALIGN_TOP_LEFT, 0, 0);

    Vn_LV_LCD_OTA_Bar = lv_bar_create(Vn_LV_LCD_OTAScreen);
    lv_obj_set_size(Vn_LV_LCD_OTA_Bar, 100, 10);
    lv_obj_set_style_bg_color(Vn_LV_LCD_OTA_Bar, Vn_ColorRGB565(0xDF03), LV_PART_INDICATOR);
    lv_obj_set_style_radius(Vn_LV_LCD_OTA_Bar, 3, LV_PART_MAIN);
    lv_bar_set_range(Vn_LV_LCD_OTA_Bar, 0, 100);
    lv_bar_set_value(Vn_LV_LCD_OTA_Bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_anim_time(Vn_LV_LCD_OTA_Bar, 500, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_OTA_Bar, LV_ALIGN_CENTER, 0, -20);

    Vn_LV_LCD_OTA_Bar_Label = lv_label_create(Vn_LV_LCD_OTAScreen);
    lv_label_set_text(Vn_LV_LCD_OTA_Bar_Label, "0%");
    lv_obj_set_style_text_font(Vn_LV_LCD_OTA_Bar_Label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_OTA_Bar_Label, LV_ALIGN_CENTER, 0, 0);

    Vn_LV_LCD_OTA_Text = lv_label_create(Vn_LV_LCD_OTAScreen);
    lv_label_set_text(Vn_LV_LCD_OTA_Text, "");
    lv_obj_set_style_text_font(Vn_LV_LCD_OTA_Text, &vn_font14, LV_PART_MAIN);
    lv_label_set_long_mode(Vn_LV_LCD_OTA_Text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(Vn_LV_LCD_OTA_Text, 120);
    lv_obj_set_style_text_align(Vn_LV_LCD_OTA_Text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_OTA_Text, LV_ALIGN_BOTTOM_MID, 0, -4);
}

static void Vn_LCD_SwitchContent(lv_obj_t *screen)
{
    if(screen == NULL)
    {
        return;
    }
    Vn_Obj_SetHidden(Vn_LV_LCD_Screen, true);
    Vn_Obj_SetHidden(Vn_LV_LCD_InitScreen, true);
    Vn_Obj_SetHidden(Vn_LV_LCD_OTAScreen, true);
    Vn_Obj_SetHidden(screen, false);
}

void Vn_Display_MainScreen_Init(void)
{
    Vn_LV_MainScreen = lv_obj_create(NULL);
    lv_obj_set_size(Vn_LV_MainScreen, VN_LCD_WIDTH, VN_LCD_HEIGHT);
    lv_obj_set_style_pad_all(Vn_LV_MainScreen, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(Vn_LV_MainScreen, Vn_ColorRGB565(0x9EF7), LV_PART_MAIN);

    Vn_LV_LCD_Status_Bar = lv_obj_create(Vn_LV_MainScreen);
    lv_obj_set_size(Vn_LV_LCD_Status_Bar, VN_LCD_WIDTH, 24);
    lv_obj_set_style_bg_opa(Vn_LV_LCD_Status_Bar, LV_OPA_TRANSP,
                            LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(Vn_LV_LCD_Status_Bar, LV_OPA_TRANSP,
                            LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    lv_obj_set_style_bg_color(Vn_LV_LCD_Status_Bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_LCD_Status_Bar, 0, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Status_Bar, LV_ALIGN_TOP_LEFT, 0, 0);

    Vn_LV_LCD_Signal_Icon = lv_label_create(Vn_LV_LCD_Status_Bar);
    lv_label_set_text(Vn_LV_LCD_Signal_Icon, Vn_IconResource[0].name);
    lv_obj_set_style_text_font(Vn_LV_LCD_Signal_Icon, &vn_iconfont14, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Signal_Icon, LV_ALIGN_LEFT_MID, -4, 0);

    Vn_LV_LCD_Status_Label = lv_label_create(Vn_LV_LCD_Status_Bar);
    lv_label_set_text(Vn_LV_LCD_Status_Label, "");
    lv_obj_set_style_text_font(Vn_LV_LCD_Status_Label, &vn_font14, LV_PART_MAIN);
    lv_obj_set_style_text_color(Vn_LV_LCD_Status_Label, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Status_Label, LV_ALIGN_CENTER, 0, 0);

    Vn_LV_LCD_Battery_Icon = lv_label_create(Vn_LV_LCD_Status_Bar);
    lv_label_set_text(Vn_LV_LCD_Battery_Icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_font(Vn_LV_LCD_Battery_Icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(Vn_LV_LCD_Battery_Icon, LV_ALIGN_RIGHT_MID, 4, 0);

    Vn_LV_Content_Container = lv_obj_create(Vn_LV_MainScreen);
    lv_obj_set_size(Vn_LV_Content_Container, VN_LCD_CONTENT_WIDTH, VN_LCD_CONTENT_HEIGHT);
    lv_obj_set_style_pad_all(Vn_LV_Content_Container, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_Content_Container, 0, LV_PART_MAIN);
    lv_obj_align(Vn_LV_Content_Container, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    Vn_Display_LCDUI();
    Vn_Display_LCD_InitUI();
    Vn_Display_LCD_OTAUI();
    Vn_LCD_MsgBox_Init();
    Vn_LCD_SwitchContent(Vn_LV_LCD_InitScreen);
}

static void Vn_LCD_LoadScreen(lv_obj_t *screen)
{
    lv_disp_t *previous;

    if(screen == NULL || Vn_LCD_Display == NULL)
    {
        return;
    }
    if(lv_disp_get_scr_act(Vn_LCD_Display) == screen)
    {
        return;
    }

    previous = lv_disp_get_default();
    lv_disp_set_default(Vn_LCD_Display);
    lv_disp_load_scr(screen);
    if(previous != NULL)
    {
        lv_disp_set_default(previous);
    }
}

void Vn_LCD_SetSignalIcon(uint8_t IconIndex)
{
    const char *icon = Vn_IconResource[0].name;
    uint32_t count = Vn_Icon_GetNum();
    uint32_t i;

    if(count != 0U && IconIndex <= 8U)
    {
        for(i = 0; i < count; ++i)
        {
            if(Vn_IconResource[i].Index == IconIndex)
            {
                icon = Vn_IconResource[i].name;
                break;
            }
        }
    }
    lv_label_set_text(Vn_LV_LCD_Signal_Icon, icon);
}

static bool Vn_LCD_DecodeImage(void)
{
    lv_img_dsc_t decoded;

    if(Vn_LCD_JPG_BUF == NULL || Vn_LCD_JPG_LENGTH == 0U)
    {
        VN_LOG_E("empty JPEG buffer");
        return false;
    }

    VN_LOG_I("JPEG length: %u", (unsigned int)Vn_LCD_JPG_LENGTH);
    memset(&decoded, 0, sizeof(decoded));
    JpegDecode_TrueColor(Vn_LCD_JPG_BUF, Vn_LCD_JPG_LENGTH, &decoded);
    if(decoded.data == NULL || decoded.data_size == 0U)
    {
        return false;
    }
    Vn_Jpg_dsc = decoded;

    if(Vn_LV_QRTimer != NULL)
    {
        (void)ql_rtos_timer_stop(Vn_LV_QRTimer);
    }
    Vn_Obj_SetHidden(Vn_LV_LCD_Emoji_Img, true);
    Vn_Obj_SetHidden(Vn_LV_LCD_Qr_Code, false);
    lv_img_set_src(Vn_LV_LCD_Qr_Code, &Vn_Jpg_dsc);
    lv_img_set_zoom(Vn_LV_LCD_Qr_Code, VN_LCD_IMAGE_ZOOM);
    lv_obj_align(Vn_LV_LCD_Qr_Code, LV_ALIGN_CENTER, 0, -15);
    return true;
}

void Vn_LCD_SetQRCode(void)
{
    if(Vn_LCD_DecodeImage() && Vn_LV_QRTimer != NULL)
    {
        (void)ql_rtos_timer_start(Vn_LV_QRTimer, VN_DISPLAY_QR_TIMEOUT_MS,
                                  false, Vn_LV_QRTimerCB, 0);
    }
}

void Vn_LCD_SetJPEG(uint8_t time)
{
    if(Vn_LCD_DecodeImage() && Vn_LV_QRTimer != NULL)
    {
        (void)ql_rtos_timer_start(Vn_LV_QRTimer, (uint32_t)time * 1000U,
                                  false, Vn_LV_QRTimerCB, 0);
    }
}

void Vn_DisplayMsg_SendOledEmojiByTxt(uint8_t *Name, uint8_t Size)
{
    if(Name == NULL)
    {
        return;
    }
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_OLED_CHAT_EMOJI, Name, Size);
}

void Vn_OLED_SetEmojiByTxt(uint8_t *Name)
{
    uint32_t count;
    uint32_t i;

    if(Name == NULL)
    {
        VN_LOG_E("OLED emoji name is NULL");
        return;
    }

    count = Vn_OLEDEmoji_GetNum();
    for(i = 0; i < count; ++i)
    {
        if(strcmp(Vn_OLEDEmojiResource[i].Name, (const char *)Name) == 0)
        {
            lv_gif_set_src(Vn_LV_OLED_EmojiGif, Vn_OLEDEmojiResource[i].Img);
            lv_obj_align(Vn_LV_OLED_EmojiGif, LV_ALIGN_TOP_LEFT, 0, 0);
            return;
        }
    }
    VN_LOG_E("emoji not found in OLED GIF resource: %s", (char *)Name);
}

void Vn_LCD_SetOTA(uint8_t Networktype, uint8_t progress)
{
    lv_anim_del_all();
    Vn_LCD_SetOTAType(Networktype);
    Vn_LCD_SetOTABar(progress);
    Vn_LCD_SwitchContent(Vn_LV_LCD_OTAScreen);
    lv_refr_now(Vn_LCD_Display);
}

void Vn_LCD_SetChatText_Scroll(char *new_text)
{
    lv_point_t size;
    const lv_font_t *font;
    lv_coord_t letter_space;
    lv_coord_t line_space;
    lv_coord_t max_width;

    if(new_text == NULL)
    {
        return;
    }

    Vn_Scroll_Start = false;
    lv_label_set_text(Vn_LV_LCD_Text_Display, new_text);
    font = lv_obj_get_style_text_font(Vn_LV_LCD_Text_Display, LV_PART_MAIN);
    letter_space = lv_obj_get_style_text_letter_space(Vn_LV_LCD_Text_Display, LV_PART_MAIN);
    line_space = lv_obj_get_style_text_line_space(Vn_LV_LCD_Text_Display, LV_PART_MAIN);
    max_width = lv_obj_get_width(Vn_LV_LCD_Text_Display);
    lv_txt_get_size(&size, lv_label_get_text(Vn_LV_LCD_Text_Display), font,
                    letter_space, line_space, max_width, LV_LABEL_LONG_WRAP);

    Vn_Total_Text_Height = size.y;
    if(size.y < VN_LCD_TEXT_HEIGHT)
    {
        size.y = VN_LCD_TEXT_HEIGHT;
    }
    Vn_Max_Scroll_Y = size.y - VN_LCD_TEXT_HEIGHT;
    lv_obj_scroll_to_y(Vn_LV_LCD_Text_Display, 0, LV_ANIM_OFF);

    if(Vn_Max_Scroll_Y > 0)
    {
        Vn_Scroll_Start = true;
        Vn_Scroll_Timeout = VN_RTOS_SYSTEM_MS;
    }
}

void Vn_LCD_SetMsgBox(uint8_t *text)
{
    if(text[0] == 0U)
    {
        VN_LOG_E("message box timeout is zero");
        return;
    }

    Vn_MsgBox_CurTime = VN_RTOS_SYSTEM_MS;
    Vn_MsgBox_Timeout_Ms = (uint32_t)text[0] * 1000U;
    Vn_MsgBox_Show = true;
    lv_label_set_text(Vn_MsgBox_Text, (const char *)(text + 1));
    Vn_Obj_SetHidden(Vn_LV_LCD_MBox, false);
}

static void Vn_LCD_SetBattery(uint8_t Status, uint8_t Percent)
{
    const char *icon = NULL;

    if(Status == 0U)
    {
        if(Percent <= 10U)
        {
            icon = LV_SYMBOL_BATTERY_EMPTY;
        }
        else if(Percent <= 40U)
        {
            icon = LV_SYMBOL_BATTERY_1;
        }
        else if(Percent <= 60U)
        {
            icon = LV_SYMBOL_BATTERY_2;
        }
        else if(Percent <= 90U)
        {
            icon = LV_SYMBOL_BATTERY_3;
        }
        else if(Percent <= 100U)
        {
            icon = LV_SYMBOL_BATTERY_FULL;
        }
    }
    else if(Status == 1U)
    {
        if(Percent <= 10U)
        {
            icon = LV_SYMBOL_CHARGE LV_SYMBOL_BATTERY_EMPTY;
        }
        else if(Percent <= 40U)
        {
            icon = LV_SYMBOL_CHARGE LV_SYMBOL_BATTERY_1;
        }
        else if(Percent <= 60U)
        {
            icon = LV_SYMBOL_CHARGE LV_SYMBOL_BATTERY_2;
        }
        else if(Percent <= 90U)
        {
            icon = LV_SYMBOL_CHARGE LV_SYMBOL_BATTERY_3;
        }
        else
        {
            icon = LV_SYMBOL_CHARGE LV_SYMBOL_BATTERY_FULL;
        }
    }

    if(icon != NULL)
    {
        lv_label_set_text(Vn_LV_LCD_Battery_Icon, icon);
    }
}

static void Vn_DisplayMsg_Handle(Vn_DisplayMsg_TypeDef *msg)
{
    msg->Data[VN_DISPLAY_MSG_DATA_SIZE - 1U] = '\0';

    switch(msg->Type)
    {
        case VN_DISPLAY_MSGTYPE_EMOJI:
            Vn_OLED_SetEmoji(msg->Data[0]);
            break;

        case VN_DISPLAY_MSGTYPE_CHAT_EMOJI:
            Vn_LCD_SetEmoji((char *)msg->Data);
            Vn_LCD_SwitchContent(Vn_LV_LCD_Screen);
            break;

        case VN_DISPLAY_MSGTYPE_CHAT_TEXT:
            Vn_LCD_SetChatText_Scroll((char *)msg->Data);
            Vn_LCD_SwitchContent(Vn_LV_LCD_Screen);
            break;

        case VN_DISPLAY_MSGTYPE_TITLE:
            lv_label_set_text(Vn_LV_LCD_Status_Label, (char *)msg->Data);
            Vn_LCD_SwitchContent(Vn_LV_LCD_Screen);
            break;

        case VN_DISPLAY_MSGTYPE_OTA:
            Vn_LCD_SetOTA(msg->Data[0], msg->Data[1]);
            break;

        case VN_DISPLAY_MSGTYPE_INITIALIZING:
            lv_label_set_text(Vn_LV_LCD_Init_Text, (char *)msg->Data);
            Vn_LCD_SwitchContent(Vn_LV_LCD_InitScreen);
            break;

        case VN_DISPLAY_MSGTYPE_SIGNAL:
            Vn_LCD_SetSignalIcon(msg->Data[0]);
            break;

        case VN_DISPLAY_MSGTYPE_BATTERY:
            Vn_LCD_SetBattery(msg->Data[0], msg->Data[1]);
            break;

        case VN_DISPLAY_MSGTYPE_QRCODE:
            Vn_LCD_SetQRCode();
            break;

        case VN_DISPLAY_MSGTYPE_JPEG:
            Vn_LCD_SetJPEG(msg->Data[0]);
            break;

        case VN_DISPLAY_MSGTYPE_SLEEP:
            Vn_LCD_LoadScreen(msg->Data[0] == 0U ? Vn_LV_SleepScreen : Vn_LV_MainScreen);
            break;

        case VN_DISPLAY_MSGTYPE_OLED_CHAT_EMOJI:
            Vn_OLED_SetEmojiByTxt(msg->Data);
            break;

        case VN_DISPLAY_MSGTYPE_MsgBox:
            Vn_LCD_SetMsgBox(msg->Data);
            break;

        default:
            break;
    }
}

static void Vn_DisplayTask(void *arg)
{
    Vn_DisplayMsg_TypeDef vn_disp_msg;

    (void)arg;
    for(;;)
    {
        if(ql_rtos_queue_wait(Vn_Queue_Display, (uint8_t *)&vn_disp_msg,
                              sizeof(vn_disp_msg), VN_DISPLAY_QUEUE_TIMEOUT_MS) == 0)
        {
            Vn_DisplayMsg_Handle(&vn_disp_msg);
        }
        lv_timer_handler();
        ql_rtos_task_sleep_ms(VN_DISPLAY_TASK_PERIOD_MS);
    }
}

void Vn_DisplayMsg_SendMsgBox(uint8_t *text, uint8_t time)
{
    uint8_t pData[VN_DISPLAY_MSG_DATA_SIZE];
    size_t length;

    if(time == 0U || text == NULL)
    {
        return;
    }

    memset(pData, 0, sizeof(pData));
    pData[0] = time;
    length = strlen((const char *)text);
    if(length > VN_DISPLAY_MSG_DATA_SIZE - 2U)
    {
        length = VN_DISPLAY_MSG_DATA_SIZE - 2U;
    }
    memcpy(pData + 1, text, length);
    pData[length + 1U] = '\0';
    Vn_DisplayMsg_Send(VN_DISPLAY_MSGTYPE_MsgBox, pData, (uint8_t)(length + 1U));
}

void Vn_Display_Init(void)
{
    lv_obj_t *parent;

    if(ql_rtos_queue_create(&Vn_Queue_Display, sizeof(Vn_DisplayMsg_TypeDef),
                            VN_DISPLAY_QUEUE_LENGTH) != 0)
    {
        VN_LOG_E("display queue create failed");
        return;
    }

    (void)Vn_LCD_Init();
    (void)Vn_OLED_Init();
    lv_init();
    lv_port_disp_init();
    if(Vn_LCD_Display == NULL || Vn_OLED_Display == NULL)
    {
        VN_LOG_E("LVGL display registration failed");
        return;
    }

    parent = lv_disp_get_scr_act(Vn_OLED_Display);
    Vn_LV_OLED_Screen = lv_obj_create(parent);
    lv_obj_set_size(Vn_LV_OLED_Screen, VN_LCD_WIDTH, 32);
    lv_obj_set_style_pad_all(Vn_LV_OLED_Screen, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Vn_LV_OLED_Screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(Vn_LV_OLED_Screen, 0, LV_PART_MAIN);
    lv_obj_align(Vn_LV_OLED_Screen, LV_ALIGN_TOP_LEFT, 0, 0);
    Vn_LV_OLED_EmojiGif = lv_gif_create(Vn_LV_OLED_Screen);
    lv_gif_set_src(Vn_LV_OLED_EmojiGif, &idle);
    lv_obj_align(Vn_LV_OLED_EmojiGif, LV_ALIGN_TOP_LEFT, 0, 0);

    Vn_Display_MainScreen_Init();
    Vn_Display_SleepScreen_Init();
    Vn_LCD_LoadScreen(Vn_LV_MainScreen);

    if(ql_rtos_timer_create(&Vn_LV_Timer) == 0)
    {
        (void)ql_rtos_timer_start(Vn_LV_Timer, 5, true, Vn_LV_TimerCB, 0);
    }
    (void)ql_rtos_timer_create(&Vn_LV_QRTimer);

    if(ql_rtos_task_create(&Vn_DisplayTaskRef, VN_DISPLAY_TASK_STACK_SIZE,
                           VN_DISPLAY_TASK_PRIORITY, "Vn_DisplayTask",
                           Vn_DisplayTask, NULL) != 0)
    {
        VN_LOG_E("Vn_DisplayTask create failed");
    }
    if(ql_rtos_task_create(&Vn_DisplayTask2Ref, VN_DISPLAY_TASK_STACK_SIZE,
                           VN_DISPLAY_TASK_PRIORITY, "Vn_DisplayTask2",
                           Vn_DisplayTask2, NULL) != 0)
    {
        VN_LOG_E("Vn_DisplayTask2 create failed");
    }
    VN_LOG_I("display initialized");
}
