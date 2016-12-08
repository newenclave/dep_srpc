#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/transport/delegates/stream.h"
#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"
#include "srpc/server/acceptor/interface.h"
#include "srpc/server/acceptor/async/tcp.h"
#include "srpc/server/acceptor/async/udp.h"
#include "srpc/common/queues/condition.h"

#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"
#include "srpc/common/sizepack/none.h"

#include <shared_mutex>

#include <memory>
#include <queue>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include <unistd.h>
#include <stdlib.h>

std::atomic<std::uint64_t> messages {0};
std::atomic<std::uint64_t> bytes {0};

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
        std::cout << messages << " ";
        std::cout << bytes << std::endl;
        messages = 0;
        sleep( 1 );
//        if( ticks_now( ) - start >= 1e7 ) {
//            ios.stop( );
//            return;
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

template <typename SizePack>
using delegate_message = common::transport::delegates::message<SizePack>;

template <typename SizePack>
using delegate_stream = common::transport::delegates::stream<SizePack>;

using size_pack_policy = common::sizepack::none;
//using size_pack_policy = common::sizepack::varint<std::uint16_t>;

class mess_delegate: public delegate_message<size_pack_policy> {

public:

    std::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    mess_delegate::pack_context ctx;

    void on_message( const char *message, size_t len )
    {
        //std::cout << "Message " << std::string(message, len) << "\n";
        ++messages;
        bytes += len;

        ctx.clear( );

        pack_begin( ctx, len );
        pack_update( ctx, message, len );
        pack_end( ctx );
        parent_->write( ctx.data( ).c_str( ), ctx.data( ).size( ) );
        parent_->read( );
    }

    bool validate_length( size_t len )
    {
        if( len >= 10000 ) {
            parent_->close( );
            return false;
        }
        return true;
    }

    void on_error( const char *message )
    {
        std::cout << "on error: " << message << "\n";
    }

    void on_read_error( const bs::error_code &err )
    {
        std::cout << "on read error: " << err.message( ) << "\n";
        parent_->close( );
    }

    void on_write_error( const bs::error_code &err)
    {
        std::cout << "on write error: " << err.message( ) << "\n";
    }

    void on_close( )
    {
        parent_.reset( );
        delete this;
    }

};


class stream_delegate: public delegate_stream<size_pack_policy> {

public:

    std::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    mess_delegate::pack_context ctx;

    void on_stream_begin( size_t len )
    {

    }

    void on_stream_update( const char *part, size_t len )
    {

    }

    void on_stream_end( )
    {

    }

    bool validate_length( size_t len )
    {
        return true;
    }

    void on_error( const char *message )
    {

    }

    void on_read_error( const bs::error_code & )
    {

    }

    void on_write_error( const bs::error_code &)
    {

    }

    void on_close( )
    {

    }


};

using acceptor = server::acceptor::interface;
using tcp_acceptor = server::acceptor::async::tcp;

using udp_acceptor = server::acceptor::async::udp;

struct udp_acceptor_del: public acceptor::delegate {

    std::shared_ptr<tcp_acceptor>   acceptor_;

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

struct data {
    int i;
    std::string str;
};

bool operator < (const data &l, const data &r)
{
    return l.i < r.i;
}

template <typename T>
using priority = common::queues::traits::priority<T>;
using pqueue   = common::queues::condition<size_t, data, priority<data> >;

int main( )
{
    pqueue q;

    auto slot = q.add_slot( 100 );

    q.push_to_slot( 100, data { 1, "q" } );
    q.push_to_slot( 100, data { 2, "w" } );
    q.push_to_slot( 100, data { 3, "e" } );
    q.push_to_slot( 100, data { 4, "r" } );

    data d;
    auto res = q.read_slot( 100, d, std::chrono::seconds(1) );

    std::cout << "res: " << d.i << " = "
              << d.str << " "
              << res << "\n";

    return 0;
    try {
        //ba::io_service::work wrk(ios);

        std::cout << getpid( ) << "\n";

        ba::io_service::work wrk0(gios[0]);
        ba::io_service::work wrk1(gios[1]);
        ba::io_service::work wrk2(gios[2]);
        ba::io_service::work wrk3(gios[3]);

        tcp_transport::endpoint uep(ba::ip::address::from_string("0.0.0.0"), 2356);

        auto acc =  tcp_acceptor::create( ios, 4096, uep );

        udp_acceptor_del udeleg;

        acc->set_delegate( &udeleg );
        udeleg.acceptor_ = acc;

        acc->open( );

#if 0 && defined(SO_REUSEPORT) && (SO_REUSEPORT != 0)
        int opt = 1;
        std::cout << setsockopt( acc->native_handle( ),
                    SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) );
        std::cout << setsockopt( acc2->native_handle( ),
                    SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) );
        std::cout << setsockopt( acc3->native_handle( ),
                    SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt) );
        std::cout << std::endl;
#endif

        acc->bind( true );

        acc->start_accept( );

        std::thread( show_messages ).detach( );

//        std::thread([]( ){ gios[0].run( ); }).detach( );
//        std::thread([]( ){ gios[1].run( ); }).detach( );
//        std::thread([]( ){ gios[2].run( ); }).detach( );
//        std::thread([]( ){ gios[3].run( ); }).detach( );

//        std::thread([]( ){ ios2.run( ); }).detach( );
//        std::thread([]( ){ gios[0].run( ); }).detach( );
//        std::thread([]( ){ ios.run( ); }).detach( );
//        std::thread([]( ){ ios.run( ); }).detach( );

        ios.run( );

        return 0;

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}
