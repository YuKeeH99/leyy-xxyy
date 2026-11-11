#ifndef __VN_SSD1306_H__
#define __VN_SSD1306_H__

#include "stdint.h"

#define VN_SSD1306_WIDTH 128
#define VN_SSD1306_HEIGHT 32
#define VN_SSD1306_PAGE_NUM (VN_SSD1306_HEIGHT / 8)

#define VN_SSD1306_I2C_ADDR 0x78
#define VN_SSD1306_DATA     0x40
#define VN_SSD1306_CMD      0x00



typedef int32_t (*Vn_SSD1306_I2cWriteBytesFunc)(uint8_t SlaveAddr, uint8_t Addr, uint8_t *pData, uint16_t Length);
typedef void (*Vn_SSD1306_DelayMsFunc)(uint32_t ms);

typedef struct
{
    Vn_SSD1306_I2cWriteBytesFunc I2cWriteBytes;
    Vn_SSD1306_DelayMsFunc DelayMs;
}Vn_SSD1306_Port_Typedef;

int32_t Vn_SSD1306_InstanceInit(
    Vn_SSD1306_I2cWriteBytesFunc I2cWriteBytes,
    Vn_SSD1306_DelayMsFunc DelayMs
);

int32_t Vn_SSD1306_WriteCmd(uint8_t Cmd);
int32_t Vn_SSD1306_WriteBytes(uint8_t *Data, uint32_t Size);
void Vn_SSD1306_DisplayOn(void);
void Vn_SSD1306_DisplayOff(void);
int32_t Vn_SSD1306_Refresh(void);
void Vn_SSD1306_Clear(void);
int32_t Vn_SSD1306_DrawBMP(uint8_t StartX, uint8_t StartY, uint8_t Width, uint8_t Heigth, uint8_t *Image);
int32_t Vn_SSD1306_DrawPoint(int32_t X, int32_t Y, uint8_t Point);
#endif