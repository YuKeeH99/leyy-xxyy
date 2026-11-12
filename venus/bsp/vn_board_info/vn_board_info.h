#ifndef BSP_VN_BOARD_INFO_VN_BOARD_INFO_H_
#define BSP_VN_BOARD_INFO_VN_BOARD_INFO_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "ql_rtc.h"
#include "ql_type.h"

typedef struct {
    uint8_t MacAddr[18];
    uint8_t UUID[64];
    uint8_t Name[32];
    uint8_t Version[32];
    uint8_t Firmware[32];
} Vn_BoardInfo_Typedef;

extern int32_t Vn_GetBoardInfo(Vn_BoardInfo_Typedef * BoardInfo, Vn_NetType_Typedef NetType);
extern int Vn_Get_IMEI(char * imei, unsigned int imei_len);
extern int Vn_Get_Cellura_MAC(uint8_t * mac_val);
extern void Vn_Get_CPU_UID(unsigned char * uid_buf);
extern void Vn_Print_Time(ql_rtc_time_t * tmp);
extern int Vn_Get_Cur_SIMICCID(char * sim_iccid);
extern void Vn_Get_Version(char * version);
extern int Vn_Generate_Client_Id(void);
extern BOOL extract_rssi_from_csq(const CHAR * csq_response, CHAR * rssi_str, INT16 max_len);
extern BOOL extract_operator_name_from_cops(const CHAR * cops_response, CHAR * operator_name, INT16 max_len);

#endif /* BSP_VN_BOARD_INFO_VN_BOARD_INFO_H_ */
