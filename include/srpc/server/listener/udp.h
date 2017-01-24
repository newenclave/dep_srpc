#ifndef SRPC_SERVER_LISTENER_UDP_H
#define SRPC_SERVER_LISTENER_UDP_H

#include "srpc/server/listener/interface.h"
#include "srpc/common/config/asio-forward.h"

SRPC_ASIO_FORWARD( class io_service; )

namespace srpc { namespace server { namespace listener {

    namespace udp {
        interface::shared_type create( SRPC_ASIO::io_service &ios,
                                       const std::string &addr,
                                       srpc::uint16_t port );
    }

}}}

#endif // TCP_H
