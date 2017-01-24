
#include "srpc/server/acceptor/async/tcp.h"
#include "base.h"

namespace srpc { namespace server { namespace listener {

    namespace {
        namespace ba = SRPC_ASIO;
        typedef base::impl<server::acceptor::async::tcp> impl;
    }

    namespace tcp {
        interface::shared_type create( ba::io_service &ios,
                                       const std::string &addr,
                                       srpc::uint16_t port,
                                       bool /*tcp_nowait*/ )
        {
            srpc::shared_ptr<impl> inst =
                    srpc::make_shared<impl>( ios, addr, port );

            inst->delegate_.lst_ = inst;
            inst->init( );

            return inst;
        }
    }

}}}
