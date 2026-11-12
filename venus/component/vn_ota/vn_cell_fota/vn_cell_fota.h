#ifndef COMPONENT_VN_OTA_VN_CELL_FOTA_VN_CELL_FOTA_H_
#define COMPONENT_VN_OTA_VN_CELL_FOTA_VN_CELL_FOTA_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

extern int Vn_Fota_Firmware_Cellular(int profile_idx, char * address, uint16_t port, char * rootCA_path);
extern int Vn_Http_Protocol_Type(char * url);
extern int Vn_Get_FotaProgress(void);

#endif /* COMPONENT_VN_OTA_VN_CELL_FOTA_VN_CELL_FOTA_H_ */
