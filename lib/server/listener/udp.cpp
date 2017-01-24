
#include "srpc/server/acceptor/async/udp.h"
#include "base.h"

namespace srpc { namespace server { namespace listener {

    namespace {
        namespace ba = SRPC_ASIO;
        typedef base::impl<server::acceptor::async::udp> impl;
    }

    namespace udp {
        interface::shared_type create( ba::io_service &ios,
                                       const std::string &addr,
                                       srpc::uint16_t port )
        {
            srpc::shared_ptr<impl> inst =
                    srpc::make_shared<impl>( ios, addr, port );

            inst->delegate_.lst_ = inst;
            inst->init( );

            return inst;
        }
    }

}}}
