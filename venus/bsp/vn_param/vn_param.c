#include "vn_param.h"

#include <stddef.h>

#include "easyflash.h"
#include "vn_log.h"

void Vn_Param_Init(void)
{
    EfErrCode status = easyflash_init();

    if (status != EF_NO_ERR)
    {
        VN_LOG_E("EasyFlash KV system init failed: %d", status);
    }
}

int32_t Vn_Param_Read(const char *Key_ID, uint8_t *Val_Buf, uint16_t Buf_Len)
{
    size_t saved_len = 0U;
    size_t read_len;

    if ((Key_ID == NULL) || (Key_ID[0] == '\0') ||
        (Val_Buf == NULL) || (Buf_Len == 0U))
    {
        return 0;
    }

    read_len = ef_get_env_blob(Key_ID, Val_Buf, (size_t)Buf_Len, &saved_len);
    if (read_len == 0U)
    {
        VN_LOG_E("Param %s read failed", Key_ID);
    }

    VN_LOG_I("Param %s saved length: %lu", Key_ID, (unsigned long)saved_len);
    return (int32_t)read_len;
}

int32_t Vn_Param_Write(const char *Key_ID, const uint8_t *Val, uint16_t Val_len)
{
    EfErrCode status;

    if ((Key_ID == NULL) || (Key_ID[0] == '\0') || (Val == NULL))
    {
        return EF_ENV_NAME_ERR;
    }

    status = ef_set_env_blob(Key_ID, Val, (size_t)Val_len);
    if (status != EF_NO_ERR)
    {
        VN_LOG_E("Param %s write failed: %d", Key_ID, status);
    }

    return (int32_t)status;
}

int32_t Vn_param_delete(const char *Key_ID)
{
    EfErrCode status;

    if ((Key_ID == NULL) || (Key_ID[0] == '\0'))
    {
        return EF_ENV_NAME_ERR;
    }

    status = ef_del_env(Key_ID);
    if (status != EF_NO_ERR)
    {
        VN_LOG_E("Param %s delete failed: %d", Key_ID, status);
    }

    return (int32_t)status;
}
