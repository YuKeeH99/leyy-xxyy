#ifndef BSP_VN_SPI_FLASH_VN_SPI_FLASH_H_
#define BSP_VN_SPI_FLASH_VN_SPI_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

#define VN_SPI_FLASH_SECTOR_SIZE  4096U
#define VN_SPI_FLASH_PAGE_SIZE    256U

extern int32_t VN_SPI_Flash_Init(void);
extern int32_t VN_SPI_Flash_ReadData(uint32_t Addr, uint8_t * pBuffer, uint32_t Len);
extern int32_t VN_SPI_Flash_EraseForWrite(uint32_t Addr, uint32_t Len);
extern int32_t VN_SPI_Flash_WriteData(uint32_t Addr, uint8_t * pBuffer, uint32_t Len);
extern void VN_SPI_Flash_Write_Enable(void);
extern int32_t VN_SPI_Flash_EraseSector(uint32_t Addr);
extern int32_t VN_SPI_Flash_Page_Write(uint32_t Addr, uint8_t * pBuffer, uint32_t Len);
extern void VN_SPI_Read_Flash_ID(void);
extern int8_t memory_compare(uint8_t * src, uint8_t * dst, uint16_t length);

#endif /* BSP_VN_SPI_FLASH_VN_SPI_FLASH_H_ */
