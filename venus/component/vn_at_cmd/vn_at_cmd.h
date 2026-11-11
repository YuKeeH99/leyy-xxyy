#ifndef __VN_AT_CMD_H__
#define __VN_AT_CMD_H__

#include <stddef.h>

#include "ql_at_srv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VN_AT_CMD_COUNT 7U

extern utlAtCommand_T Vn_At_Cmds[VN_AT_CMD_COUNT];

void ql_at_srv_set_app_shell_cmds(utlAtCommand_T *shell_cmds, int num);

utlReturnCode_T Vn_AT_OTA_Test_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

utlReturnCode_T Vn_AT_VOLUME_Test_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

utlReturnCode_T Vn_AT_AUDIO_Test_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

utlReturnCode_T Vn_AT_WifiSsidDel_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

utlReturnCode_T Vn_AT_WifiOta_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

utlReturnCode_T Vn_AT_WifiEnable_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

utlReturnCode_T Vn_AT_WifiVer_Cmd(
    const utlAtParameterOp_T op, const char *command_name_p,
    const utlAtParameterValue_P2c parameter_values_p,
    const size_t num_parameters, const char *info_text_p,
    unsigned int *xid_p, void *arg_p);

#ifdef __cplusplus
}
#endif

#endif /* __VN_AT_CMD_H__ */
