#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"
#include "srpc/server/acceptor/interface.h"
#include "srpc/server/acceptor/async/tcp.h"

#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"


#include <memory>
#include <queue>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include <stdlib.h>

std::atomic<std::uint64_t> messages {0};

boost::asio::io_service gios[4];

std::uint64_t ticks_now( )
{
    using std::chrono::duration_cast;
    using microsec = std::chrono::microseconds;
    auto n = std::chrono::high_resolution_clock::now( );
    return duration_cast<microsec>(n.time_since_epoch( )).count( );
}

boost::asio::io_service test_io;

void show_messages(  )
{
    while( 1 ) {
        std::cout << messages << std::endl;
        messages = 0;
        sleep( 1 );
    }
}


namespace ba = boost::asio;
namespace bs = boost::system;

using namespace srpc;

template <typename StreamType>
using common_transport = common::transport::async::base<StreamType>;

template <typename StreamType>
using stream_transport = common::transport::async::stream<StreamType>;

using tcp_transport = common::transport::async::tcp;
using udp_transport = common::transport::async::udp;

std::set< std::shared_ptr<common::transport::interface> > gclients;

struct tcp_echo_delegate final: public common::transport::interface::delegate {
    using cb = common::transport::interface::write_callbacks;

    std::shared_ptr<common::transport::interface> parent_;

    void on_read_error( const bs::error_code & )
    {
        std::cout << "Read error!\n";
        gclients.erase( parent_ );
        parent_->close( );
        parent_.reset( );
    }

    void on_write_error( const bs::error_code &)
    {

    }

    void on_data( const char *data, size_t len )
    {
        std::shared_ptr<std::string> echo
                = std::make_shared<std::string>( data, len );
        parent_->write( echo->c_str( ), echo->size( ),
                        cb::post([echo](...){ }) );
        start_read( );
    }

    void on_close( )
    {
        std::cout << "Close client\n";
    }

    void start_read( )
    {
        parent_->read( );
    }

};

struct udp_echo_delegate final: public common::transport::async::udp::delegate {

    std::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;

    void on_read_error( const bs::error_code & )
    {

    }

    void on_write_error( const bs::error_code &)
    {

    }

    void on_data( const char *data, size_t len )
    {
        ++messages;
        std::shared_ptr<std::string> echo
                = std::make_shared<std::string>( data, len );

        parent_->write( echo->c_str( ), echo->size( ),
                        cb::post([echo](...){ }) );
        start_read( );
    }

    void on_close( )
    {
        std::cout << "close\n";
    }

    void start_read( )
    {
        parent_->read( );
    }
};

using acceptor = server::acceptor::interface;
using tcp_acceptor = server::acceptor::async::tcp;

class udp_acceptor: public server::acceptor::interface {

    typedef ba::ip::udp::endpoint endpoint;


    class client_type: public common::transport::interface {

    public:

        client_type( udp_acceptor *parent, endpoint ep )
            :parent_(parent)
            //,dispatcher_(parent_->acceptor_->get_io_service( ))
            ,dispatcher_(gios[ep.port( ) % 4])
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

        srpc::weak_ptr<common::transport::interface> weak_from_this( )
        {
            typedef common::transport::interface iface;
            return srpc::weak_ptr<iface>(shared_from_this( ));
        }

        void set_read_impl( srpc::weak_ptr<common::transport::interface> inst )
        {

            srpc::shared_ptr<common::transport::interface> lck(inst.lock( ));
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
            dispatcher_.post(
                srpc::bind( &client_type::set_read_impl, this,
                                          weak_from_this( ) ) );
        }

        void set_delegate( delegate *val )
        {
            delegate_ = val;
        }

        void push_data_impl( srpc::weak_ptr<common::transport::interface> inst,
                             srpc::shared_ptr<std::string> data )
        {
            srpc::shared_ptr<common::transport::interface> lck(inst.lock( ));
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
            dispatcher_.post( srpc::bind( &client_type::push_data_impl, this,
                                           weak_from_this( ), data ) );
        }

        udp_acceptor            *parent_;
        ba::io_service  &dispatcher_;
        bool read_;
        std::deque<srpc::shared_ptr<std::string> >  read_queue_;
        endpoint                 ep_;
        delegate                *delegate_;
    };

    typedef std::map<endpoint, srpc::shared_ptr<client_type> > client_map;

    struct my_delegate: public common::transport::interface::delegate {

        udp_acceptor *parent_;

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
            endpoint &ep(parent_->acceptor_->get_endpoint( ));
            srpc::shared_ptr<std::string> dat
                    = srpc::make_shared<std::string>(data, len);

//            std::cout << "delegate_data "
//                      << ep.address( ).to_string( )
//                      << ":" << ep.port( )
//                      << std::endl;

            client_map::iterator f = parent_->clients_.find( ep );
            if( f != parent_->clients_.end( ) ) {
                auto c = f->second;
                parent_->acceptor_->get_io_service( ).post( [c, dat]( ) {
                    c->push_data( dat );
                } );
                //f->second->push_data( dat );
            } else {
                if( parent_->accept_ ) {
                    srpc::shared_ptr<client_type> next
                            = std::make_shared<client_type>(parent_, ep);
                    parent_->clients_[ep] = next;
                    next->read_queue_.push_back( dat );
                    parent_->delegate_->on_accept_client( next.get( ) );
                }
            }
            parent_->acceptor_->read_from( endpoint( ) );
        }

    };

    friend class client_type;
    friend class my_delegate;

    void write( const endpoint &ep,
                const char *data, size_t len )
    {
        acceptor_->write_to( ep, data, len );
    }

    void write( const endpoint &ep,
                const char *data, size_t len,
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
        }
    }

    void client_close( const endpoint &ep )
    {
        namespace ph = srpc::placeholders;
        acceptor_->get_dispatcher( ).post(
            srpc::bind( &udp_acceptor::close_impl, this,
            ep,
            srpc::weak_ptr<server::acceptor::interface>(shared_from_this( ) ) )
        );
    }

    typedef common::transport::async::udp parent_transport;
    srpc::shared_ptr<parent_transport> create_acceptor( ba::io_service &ios,
                                                        size_t bs )
    {
        srpc::shared_ptr<parent_transport> res
                = srpc::make_shared<parent_transport>( srpc::ref(ios), bs );

        return res;
    }

public:

    udp_acceptor( ba::io_service &ios, size_t bufsize )
        :delegate_(NULL)
    {
        accept_delegate_.parent_ = this;
        acceptor_ = create_acceptor( ios, bufsize );
        acceptor_->set_delegate( &accept_delegate_ );
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

private:

    delegate    *delegate_;
    my_delegate  accept_delegate_;
    srpc::shared_ptr<common::transport::async::udp> acceptor_;
    bool accept_;
    client_map  clients_;
};

struct acceptor_del: public acceptor::delegate {

    tcp_echo_delegate               delegate_;
    std::shared_ptr<tcp_acceptor>   acceptor_;

    void on_disconnect_client( common::transport::interface * )
    {

    }

    void on_accept_client( common::transport::interface *c )
    {
        gclients.insert( c->shared_from_this( ) );
        c->set_delegate( &delegate_ );
        delegate_.parent_ = c->shared_from_this( );
        c->read( );
        acceptor_->start_accept( );
    }

    void on_accept_error( const bs::error_code &ex )
    {
        std::cout << "Error accept " << ex.message( ) << "\n";
    }

    void on_close( )
    {
        std::cout << "Acceptor closed\n";
    }
};

struct udp_acceptor_del: public acceptor::delegate {

    udp_echo_delegate               delegate_;
    std::shared_ptr<udp_acceptor>   acceptor_;

    void on_disconnect_client( common::transport::interface * )
    {

    }

    void on_accept_client( common::transport::interface *c )
    {
        std::cout << "New client!!!\n";
        gclients.insert( c->shared_from_this( ) );
        udp_echo_delegate *deleg = new udp_echo_delegate;
        c->set_delegate( deleg );
        deleg->parent_ = c->shared_from_this( );
        c->read( );
        acceptor_->start_accept( );
    }

    void on_accept_error( const bs::error_code &ex )
    {
        std::cout << "Error accept " << ex.message( ) << "\n";
    }

    void on_close( )
    {
        std::cout << "Acceptor closed\n";
    }
};


int main( )
{
    try {
        ba::io_service ios;
        ba::io_service::work wrk0(gios[0]);
        ba::io_service::work wrk1(gios[1]);
        ba::io_service::work wrk2(gios[2]);
        ba::io_service::work wrk3(gios[3]);

        udp_transport::endpoint uep(ba::ip::address::from_string("0.0.0.0"), 2356);
        auto acc = srpc::make_shared<udp_acceptor>( srpc::ref(ios), 4096 );
        udp_acceptor_del udeleg;

        acc->set_delegate( &udeleg );
        udeleg.acceptor_ = acc;

        acc->bind(uep);
        acc->start_accept( );

        std::thread( show_messages ).detach( );

        std::thread([]( ){ gios[0].run( ); }).detach( );
        std::thread([]( ){ gios[1].run( ); }).detach( );
        std::thread([]( ){ gios[2].run( ); }).detach( );
        std::thread([]( ){ gios[3].run( ); }).detach( );

//        std::thread([&ios]( ){ ios.run( ); }).detach( );
//        std::thread([&ios]( ){ ios.run( ); }).detach( );
//        std::thread([&ios]( ){ ios.run( ); }).detach( );
//        std::thread([&ios]( ){ ios.run( ); }).detach( );

        ios.run( );

        return 0;

        using transtort_type      = tcp_transport;
        using transtort_delegate  = udp_echo_delegate;

        transtort_type::endpoint ep(ba::ip::address::from_string("0.0.0.0"), 2356);
        acceptor_del deleg;
        auto t = std::make_shared<tcp_acceptor>(std::ref(ios), 4096);
        t->set_delegate( &deleg );
        deleg.acceptor_ = t;
        t->open( );
        t->bind( ep, true );
        t->listen( 5 );
        t->start_accept( );
        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

//class client_type: public common::transport::interface {

//public:

//    client_type( udp_acceptor *parent, endpoint ep )
//        :parent_(parent)
//        ,dispatcher_(parent_->acceptor_->get_io_service( ))
//        //,dispatcher_(gios[ep.port( ) % 4])
//        ,read_(false)
//        ,ep_(ep)
//        ,delegate_(NULL)
//    {

//    }

//    void open( )
//    {

//    }

//    void close( )
//    {
//        parent_->client_close( ep_ );
//    }

//    void write( const char *data, size_t len )
//    {
//        parent_->write( ep_, data, len );
//    }

//    void write( const char *data, size_t len,
//                        write_callbacks cback )
//    {
//        parent_->write( ep_, data, len, cback );
//    }

//    srpc::weak_ptr<common::transport::interface> weak_from_this( )
//    {
//        typedef common::transport::interface iface;
//        return srpc::weak_ptr<iface>(shared_from_this( ));
//    }

//    void set_read_impl( srpc::weak_ptr<common::transport::interface> inst )
//    {

//        srpc::shared_ptr<common::transport::interface> lck(inst.lock( ));
//        if( lck ) {
//            if( !read_ ) {
//                read_ = true;
//                if( !read_queue_.empty( ) ) {
//                    std::string &data(*read_queue_.front( ));
//                    delegate_->on_data( data.c_str( ), data.size( ) );
//                    read_queue_.pop_front( );
//                    read_ = false;
//                }
//            }
//        }
//    }

//    void read( )
//    {
//        dispatcher_.post(
//            srpc::bind( &client_type::set_read_impl, this,
//                                      weak_from_this( ) ) );
//    }

//    void set_delegate( delegate *val )
//    {
//        delegate_ = val;
//    }

//    void push_data_impl( srpc::weak_ptr<common::transport::interface> inst,
//                         srpc::shared_ptr<std::string> data )
//    {
//        srpc::shared_ptr<common::transport::interface> lck(inst.lock( ));
//        if( lck ) {
//            if( read_ ) {
//                delegate_->on_data( data->c_str( ), data->size( ) );
//            } else {
//                read_queue_.push_back( data );
//            }
//        }
//    }

//    void push_data( srpc::shared_ptr<std::string> data )
//    {
//        dispatcher_.post( srpc::bind( &client_type::push_data_impl, this,
//                                       weak_from_this( ), data ) );
//    }

//    udp_acceptor            *parent_;
//    ba::io_service::strand  dispatcher_;
//    bool read_;
//    std::deque<srpc::shared_ptr<std::string> >  read_queue_;
//    endpoint                 ep_;
//    delegate                *delegate_;
//};
