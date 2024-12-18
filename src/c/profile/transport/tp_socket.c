#include <uxr/client/core/communication/communication.h>
#include <profile/transport/tp_socket.h>


uxrCommunication* tp_socket_init_transport(
        tpSocketProtocol ip_protocol)
{
    #if 0
    bool rv = false;

    if (uxr_init_udp_platform(&transport->platform, ip_protocol, ip, port))
    {
        /* Setup interface. */
        transport->comm.instance = (void*)transport;
        transport->comm.send_msg = send_udp_msg;
        transport->comm.recv_msg = recv_udp_msg;
        transport->comm.comm_error = get_udp_error;
        transport->comm.mtu = UXR_CONFIG_UDP_TRANSPORT_MTU;
        UXR_INIT_LOCK(&transport->comm.mutex);
        rv = true;
    }
    return rv;
    #endif
    return 0;
}