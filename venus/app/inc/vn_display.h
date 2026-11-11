#ifndef __VN_DISPLAY_H__
#define __VN_DISPLAY_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VN_DISPLAY_MSG_DATA_SIZE 128U

typedef enum
{
    VN_DISPLAY_MSGTYPE_EMOJI = 0,
    VN_DISPLAY_MSGTYPE_CHAT_EMOJI,
    VN_DISPLAY_MSGTYPE_CHAT_TEXT,
    VN_DISPLAY_MSGTYPE_TITLE,
    VN_DISPLAY_MSGTYPE_OTA,
    VN_DISPLAY_MSGTYPE_INITIALIZING,
    VN_DISPLAY_MSGTYPE_SIGNAL,
    VN_DISPLAY_MSGTYPE_BATTERY,
    VN_DISPLAY_MSGTYPE_QRCODE,
    VN_DISPLAY_MSGTYPE_JPEG,
    VN_DISPLAY_MSGTYPE_SLEEP,
    VN_DISPLAY_MSGTYPE_OLED_CHAT_EMOJI,
    VN_DISPLAY_MSGTYPE_MsgBox
} __attribute__((__packed__)) Vn_DisplayMsgType_TypeDef;

typedef struct
{
    Vn_DisplayMsgType_TypeDef Type;
    uint8_t Data[VN_DISPLAY_MSG_DATA_SIZE];
} Vn_DisplayMsg_TypeDef;

void Vn_Display_Init(void);

void Vn_DisplayMsg_Send(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData, uint8_t Size);
void Vn_DisplayMsg_SendString(Vn_DisplayMsgType_TypeDef Type, uint8_t *pData);
void Vn_DisplayMsg_SendMsgBattery(uint8_t Status, uint8_t Percent);
void Vn_DisplayMsg_SendOledEmoji(uint8_t Index);
void Vn_DisplayMsg_SendLcdSignal(uint8_t Index);
void Vn_DisplayMsg_SendQrCode(uint8_t *pData, uint16_t Size);
void Vn_DisplayMsg_SendJPEG(uint8_t *pData, uint16_t Size, uint8_t duration);
void Vn_DisplayMsg_SendSleepMode(uint8_t Index);
void Vn_DisplayMsg_SendOledEmojiByTxt(uint8_t *Name, uint8_t Size);
void Vn_DisplayMsg_SendMsgBox(uint8_t *text, uint8_t time);
void Vn_LCD_SetOTA(uint8_t Networktype, uint8_t progress);

#ifdef __cplusplus
}
#endif

#endif /* __VN_DISPLAY_H__ */
