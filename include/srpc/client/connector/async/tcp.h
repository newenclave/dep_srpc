#ifndef SRPC_CONNECTOR_ASYNC_TCP_H
#define SRPC_CONNECTOR_ASYNC_TCP_H

#include "srpc/common/transport/async/tcp.h"
#include "srpc/client/connector/async/impl/templ.h"

namespace srpc { namespace client { namespace connector {

    namespace async {
        typedef impl::templ<common::transport::async::tcp> tcp;
    }

}}}

#endif // SRPC_CONNECTOR_ASYNC_TCP_H
