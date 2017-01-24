#ifndef SRPC_ACCEPTOR_ASYNC_TCP_H
#define SRPC_ACCEPTOR_ASYNC_TCP_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/memory.h"
#include "srpc/common/config/system.h"

#include "srpc/common/transport/async/tcp.h"
#include "srpc/server/acceptor/interface.h"

namespace srpc { namespace server { namespace acceptor {

namespace async {

    class tcp: public interface {

        class client_type: public common::transport::async::tcp {

            typedef common::transport::async::tcp parent_type;

        public:

            typedef SRPC_ASIO::io_service io_service;

            client_type( io_service &ios, size_t bufsize )
                :parent_type(ios, bufsize)
            { }

            static
            srpc::shared_ptr<client_type> create( io_service &ios, size_t bs )
            {
                return srpc::make_shared<client_type>(srpc::ref(ios), bs);
            }
        };

        void handle_accept( const SRPC_SYSTEM::error_code &err,
                            srpc::shared_ptr<client_type> client,
                            srpc::weak_ptr<interface> inst)
        {
            srpc::shared_ptr<interface> lck(inst.lock( ));
            if( lck ) {
                if( !err ) {
                    common::transport::async::tcp::endpoint ep
                                = client->get_socket( ).remote_endpoint( );
                    delegate_->on_accept_client( client.get( ),
                                                 ep.address( ).to_string( ),
                                                 ep.port( ) );
                } else {
                    delegate_->on_accept_error( err );
                }
            }
        }

    protected:

        struct key { };

    public:

        typedef interface::delegate          delegate;
        typedef SRPC_ASIO::ip::tcp::endpoint endpoint;
        typedef SRPC_ASIO::io_service        io_service;
        typedef SRPC_ASIO::ip::tcp::acceptor io_acceptor_type;

        typedef io_acceptor_type::native_handle_type native_handle_type;

        tcp( io_service &ios, size_t bufsize, const endpoint &ep, key )
            :ios_(ios)
            ,acceptor_(ios)
            ,bufsize_(bufsize)
            ,delegate_(NULL)
            ,ep_(ep)
        { }

        static
        srpc::shared_ptr<tcp> create( io_service &ios, size_t bufsize,
                                      const endpoint &ep )
        {
            return srpc::make_shared<tcp>( srpc::ref(ios), bufsize,
                                           ep, key( ) );
        }

        srpc::weak_ptr<interface> weak_from_this( )
        {
            return srpc::weak_ptr<interface>( shared_from_this( ) );
        }

        void open( )
        {
            typedef SRPC_ASIO::ip::tcp asio_tcp;

            ep_.address( ).is_v6( )
                    ? acceptor_.open( asio_tcp::v6( ) )
                    : acceptor_.open( asio_tcp::v4( ) );
        }

        void bind( )
        {
            typedef SRPC_ASIO::socket_base socket_base;
            acceptor_.set_option( socket_base::reuse_address(true) );

            acceptor_.bind( ep_ );
            acceptor_.listen( 5 );
        }

        void close( )
        {
            delegate_->on_close( );
            acceptor_.close( );
        }

        void start_accept( )
        {
            namespace ph = srpc::placeholders;

            srpc::shared_ptr<client_type> next
                            = client_type::create( ios_, bufsize_ );

            acceptor_.async_accept( next->get_socket( ),
                    srpc::bind( &tcp::handle_accept, this,
                                 ph::_1, next, weak_from_this( ) ) );
        }

        void set_delegate( delegate *val )
        {
            delegate_ = val;
        }

        srpc::handle_type native_handle( )
        {
            return acceptor_.native_handle( );
        }

        io_acceptor_type &get_acceptor( )
        {
            return acceptor_;
        }

    private:

        io_service       &ios_;
        io_acceptor_type  acceptor_;
        size_t            bufsize_;
        delegate         *delegate_;
        endpoint          ep_;
    };
}

}}}


#endif // SRPC_ACCEPTOR_ASYNC_TCP_H
