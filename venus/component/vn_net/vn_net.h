#ifndef COMPONENT_VN_NET_VN_NET_H_
#define COMPONENT_VN_NET_VN_NET_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "ql_data_call.h"
#include "inet.h"

typedef struct {
    int32_t ProfileIdx;
    int32_t IpVersion;
    int32_t State;
    char IpaddrV4[24];
    struct in_addr InIpAddrv4;
} Vn_DataCallInfo_t;

extern int Vn_Cellular_RegisterNetwork(int Timeout);
extern int Vn_Cellular_StartDataCall(int ProfileIdx, int32_t Timeout);
extern int Vn_Cellular_GetDataCallInfo(int ProfileIdx, Vn_DataCallInfo_t * Info);
extern uint8_t Vn_Extract_Domain_From_Url(const char * Url, char * Domain);
extern int Vn_Parse_DNS(char * Ip, const uint8_t * Domain);
extern void Vn_ParseUrl(const char * Url, char * Protocol, char * Host, char * Path);
extern void Vn_DataCallCallback(int ProfileIdx, int Status);
extern int Vn_Cellular_DetctSimHotPlug(void);
extern int Vn_Cellular_CheckSim(void);
extern int Vn_Cellular_GetImsi(char * Buff, int Length);

#endif /* COMPONENT_VN_NET_VN_NET_H_ */
