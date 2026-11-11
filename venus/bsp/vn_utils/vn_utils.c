#include "vn_utils.h"

#include <stddef.h>

#include "ql_log.h"
#include "ql_rtc.h"
#include "vn_log.h"

static uint32_t Vn_Utils_Mktime(uint32_t Year, uint32_t Month, uint32_t Day,
                                uint32_t Hour, uint32_t Second, uint32_t Minute)
{
    uint32_t days;

    if (Month <= 2U)
    {
        Year--;
        Month += 10U;
    }
    else
    {
        Month -= 2U;
    }

    days = 365U * Year + Year / 400U + Year / 4U - Year / 100U +
           (367U * Month) / 12U + Day;
    return days * 86400U + Hour * 3600U + Minute * 60U + Second + 0x86B1BF80U;
}

uint64_t ql_rtc_to_timestamp(void)
{
    ql_rtc_time_t rtc = {0};

    if (ql_rtc_get_time(&rtc) != 0)
    {
        return 0;
    }

    VN_LOG_D("RTC %04d-%02d-%02d %02d:%02d:%02d",
             rtc.tm_year, rtc.tm_mon, rtc.tm_mday,
             rtc.tm_hour, rtc.tm_min, rtc.tm_sec);
    return Vn_Utils_Mktime((uint32_t)rtc.tm_year, (uint32_t)rtc.tm_mon,
                           (uint32_t)rtc.tm_mday, (uint32_t)rtc.tm_hour,
                           (uint32_t)rtc.tm_sec, (uint32_t)rtc.tm_min);
}

static int Vn_Utils_HexNibble(char c)
{
    if ((c >= '0') && (c <= '9'))
    {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'f'))
    {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'F'))
    {
        return c - 'A' + 10;
    }

    return -1;
}

uint32_t Vn_DiffTime(uint32_t StartTime, uint32_t EndTime)
{
    uint32_t diff = (StartTime >= EndTime)
                        ? StartTime - EndTime
                        : EndTime - StartTime;

    if (diff > (UINT32_MAX / 2U))
    {
        diff = UINT32_MAX - diff + 1U;
    }

    return diff;
}

bool Vn_HexStringToBin(const char *pStrData, uint16_t DataLen, uint8_t *pBinData)
{
    uint16_t index;

    if ((pStrData == NULL) || (pBinData == NULL) || ((DataLen & 1U) != 0U))
    {
        return false;
    }

    for (index = 0U; index < DataLen; index = (uint16_t)(index + 2U))
    {
        int high = Vn_Utils_HexNibble(pStrData[index]);
        int low = Vn_Utils_HexNibble(pStrData[index + 1U]);

        if ((high < 0) || (low < 0))
        {
            return false;
        }

        pBinData[index / 2U] = (uint8_t)((high << 4) | low);
    }

    return true;
}

bool Vn_BinToHexString(const uint8_t *pBinData, uint16_t DataLen, char *pStrData)
{
    static const char hex_digits[] = "0123456789abcdef";
    uint16_t index;

    if ((pBinData == NULL) || (pStrData == NULL))
    {
        return false;
    }

    for (index = 0U; index < DataLen; ++index)
    {
        pStrData[(uint32_t)index * 2U] = hex_digits[pBinData[index] >> 4];
        pStrData[(uint32_t)index * 2U + 1U] = hex_digits[pBinData[index] & 0x0FU];
    }
    pStrData[(uint32_t)DataLen * 2U] = '\0';

    return true;
}

void Vn_CrLfToString(const uint8_t *pDataIn, uint32_t Len, uint8_t *pDataOut)
{
    uint32_t input_pos = 0U;
    uint32_t output_pos = 0U;

    if (pDataOut == NULL)
    {
        return;
    }
    if ((pDataIn == NULL) || (Len == 0U))
    {
        pDataOut[0] = '\0';
        return;
    }

    while (input_pos < Len)
    {
        if ((pDataIn[input_pos] == (uint8_t)'\r') &&
            ((input_pos + 1U) < Len) &&
            (pDataIn[input_pos + 1U] == (uint8_t)'\n'))
        {
            pDataOut[output_pos++] = (uint8_t)'\\';
            pDataOut[output_pos++] = (uint8_t)'r';
            pDataOut[output_pos++] = (uint8_t)'\\';
            pDataOut[output_pos++] = (uint8_t)'n';
            input_pos += 2U;
        }
        else
        {
            pDataOut[output_pos++] = pDataIn[input_pos++];
        }
    }

    pDataOut[output_pos] = '\0';
}
