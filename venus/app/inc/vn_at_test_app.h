#ifndef APP_INC_VN_AT_TEST_APP_H_
#define APP_INC_VN_AT_TEST_APP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef struct {
    char url[256];
    char enable;
} Vn_OTA_AT_Params_TypeDef;

extern void Vn_At_Cmd_App(void * arg);
extern Vn_OTA_AT_Params_TypeDef Vn_OTA_AT_Params;

#endif /* APP_INC_VN_AT_TEST_APP_H_ */
