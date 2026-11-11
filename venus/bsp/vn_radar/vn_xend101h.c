#include "vn_xend101h.h"
#include <string.h>
#include <stdio.h>


#define VN_XenD101H_RESPONSE_TIMEOUT_MS   1000

#define BYTES_TO_U16(payload, offset) \
    ((uint16_t)((payload)[(offset)]) | \
     ((uint16_t)((payload)[(offset)+1]) << 8))

#define BYTES_TO_U32(payload, offset) \
    ((uint32_t)((payload)[(offset)]) | \
     ((uint32_t)((payload)[(offset)+1]) << 8) | \
     ((uint32_t)((payload)[(offset)+2]) << 16) | \
     ((uint32_t)((payload)[(offset)+3]) << 24))


static Vn_XenD101H_Port_Tyepedef Vn_XenD101H_Port = { 0 };

static const uint8_t FrameHeaders[4] = {0xFD, 0xFC, 0xFB, 0xFA};

static const uint8_t FrameTails[4] = {0x04, 0x03, 0x02, 0x01};

static void FrameResetParam(Vn_XenD101H_Protocol_Typedef *Prot)
{
    Prot->Step = 0;
    Prot->MsgLen = 0;
    Prot->State = VN_XEND101H_FRAME_STATE_IDLE;
} 

Vn_XenD101H_FramerState_Typedef Vn_XenD101H_Framer(uint8_t Data, Vn_XenD101H_Protocol_Typedef *Prot)
{

    if(VN_XEND101H_FRAME_STATE_OK == Prot->State)
    {
        Prot->Step = 0;
        Prot->MsgLen = 0;
        Prot->State = VN_XEND101H_FRAME_STATE_IDLE;
    }

    switch(Prot->Step)
    {
        /* Header */
        case 0:
        {
            if((Prot->MsgLen < 4) && (FrameHeaders[Prot->MsgLen] == Data))
            {
                Prot->Msg[Prot->MsgLen] = Data;
                Prot->MsgLen++;

                // Header check success
                if(Prot->MsgLen == 4)
                {
                    Prot->PayloadLen = 0;
                    Prot->Step = 1;
                }
                Prot->State = VN_XEND101H_FRAME_STATE_START;
            }
            else
            {
                FrameResetParam(Prot);
            }
        }
        break;
        /* Length  */
        case 1:
        {
            Prot->Msg[Prot->MsgLen] = Data;
            Prot->MsgLen++;

            Prot->PayloadLen = Data;
            Prot->Step = 2;
        }
        break;
        case 2:
        {
            Prot->Msg[Prot->MsgLen] = Data;
            Prot->MsgLen++;

            Prot->PayloadLen |= (Data << 8);
            if (Prot->PayloadLen < VN_XEND101H_FRAME_PAYLOAD_LEN)
            {
                Prot->Step = 3;
            }
            else
            {
                FrameResetParam(Prot);
            }
        }
        break;
        /* Payload */
        case 3:
        {
            Prot->Msg[Prot->MsgLen] = Data;
            Prot->MsgLen++;

            if ((Prot->MsgLen - 6) >= Prot->PayloadLen)
            {
                Prot->Step = 4;
            }
        }
        break;
        /* Tail */
        case 4:
        {
            uint16_t tail_offset = Prot->MsgLen - 4 - 2 - Prot->PayloadLen;
            if (FrameTails[tail_offset] == Data)
            {
                Prot->Msg[Prot->MsgLen] = Data;
                Prot->MsgLen++;

                if((tail_offset + 1) == 4)
                {
                    Prot->State = VN_XEND101H_FRAME_STATE_OK;
                }
            }
            else
            {
                FrameResetParam(Prot);
            }
        }
        break;
        default:
        {
            FrameResetParam(Prot);
        }
        break;
    }

    return Prot->State;
}

int32_t Vn_XenD101H_EncodeFrame(uint8_t *pBuffer, uint16_t CMD, uint8_t *pCmdData, uint16_t CmdDataLen)
{

    // header
    memcpy(pBuffer, FrameHeaders, 4);

    // palyload length
    pBuffer[4] = (uint8_t)(CmdDataLen + 2);
    pBuffer[5] = (uint8_t)(CmdDataLen + 2) >> 8;

    // CMD
    pBuffer[6] = (uint8_t)CMD;
    pBuffer[7] = (uint8_t)CMD >> 8;

    // Data
    if((CmdDataLen > 0) && (pCmdData != NULL))
    {
        memcpy(pBuffer + 8, pCmdData, CmdDataLen);
    }

    // Tail
    memcpy(pBuffer + 8 + CmdDataLen, FrameTails, 4);

    return (8 + CmdDataLen + 4);
}

int32_t Vn_XenD101H_FramerGetAckCmd(const uint8_t* FramerMsg, uint16_t *Cmd)
{

    if(NULL == FramerMsg)
    {
        return -1;
    }

    *Cmd = BYTES_TO_U16(FramerMsg, 6);

    return 0;
}


int32_t Vn_XenD101H_FramerGetAckStatus(const uint8_t* FramerMsg, uint16_t* Status)
{
    if(NULL == FramerMsg)
    {
        return -1;
    }

    *Status = BYTES_TO_U16(FramerMsg, 8);

    return 0;
}

int32_t Vn_XenD101H_FramerGetAckPayload(const uint8_t* FramerMsg, uint8_t* Payload, uint16_t* PayloadLen)
{
    uint16_t length;

    if(NULL == FramerMsg || NULL == Payload || NULL == PayloadLen)
    {
        return -1;
    }

    length = BYTES_TO_U16(FramerMsg, 4);
    *PayloadLen = length;

    memcpy(Payload, FramerMsg + 6, length);

    return 0;
}

int32_t Vn_XenD101H_Init(
    Vn_XenD101H_UartWriteFunc UartWrite, 
    Vn_XenD101H_UartReadFunc UartRead,
    Vn_XenD101H_DelayMsFunc DelayMs,
    Vn_XenD101H_SystemTimeFunc SystemTime,
    Vn_XenD101H_RtosYieldFunc RtosYield)
{

    if((NULL == UartWrite) || (NULL == UartRead) || (NULL == DelayMs) || (NULL == SystemTime) || (NULL == RtosYield))
    {
        return -1;
    }

    Vn_XenD101H_Port.Write = UartWrite;
    Vn_XenD101H_Port.Read = UartRead;
    Vn_XenD101H_Port.DelayMs = DelayMs;
    Vn_XenD101H_Port.SystemTime = SystemTime;
    Vn_XenD101H_Port.RtosYield = RtosYield;

    return 0;
}

int32_t Vn_XenD101H_GetCmdResponse(uint16_t CMD, uint8_t *pPayload, uint16_t* pPayloadLen, uint32_t TimeoutMs)
{
    Vn_XenD101H_Protocol_Typedef Protocol =
        {
            .MsgLen = 0,
            .State = VN_XEND101H_FRAME_STATE_IDLE
        };

    uint8_t buffer[256] = {0};
    uint16_t read_len = 0;
    uint16_t cmd_ack;

    Vn_XenD101H_FramerState_Typedef state;

    uint32_t timestamp = Vn_XenD101H_Port.SystemTime();

    do
    {
        read_len = Vn_XenD101H_Port.Read(buffer, sizeof(buffer));

        for(int i = 0; i < read_len; i++)
        {
            state = Vn_XenD101H_Framer(buffer[i], &Protocol);

            if(state == VN_XEND101H_FRAME_STATE_OK)
            {
                uint16_t rsp_cmd = 0;

                Vn_XenD101H_FramerGetAckCmd(Protocol.Msg, &rsp_cmd);

                if(CMD == (rsp_cmd & 0x00FF))
                {
                    // check ack
                    uint16_t cmd_ack;

                    Vn_XenD101H_FramerGetAckStatus(Protocol.Msg, &cmd_ack);
                    if(cmd_ack != 0)
                    {
                        return -1;
                    }
                    if((pPayload != NULL) && (pPayloadLen != NULL))
                    {
                        Vn_XenD101H_FramerGetAckPayload(Protocol.Msg, pPayload, pPayloadLen);
                    }

                    return 0;
                }
            }
        }

        Vn_XenD101H_Port.DelayMs(1);
        //Vn_XenD101H_Port.RtosYield;

    } while ((Vn_XenD101H_Port.SystemTime() - timestamp) < TimeoutMs);

    return -1;
}

static int32_t Vn_XenD101H_ExecCommandWithResp(uint16_t CMD, const uint8_t* pCmdData, uint16_t CmdDataLen, uint8_t* pPayload, uint16_t* pPayloadLen) 
{
    uint8_t Buffer[VN_XEND101H_FRAME_LEN];
    int32_t ret;
    uint8_t data[8];
    uint16_t frame_length;


    if((NULL == Vn_XenD101H_Port.Write) || (NULL == Vn_XenD101H_Port.Read)
        || (NULL == Vn_XenD101H_Port.DelayMs) || (NULL == Vn_XenD101H_Port.SystemTime))
    {
        return -1;
    }


    data[0] = 0x01;
    data[1] = 0x00;

    const static uint8_t cmd_cnfig_enable_frame[] = {0xFD, 0xFC, 0xFB, 0xFA, 0x04, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x03, 0x02, 0x01};
    const static uint8_t cmd_config_disable_frame[] = {0xFD,0xFC,0xFB,0xFA,0x02,0x00,0xFE,0x00,0x04,0x03,0x02,0x01};

    // Config Enable
    Vn_XenD101H_Port.Write((uint8_t*)cmd_cnfig_enable_frame, sizeof(cmd_cnfig_enable_frame));
    ret = Vn_XenD101H_GetCmdResponse(VN_XEND101H_CMD_CONFIG_ENABLE, NULL, NULL, VN_XenD101H_RESPONSE_TIMEOUT_MS);
    if(ret != 0)
    {
        return -1;
    }

    frame_length = Vn_XenD101H_EncodeFrame(Buffer, CMD, (uint8_t*)pCmdData, CmdDataLen);
    Vn_XenD101H_Port.Write(Buffer,frame_length);
    ret = Vn_XenD101H_GetCmdResponse(CMD, pPayload, pPayloadLen, VN_XenD101H_RESPONSE_TIMEOUT_MS);

    // Config Disable
    Vn_XenD101H_Port.Write((uint8_t*)cmd_config_disable_frame, sizeof(cmd_config_disable_frame));
    Vn_XenD101H_GetCmdResponse(VN_XEND101H_CMD_CONFIG_DISABLE, NULL, NULL, VN_XenD101H_RESPONSE_TIMEOUT_MS);

    if(ret != 0)
    {
        return -1;
    }

    return 0;
}

static int32_t Vn_XenD101H_ExecCommand(uint16_t CMD, const uint8_t* pCmdData, uint16_t CmdDataLen)
{
    return Vn_XenD101H_ExecCommandWithResp(CMD, pCmdData, CmdDataLen, NULL, NULL);
}

/**
 *  @brief Read version
 *  @param[out] pVersion: version
 *  @return 0: success, -1: fail
 */
int32_t Vn_XenD101H_ReadVerison(uint8_t *pVersion)
{

    int16_t ver_length;
    uint8_t payload[VN_XEND101H_FRAME_PAYLOAD_LEN];
    uint16_t payload_length;

    if(Vn_XenD101H_ExecCommandWithResp(VN_XEND101H_CMD_READ_VER, NULL, 0, payload, &payload_length) == 0)
    {
        ver_length = BYTES_TO_U16(payload, 4);
        memcpy(pVersion, payload + 6, ver_length);

        return ver_length;
    }

    return -1;
}

/**
 * @brief Get the target distance
 * @param[out] pDistance: target distance ( if < 0, no target), util: 0.1m
 * @return 0: success; -1: fail
 */
int32_t Vn_XenD101H_GetTargetDistance(int32_t *pDistance)
{ 
    uint8_t buff[VN_XEND101H_FRAME_LEN];
    uint16_t recv_length;
    int32_t  ret = -1;
    uint8_t* offset_ptr;
    uint8_t* matched_ptr;
    do
    {
        recv_length = Vn_XenD101H_Port.Read(buff, sizeof(buff) - 1);

        buff[recv_length] = '\0';
        offset_ptr = buff;

        while(offset_ptr < buff + recv_length)
        {
            // ascll
            matched_ptr = strstr((char*)offset_ptr, "OFF");
            if(matched_ptr != NULL)
            {
                *pDistance = -1;
                offset_ptr = matched_ptr + sizeof("OFF");
                ret = 0;
                continue;
            }

            matched_ptr = strstr((char*)offset_ptr, "distance:");
            if(matched_ptr != NULL)
            {
                if(sscanf((char*)matched_ptr, "distance:%d", pDistance) == 1)
                {
                    ret = 0;
                    offset_ptr = matched_ptr + sizeof("distance:");
                    continue;
                }
            }

            break;
        }
    } while(recv_length >= (sizeof(buff) - 1));

    // binary

    /**
     *  TODO ..
     */ 

    return ret;
}

/**
 * @brief Get Radar Min Distance
 * @param [out] pValue: min distance, util: 0.1m
 * @return 0:success, -1:fail
 */
int32_t Vn_XenD101H_GetRadarMinDistance(uint32_t* pValue)
{

    uint8_t payload[VN_XEND101H_FRAME_PAYLOAD_LEN];
    uint16_t payload_length;
    uint16_t param = 0x0000;

    if(Vn_XenD101H_ExecCommandWithResp(VN_XEND101H_CMD_READ_RADAR_CONFIG, (const uint8_t*)&param, 2, payload, &payload_length) == 0)
    {

        *pValue = BYTES_TO_U32(payload, 4);
        return 0;
    }

    return -1;
}

/**
 * @brief Get radar max distance
 * @param[out] pValue: max distance, util: 0.1m
 * @return 0: success; -1: fail
 */
int32_t Vn_XenD101H_GetRadarMaxDistance(uint32_t* pValue)
{

    uint8_t payload[VN_XEND101H_FRAME_PAYLOAD_LEN];
    uint16_t payload_length;
    uint16_t param = 0x0001;

    if(Vn_XenD101H_ExecCommandWithResp(VN_XEND101H_CMD_READ_RADAR_CONFIG, (const uint8_t*)&param, 2, payload, &payload_length) == 0)
    {
        *pValue = BYTES_TO_U32(payload, 4);
        
        return 0;
    }

    return -1;
}

/**
 * @brief Get radar max distance
 * @param[in] Value: max distance, util: 0.1m
 * @retval 0: success; -1: fail
 */
int32_t Vn_XenD101H_SetRadarMaxDistance(uint32_t Value)
{
    uint8_t cmd_data[6];
    uint16_t param = 0x0001;

    memcpy(cmd_data, &param, 2);
    Value /= 10;
    memcpy(cmd_data + 2, &Value, 4);

    if(Vn_XenD101H_ExecCommandWithResp(VN_XEND101H_CMD_WRITE_RADAR_CONFIG, cmd_data, 6, NULL, NULL) == 0)
    {
        return 0;
    }

    return -1;
}

/**
 * @brief Set radar target disappear time
 * @param[out] Value: target disappear time, unit: seconds
 * @retval 0: success; -1: fail
 */
int32_t Vn_XenD101H_GetRadarTargetDisappearTime(uint32_t* pValue)
{

    uint8_t payload[VN_XEND101H_FRAME_PAYLOAD_LEN];
    uint16_t payload_length;
    uint16_t param = 0x0004;

    if(Vn_XenD101H_ExecCommandWithResp(VN_XEND101H_CMD_READ_RADAR_CONFIG, (const uint8_t*)&param, 2, payload, &payload_length) == 0)
    {
        *pValue = BYTES_TO_U32(payload, 4);

        return 0;
    }

    return -1;
}

/**
 * @brief Set radar target disappear time
 * @param Value: disappear time, unit: seconds
 * @return 0: success, -1: fail
 */
int32_t Vn_XenD101H_SetRadarTargetDisappearTime(uint32_t Value)
{
    uint8_t cmd_data[6];
    uint16_t param = 0x0004;

    memcpy(cmd_data, &param, 2);
    memcpy(cmd_data + 2, &Value, 4);

    if(Vn_XenD101H_ExecCommand(VN_XEND101H_CMD_WRITE_RADAR_CONFIG, cmd_data, 6) == 0)
    {
        return 0;
    }

    return -1;
}

/**
 * @brief Save radar parameter
 * @return 0: success, -1: fail
 */
int32_t Vn_XenD101H_SaveRadarParam(void)
{
    uint8_t cmd_data[6];

    if(Vn_XenD101H_ExecCommand(VN_XEND101H_CMD_SAVE_RADAR_CONFIG, cmd_data, 0) == 0)
    {
        return 0;
    }

    return -1;
}

