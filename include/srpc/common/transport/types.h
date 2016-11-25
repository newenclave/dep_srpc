#ifndef SRPC_TRANSPORT_TYPES_H
#define SRPC_TRANSPORT_TYPES_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/system.h"

namespace srpc { namespace common { namespace transport {
    typedef SRPC_ASIO::io_service   io_service;
    typedef SRPC_SYSTEM::error_code error_code;
}}}

#endif // SRPC_TRANSPORT_TYPES_H
