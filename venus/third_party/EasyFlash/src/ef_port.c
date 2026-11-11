#include "easyflash.h"

#include <stdarg.h>
#include <stdio.h>

#include "ql_rtos.h"
#include "vn_log.h"
#include "vn_spi_flash.h"

static ql_sem_t Env_Sem;
static char default_value[] = "0";
static ef_env default_env_set[] = {
    {"iap_need_copy_app", default_value, 0},
};

EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size)
{
    *default_env = default_env_set;
    *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

    if (VN_SPI_Flash_Init() != VN_RET_OK)
    {
        return EF_ENV_INIT_FAILED;
    }
    if (Env_Sem == NULL && ql_rtos_semaphore_create(&Env_Sem, 1) != 0)
    {
        return EF_ENV_INIT_FAILED;
    }
    return EF_NO_ERR;
}

EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size)
{
    return VN_SPI_Flash_ReadData(addr, (uint8_t *)buf, size) == VN_RET_OK ? EF_NO_ERR : EF_READ_ERR;
}

EfErrCode ef_port_erase(uint32_t addr, size_t size)
{
    EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);
    return VN_SPI_Flash_EraseForWrite(addr, size) == VN_RET_OK ? EF_NO_ERR : EF_ERASE_ERR;
}

EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size)
{
    return VN_SPI_Flash_WriteData(addr, (uint8_t *)buf, size) == VN_RET_OK ? EF_NO_ERR : EF_WRITE_ERR;
}

void ef_port_env_lock(void)
{
    if (Env_Sem != NULL)
    {
        ql_rtos_semaphore_wait(Env_Sem, QL_WAIT_FOREVER);
    }
}

void ef_port_env_unlock(void)
{
    if (Env_Sem != NULL)
    {
        ql_rtos_semaphore_release(Env_Sem);
    }
}

static void Vn_EasyFlashLog(const char *format, va_list args)
{
    char buffer[256];

    vsnprintf(buffer, sizeof(buffer), format, args);
    Vn_Log_Printf("%s", buffer);
}

void ef_log_debug(const char *file, const long line, const char *format, ...)
{
    va_list args;

    (void)file;
    (void)line;
    va_start(args, format);
    Vn_EasyFlashLog(format, args);
    va_end(args);
}

void ef_log_info(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    Vn_EasyFlashLog(format, args);
    va_end(args);
}

void ef_print(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    Vn_EasyFlashLog(format, args);
    va_end(args);
}
