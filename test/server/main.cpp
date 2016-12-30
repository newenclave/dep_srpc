#include <iostream>
#include "protocol/t.pb.h"

#include "boost/lexical_cast.hpp"

#include "srpc/common/config/chrono.h"

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

#include "srpc/common/timers/once.h"
#include "srpc/common/timers/periodical.h"
#include "srpc/common/timers/ticks.h"

#include "srpc/common/hash/crc32.h"

#include <memory>
#include <queue>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include <unistd.h>
#include <stdlib.h>

using namespace srpc;

#include "boost/thread/tss.hpp"

namespace ba = SRPC_ASIO;
namespace bs = SRPC_SYSTEM;

std::atomic<std::uint64_t> messages {0};
std::atomic<std::uint64_t> bytes {0};

SRPC_ASIO::io_service gios[4];

std::uint64_t ticks_now( )
{
    using tick = common::timers::ticks<chrono::milliseconds>;
    return tick::now( );
}

ba::io_service test_io;
ba::io_service ios;
ba::io_service ios2;

template <typename StreamType>
using common_transport = common::transport::async::base<StreamType>;

template <typename StreamType>
using stream_transport = common::transport::async::stream<StreamType>;

using tcp_transport = common::transport::async::tcp;
using udp_transport = common::transport::async::udp;

std::set< srpc::shared_ptr<common::transport::interface> > gclients;

template <typename SizePack>
using delegate_message = common::transport::delegates::message<SizePack>;

template <typename SizePack>
using delegate_stream = common::transport::delegates::stream<SizePack>;

///using size_pack_policy = common::sizepack::none;
using size_pack_policy = common::sizepack::varint<std::uint16_t>;

class mess_delegate: public delegate_message<size_pack_policy> {

public:

    srpc::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    //mess_delegate::pack_context ctx;
    std::uint64_t last_tick_ = ticks_now( );
    common::timers::periodical timer_;

    mess_delegate(SRPC_ASIO::io_service &ios)
        :timer_(ios)
    {
        timer_.call( [this]( ... ) {
            auto now = ticks_now( );
            if( now - last_tick_ > 10000 ) {
                std::cout << "Client expired\n";
                parent_->close( );
                timer_.cancel( );
            }
        }, chrono::seconds(1) );
    }

    void on_need_read( )
    {
        parent_->read( );
    }

    void on_message( const char *message, size_t len )
    {
        //std::cout << "Message " << std::string(message, len) << "\n";
//        ++messages;
//        bytes += len;
//        last_tick_ = ticks_now( );

//        ctx.clear( );

//        last_tick_ = ticks_now( );

//        pack_begin( ctx, len );
//        pack_update( ctx, message, len );
//        pack_end( ctx );
//        parent_->write( ctx.data( ).c_str( ), ctx.data( ).size( ) );
    }

    bool validate_length( size_t len )
    {
        if( len >= 100000 ) {
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
        std::cout << "on close\n";
        parent_.reset( );
        delete this;
    }

};

class stream_delegate: public delegate_stream<size_pack_policy> {

public:

    std::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    //stream_delegate::pack_context ctx;
    size_t len_;

    void on_stream_begin( size_t len )
    {
        len_ = len;
        //std::cout << "stream begin " << len << "\n";
    }

    void on_stream_update( const char *part, size_t len )
    {
        //std::cout << "stream update " << std::string(part, len) << "\n";
//        auto ctx = std::make_shared<stream_delegate::pack_context>( );
//        if( len_ > 0) {
//            pack_length( *ctx, len_ );
//            len_ = 0;
//            ctx->data( ).append( part, part + len );
//        } else {
//            ctx->data( ).assign( part, part + len );
//        }
//        parent_->write( ctx->data( ).c_str( ), ctx->data( ).size( ),
//                        cb::post( [ctx]( ... ){  } ));
    }

    void on_stream_end( )
    {
        messages++;
        //std::cout << "stream end "<< "\n";
    }

    void on_need_read( )
    {
        //std::cout << "new read "<< "\n";
        parent_->read( );
    }

    bool validate_length( size_t len )
    {
        return true;
    }

    void on_error( const char *message )
    {
        std::cerr << "Stream Error: " << message << "\n";
        parent_->close( );
    }

    void on_read_error( const bs::error_code &err )
    {
        std::cout << "on read error: " << err.message( ) << "\n";
        parent_->close( );
    }

    void on_write_error( const bs::error_code &err )
    {
        std::cout << "on write error: " << err.message( ) << "\n";
        parent_->close( );
    }

    void on_close( )
    {
        delete this;
    }

};

using acceptor = server::acceptor::interface;
using tcp_acceptor = server::acceptor::async::tcp;

using udp_acceptor = server::acceptor::async::udp;

struct udp_acceptor_del: public acceptor::delegate {

    srpc::shared_ptr<udp_acceptor>   acceptor_;

    void on_disconnect_client( common::transport::interface * )
    {

    }

    void on_accept_client( common::transport::interface *c,
                           const std::string& ip, std::uint16_t port )
    {
        std::cout << "New client!!!\n";
        gclients.insert( c->shared_from_this( ) );
        auto deleg = new mess_delegate( acceptor_->get_io_service( ) );//stream_delegate;
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

using squeue = common::queues::condition<size_t, data>;


using period_timer  = common::timers::periodical;
using once_timer    = common::timers::once;

int main_( )
{
    pqueue q;

    auto slot = q.add_slot( 100 );

    q.push_to_slot( 100, data { 1, "q" } );
    q.push_to_slot( 100, data { 2, "w" } );
    q.push_to_slot( 100, data { 3, "e" } );
    q.push_to_slot( 100, data { 4, "r" } );

    data d;
    auto res = slot->read_for( d, srpc::chrono::seconds(1) );

    period_timer dt(ios);
    dt.call( [ ](...) {
        std::cout << messages << " ";
        std::cout << bytes << std::endl;
        messages = 0;
    }, chrono::seconds(1) );

    std::cout << "res: " << d.i << " = "
              << d.str << " "
              << res << "\n";

//    /return 0;
    try {
        //ba::io_service::work wrk(ios);

        std::cout << getpid( ) << "\n";

        ba::io_service::work wrk0(gios[0]);
        ba::io_service::work wrk1(gios[1]);
        ba::io_service::work wrk2(gios[2]);
        ba::io_service::work wrk3(gios[3]);

        udp_transport::endpoint uep(ba::ip::address::from_string("0.0.0.0"), 12347);

        auto acc = udp_acceptor::create( ios, 4096, uep );

        udp_acceptor_del udeleg;

        acc->set_delegate( &udeleg );
        udeleg.acceptor_ = acc;

        acc->open( );

        acc->resize_buffer( 12000 );

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

        acc->bind( );

        acc->start_accept( );

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
