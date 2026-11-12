#ifndef BSP_VN_LCD_VN_LCD_H_
#define BSP_VN_LCD_VN_LCD_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

#define VN_LCD_WIDTH   128U
#define VN_LCD_HEIGHT  128U

extern int32_t Vn_LCD_Init(void);
extern void Vn_LCD_TeSetPwmDuty(uint16_t Duty);
extern void Vn_LCD_Full(uint16_t Color);
extern int32_t Vn_LCD_FullArea(uint16_t StartX, uint16_t StartY, uint16_t Width, uint16_t Height, uint16_t Color);
extern int32_t Vn_LCD_SetDisplayArea(uint16_t StartX, uint16_t StartY, uint16_t Width, uint16_t Height);
extern void Vn_LCD_DrawPixel(uint16_t Color);
extern void Vn_LCD_DrawMultiPixels(uint16_t * pColor, uint16_t Count);
extern int32_t Vn_LCD_DrawString(uint16_t X, uint16_t Y, uint32_t FontSize, char * pData, uint16_t Color);

#endif /* BSP_VN_LCD_VN_LCD_H_ */
