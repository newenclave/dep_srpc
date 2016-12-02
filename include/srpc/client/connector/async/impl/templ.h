#ifndef SRPC_CONNECTOR_ASYNC_TEMPL_H
#define SRPC_CONNECTOR_ASYNC_TEMPL_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/memory.h"
#include "srpc/common/config/system.h"
#include "srpc/common/config/functional.h"

#include "srpc/client/connector/interface.h"

namespace srpc { namespace client { namespace connector {

namespace async { namespace impl {

    template<typename TransportType>
    class templ: public interface {

        typedef TransportType transport_type;

        class client_type: public transport_type {

            typedef TransportType parent_type;

        public:

            client_type( SRPC_ASIO::io_service &ios, size_t bs )
                :transport_type(ios, bs)
            { }

            static
            srpc::shared_ptr<client_type> create( SRPC_ASIO::io_service &ios,
                                                  size_t bs )
            {
                return srpc::make_shared<client_type>( srpc::ref(ios), bs );
            }
        };

        void handle_connect( const SRPC_SYSTEM::error_code &error,
                             srpc::weak_ptr<interface> inst )
        {
            srpc::shared_ptr<interface> lck(inst.lock( ));
            if( lck ) {
                if( !error ) {
                    delegate_->on_connect( client_.get( ) );
                } else {
                    delegate_->on_connect_error( error );
                }
            }
        }

    public:

        typedef typename transport_type::endpoint           endpoint;
        typedef typename transport_type::native_handle_type native_handle_type;
        typedef SRPC_ASIO::io_service                       io_service;

        templ( io_service &ios, size_t buflen, const endpoint &ep )
            :ios_(ios)
            ,buflen_(buflen)
            ,client_(client_type::create( ios_, buflen_ ))
            ,ep_(ep)
            ,delegate_(NULL)
        { }

        void open( )
        { }

        void close( )
        {
            if( client_ ) {
                delegate_->on_close( );
                client_->close( );
            }
        }

        srpc::weak_ptr<interface> weak_from_this( )
        {
            return srpc::weak_ptr<interface>(shared_from_this( ));
        }

        srpc::weak_ptr<interface const> weak_from_this( ) const
        {
            return srpc::weak_ptr<interface const>(shared_from_this( ));
        }

        void connect( )
        {
            namespace ph = srpc::placeholders;
            {
                srpc::shared_ptr<client_type> tmp =
                                 client_type::create( ios_, buflen_ );
                tmp->set_endpoint( ep_ );
                tmp->open( );
                client_.swap( tmp );
            }
            client_->get_socket( ).async_connect( ep_,
                srpc::bind( &templ::handle_connect, this,
                             ph::_1, weak_from_this( ) ) );
        }

        void set_delegate( delegate *val )
        {
            delegate_ = val;
        }

        void set_endpoint( const endpoint &ep )
        {
            ep_ = ep;
        }

        endpoint &get_endpoint( )
        {
            return ep_;
        }

        const endpoint &get_endpoint( ) const
        {
            return ep_;
        }

        native_handle_type native_handle( )
        {
            return client_->native_handle( );
        }

    private:

        io_service         &ios_;
        size_t              buflen_;
        srpc::shared_ptr<client_type> client_;
        endpoint            ep_;
        delegate           *delegate_;
    };

}}

}}}

#endif // SRPC_CONNECTOR_ASYNC_TCP_H
