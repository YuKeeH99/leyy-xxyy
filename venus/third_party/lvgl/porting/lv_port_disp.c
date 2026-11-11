#include "lv_port_disp.h"

#include "vn_gc9107.h"
#include "vn_lcd.h"
#include "vn_oled.h"
#include "vn_ssd1306.h"

lv_disp_t *Vn_LCD_Display;
lv_disp_t *Vn_OLED_Display;

static lv_disp_drv_t Vn_LCD_Drv;
static lv_disp_drv_t Vn_OLED_Drv;
static lv_disp_draw_buf_t Vn_LCD_DrawBuf;
static lv_disp_draw_buf_t Vn_OLED_DrawBuf;
static lv_color_t Vn_LCD_Buf[VN_LCD_WIDTH * VN_LCD_HEIGHT];
static lv_color_t Vn_OLED_Buf[VN_SSD1306_WIDTH * VN_SSD1306_HEIGHT];

static void Vn_OLED_DisplayFlush(lv_disp_drv_t *DispDrv, const lv_area_t *Area, lv_color_t *Color_p)
{
    Vn_OLED_Flush((uint16_t)Area->x1,
                  (uint16_t)Area->y1,
                  (uint16_t)Area->x2,
                  (uint16_t)Area->y2,
                  (uint16_t *)Color_p);
    lv_disp_flush_ready(DispDrv);
}

static void Vn_LCD_DisplayFlush(lv_disp_drv_t *DispDrv, const lv_area_t *Area, lv_color_t *Color_p)
{
    (void)Vn_GC9107_FastFlushArea((uint16_t)Area->x1,
                                  (uint16_t)Area->y1,
                                  (uint16_t)Area->x2,
                                  (uint16_t)Area->y2,
                                  (uint16_t *)Color_p);
    lv_disp_flush_ready(DispDrv);
}

void lv_port_disp_init(void)
{
    lv_disp_drv_init(&Vn_LCD_Drv);
    lv_disp_draw_buf_init(&Vn_LCD_DrawBuf, Vn_LCD_Buf, NULL,
                          VN_LCD_WIDTH * VN_LCD_HEIGHT);

    Vn_LCD_Drv.hor_res = VN_LCD_WIDTH;
    Vn_LCD_Drv.ver_res = VN_LCD_HEIGHT;
    Vn_LCD_Drv.flush_cb = Vn_LCD_DisplayFlush;
    Vn_LCD_Drv.draw_buf = &Vn_LCD_DrawBuf;
    Vn_LCD_Display = lv_disp_drv_register(&Vn_LCD_Drv);

    if(Vn_LCD_Display != NULL)
    {
        lv_obj_t *label = lv_label_create(lv_disp_get_scr_act(Vn_LCD_Display));
        lv_label_set_text(label, "Hello LVGL");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    lv_disp_drv_init(&Vn_OLED_Drv);
    lv_disp_draw_buf_init(&Vn_OLED_DrawBuf, Vn_OLED_Buf, NULL,
                          VN_SSD1306_WIDTH * VN_SSD1306_HEIGHT);

    Vn_OLED_Drv.hor_res = VN_SSD1306_WIDTH;
    Vn_OLED_Drv.ver_res = VN_SSD1306_HEIGHT;
    Vn_OLED_Drv.flush_cb = Vn_OLED_DisplayFlush;
    Vn_OLED_Drv.draw_buf = &Vn_OLED_DrawBuf;
    Vn_OLED_Drv.sw_rotate = 1;
    Vn_OLED_Drv.rotated = LV_DISP_ROT_180;
    Vn_OLED_Display = lv_disp_drv_register(&Vn_OLED_Drv);
}
