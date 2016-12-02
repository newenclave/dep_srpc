#ifndef SRPC_CONNECTOR_ASYNC_UDP_H
#define SRPC_CONNECTOR_ASYNC_UDP_H

#include "srpc/common/transport/async/udp.h"
#include "srpc/client/connector/async/impl/templ.h"

namespace srpc { namespace client { namespace connector {

    namespace async {
        typedef impl::templ<common::transport::async::udp> udp;
    }

}}}

#endif // SRPC_CONNECTOR_ASYNC_TCP_H
