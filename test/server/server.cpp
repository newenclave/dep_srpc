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

using namespace srpc;
namespace gpb = google::protobuf;
namespace spb = srpc::common::protobuf;

using message_sptr = srpc::shared_ptr<gpb::Message>;

using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::protocol::noname<>;

using service_wrapper = spb::service;

srpc::atomic<srpc::uint64_t> g_counter(0);
srpc::atomic<srpc::uint64_t> g_counter_total(0);

class test_service: public test::test_service {
    void call6(::google::protobuf::RpcController* controller,
               const ::test::run* request,
               ::test::run* response,
               ::google::protobuf::Closure* done )
    {
        g_counter++;
        response->set_name( "!!!!?????" );
        //std::cout << "Call!\n";
        if(done) done->Run( );
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

template <typename Acceptor>
class lister: public srpc::enable_shared_from_this<lister<Acceptor> >
{
public:
    using error_code  = common::transport::error_code;
    using io_service  = common::transport::io_service;
private:

    SRPC_OBSERVER_DEFINE( on_accept,
                          void (common::transport::interface *,
                                const std::string &addr, srpc::uint16_t svc ) );
    SRPC_OBSERVER_DEFINE( on_accept_error, void (const error_code &) );
    SRPC_OBSERVER_DEFINE( on_close, void ( ) );

    struct accept_delegate: public server::acceptor::interface::delegate {

        typedef lister<Acceptor> parent_type;

        accept_delegate( srpc::shared_ptr<parent_type> lst )
            :lst_(lst)
        { }

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

    friend class accept_delegate;

    struct key { };

public:

    typedef Acceptor acceptor_type;
    typedef srpc::shared_ptr<Acceptor> acceptor_sptr;


    lister( io_service &ios, const std::string &addr, srpc::uint16_t svc, key )
    {
        typename acceptor_type::endpoint ep(
                    SRPC_ASIO::ip::address::from_string(addr), svc );

        acceptor_ = acceptor_type::create( ios, 45000, ep );
    }

    static
    srpc::shared_ptr<lister> create( io_service &ios,
                                     const std::string &addr,
                                     srpc::uint16_t svc )
    {
        srpc::shared_ptr<lister> inst
            = srpc::make_shared<lister>( srpc::ref(ios), addr, svc, key( ) );

        inst->delegate_.reset( new accept_delegate(inst) );
        inst->acceptor_->set_delegate( inst->delegate_.get( ) );
        return inst;
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

private:
    srpc::unique_ptr<accept_delegate> delegate_;
    acceptor_sptr                     acceptor_;
};

//using listener = lister<server::acceptor::async::udp>;
using factory  = common::factory<std::string, srpc::shared_ptr<spb::service> >;

using listener = lister<server::acceptor::async::tcp>;

int main( int argc, char *argv[ ] )
{
    try {

        srpc::uint16_t port = 23456;
        if( argc >= 2 ) {
            port = atoi(argv[1]);
        }

        std::uint64_t last_calls = 0;

        listener::io_service ios;

        common::timers::periodical tt(ios);

        tt.call( [&ios, &tt, &last_calls](...) {
            std::cerr << g_counter - last_calls << " ";
            g_counter_total += (g_counter - last_calls);
            last_calls = g_counter;
            if( g_counter_total >= 5000 ) {
                ios.stop( );
            }
            std::cout << "Total " << g_counter_total << "\n";
        }, srpc::chrono::milliseconds(1000) );

        auto l = listener::create( ios, "0.0.0.0", port );

        factory fac;
        fac.assign( "test", &create_svc );

        auto rrr = fac.create( "test" );

        l->subscribe_on_accept_error(
            [](const SRPC_SYSTEM::error_code &e)
            {

            } );

        l->subscribe_on_accept(
            [&ios](common::transport::interface *c,
               const std::string &addr, srpc::uint16_t svc)
            {
                std::cout << "New client " << addr << ":" << svc << "\n";

                protocol_client_sptr next
                        = protocol_client::create( srpc::ref(ios) );

                next->assign_transport( c );
                next->get_transport( )->set_delegate( next.get( ) );
                next->get_transport( )->read( );

                g_clients[next->get_transport( )] = next;
            } );

        l->start( );

        std::thread t1([&ios]( ) { ios.run( ); });
        std::thread t2([&ios]( ) { ios.run( ); });
        std::thread t3([&ios]( ) { ios.run( ); });
        std::thread t4([&ios]( ) { ios.run( ); });
        std::thread t5([&ios]( ) { ios.run( ); });

        ios.run( );

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
