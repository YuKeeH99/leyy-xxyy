#include "vn_spi_flash.h"

#include "ql_gpio.h"
#include "ql_spi.h"

#define VN_FLASH_CMD_READ          0x03
#define VN_FLASH_CMD_WRITE_ENABLE  0x06
#define VN_FLASH_CMD_PAGE_PROGRAM  0x02
#define VN_FLASH_CMD_SECTOR_ERASE  0x20
#define VN_FLASH_CMD_READ_STATUS   0x05
#define VN_FLASH_STATUS_BUSY       0x01

static void VN_SPI_Flash_Select(bool Select)
{
    ql_gpio_set_level(VN_PIN_FLASH_SPI_CS, Select ? PIN_LEVEL_LOW : PIN_LEVEL_HIGH);
}

static void VN_SPI_Flash_WaitWriteEnd(void)
{
    uint8_t command = VN_FLASH_CMD_READ_STATUS;
    uint8_t status = 0xFF;

    VN_SPI_Flash_Select(true);
    do
    {
        ql_spi_write(VN_SPI_PORT_FLASH, &command, 1);
        ql_spi_read(VN_SPI_PORT_FLASH, &status, 1);
    } while ((status & VN_FLASH_STATUS_BUSY) != 0);
    VN_SPI_Flash_Select(false);
}

int32_t VN_SPI_Flash_Init(void)
{
    int32_t ret = ql_spi_init(VN_SPI_PORT_FLASH, QL_SPI_MODE0, QL_SPI_CLK_26MHZ);
    VN_SPI_Flash_Select(false);
    return ret == 0 ? VN_RET_OK : VN_RET_ERROR;
}

int32_t VN_SPI_Flash_ReadData(uint32_t Addr, uint8_t *pBuffer, uint32_t Len)
{
    uint8_t command = VN_FLASH_CMD_READ;
    uint8_t address[3] = {(uint8_t)(Addr >> 16), (uint8_t)(Addr >> 8), (uint8_t)Addr};
    uint32_t i;

    if (pBuffer == NULL && Len != 0)
    {
        return VN_RET_ERROR;
    }

    VN_SPI_Flash_Select(true);
    ql_spi_write(VN_SPI_PORT_FLASH, &command, 1);
    ql_spi_write(VN_SPI_PORT_FLASH, address, sizeof(address));
    for (i = 0; i < Len; i++)
    {
        ql_spi_read(VN_SPI_PORT_FLASH, &pBuffer[i], 1);
    }
    VN_SPI_Flash_Select(false);
    return VN_RET_OK;
}

void VN_SPI_Flash_Write_Enable(void)
{
    uint8_t command = VN_FLASH_CMD_WRITE_ENABLE;

    VN_SPI_Flash_Select(true);
    ql_spi_write(VN_SPI_PORT_FLASH, &command, 1);
    VN_SPI_Flash_Select(false);
}

int32_t VN_SPI_Flash_EraseSector(uint32_t Addr)
{
    uint8_t command = VN_FLASH_CMD_SECTOR_ERASE;
    uint8_t address[3] = {(uint8_t)(Addr >> 16), (uint8_t)(Addr >> 8), (uint8_t)Addr};
    int32_t ret;

    VN_SPI_Flash_Write_Enable();
    VN_SPI_Flash_Select(true);
    ql_spi_write(VN_SPI_PORT_FLASH, &command, 1);
    ret = ql_spi_write(VN_SPI_PORT_FLASH, address, sizeof(address));
    VN_SPI_Flash_Select(false);
    VN_SPI_Flash_WaitWriteEnd();
    return ret == 0 ? VN_RET_OK : VN_RET_ERROR;
}

int32_t VN_SPI_Flash_EraseForWrite(uint32_t Addr, uint32_t Len)
{
    uint32_t current;
    uint32_t end;

    if (Len == 0)
    {
        return VN_RET_ERROR;
    }

    current = Addr & ~(VN_SPI_FLASH_SECTOR_SIZE - 1U);
    end = (Addr + Len + VN_SPI_FLASH_SECTOR_SIZE - 1U) & ~(VN_SPI_FLASH_SECTOR_SIZE - 1U);
    while (current < end)
    {
        if (VN_SPI_Flash_EraseSector(current) != VN_RET_OK)
        {
            return VN_RET_ERROR;
        }
        current += VN_SPI_FLASH_SECTOR_SIZE;
    }
    return VN_RET_OK;
}

int32_t VN_SPI_Flash_Page_Write(uint32_t Addr, uint8_t *pBuffer, uint32_t Len)
{
    uint8_t command = VN_FLASH_CMD_PAGE_PROGRAM;
    uint8_t address[3] = {(uint8_t)(Addr >> 16), (uint8_t)(Addr >> 8), (uint8_t)Addr};
    uint8_t dummy = 0;
    int32_t ret;

    if (pBuffer == NULL || Len == 0 || Len > VN_SPI_FLASH_PAGE_SIZE)
    {
        return VN_RET_ERROR;
    }

    VN_SPI_Flash_Write_Enable();
    VN_SPI_Flash_Select(true);
    ql_spi_write(VN_SPI_PORT_FLASH, &command, 1);
    ql_spi_write(VN_SPI_PORT_FLASH, address, sizeof(address));
    ql_spi_write(VN_SPI_PORT_FLASH, &dummy, 1);
    ret = ql_spi_write(VN_SPI_PORT_FLASH, pBuffer, Len);
    VN_SPI_Flash_Select(false);
    VN_SPI_Flash_WaitWriteEnd();
    return ret == 0 ? VN_RET_OK : VN_RET_ERROR;
}

int32_t VN_SPI_Flash_WriteData(uint32_t Addr, uint8_t *pBuffer, uint32_t Len)
{
    uint32_t chunk;

    if (pBuffer == NULL && Len != 0)
    {
        return VN_RET_ERROR;
    }

    while (Len != 0)
    {
        chunk = VN_SPI_FLASH_PAGE_SIZE - (Addr % VN_SPI_FLASH_PAGE_SIZE);
        if (chunk > Len)
        {
            chunk = Len;
        }
        if (VN_SPI_Flash_Page_Write(Addr, pBuffer, chunk) != VN_RET_OK)
        {
            return VN_RET_ERROR;
        }
        Addr += chunk;
        pBuffer += chunk;
        Len -= chunk;
    }
    return VN_RET_OK;
}
