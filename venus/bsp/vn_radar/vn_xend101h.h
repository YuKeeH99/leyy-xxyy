#ifndef __VN_XEND101H_H__
#define __VN_XEND101H_H__ 

#include "stdint.h"

#define VN_XEND101H_CMD_READ_VER                0x0000  // read version
#define VN_XEND101H_CMD_CONFIG_ENABLE           0x00FF  // enable config
#define VN_XEND101H_CMD_CONFIG_DISABLE          0x00FE  // disable config
#define VN_XEND101H_CMD_READ_SN                 0x0011  // read serial number
#define VN_XEND101H_CMD_WRITE_SN                0x0010  // write serial number
#define VN_XEND101H_CMD_READ_RADAR_CONFIG       0x0008  // read radar config
#define VN_XEND101H_CMD_WRITE_RADAR_CONFIG      0x0007  // write radar config
#define VN_XEND101H_CMD_SAVE_RADAR_CONFIG       0x00FD  // save radar config
#define VN_XEND101H_CMD_WRITE_SYSTEM_CONFIG     0x0012  // write system config
#define VN_XEND101H_CMD_START_AUTO_THRESHOD     0x0009  // start auto threshod
#define VN_XEND101H_CMD_QUERY_THRESHOD_PROGRESS 0x000A  // query threshod progress
#define VN_XEND101H_CMD_REPORT_INTERFERENCE     0x0014  // report interference


#define VN_XEND101H_FRAME_PAYLOAD_LEN 256
#define VN_XEND101H_FRAME_LEN         (VN_XEND101H_FRAME_PAYLOAD_LEN + 10)


typedef enum 
{
    VN_XEND101H_FRAME_STATE_IDLE,
    VN_XEND101H_FRAME_STATE_START,
    VN_XEND101H_FRAME_STATE_OK,
} Vn_XenD101H_FramerState_Typedef;


typedef struct
{
    uint8_t Msg[VN_XEND101H_FRAME_LEN];
    uint32_t MsgLen;
    uint16_t PayloadLen;
    uint8_t  Step;
    Vn_XenD101H_FramerState_Typedef State;
} Vn_XenD101H_Protocol_Typedef;

typedef int32_t (*Vn_XenD101H_UartWriteFunc)(uint8_t *pData, uint32_t Len);
typedef int32_t (*Vn_XenD101H_UartReadFunc)(uint8_t *pData, uint32_t Len);
typedef void (*Vn_XenD101H_DelayMsFunc)(uint32_t Time);
typedef uint32_t (*Vn_XenD101H_SystemTimeFunc)(void);
typedef void (*Vn_XenD101H_RtosYieldFunc)(void);

typedef struct
{
    Vn_XenD101H_UartWriteFunc Write;
    Vn_XenD101H_UartReadFunc Read;
    Vn_XenD101H_DelayMsFunc DelayMs;
    Vn_XenD101H_SystemTimeFunc SystemTime;
    Vn_XenD101H_RtosYieldFunc RtosYield;
}Vn_XenD101H_Port_Tyepedef;

int32_t Vn_XenD101H_Init(
    Vn_XenD101H_UartWriteFunc UartWrite,
    Vn_XenD101H_UartReadFunc UartRead,
    Vn_XenD101H_DelayMsFunc DelayMs,
    Vn_XenD101H_SystemTimeFunc SystemTime,
    Vn_XenD101H_RtosYieldFunc RtosYield);

int32_t Vn_XenD101H_ReadVerison(uint8_t *pVersion);

int32_t Vn_XenD101H_GetTargetDistance(int32_t *pDistance);

int32_t Vn_XenD101H_GetRadarMinDistance(uint32_t* pValue);

int32_t Vn_XenD101H_SetRadarMaxDistance(uint32_t Value);
int32_t Vn_XenD101H_GetRadarMaxDistance(uint32_t* pValue);

int32_t Vn_XenD101H_GetRadarTargetDisappearTime(uint32_t* pValue);
int32_t Vn_XenD101H_SetRadarTargetDisappearTime(uint32_t Value);

int32_t Vn_XenD101H_SaveRadarParam(void);

#endif