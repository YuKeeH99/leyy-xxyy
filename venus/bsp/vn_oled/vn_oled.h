#ifndef BSP_VN_OLED_VN_OLED_H_
#define BSP_VN_OLED_VN_OLED_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

extern int32_t Vn_OLED_Init(void);
extern void Vn_OLED_Flush(uint16_t StartX, uint16_t StartY, uint16_t EndX, uint16_t EndY, uint16_t * Colorp);
extern void Vn_OLED_Clear(void);
extern void Vn_OLED_Refresh(void);
extern int32_t Vn_OLED_DrawBMP(uint8_t StartX, uint8_t StartY, uint8_t Width, uint8_t Heigth, uint8_t * Image);
extern void Vn_OLED_DrawPoint(uint8_t X, uint8_t Y, uint8_t Point);

#endif /* BSP_VN_OLED_VN_OLED_H_ */
