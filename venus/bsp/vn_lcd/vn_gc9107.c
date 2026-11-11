#include "vn_gc9107.h"
#include <stddef.h>

#include "vn_lcd_fonts.h"

#define VN_GC9107_COLUMN_NUMBER 128
#define VN_GC9107_ROW_NUMBER    128   /// 160
#define VN_GC9107_COLUMN_OFFSET 0
#define VN_GC9107_ROW_OFFSET    0

#define VN_GC9107_DISPLAY_MODE_0    0x00
#define VN_GC9107_DISPLAY_MODE_180  0xC0

#define VN_GC9107_DISPLAY_MODE      (VN_GC9107_DISPLAY_MODE_180)

static uint32_t Vn_GC9107_BackgroudColor = 0x0000; // Black

static void Vn_GC9107_Init(void);

Vn_GC9107_Port_Typedef Vn_GC9107_Port = { 0 };

int32_t Vn_GC9107_InstanceInit(
    Vn_GC9107_SpiWriteBytesFunc SpiWriteBytes,
    Vn_GC9107_DelayMsFunc DelayMs,
    Vn_GC9107_ControlDCFunc ControlDC
)
{
    if(NULL == SpiWriteBytes || NULL == DelayMs || NULL == ControlDC)
    {
        return -1;
    }

    Vn_GC9107_Port.SpiWriteBytes = SpiWriteBytes;
    Vn_GC9107_Port.DelayMs = DelayMs;
    Vn_GC9107_Port.ControlDC = ControlDC;

    Vn_GC9107_Init();

    return 0;
}

static void Vn_GC9107_SendCmd(uint8_t Cmd)
{
    if(Vn_GC9107_Port.ControlDC != NULL && Vn_GC9107_Port.SpiWriteBytes != NULL)
    {
        Vn_GC9107_Port.ControlDC(VN_GC9107_DC_LEVEL_LOW);
        Vn_GC9107_Port.SpiWriteBytes(&Cmd, 1);
    }
}

static void Vn_GC9107_SendData(uint8_t Data)
{
    if(Vn_GC9107_Port.ControlDC != NULL && Vn_GC9107_Port.SpiWriteBytes != NULL)
    {
        Vn_GC9107_Port.ControlDC(VN_GC9107_DC_LEVEL_HIGH);
        Vn_GC9107_Port.SpiWriteBytes(&Data, 1);
    }
}

static void Vn_GC9107_SendMultiData(uint8_t *pData, uint16_t Len)
{
    if(Vn_GC9107_Port.ControlDC != NULL && Vn_GC9107_Port.SpiWriteBytes != NULL)
    {
        Vn_GC9107_Port.ControlDC(VN_GC9107_DC_LEVEL_HIGH);
        Vn_GC9107_Port.SpiWriteBytes(pData, Len);
    }
}

static void Vn_GC9107_Init(void)
{
    if(Vn_GC9107_Port.DelayMs == NULL)
    {
        return;
    }
    Vn_GC9107_Port.DelayMs(120);

    Vn_GC9107_SendCmd(0x11);  // Sleep Out
    Vn_GC9107_Port.DelayMs(120);

    Vn_GC9107_SendCmd(0x29);
    Vn_GC9107_Port.DelayMs(120);

    Vn_GC9107_SendCmd(0xFE);
    Vn_GC9107_SendCmd(0xFE);
    Vn_GC9107_SendCmd(0xEF);

    Vn_GC9107_SendCmd(0xB0);
    Vn_GC9107_SendData(0xC0);

    Vn_GC9107_SendCmd(0xB2);
    Vn_GC9107_SendData(0x24);

    Vn_GC9107_SendCmd(0xB3);
    Vn_GC9107_SendData(0x03);

    Vn_GC9107_SendCmd(0xB7);
    Vn_GC9107_SendData(0x01);

    Vn_GC9107_SendCmd(0xB6);
    Vn_GC9107_SendData(0x19);

    Vn_GC9107_SendCmd(0xAC);
    Vn_GC9107_SendData(0xDB);
    Vn_GC9107_SendCmd(0xAB);
    Vn_GC9107_SendData(0x0f);
    Vn_GC9107_SendCmd(0x3A);
    Vn_GC9107_SendData(0x05);

    Vn_GC9107_SendCmd(0xB4);
    Vn_GC9107_SendData(0x04);

    Vn_GC9107_SendCmd(0xA8);
    Vn_GC9107_SendData(0x0C);

    Vn_GC9107_SendCmd(0xb8);
    Vn_GC9107_SendData(0x08);

    Vn_GC9107_SendCmd(0xED);
    Vn_GC9107_SendData(0x03);

    Vn_GC9107_SendCmd(0xea);
    Vn_GC9107_SendData(0x9f);

    Vn_GC9107_SendCmd(0xc6);
    Vn_GC9107_SendData(0x2a);

    Vn_GC9107_SendCmd(0xc7);
    Vn_GC9107_SendData(0x10);

    Vn_GC9107_SendCmd(0xF0);
    Vn_GC9107_SendData(0x09);
    Vn_GC9107_SendData(0x35);
    Vn_GC9107_SendData(0x2a);
    Vn_GC9107_SendData(0x4a);
    Vn_GC9107_SendData(0xA9);
    Vn_GC9107_SendData(0x39);
    Vn_GC9107_SendData(0x35);
    Vn_GC9107_SendData(0x60);
    Vn_GC9107_SendData(0x00);
    Vn_GC9107_SendData(0x14);
    Vn_GC9107_SendData(0x0a);
    Vn_GC9107_SendData(0x16);
    Vn_GC9107_SendData(0x10);
    Vn_GC9107_SendData(0x1F);

    Vn_GC9107_SendCmd(0xF1);
    Vn_GC9107_SendData(0x13);
    Vn_GC9107_SendData(0x24);
    Vn_GC9107_SendData(0x55);
    Vn_GC9107_SendData(0x3c);
    Vn_GC9107_SendData(0xc6);
    Vn_GC9107_SendData(0x16);
    Vn_GC9107_SendData(0x3f);
    Vn_GC9107_SendData(0x60);
    Vn_GC9107_SendData(0x08);
    Vn_GC9107_SendData(0x06);
    Vn_GC9107_SendData(0x0d);
    Vn_GC9107_SendData(0x1f);
    Vn_GC9107_SendData(0x1c);
    Vn_GC9107_SendData(0x10);

    Vn_GC9107_SendCmd(0x36);
    Vn_GC9107_SendData(VN_GC9107_DISPLAY_MODE);
}


void Vn_GC9107_SetBackgroudColor(uint16_t Color)
{
    Vn_GC9107_BackgroudColor = Color;
}

/**
 * @brief  Fill the screen with a color
 * @param  Color: 16-bit RGB format
 * @retval None
 */
void Vn_GC9107_Full(uint32_t Color)
{

    uint8_t column_datas[VN_GC9107_COLUMN_NUMBER * 2];
    uint8_t color_high = Color >> 8;
    for(int32_t column = VN_GC9107_COLUMN_OFFSET; column < VN_GC9107_COLUMN_NUMBER; column++)  // column loop
    {
        column_datas[column * 2 + VN_GC9107_COLUMN_OFFSET] = color_high;
        column_datas[column * 2 + 1 + VN_GC9107_COLUMN_OFFSET] = Color;
    }

    Vn_GC9107_SetDisplayArea(0, 0, VN_GC9107_COLUMN_NUMBER, VN_GC9107_ROW_NUMBER);

    for(int32_t row = VN_GC9107_ROW_OFFSET; row < VN_GC9107_ROW_NUMBER; row++)  // row loop
    {
        Vn_GC9107_DrawMultiPixels((uint16_t*)column_datas, (VN_GC9107_COLUMN_NUMBER - VN_GC9107_COLUMN_OFFSET));
    }
}

/**
 * @brief  Set display area
 * @param  StartX: start column
 * @param  StartY: start row
 * @param  Width: width
 * @param  Height: height
 * @retval 0 if success, -1 otherwise
 */
int32_t Vn_GC9107_SetDisplayArea(uint16_t StartX, uint16_t StartY, uint16_t Width, uint16_t Height)
{
    uint8_t offsetX = StartX + VN_GC9107_COLUMN_OFFSET;
    uint8_t offsetY = StartY + VN_GC9107_ROW_OFFSET;

    if((offsetX > VN_GC9107_COLUMN_NUMBER) || (offsetY > VN_GC9107_ROW_NUMBER))
    {
        return -1;
    }

    Vn_GC9107_SendCmd(0x2a);                  // Column address set
    Vn_GC9107_SendData(0x00);                 // start column high
    Vn_GC9107_SendData(0x00 + offsetX);       // start column low
    Vn_GC9107_SendData(0x00);                 // end column high
    Vn_GC9107_SendData(offsetX + Width - 1);  // end column low

    Vn_GC9107_SendCmd(0x2b);                   // Row address set
    Vn_GC9107_SendData(0x00);                  // start row high
    Vn_GC9107_SendData(0x00 + offsetY);        // start row low
    Vn_GC9107_SendData(0x00);                  // end row high
    Vn_GC9107_SendData(offsetY + Height - 1);  // end row low

    Vn_GC9107_SendCmd(0x2C);  // Memory write

    return 0;
}

/**
 * @brief  Draw a pixel
 * @param  Color: 16-bit RGB format
 * @retval None
 */
void Vn_GC9107_DrawPixel(uint16_t Color)
{
    Vn_GC9107_SendData(Color >> 8);
    Vn_GC9107_SendData(Color);
}

/**
 * @brief  Draw pixels
 * @param  pColors: pointer to the pixel colors (MSB first)
 * @param  Count: number of pixels
 * @retval None
 */
void Vn_GC9107_DrawMultiPixels(uint16_t* pColors, uint16_t Count)
{
    Vn_GC9107_SendMultiData((uint8_t*)pColors, Count * 2);
}

int32_t Vn_GC9107_FastFlushArea(uint16_t StartX, uint16_t StartY, uint16_t EndX, uint16_t EndY, uint16_t *color_p)
{
    uint16_t width;
    uint16_t height;

    if (StartX > EndX || StartY > EndY || EndX >= VN_GC9107_COLUMN_NUMBER ||
        EndY >= VN_GC9107_ROW_NUMBER || color_p == NULL)
    {
        return -1;
    }

    width = EndX - StartX + 1;
    height = EndY - StartY + 1;
    if (Vn_GC9107_SetDisplayArea(StartX, StartY, width, height) != 0)
    {
        return -1;
    }

    Vn_GC9107_DrawMultiPixels(color_p, width * height);
    return 0;
}

/**
 * @brief  Show a character on the screen
 * @param  X: X coordinate (column)
 * @param  Y: Y coordinate (row)
 * @param  FontSize: font size (16 or 32)
 * @param  Chr: character to display
 * @param  Color: 16-bit RGB color
 * @return 0 if success, -1 otherwise
 */
int32_t Vn_GC9107_ShowChar(uint16_t X, uint16_t Y, uint32_t FontSize, char Chr, uint16_t Color)
{
    int32_t ret = 0;
    uint16_t bytes_count = 0;
    uint16_t offset = 0;
    uint8_t *pfont = NULL;

    /* Obtain the offset value (the ASCII character set starts from space, so - ' ' is the corresponding character in the character set) */
    offset = Chr - ' '; 
    /* Obtain the number of bytes occupied by the bitmap set corresponding to one character of the font */
    bytes_count = (FontSize / 8 + ((FontSize % 8) ? 1 : 0)) * (FontSize / 2); 

    switch (FontSize)
    {
        case 16:
            pfont = (uint8_t *)Vn_LCD_Font1608[offset];  /* Call the 1608 font */
            break;
        case 32:
            pfont = (uint8_t *)Vn_LCD_Font3216[offset];  /* Call the 3216 font */
            break;
        default:
            break;
    }

    if(pfont == NULL)
    {
        return -1;
    }

    ret = Vn_GC9107_SetDisplayArea(X, Y, FontSize / 2, FontSize);

    if(ret != 0)
    {
        return -1;
    }

    for(int32_t i = 0; i < bytes_count; i++)
    {
        for(int32_t bit = 0; bit < 8; bit++)
        {
            if((pfont[i] >> (7 - bit)) & 0x01)
            {
                Vn_GC9107_DrawPixel(Color);
            }
            else
            {
                Vn_GC9107_DrawPixel(Vn_GC9107_BackgroudColor); // background Color
            }
        }
    }

    return 0;
}

/**
 * @brief  Show a string on the screen
 * @param  X: X coordinate (column)
 * @param  Y: Y coordinate (row)
 * @param  FontSize: font size (16 or 32)
 * @param  pData: pointer to the string data
 * @param  Color: 16-bit RGB color
 * @return 0 if success, -1 if there is no space to display the string
 * @note   The string is displayed in a single line, and if it exceeds the screen width, it will wrap to the next line.
 */
int32_t Vn_GC9107_ShowShring(uint16_t X, uint16_t Y, uint32_t FontSize, char *pData, uint16_t Color)
{
    uint8_t offsetX = X;
    uint8_t offsetY = Y;
    uint8_t* ptr = pData;

    while ((*ptr <= '~') && (*ptr >= ' '))
    {
        if (offsetX + FontSize / 2 >= VN_GC9107_ROW_NUMBER)
        {
            offsetX = 0;  // Reset to the beginning of the line
            offsetY += FontSize;  // Move to the next line
        }

        if(offsetY >= VN_GC9107_COLUMN_NUMBER)
        {
            return -1;  // No more space to display
        }
        
        Vn_GC9107_ShowChar(offsetX, offsetY, FontSize, *ptr, Color);

        offsetX += FontSize / 2;
        ptr++;
    }

    return 0;
}