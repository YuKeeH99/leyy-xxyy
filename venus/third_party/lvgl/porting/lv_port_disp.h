#ifndef THIRD_PARTY_LVGL_PORTING_LV_PORT_DISP_H_
#define THIRD_PARTY_LVGL_PORTING_LV_PORT_DISP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "lvgl.h"

extern void lv_port_disp_init(void);
extern lv_disp_t * Vn_LCD_Display;
extern lv_disp_t * Vn_OLED_Display;

#endif /* THIRD_PARTY_LVGL_PORTING_LV_PORT_DISP_H_ */
