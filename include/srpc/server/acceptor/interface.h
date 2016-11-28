#ifndef SEPC_ACCEPTOR_INTERFACE_H
#define SEPC_ACCEPTOR_INTERFACE_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/system.h"
#include "srpc/common/transport/interface.h"

namespace srpc { namespace server { namespace acceptor {


    struct interface: public srpc::enable_shared_from_this<interface> {

        typedef SRPC_SYSTEM::error_code  error_code;

        struct delegate {
            virtual void on_accept_client( common::transport::interface * ) = 0;
            //virtual void on_close_client( common::transport::interface * ) = 0;
            virtual void on_accept_error( const error_code & ) = 0;
            virtual void on_close( ) = 0;
        };
        virtual void open( ) = 0;
        virtual void close( ) = 0;
        virtual void start_accept( ) = 0;
        virtual void set_delegate( delegate * ) = 0;
    };

}}}

#endif // INTERFACE_H
