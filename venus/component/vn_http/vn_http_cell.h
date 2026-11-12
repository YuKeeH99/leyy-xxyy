#ifndef COMPONENT_VN_HTTP_VN_HTTP_CELL_H_
#define COMPONENT_VN_HTTP_VN_HTTP_CELL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "ql_http_client.h"

typedef struct {
    uint8_t * Data;
    uint32_t Size;
    uint32_t Offset;
    _Bool IsHadRecv;
} Vn_HttpData_Typedef;

extern int Vn_HttpCell_EventCallback(QL_HTTP_CLIENT_T * client, QL_HTTP_CLIENT_EVENT_E event, int status_code, char * data, int data_len, void * private_data);
extern int32_t Vn_HttpCell_Init(void);
extern int32_t Vn_HttpCell_DeInit(void);
extern int32_t Vn_HttpCell_ClientPostSync(const char * Url, const char * Header, const char * Body, char * Response, int32_t ResponseBuffLen);
extern int32_t Vn_HttpCell_ClientGetSync(const char * Url, const char * Header, const char * Body, char * Response, int32_t ResponseBuffLen);

#endif /* COMPONENT_VN_HTTP_VN_HTTP_CELL_H_ */
