#ifndef SRPC_SERVER_LISTENER_BASE_H
#define SRPC_SERVER_LISTENER_BASE_H

#include "srpc/common/config/asio.h"
#include "srpc/server/acceptor/interface.h"
#include "srpc/server/listener/interface.h"

namespace srpc { namespace server { namespace listener {

namespace base {

    template <typename AcceptorType>
    struct impl: public interface {

        typedef AcceptorType                        acceptor_type;
        typedef typename acceptor_type::endpoint    endpoint;
        typedef srpc::shared_ptr<acceptor_type>     acceptor_sptr;

        struct accept_delegate: public server::acceptor::interface::delegate {

            typedef impl<AcceptorType> parent_type;

            void on_accept_client( common::transport::interface *c,
                                   const std::string &addr, srpc::uint16_t svc )
            {

                srpc::shared_ptr<parent_type> lck(lst_.lock( ));
                if( lck ) {
                    lck->on_accept( c, addr, svc );
                    lck->acceptor_->start_accept( );
                }
            }

            void on_accept_error( const error_code &e )
            {
                srpc::shared_ptr<parent_type> lck(lst_.lock( ));
                if( lck ) {
                    lck->on_accept_error( e );
                }
            }

            void on_close( )
            {
                srpc::shared_ptr<parent_type> lck(lst_.lock( ));
                if( lck ) {
                    lck->on_close( );
                }
            }

            srpc::weak_ptr<parent_type> lst_;
        };

        //friend class accept_delegate;

        static endpoint epcreate( const std::string &addr, srpc::uint16_t port )
        {
            return endpoint( SRPC_ASIO::ip::address::from_string(addr), port );
        }

        impl( SRPC_ASIO::io_service &ios,
              const std::string &addr, srpc::uint16_t port )
            :ios_(ios)
            ,ep_(epcreate(addr, port))
        { }

        void init( )
        {
            acceptor_ = acceptor_type::create( ios_, 4096, ep_ );
            acceptor_->set_delegate( &delegate_ );
        }

        void start( )
        {
            acceptor_->open( );
            acceptor_->bind( );
            acceptor_->start_accept( );
        }

        void stop( )
        {
            acceptor_->close( );
        }

        acceptor_type *acceptor( )
        {
            return acceptor_.get( );
        }

        SRPC_ASIO::io_service  &ios_;
        endpoint                ep_;
        bool                    nowait_;
        acceptor_sptr           acceptor_;
        accept_delegate         delegate_;
    };

}
}}}


#endif // BASE_H
