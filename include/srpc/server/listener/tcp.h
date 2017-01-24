#ifndef SRPC_SERVER_LISTENER_TCP_H
#define SRPC_SERVER_LISTENER_TCP_H

#include "srpc/server/listener/interface.h"

namespace srpc { namespace server { namespace listener {

    namespace tcp {
        interface::shared_type create( const std::string &addr,
                                       srpc::uint16_t port, bool tcp_nowait );
    }

}}}

#endif // TCP_H
