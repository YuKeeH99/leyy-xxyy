#ifndef COMPONENT_VN_HTTP_VN_HTTP_H_
#define COMPONENT_VN_HTTP_VN_HTTP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"

typedef struct {
    int32_t (*Init)(void);
    int32_t (*ClientPostSync)(const char *, const char *, const char *, char *, int32_t);
    int32_t (*ClientGetSync)(const char *, const char *, const char *, char *, int32_t);
    int32_t (*DeInit)(void);
} Vn_Http_Typedef;

extern int32_t Vn_Http_Instance(Vn_Http_Typedef * Instance, Vn_NetType_Typedef NetType);

#endif /* COMPONENT_VN_HTTP_VN_HTTP_H_ */
