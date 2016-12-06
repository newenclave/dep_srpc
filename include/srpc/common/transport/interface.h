#ifndef TRANSPORT_IFACE_H
#define TRANSPORT_IFACE_H

#include "srpc/common/buffer.h"
#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/config/stdint.h"
#include "srpc/common/config/handle.h"
#include "srpc/common/config/system.h"

namespace srpc { namespace common { namespace transport {

    struct interface: public srpc::enable_shared_from_this<interface> {

        typedef srpc::shared_ptr<interface> shared_type;
        typedef srpc::weak_ptr<interface>   weak_type;

        typedef SRPC_SYSTEM::error_code  error_code;

        struct delegate {
            virtual void on_read_error( const error_code & ) = 0;
            virtual void on_write_error( const error_code &) = 0;
            virtual void on_data( const char *data, size_t len ) = 0;
            virtual void on_close( ) = 0;
            virtual ~delegate( ) { }
        };

        struct write_callbacks {

            typedef boost::system::error_code error_code;
            typedef srpc::function<void ( )> pre_call_type;
            typedef srpc::function<void (const error_code &,
                                         size_t)> post_call_type;

            static void empty_pre_callback( ) { }
            static void empty_post_callback(const error_code &, size_t) { }

            pre_call_type   pre_call;
            post_call_type  post_call;

            write_callbacks( )
                :pre_call(&write_callbacks::empty_pre_callback)
                ,post_call(&write_callbacks::empty_post_callback)
            { }

            static
            write_callbacks pre_post( pre_call_type pre, post_call_type pst )
            {
                write_callbacks res;
                res.post_call.swap( pst );
                res.pre_call.swap( pre );
                return res;
            }

            static
            write_callbacks post( post_call_type call )
            {
                write_callbacks res;
                res.post_call.swap( call );
                return res;
            }

            static
            write_callbacks pre( pre_call_type call )
            {
                write_callbacks res;
                res.pre_call.swap( call );
                return res;
            }
        };

        weak_type weak_from_this( )
        {
            return weak_type( shared_from_this( ) );
        }

        srpc::weak_ptr<interface const> weak_from_this( ) const
        {
            return srpc::weak_ptr<interface const>( shared_from_this( ));
        }


        virtual void open( )  = 0;
        virtual void close( ) = 0;

        virtual void write( const char *data, size_t len ) = 0;
        virtual void write( const char *data, size_t len,
                            write_callbacks cback ) = 0;

        virtual void read( ) = 0;
        virtual void set_delegate( delegate *val ) = 0;
        virtual srpc::handle_type native_handle( ) = 0;

        virtual ~interface( ) { }
    };

}}}

#endif // TRANSPORT_IFACE_H
