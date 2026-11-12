#ifndef COMPONENT_VN_HTTP_VN_HTTP_WIFI_H_
#define COMPONENT_VN_HTTP_VN_HTTP_WIFI_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

extern int32_t Vn_HttpWifi_Init(void);
extern int32_t Vn_HttpWifi_DeInit(void);
extern int32_t Vn_HttpWifi_ClientPostSync(const char * Url, const char * Header, const char * Body, char * Response, int32_t ResponseBuffLen);
extern int32_t Vn_HttpWifi_ClientGetSync(const char * Url, const char * Header, const char * Body, char * Response, int32_t ResponseBuffLen);

#endif /* COMPONENT_VN_HTTP_VN_HTTP_WIFI_H_ */
