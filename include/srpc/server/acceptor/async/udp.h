#ifndef SRPC_ACCEPTOR_ASYNC_UDP_H
#define SRPC_ACCEPTOR_ASYNC_UDP_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/memory.h"
#include "srpc/common/config/system.h"

#include "srpc/common/transport/async/udp.h"
#include "srpc/server/acceptor/interface.h"

namespace srpc { namespace server { namespace acceptor {

namespace async {

    class udp: public server::acceptor::interface {

        typedef SRPC_ASIO::ip::udp::endpoint    endpoint;
        typedef common::transport::async::udp   parent_transport;
        typedef server::acceptor::interface     parent_acceptor;
        typedef SRPC_ASIO::io_service           io_service;

        class client_type: public common::transport::interface {

            typedef std::deque<srpc::shared_ptr<std::string> > queue_type;
            typedef common::transport::interface parent_type;

        public:

            client_type( udp *parent, endpoint ep )
                :parent_(parent)
                ,dispatcher_(parent_->acceptor_->get_io_service( ))
                ,read_(false)
                ,ep_(ep)
                ,delegate_(NULL)
            {

            }

            void open( )
            {

            }

            void close( )
            {
                parent_->client_close( ep_ );
            }

            void write( const char *data, size_t len )
            {
                parent_->write( ep_, data, len );
            }

            void write( const char *data, size_t len,
                                write_callbacks cback )
            {
                parent_->write( ep_, data, len, cback );
            }

            srpc::weak_ptr<parent_type> weak_from_this( )
            {
                return srpc::weak_ptr<parent_type>(shared_from_this( ));
            }

            void set_read_impl( srpc::weak_ptr<parent_type> inst )
            {
                srpc::shared_ptr<parent_type> lck(inst.lock( ));
                if( lck ) {
                    if( !read_ ) {
                        read_ = true;
                        if( !read_queue_.empty( ) ) {
                            std::string &data(*read_queue_.front( ));
                            delegate_->on_data( data.c_str( ), data.size( ) );
                            read_queue_.pop_front( );
                            read_ = false;
                        }
                    }
                }
            }

            void read( )
            {
                parent_->acceptor_->get_dispatcher( ).post(
                    srpc::bind( &client_type::set_read_impl, this,
                                              weak_from_this( ) ) );
            }

            void set_delegate( delegate *val )
            {
                delegate_ = val;
            }

            void push_data_impl( srpc::weak_ptr<parent_type> inst,
                                 srpc::shared_ptr<std::string> data )
            {
                srpc::shared_ptr<parent_type> lck(inst.lock( ));
                if( lck ) {
                    if( read_ ) {
                        delegate_->on_data( data->c_str( ), data->size( ) );
                    } else {
                        read_queue_.push_back( data );
                    }
                }
            }

            void push_data( srpc::shared_ptr<std::string> data )
            {
                parent_->acceptor_->get_dispatcher( ).post(
                            srpc::bind( &client_type::push_data_impl, this,
                                         weak_from_this( ), data ) );
            }

            udp                            *parent_;
            SRPC_ASIO::io_service::strand   dispatcher_;
            bool                            read_;
            queue_type                      read_queue_;
            endpoint                        ep_;
            delegate                       *delegate_;

        };

        typedef std::map<endpoint, srpc::shared_ptr<client_type> > client_map;

        struct accept_delegate: public common::transport::interface::delegate {

            udp *parent_;

            void on_read_error( const error_code &err )
            {
                endpoint &ep(parent_->acceptor_->get_endpoint( ));
                client_map::iterator f = parent_->clients_.find( ep );
                if( f != parent_->clients_.end( ) ) {
                    f->second->delegate_->on_write_error( err );
                }
            }

            void on_close( )
            {
                endpoint &ep(parent_->acceptor_->get_endpoint( ));
                client_map::iterator f = parent_->clients_.find( ep );
                if( f != parent_->clients_.end( ) ) {
                    f->second->delegate_->on_close( );
                }
            }

            void on_write_error( const error_code &err )
            {
                endpoint &ep(parent_->acceptor_->get_endpoint( ));
                client_map::iterator f = parent_->clients_.find( ep );
                if( f != parent_->clients_.end( ) ) {
                    f->second->delegate_->on_read_error( err );
                }
            }

            /// dispatcher
            void on_data( const char *data, size_t len )
            {
                srpc::shared_ptr<parent_transport> acc = parent_->acceptor_;

                endpoint &ep(acc->get_endpoint( ));

                srpc::shared_ptr<std::string> dat
                        = srpc::make_shared<std::string>(data, len);

                client_map::iterator f = parent_->clients_.find( ep );

                if( f != parent_->clients_.end( ) ) {
                    f->second->push_data( dat );
                } else {
                    if( parent_->accept_ ) {
                        srpc::shared_ptr<client_type> next
                                = std::make_shared<client_type>(parent_, ep);
                        parent_->clients_[ep] = next;
                        next->read_queue_.push_back( dat );
                        parent_->delegate_->on_accept_client( next.get( ),
                                    ep.address( ).to_string( ),
                                    ep.port( ) );
                    }
                }

                acc->read_from( endpoint( ) );
            }

        };

        friend class client_type;
        friend class accept_delegate;

        void write( const endpoint &ep, const char *data, size_t len )
        {
            acceptor_->write_to( ep, data, len );
        }

        void write( const endpoint &ep, const char *data, size_t len,
                    common::transport::interface::write_callbacks cback )
        {
            acceptor_->write_to( ep, data, len, cback );
        }

        void close_impl( const endpoint &ep,
                         srpc::weak_ptr<server::acceptor::interface> inst )
        {
            srpc::shared_ptr<server::acceptor::interface> lck(inst.lock( ));
            if( lck ) {
                typedef client_map::iterator itr;
                itr f = clients_.find( ep );
                if( f != clients_.end( ) ) {
                    f->second->delegate_->on_close( );
                    clients_.erase( f );
                }
                delegate_->on_close( );
                acceptor_->close( );
            }
        }

        void client_close( const endpoint &ep )
        {
            namespace ph = srpc::placeholders;
            acceptor_->get_dispatcher( ).post(
                        srpc::bind( &udp::close_impl, this, ep,
                        srpc::weak_ptr<parent_acceptor>(shared_from_this( ) ) )
            );
        }

        srpc::shared_ptr<parent_transport> create_acceptor( io_service &ios,
                                                            size_t bs )
        {
            srpc::shared_ptr<parent_transport> res
                    = srpc::make_shared<parent_transport>( srpc::ref(ios), bs );

            return res;
        }

        struct key { };

    public:

        typedef parent_transport::native_handle_type native_handle_type;

        udp( io_service &ios, size_t bufsize, key & )
            :delegate_(NULL)
        {
            accept_delegate_.parent_ = this;
            acceptor_ = create_acceptor( ios, bufsize );
            acceptor_->set_delegate( &accept_delegate_ );
        }

        static
        srpc::shared_ptr<udp> create( io_service &ios, size_t bufsize )
        {   key k;
            return srpc::make_shared<udp>( srpc::ref(ios),
                                           bufsize, std::ref( k ) );
        }

        void open( )
        { }

        void bind( const endpoint &ep, bool reuse = true )
        {
            ep.address( ).is_v6( )
                ? acceptor_->get_socket( ).open( SRPC_ASIO::ip::udp::v6( ) )
                : acceptor_->get_socket( ).open( SRPC_ASIO::ip::udp::v4( ) );

            if( reuse ) {
                SRPC_ASIO::socket_base::reuse_address opt(true);
                acceptor_->get_socket( ).set_option( opt );
            }

            acceptor_->get_socket( ).bind( ep );
            acceptor_->read_from( endpoint( ) );
        }

        void close( )
        {
            acceptor_->close( );
        }

        void start_accept( )
        {
            accept_ = true;
        }

        void set_delegate( delegate *val )
        {
            delegate_ = val;
        }

        native_handle_type native_handle( )
        {
            return acceptor_->native_handle( );
        }

    private:

        delegate                          *delegate_;
        accept_delegate                    accept_delegate_;
        srpc::shared_ptr<parent_transport> acceptor_;
        bool                               accept_;
        client_map                         clients_;
    };


}

}}}
#endif // SRPC_ACCEPTOR_ASYNC_UDP_H
