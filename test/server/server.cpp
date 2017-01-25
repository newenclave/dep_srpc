#include <iostream>
#include <list>
#include <set>
#include <atomic>
#include <thread>

#include "srpc/common/config/memory.h"
#include "srpc/common/config/mutex.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"
#include "srpc/common/sizepack/zigzag.h"
#include "srpc/common/timers/once.h"
#include "srpc/common/timers/periodical.h"
#include "srpc/common/timers/ticks.h"
#include "srpc/common/factory.h"

#include "srpc/common/observers/simple.h"
#include "srpc/server/acceptor/async/tcp.h"
#include "srpc/server/acceptor/async/udp.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/cache/simple.h"

#include "srpc/common/protocol/noname.h"

#include "srpc/common/observers/define.h"
#include "srpc/common/protobuf/service.h"

#include "protocol/t.pb.h"

#include "srpc/server/listener/tcp.h"
#include "srpc/server/listener/udp.h"

using namespace srpc;
namespace gpb = google::protobuf;
namespace spb = srpc::common::protobuf;

using message_sptr = srpc::shared_ptr<gpb::Message>;

using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::protocol::noname<>;

using service_wrapper = spb::service;

namespace {
    SRPC_ASIO::io_service g_ios;
    srpc::atomic<srpc::uint64_t> g_counter(0);
    srpc::atomic<srpc::uint64_t> g_counter_total(0);
}

class test_service: public test::test_service {
    void call6(::google::protobuf::RpcController* controller,
               const ::test::run* request,
               ::test::run* response,
               ::google::protobuf::Closure* done )
    {
        g_counter++;
        response->set_name( "!!!!?????" );
        //std::cout << "Call!\n";
        g_ios.post( [done]( ) { if(done) done->Run( ); } );
    }
};

srpc::shared_ptr<spb::service> create_svc( )
{
    srpc::shared_ptr<test_service> svc = srpc::make_shared<test_service>( );
    return srpc::make_shared<spb::service>(svc);
}

class protocol_client: public client_delegate {

    typedef typename client_delegate::tag_type           tag_type;
    typedef typename client_delegate::buffer_type        buffer_type;
    typedef typename client_delegate::const_buffer_slice const_buffer_slice;
    typedef typename client_delegate::buffer_slice       buffer_slice;

    typedef common::cache::simple<std::string>           cache_type;

    typedef SRPC_ASIO::io_service io_service;
    typedef common::transport::interface::write_callbacks cb_type;

protected:
    struct key { };
public:

    protocol_client( io_service &ios, key )
        :client_delegate(true, 44000)
        ,ios_(ios)
        ,svc_(create_svc( ))
    { }

    static
    srpc::shared_ptr<protocol_client> create( io_service &ios )
    {
        srpc::shared_ptr<protocol_client> inst
            = srpc::make_shared<protocol_client>( srpc::ref(ios), key( ) );
        inst->track( inst );
        inst->init( );
        inst->set_ready( true );
        return inst;
    }

    void init( )
    {
        test::run rr;
        set_default_call( );
        send_message( rr );
    }

    void on_close( );

    virtual
    service_sptr get_service( const message_type & )
    {
        //std::cout << "request service!\n";
        return svc_;// create_svc( );
    }

    io_service &ios_;
    service_sptr svc_;

};

using protocol_client_sptr = srpc::shared_ptr<protocol_client>;
std::map<common::transport::interface *, protocol_client_sptr> g_clients;

void protocol_client::on_close( )
{
    std::cout << "Erase client " << get_transport( ) << std::endl;
    g_clients.erase( get_transport( ) );
}

namespace lister_ns = server::listener::tcp;

int main( int argc, char *argv[ ] )
{
    try {

        srpc::uint16_t port = 23456;
        if( argc >= 2 ) {
            port = atoi(argv[1]);
        }

        std::uint64_t last_calls = 0;

        common::timers::periodical tt(g_ios);

        tt.call( [&tt, &last_calls](...) {
            std::cerr << g_counter - last_calls << " ";
            g_counter_total += (g_counter - last_calls);
            last_calls = g_counter;
//            if( g_counter_total >= 5000 ) {
//                ios.stop( );
//            }
            std::cout << "Total " << g_counter_total << "\n";
        }, srpc::chrono::milliseconds(1000) );

        auto l = lister_ns::create( g_ios, "0.0.0.0", port, true );

        l->subscribe_on_accept_error(
            [](const SRPC_SYSTEM::error_code &e)
            {

            } );

        l->subscribe_on_accept(
            [ ](common::transport::interface *c,
               const std::string &addr, srpc::uint16_t svc)
            {
                std::cout << "New client " << addr << ":" << svc << "\n";

                protocol_client_sptr next
                        = protocol_client::create( srpc::ref(g_ios) );

                next->assign_transport( c );
                next->get_transport( )->set_delegate( next.get( ) );
                next->get_transport( )->read( );

                g_clients[next->get_transport( )] = next;
            } );

        l->start( );

        std::thread t1([ ]( ) { g_ios.run( ); });
        std::thread t2([ ]( ) { g_ios.run( ); });
        std::thread t3([ ]( ) { g_ios.run( ); });
        std::thread t4([ ]( ) { g_ios.run( ); });
        std::thread t5([ ]( ) { g_ios.run( ); });

        g_ios.run( );

        t1.join( );
        t2.join( );
        t3.join( );
        t4.join( );
        t5.join( );

        l->stop( );
        g_clients.clear( );

        google::protobuf::ShutdownProtobufLibrary( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error: " << ex.what( ) << "\n";
        return 1;
    }
    return 0;
}
