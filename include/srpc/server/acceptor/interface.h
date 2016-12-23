#ifndef SRPC_ACCEPTOR_INTERFACE_H
#define SRPC_ACCEPTOR_INTERFACE_H

#include <string>
#include "srpc/common/config/memory.h"
#include "srpc/common/config/stdint.h"
#include "srpc/common/config/system.h"
#include "srpc/common/transport/interface.h"

namespace srpc { namespace server { namespace acceptor {


    struct interface: public srpc::enable_shared_from_this<interface> {

        typedef SRPC_SYSTEM::error_code  error_code;

        virtual ~interface( ) { }

        struct delegate {
            virtual ~delegate( ) { }
            virtual void on_accept_client( common::transport::interface *,
                                           const std::string &,
                                           srpc::uint16_t  ) = 0;
            virtual void on_accept_error( const error_code & ) = 0;
            virtual void on_close( ) = 0;
        };

        virtual void open( )         = 0;
        virtual void close( )        = 0;
        virtual void start_accept( ) = 0;
        virtual srpc::handle_type native_handle( ) = 0;
        virtual void set_delegate( delegate * ) = 0;
    };

}}}

#endif // INTERFACE_H
