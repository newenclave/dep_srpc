#ifndef SRPC_CONNECTOR_INTERFACE_H
#define SRPC_CONNECTOR_INTERFACE_H

#include "srpc/common/config/stdint.h"
#include "srpc/common/config/memory.h"
#include "srpc/common/config/system.h"

#include "srpc/common/transport/interface.h"

namespace srpc { namespace client { namespace connector {

    struct interface: public srpc::enable_shared_from_this<interface> {

        typedef SRPC_SYSTEM::error_code error_code;

        struct delegate {
            virtual ~delegate( ) { }
            virtual void on_connect( common::transport::interface * ) = 0;
            virtual void on_connect_error( const error_code &err ) = 0;
            virtual void on_close( ) = 0;
        };

        virtual void open( ) = 0;
        virtual void close( ) = 0;
        virtual void connect( ) = 0;
        virtual void set_delegate( delegate * ) = 0;
        virtual ~interface( ) { }
    };

}}}

#endif
