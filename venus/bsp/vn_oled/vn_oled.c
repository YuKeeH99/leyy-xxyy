#include "vn_oled.h"

#include "ql_iic.h"
#include "ql_rtos.h"
#include "vn_plat.h"
#include "vn_ssd1306.h"

static void Vn_OLED_DelayMs(uint32_t Time)
{
    (void)ql_rtos_task_sleep_ms(Time);
}

static int32_t Vn_OLCD_I2cWrite(uint8_t SlaveAddr, uint8_t Addr, uint8_t *pData, uint16_t Length)
{
    int32_t ret;

    SlaveAddr >>= 1; // The bottom layer also shifts the slave address.
    ret = ql_i2c_write(VN_I2C_PORT_OLED, SlaveAddr, Addr, pData, Length);
    if (ret != 0)
    {
        VN_LOG_D("ql_i2c_write fail, ret = %d", ret);
        return VN_RET_ERROR;
    }

    return VN_RET_OK;
}

/**
 * @brief OLED Init
 * @return 0:success, others:fail
 */
int32_t Vn_OLED_Init(void)
{
    int32_t ret = 0;

    ret = ql_i2c_init(VN_I2C_PORT_OLED, FAST_MODE);

    VN_LOG_D("ql_i2c_init ret:%d", ret);

    Vn_SSD1306_InstanceInit(Vn_OLCD_I2cWrite, Vn_OLED_DelayMs);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

void Vn_OLED_Flush(uint16_t StartX, uint16_t StartY, uint16_t EndX, uint16_t EndY, uint16_t *Colorp)
{
    uint8_t data[VN_SSD1306_WIDTH];
    uint16_t width;
    uint16_t page;
    uint16_t x;
    uint16_t y;

    if (Colorp == NULL || StartX > EndX || StartY > EndY)
    {
        return;
    }

    StartX = StartX < VN_SSD1306_WIDTH ? StartX : VN_SSD1306_WIDTH - 1;
    EndX = EndX < VN_SSD1306_WIDTH ? EndX : VN_SSD1306_WIDTH - 1;
    StartY = StartY < VN_SSD1306_HEIGHT ? StartY : VN_SSD1306_HEIGHT - 1;
    EndY = EndY < VN_SSD1306_HEIGHT ? EndY : VN_SSD1306_HEIGHT - 1;
    width = EndX - StartX + 1;

    for (page = StartY / 8; page <= EndY / 8; page++)
    {
        if (Vn_SSD1306_WriteCmd(0xB0 + page) != 0 ||
            Vn_SSD1306_WriteCmd(StartX & 0x0F) != 0 ||
            Vn_SSD1306_WriteCmd(0x10 | ((StartX >> 4) & 0x0F)) != 0)
        {
            return;
        }

        memset(data, 0, width);
        for (y = page * 8; y < page * 8 + 8 && y <= EndY; y++)
        {
            if (y < StartY)
            {
                continue;
            }
            for (x = 0; x < width; x++)
            {
                if (Colorp[(y - StartY) * width + x] != 0xFFFF)
                {
                    data[x] |= (uint8_t)(1U << (y & 7));
                }
            }
        }

        if (Vn_SSD1306_WriteBytes(data, width) != 0)
        {
            return;
        }
    }
}

/**
 * @brief OLED Clear
 * @param None
 * @return None
 */
void Vn_OLED_Clear(void)
{
    Vn_SSD1306_Clear();
}

/**
 * @brief OLED Refresh cache to display
 * @param None
 * @return None
 */
void Vn_OLED_Refresh(void)
{
    Vn_SSD1306_Refresh();
}

/** 
 * @brief OLED Draw BMP
 * @param [in] StartX: Start X coordinate
 * @param [in] StartY: Start Y coordinate
 * @param [in] Width: BMP width
 * @param [in] Heigth: BMP heigth
 * @param [in] Image: BMP image
 * @return 0: Success, -1: Failed
 */
int32_t Vn_OLED_DrawBMP(uint8_t StartX, uint8_t StartY, uint8_t Width, uint8_t Heigth, uint8_t *Image)
{
    int32_t ret = 0;

    ret = Vn_SSD1306_DrawBMP(StartX, StartY, Width, Heigth, Image);

    return ret != 0 ? VN_RET_ERROR : VN_RET_OK;
}

/**
 * @brief Draw a point
 * @param [in] X: X coordinate
 * @param [in] Y: Y coordinate
 * @param [in] Point: Point
 * @return None
 */
void Vn_OLED_DrawPoint(uint8_t X, uint8_t Y, uint8_t Point)
{
    Vn_SSD1306_DrawPoint(X, Y, Point);
} 