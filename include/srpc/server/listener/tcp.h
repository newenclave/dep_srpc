#ifndef SRPC_SERVER_LISTENER_TCP_H
#define SRPC_SERVER_LISTENER_TCP_H

#include "srpc/server/listener/interface.h"
#include "srpc/common/config/asio-forward.h"

SRPC_ASIO_FORWARD( class io_service; )

namespace srpc { namespace server { namespace listener {

    namespace tcp {
        interface::shared_type create( const std::string &addr,
                                       srpc::uint16_t port, bool tcp_nowait );
    }

}}}

#endif // TCP_H
