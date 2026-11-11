#include "vn_ssd1306.h"
#include <stddef.h>
#include <string.h>

Vn_SSD1306_Port_Typedef Vn_SSD1306_Port = { 0 };
static uint8_t Vn_SSD1306_GRAM[VN_SSD1306_PAGE_NUM][VN_SSD1306_WIDTH];

static void Vn_SSD1306_Init(void);

int32_t Vn_SSD1306_WriteCmd(uint8_t Cmd)
{
    if (Vn_SSD1306_Port.I2cWriteBytes == NULL)
    {
        return -1;
    }

    return Vn_SSD1306_Port.I2cWriteBytes(VN_SSD1306_I2C_ADDR, VN_SSD1306_CMD, &Cmd, 1);
}

int32_t Vn_SSD1306_WriteBytes(uint8_t *Data, uint32_t Size)
{
    if (Vn_SSD1306_Port.I2cWriteBytes == NULL)
    {
        return -1;
    }

    return Vn_SSD1306_Port.I2cWriteBytes(VN_SSD1306_I2C_ADDR, VN_SSD1306_DATA, Data, (uint16_t)Size);
}

int32_t Vn_SSD1306_InstanceInit(
    Vn_SSD1306_I2cWriteBytesFunc I2cWriteBytes,
    Vn_SSD1306_DelayMsFunc DelayMs
)
{
    if(NULL == I2cWriteBytes || NULL == DelayMs) 
    {
        return -1;
    }

    Vn_SSD1306_Port.I2cWriteBytes = I2cWriteBytes;
    Vn_SSD1306_Port.DelayMs = DelayMs;

    Vn_SSD1306_Init();

    return 0;
}

static void Vn_SSD1306_Init(void)
{
    Vn_SSD1306_WriteCmd(0xAE); /*display off*/

    Vn_SSD1306_WriteCmd(0x00); /*set lower column address*/
    Vn_SSD1306_WriteCmd(0x10); /*set higher column address*/

    Vn_SSD1306_WriteCmd(0x00); /*set display start line*/

    Vn_SSD1306_WriteCmd(0xB0); /*set page address*/

    Vn_SSD1306_WriteCmd(0x81); /*contract control*/
    Vn_SSD1306_WriteCmd(0x8f); /*128*/

    Vn_SSD1306_WriteCmd(0xA1); /*set segment remap*/

    Vn_SSD1306_WriteCmd(0xA6); /*normal / reverse*/

    Vn_SSD1306_WriteCmd(0xA8); /*multiplex ratio*/
    Vn_SSD1306_WriteCmd(0x1F); /*duty = 1/32*/

    Vn_SSD1306_WriteCmd(0xC8); /*Com scan direction*/

    Vn_SSD1306_WriteCmd(0xD3); /*set display offset*/
    Vn_SSD1306_WriteCmd(0x00);

    Vn_SSD1306_WriteCmd(0xD5); /*set osc division*/
    Vn_SSD1306_WriteCmd(0x80);

    Vn_SSD1306_WriteCmd(0xD9); /*set pre-charge period*/
    Vn_SSD1306_WriteCmd(0x1f);

    Vn_SSD1306_WriteCmd(0xDA); /*set COM pins*/
    Vn_SSD1306_WriteCmd(0x00);

    Vn_SSD1306_WriteCmd(0xdb); /*set vcomh*/
    Vn_SSD1306_WriteCmd(0x40);

    Vn_SSD1306_DisplayOff();
    Vn_SSD1306_Clear();
    Vn_SSD1306_DisplayOn();
}


void Vn_SSD1306_DisplayOn(void)
{
    Vn_SSD1306_WriteCmd(0X8D);  // SET DCDC
    Vn_SSD1306_WriteCmd(0X14);  // DCDC ON
    Vn_SSD1306_WriteCmd(0XAF);  // DISPLAY ON
}

void Vn_SSD1306_DisplayOff(void)
{
    Vn_SSD1306_WriteCmd(0X8D);  // SET DCDC
    Vn_SSD1306_WriteCmd(0X10);  // DCDC OFF
    Vn_SSD1306_WriteCmd(0XAE);  // DISPLAY OFF
}

int32_t Vn_SSD1306_Refresh(void)
{
    for (int32_t i = 0; i < VN_SSD1306_PAGE_NUM; i++)
    {
        Vn_SSD1306_WriteCmd(0xb0 + i);
        Vn_SSD1306_WriteCmd(0x00);
        Vn_SSD1306_WriteCmd(0x10);

        for (int32_t n = 0; n < VN_SSD1306_WIDTH; n++)
        {
            if (Vn_SSD1306_WriteBytes(&Vn_SSD1306_GRAM[i][n], 1) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

void Vn_SSD1306_Clear(void)
{
    for(int32_t i = 0; i < VN_SSD1306_PAGE_NUM; i++)
    {
        memset(Vn_SSD1306_GRAM[i], 0x00, VN_SSD1306_WIDTH);
    }

    Vn_SSD1306_Refresh();
}

int32_t Vn_SSD1306_DrawBMP(uint8_t StartX, uint8_t StartY, uint8_t Width, uint8_t Heigth, uint8_t *Image)
{

    if((Width + StartX > VN_SSD1306_WIDTH) || (Heigth + StartY > VN_SSD1306_HEIGHT) || (NULL == Image))
    {
        return -1;
    }

    for(int32_t i = 0; i < Heigth / 8; i++)
    {
        uint8_t Page = StartY / 8 + i;
        for(int32_t j = 0; j < Width; j++)
        {
            Vn_SSD1306_GRAM[Page][StartX + j] = Image[i * Width + j];
        }
    }

    return 0;
}

int32_t Vn_SSD1306_DrawPoint(int32_t X, int32_t Y, uint8_t Point)
{ 

    if((X > VN_SSD1306_WIDTH) || (Y > VN_SSD1306_HEIGHT))
    {
        return -1;
    }
    uint8_t Page = Y / 8;

    Vn_SSD1306_GRAM[Page][X] = Point;
    return 0;
}