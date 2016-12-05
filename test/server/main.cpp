#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"
#include "srpc/server/acceptor/interface.h"
#include "srpc/server/acceptor/async/tcp.h"
#include "srpc/server/acceptor/async/udp.h"

#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"


#include <memory>
#include <queue>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include <unistd.h>
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
boost::asio::io_service ios;
boost::asio::io_service ios2;

void show_messages(  )
{
    auto start = ticks_now( );
    while( 1 ) {
        std::cout << messages << std::endl;
        messages = 0;
        sleep( 1 );
//        if( ticks_now( ) - start >= 1e7 ) {
//            ios.stop( );
//        }
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

template <typename SizePack>
using delegate_message = common::transport::delegates::message<SizePack>;


class mess_delegate: public delegate_message<common::sizepack::varint<std::uint16_t> > {

public:

    std::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    mess_delegate::pack_context ctx;

    void on_message( const char *message, size_t len )
    {
        //std::cout << "Message " << std::string(message, len) << "\n";
        ++messages;

        ctx.clear( );

        pack_begin( ctx, len );
        pack_update( ctx, message, len );
        pack_end( ctx );
        parent_->write( ctx.data( ).c_str( ), ctx.data( ).size( ) );
        parent_->read( );
    }

    bool validate_length( size_t len )
    {
        return true;
    }

    void on_error( const char *message )
    {
        std::cout << "on error: " << message << "\n";
    }

    void on_read_error( const bs::error_code &err )
    {
        std::cout << "on read error: " << err.message( ) << "\n";
    }

    void on_write_error( const bs::error_code &err)
    {
        std::cout << "on write error: " << err.message( ) << "\n";
    }

    void on_close( )
    {

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

using udp_acceptor = server::acceptor::async::udp;

struct acceptor_del: public acceptor::delegate {

    tcp_echo_delegate               delegate_;
    std::shared_ptr<tcp_acceptor>   acceptor_;

    void on_disconnect_client( common::transport::interface * )
    {

    }

    void on_accept_client( common::transport::interface *c,
                           const std::string& ip, std::uint16_t port )
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

    void on_accept_client( common::transport::interface *c,
                           const std::string& ip, std::uint16_t port )
    {
        std::cout << "New client!!!\n";
        gclients.insert( c->shared_from_this( ) );
        auto deleg = new mess_delegate;
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
        //ba::io_service::work wrk(ios);

        std::cout << getpid( ) << "\n";

        ba::io_service::work wrk0(gios[0]);
        ba::io_service::work wrk1(gios[1]);
        ba::io_service::work wrk2(gios[2]);
        ba::io_service::work wrk3(gios[3]);

        udp_transport::endpoint uep(ba::ip::address::from_string("0.0.0.0"), 2356);

        auto acc =  udp_acceptor::create( ios, 4096, uep );
        auto acc2 = udp_acceptor::create( ios2, 4096, uep );
        auto acc3 = udp_acceptor::create( gios[0], 4096, uep );


        udp_acceptor_del udeleg;
        udp_acceptor_del udeleg2;
        udp_acceptor_del udeleg3;

        acc->set_delegate( &udeleg );
        acc2->set_delegate( &udeleg );
        acc3->set_delegate( &udeleg );

        udeleg.acceptor_ = acc;
        udeleg2.acceptor_ = acc2;
        udeleg3.acceptor_ = acc3;

        acc->open( );
        acc2->open( );
        acc3->open( );

#if defined(SO_REUSEPORT) && (SO_REUSEPORT != 0)
        int opt = 1;
        std::cout << setsockopt( acc->native_handle( ),
                    SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) );
        std::cout << setsockopt( acc2->native_handle( ),
                    SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) );
        std::cout << setsockopt( acc3->native_handle( ),
                    SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) );
        std::cout << std::endl;
#endif


        acc->bind( );
        acc->start_accept( );

        acc2->bind( );
        acc2->start_accept( );

        acc3->bind( );
        acc3->start_accept( );

        std::thread( show_messages ).detach( );

//        std::thread([]( ){ gios[0].run( ); }).detach( );
//        std::thread([]( ){ gios[1].run( ); }).detach( );
//        std::thread([]( ){ gios[2].run( ); }).detach( );
//        std::thread([]( ){ gios[3].run( ); }).detach( );

        std::thread([]( ){ ios2.run( ); }).detach( );
        std::thread([]( ){ gios[0].run( ); }).detach( );
//        std::thread([]( ){ ios.run( ); }).detach( );
//        std::thread([]( ){ ios.run( ); }).detach( );

        ios.run( );

        return 0;

        using transtort_type      = tcp_transport;
        using transtort_delegate  = udp_echo_delegate;

        transtort_type::endpoint ep(ba::ip::address::from_string("0.0.0.0"), 2356);
        acceptor_del deleg;
        auto t = tcp_acceptor::create( ios, 4096, ep );
        t->set_delegate( &deleg );
        deleg.acceptor_ = t;
        t->open( );
        t->bind( true );
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
