#ifndef TRANSPORT_IFACE_H
#define TRANSPORT_IFACE_H

#include <memory>
#include <functional>

#include "boost/system/error_code.hpp"
#include "buffer.hpp"

namespace srpc { namespace common {

    struct transport_callbacks {

        typedef boost::system::error_code error_code;
        typedef std::function<void ( )> pre_call_type;
        typedef std::function<void (const error_code &)> post_call_type;

        static void default_pre_callback( ) { }
        static void default_post_callback(const error_code &) { }

        pre_call_type   pre_call;
        post_call_type  post_call;

        transport_callbacks( )
            :pre_call(&transport_callbacks::default_pre_callback)
            ,post_call(&transport_callbacks::default_post_callback)
        { }

        static
        transport_callbacks pre_post( pre_call_type pre, post_call_type pst )
        {
            transport_callbacks res;
            res.post_call = pst;
            res.pre_call  = pre;
            return res;
        }

        static
        transport_callbacks post( post_call_type call )
        {
            transport_callbacks res;
            res.post_call = call;
            return res;
        }

        static
        transport_callbacks pre( pre_call_type call )
        {
            transport_callbacks res;
            res.pre_call = call;
            return res;
        }
    };

    struct transport_iface:
            public std::enable_shared_from_this<transport_iface> {

        typedef std::shared_ptr<transport_iface> shared_type;
        typedef std::weak_ptr<transport_iface>   weak_type;

        struct delegate {
            virtual ~delegate( ) { }
            virtual void on_read_error( const boost::system::error_code & ) = 0;
            virtual void on_write_error( const boost::system::error_code &) = 0;
            virtual void on_data( const char *data, size_t len ) = 0;
            virtual void on_close( ) = 0;
        };

        virtual void open( ) = 0;
        virtual void close( ) = 0;
        virtual void write( const char *data, size_t len,
                            transport_callbacks cback ) = 0;

        virtual void read( ) = 0;
        virtual void set_delegate( delegate *val ) = 0;

        virtual ~transport_iface( ) { }
    };
}}

#endif // TRANSPORT_IFACE_H
