#ifndef BSP_VN_UTILS_VN_UTILS_H_
#define BSP_VN_UTILS_VN_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

extern uint32_t Vn_DiffTime(uint32_t StartTime, uint32_t EndTime);
extern uint64_t ql_rtc_to_timestamp(void);
extern _Bool Vn_HexStringToBin(const char * pStrData, uint16_t DataLen, uint8_t * pBinData);
extern _Bool Vn_BinToHexString(const uint8_t * pBinData, uint16_t DataLen, char * pStrData);
extern void Vn_CrLfToString(const uint8_t * pDataIn, uint32_t Len, uint8_t * pDataOut);
extern void __wrap_free(void * ptr);
extern long unsigned int mktime1(unsigned int year, unsigned int mon, unsigned int day, unsigned int hour, unsigned int min, unsigned int sec);

#endif /* BSP_VN_UTILS_VN_UTILS_H_ */
