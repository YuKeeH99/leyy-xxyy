#include "vn_lcd.h"

#include "ql_spi.h"
#include "ql_gpio.h"
#include "ql_rtos.h"
#include  "ql_pwm.h"

#include "vn_plat.h"
#include "vn_gc9107.h"
#include "vn_gpio.h"

static uint16_t  Vn_Lcd_BackgroudColor = 0x0000;

static void Vn_LCD_DelayMs(uint32_t Time)
{
    (void)ql_rtos_task_sleep_ms(Time);
}

static void Vn_LCD_SpiWriteBytes(uint8_t *pData, uint16_t size)
{
    ql_spi_write(VN_SPI_PORT_LCD, pData, size);
}

static void Vn_LCD_ControlDC(uint8_t Level)
{
    if(Level == 1)
    {
        ql_gpio_set_level(VN_PIN_LCD_DC, PIN_LEVEL_HIGH);
    }
    else
    {
        ql_gpio_set_level(VN_PIN_LCD_DC, PIN_LEVEL_LOW);
    }
}

static void Vn_LCD_ControlRST(uint8_t Level)
{
    if(Level == 1)
    {
        ql_gpio_set_level(VN_PIN_LCD_RST, PIN_LEVEL_HIGH);
    }
    else
    {
        ql_gpio_set_level(VN_PIN_LCD_RST, PIN_LEVEL_LOW);
    }
}

void Vn_LCD_TeSetPwmDuty(uint16_t Duty)
{
    QL_PWM_CONFIG_EX pwm_parameter;
    
    pwm_parameter.pwm_psc = 1;
    pwm_parameter.total_one_cycle_duration = 389;
    pwm_parameter.clk_src = QL_PWM_CLK_SRC_13M;
    
    if(Duty > 100) Duty = 100;

    Duty = 100 - Duty;

    pwm_parameter.high_one_cycle_duration = (Duty * 389) / 100;
    
    if(pwm_parameter.high_one_cycle_duration == 0 && Duty > 0) {
        pwm_parameter.high_one_cycle_duration = 1;
    }
    
    ql_pwm_config_plus_ex(VN_PIN_LCD_TE, &pwm_parameter);
    ql_pwm_enable(VN_PIN_LCD_TE);
}

static void Vn_LCD_TeInit(void)
{
    Vn_LCD_TeSetPwmDuty(0);
}

int32_t Vn_LCD_Init(void)
{
    int32_t ret = 0;

    ret = ql_spi_init(VN_SPI_PORT_LCD, QL_SPI_MODE0, QL_SPI_CLK_26MHZ);

    // init rst pin
    ret = ql_gpio_init(VN_PIN_LCD_RST,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);
    // init dc pin
    ret = ql_gpio_init(VN_PIN_LCD_DC,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    ret = ql_gpio_init(VN_PIN_LCD_TE,
                       PIN_DIRECTION_OUT,
                       PIN_PULL_DISABLE,
                       PIN_LEVEL_HIGH);

    VN_LOG_D("ql_spi_init: %d", ret);
    
    Vn_LCD_TeInit();

    Vn_GC9107_InstanceInit(Vn_LCD_SpiWriteBytes, Vn_LCD_DelayMs, Vn_LCD_ControlDC);

    Vn_GC9107_SetBackgroudColor(Vn_Lcd_BackgroudColor);

    Vn_GC9107_Full(Vn_Lcd_BackgroudColor);

    // wait for lcd ready
    ql_rtos_task_sleep_ms(100);

    Vn_LCD_TeSetPwmDuty(50);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

void Vn_LCD_Full(uint16_t Color)
{
    Vn_GC9107_Full(Color);
}

int32_t Vn_LCD_FullArea(uint16_t StartX, uint16_t StartY, uint16_t Width, uint16_t Height, uint16_t Color)
{
    int32_t ret = 0;
    uint16_t color_buff[512] = {0};
    uint32_t pixel_count = Width * Height;
    uint32_t  one_draw_pixel_count = 0;
    uint32_t had_draw_pixel_count = 0;

    ret = Vn_GC9107_SetDisplayArea(StartX, StartY, Width, Height);

    if(ret != 0)
    {
        return VN_RET_ERROR;
    }

    for (uint32_t i = 0; i < sizeof(color_buff) / sizeof(color_buff[0]); i++)
    {
        color_buff[i] = Color;
    }

    while(had_draw_pixel_count < pixel_count)
    {
        one_draw_pixel_count = pixel_count - had_draw_pixel_count;

        if(one_draw_pixel_count > sizeof(color_buff) / sizeof(uint16_t))
        {
            one_draw_pixel_count = sizeof(color_buff) / sizeof(uint16_t);
        }
        had_draw_pixel_count += one_draw_pixel_count;

        Vn_LCD_DrawMultiPixels(color_buff, one_draw_pixel_count);
    }

    return VN_RET_OK;
}

int32_t Vn_LCD_SetDisplayArea(uint16_t StartX, uint16_t StartY, uint16_t Width, uint16_t Height)
{
    int32_t ret = 0;

    ret = Vn_GC9107_SetDisplayArea(StartX, StartY, Width, Height);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

void Vn_LCD_DrawPixel(uint16_t Color)
{
    Vn_GC9107_DrawPixel(Color);
}

void Vn_LCD_DrawMultiPixels(uint16_t *pColor, uint16_t Count)
{
    Vn_GC9107_DrawMultiPixels(pColor, Count);
}

int32_t Vn_LCD_DrawString(uint16_t X, uint16_t Y, uint32_t FontSize, char *pData, uint16_t Color)
{
    int32_t ret = 0;

    ret = Vn_GC9107_ShowShring(X, Y, FontSize, pData, Color);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}
