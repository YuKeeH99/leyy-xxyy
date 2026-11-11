#include "vn_net.h"

#include <stdio.h>
#include <string.h>
#include "inet6.h"
#include "netdb.h"
#include "ql_rtos.h"
#include "sockets.h"
#include "vn_log.h"

void Vn_DataCallCallback(int ProfileIdx, int Status)
{
    VN_LOG_D("Data call status changed, ProfileIdx=%d, Status=%d", ProfileIdx, Status);
}

int Vn_Cellular_RegisterNetwork(int Timeout)
{
    int ret = ql_network_register_wait((uint32_t)Timeout);

    if (ret != 0)
    {
        VN_LOG_E("Network register failed");
        return -1;
    }
    return ret;
}

int Vn_Cellular_StartDataCall(int ProfileIdx, int32_t Timeout)
{
    int ret;

    VN_LOG_D("ql_network_register_wait");
    ret = Vn_Cellular_RegisterNetwork(Timeout);
    if (ret != 0)
    {
        VN_LOG_E("Network register failed");
        return -1;
    }

    VN_LOG_D("ql_set_data_call_asyn_mode");
    (void)ql_set_data_call_asyn_mode(0, Vn_DataCallCallback);

    VN_LOG_D("ql_start_data_call");
    ret = ql_start_data_call(ProfileIdx, 0, NULL, NULL, NULL, 0);
    if (ret != 0)
    {
        VN_LOG_E("ql_start_data_call failed, ret=%d", ret);
        return -1;
    }
    return 0;
}

int Vn_Cellular_GetDataCallInfo(int ProfileIdx, Vn_DataCallInfo_t *Info)
{
    struct ql_data_call_info ql_info;
    uint32_t ip;
    int ret;

    if (Info == NULL)
    {
        VN_LOG_E("Data call info pointer is NULL");
        return -1;
    }

    memset(&ql_info, 0, sizeof(ql_info));
    ret = ql_get_data_call_info(ProfileIdx, 0, &ql_info);
    if (ret != 0)
    {
        VN_LOG_E("ql_get_data_call_info failed, ret=%d", ret);
        return -1;
    }

    Info->ProfileIdx = ql_info.profile_idx;
    Info->IpVersion = ql_info.ip_version;
    Info->State = ql_info.v4.state;
    Info->InIpAddrv4.s_addr = 0;
    Info->IpaddrV4[0] = '\0';

    if (ql_info.v4.state == 1)
    {
        Info->InIpAddrv4 = ql_info.v4.addr.ip;
        ip = Info->InIpAddrv4.s_addr;
        (void)snprintf(Info->IpaddrV4, sizeof(Info->IpaddrV4), "%u.%u.%u.%u",
                       (unsigned int)(ip & 0xffU),
                       (unsigned int)((ip >> 8) & 0xffU),
                       (unsigned int)((ip >> 16) & 0xffU),
                       (unsigned int)((ip >> 24) & 0xffU));
    }
    return 0;
}

uint8_t Vn_Extract_Domain_From_Url(const char *Url, char *Domain)
{
    const char *host;
    const char *path;
    size_t len;

    if ((Url == NULL) || (Domain == NULL))
    {
        return UINT8_MAX;
    }

    host = strstr(Url, "://");
    host = (host != NULL) ? host + 3 : Url;
    path = strchr(host, '/');
    len = (path != NULL) ? (size_t)(path - host) : strlen(host);

    strncpy(Domain, host, len);
    Domain[len] = '\0';
    return (uint8_t)len;
}

int Vn_Parse_DNS(char *Ip, const uint8_t *Domain)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct sockaddr_in *addr;
    int ret;

    if ((Ip == NULL) || (Domain == NULL))
    {
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    ret = getaddrinfo_with_pcid((const char *)Domain, NULL, &hints, &res, 4);
    if ((ret != 0) || (res == NULL) || (res->ai_addr == NULL))
    {
        VN_LOG_E("DNS parse failed");
        if (res != NULL)
        {
            freeaddrinfo(res);
        }
        return -1;
    }

    addr = (struct sockaddr_in *)res->ai_addr;
    if (inet_ntop(AF_INET, &addr->sin_addr, Ip, 16) == NULL)
    {
        freeaddrinfo(res);
        VN_LOG_E("DNS address conversion failed");
        return -1;
    }

    VN_LOG_I("DNS parse IP: %s", Ip);
    freeaddrinfo(res);
    return 0;
}

void Vn_ParseUrl(const char *Url, char *Protocol, char *Host, char *Path)
{
    const char *scheme;
    const char *host;
    const char *path;
    size_t len;

    if ((Url == NULL) || (Protocol == NULL) || (Host == NULL) || (Path == NULL))
    {
        return;
    }

    scheme = strstr(Url, "://");
    if (scheme == NULL)
    {
        strcpy(Host, Url);
        Protocol[0] = '\0';
        Path[0] = '\0';
        return;
    }

    len = (size_t)(scheme - Url);
    memcpy(Protocol, Url, len);
    Protocol[len] = '\0';

    host = scheme + 3;
    path = strchr(host, '/');
    if (path == NULL)
    {
        strcpy(Host, host);
        Path[0] = '\0';
        return;
    }

    len = (size_t)(path - host);
    memcpy(Host, host, len);
    Host[len] = '\0';
    strcpy(Path, path);
}
