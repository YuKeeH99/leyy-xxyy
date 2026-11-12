#ifndef COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_H_
#define COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "vn_websocket_common.h"

typedef void (*Vn_Websocket_MsgHandlerFunc)(uint8_t, uint8_t *, uint32_t);

typedef struct {
    int32_t (*Init)(void (*)(uint8_t, uint8_t *, uint32_t));
    int32_t (*Connect)(uint32_t, uint16_t, int8_t *, int8_t *);
    int32_t (*DisConnect)(void);
    _Bool (*IsConnected)(void);
    int32_t (*Send)(uint8_t *, uint32_t, uint8_t);
    int32_t (*Poll)(uint32_t);
    int32_t (*DeInit)(void);
} Vn_Websocket_Typedef;

extern int32_t Vn_Websocket_Instance(Vn_Websocket_Typedef * Instance, Vn_NetType_Typedef NetType);

#endif /* COMPONENT_VN_WEBSOCKET_VN_WEBSOCKET_H_ */
