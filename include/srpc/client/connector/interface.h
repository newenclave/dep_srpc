#ifndef SRPC_CONNECTOR_INTERFACE_H
#define SRPC_CONNECTOR_INTERFACE_H

#include "srpc/common/config/stdint.h"
#include "srpc/common/config/memory.h"

namespace srpc { namespace client { namespace connector {

    struct interface: public srpc::enable_shared_from_this<interface> {
        struct delegate {
            virtual ~delegate( ) { }
            virtual void on_connect( common::transport::interface * ) = 0;
            virtual void on_connect_error( const bs::error_code &err ) = 0;
            virtual void on_close( ) = 0;
        };
        virtual void open( ) = 0;
        virtual void close( ) = 0;
        virtual void connect( ) = 0;
        virtual void set_delegate( delegate * ) = 0;
        virtual ~interface( ) { }
    };

}}}
