#ifndef BSP_VN_PARAM_VN_PARAM_H_
#define BSP_VN_PARAM_VN_PARAM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

extern void Vn_Param_Init(void);
extern int32_t Vn_Param_Read(const char * Key_ID, uint8_t * Val_Buf, uint16_t Buf_Len);
extern int32_t Vn_Param_Write(const char * Key_ID, const uint8_t * Val, uint16_t Val_len);
extern int32_t Vn_param_delete(const char * Key_ID);

#endif /* BSP_VN_PARAM_VN_PARAM_H_ */
