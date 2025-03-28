#include <uxr/client/profile/transport/ip/udp/udp_transport_windows.h>
#include "udp_transport_internal.h"

#include <ws2tcpip.h>

bool uxr_init_udp_platform(
        uxrUDPPlatform* platform,
        uxrIpProtocol ip_protocol,
        const char* ip,
        const char* port)
{
#if 0
    bool rv = false;

    WSADATA wsa_data;
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        return false;
    }

    switch (ip_protocol)
    {
        case UXR_IPv4:
            platform->poll_fd.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            break;
        case UXR_IPv6:
            platform->poll_fd.fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
            break;
    }

    if (INVALID_SOCKET != platform->poll_fd.fd)
    {
        struct addrinfo hints;
        struct addrinfo* result;
        struct addrinfo* ptr;

        ZeroMemory(&hints, sizeof(hints));
        switch (ip_protocol)
        {
            case UXR_IPv4:
                hints.ai_family = AF_INET;
                break;
            case UXR_IPv6:
                hints.ai_family = AF_INET6;
                break;
        }
        hints.ai_socktype = SOCK_DGRAM;

        if (0 == getaddrinfo(ip, port, &hints, &result))
        {
            for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
            {
                if (0 == connect(platform->poll_fd.fd, ptr->ai_addr, (int)ptr->ai_addrlen))
                {
                    platform->poll_fd.events = POLLIN;
                    rv = true;
                    break;
                }
            }
            freeaddrinfo(result);
        }
    }
    return rv;
    #endif
}

bool uxr_close_udp_platform(
        uxrUDPPlatform* platform)
{
    #if 0
    bool rv = (INVALID_SOCKET == platform->poll_fd.fd) ? true : (0 == closesocket(platform->poll_fd.fd));
    return (0 == WSACleanup()) && rv;
#endif
}

size_t uxr_write_udp_data_platform(
        uxrUDPPlatform* platform,
        const uint8_t* buf,
        size_t len,
        uint8_t* errcode)
{
    #if 0
    size_t rv = 0;
    int bytes_sent = send(platform->poll_fd.fd, (const char*)buf, (int)len, 0);
    if (SOCKET_ERROR != bytes_sent)
    {
        rv = (size_t)bytes_sent;
        *errcode = 0;
    }
    else
    {
        *errcode = 1;
    }
    return rv;
    #endif
}

size_t uxr_read_udp_data_platform(
        uxrUDPPlatform* platform,
        uint8_t* buf,
        size_t len,
        int timeout,
        uint8_t* errcode)
{
    #if 0
    size_t rv = 0;
    int poll_rv = WSAPoll(&platform->poll_fd, 1, timeout);
    if (0 < poll_rv)
    {
        int bytes_received = recv(platform->poll_fd.fd, (char*)buf, (int)len, 0);
        if (SOCKET_ERROR != bytes_received)
        {
            rv = (size_t)bytes_received;
            *errcode = 0;
        }
        else
        {
            *errcode = 1;
        }
    }
    else
    {
        *errcode = (0 == poll_rv) ? 0 : 1;
    }
    return rv;
    #endif
}
