#ifndef __VN_GC9107_H__
#define __VN_GC9107_H__ 

#include "stdint.h"

#define VN_GC9107_DC_LEVEL_HIGH      1
#define VN_GC9107_DC_LEVEL_LOW       0

typedef void (*Vn_GC9107_SpiWriteBytesFunc)(uint8_t *pData, uint16_t Length);
typedef void (*Vn_GC9107_DelayMsFunc)(uint32_t ms);
typedef void (*Vn_GC9107_ControlDCFunc)(uint8_t Level);

typedef struct
{
    Vn_GC9107_SpiWriteBytesFunc SpiWriteBytes;
    Vn_GC9107_DelayMsFunc DelayMs;
    Vn_GC9107_ControlDCFunc ControlDC;
}Vn_GC9107_Port_Typedef;

int32_t Vn_GC9107_InstanceInit(
    Vn_GC9107_SpiWriteBytesFunc SpiWriteBytes,
    Vn_GC9107_DelayMsFunc DelayMs,
    Vn_GC9107_ControlDCFunc ControlDC
);

void Vn_GC9107_SetBackgroudColor(uint16_t Color);
void Vn_GC9107_Full(uint32_t Color);

int32_t Vn_GC9107_SetDisplayArea(uint16_t StartX, uint16_t StartY, uint16_t Width, uint16_t Height);
void Vn_GC9107_DrawPixel(uint16_t Color);
void Vn_GC9107_DrawMultiPixels(uint16_t* pColors, uint16_t Count);
int32_t Vn_GC9107_FastFlushArea(uint16_t StartX, uint16_t StartY, uint16_t EndX, uint16_t EndY, uint16_t *color_p);

int32_t Vn_GC9107_ShowChar(uint16_t X, uint16_t Y, uint32_t FontSize, char Chr, uint16_t Color);
int32_t Vn_GC9107_ShowShring(uint16_t X, uint16_t Y, uint32_t FontSize, char *pData, uint16_t Color);
#endif