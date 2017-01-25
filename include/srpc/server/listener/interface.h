#ifndef SRPC_SERVER_LISTENER_INTERFACE_H
#define SRPC_SERVER_LISTENER_INTERFACE_H

#include <string>
#include "srpc/common/config/memory.h"
#include "srpc/common/config/stdint.h"

#include "srpc/common/observers/define.h"
#include "srpc/common/observers/simple.h"

#include "srpc/server/acceptor/interface.h"
#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/types.h"

namespace srpc { namespace server { namespace listener {

    class interface: public srpc::enable_shared_from_this<interface> {

    public:

        typedef srpc::shared_ptr<interface>     shared_type;
        typedef common::transport::error_code   error_code;
        typedef common::transport::io_service   io_service;
        typedef common::transport::interface    transport_type;
        typedef server::acceptor::interface     acceptor_type;

        SRPC_OBSERVER_DEFINE( on_accept, void (transport_type *,
                                               const std::string &,
                                               srpc::uint16_t) );
        SRPC_OBSERVER_DEFINE( on_accept_error, void (const error_code &) );
        SRPC_OBSERVER_DEFINE( on_close, void ( ) );

    public:
        virtual void start( ) = 0;
        virtual void stop(  ) = 0;
        virtual acceptor_type *acceptor( ) = 0;
    };


}}}

#endif // SRPC_SERVER_LISTENER_INTERFACE_H
