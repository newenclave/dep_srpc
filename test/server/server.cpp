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

#include "srpc/common/observers/simple.h"
#include "srpc/server/acceptor/async/tcp.h"
#include "srpc/server/acceptor/async/udp.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/cache/simple.h"

#include "srpc/common/protocol/binary.h"

#include "srpc/common/observers/define.h"

#include "protocol/t.pb.h"

using namespace srpc;
namespace gpb = google::protobuf;

using message_sptr = srpc::shared_ptr<gpb::Message>;

using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::protocol::binary<message_sptr,
                                    common::sizepack::fixint<srpc::uint16_t>,
                                    common::sizepack::fixint<srpc::uint32_t> >;

class protocol_client: public client_delegate {

    typedef typename client_delegate::tag_type           tag_type;
    typedef typename client_delegate::buffer_type        buffer_type;
    typedef typename client_delegate::const_buffer_slice const_buffer_slice;
    typedef typename client_delegate::buffer_slice       buffer_slice;

    typedef common::cache::simple<std::string>           cache_type;

    typedef SRPC_ASIO::io_service io_service;
    typedef common::transport::interface::write_callbacks cb_type;

public:

    protocol_client( io_service &ios )
        :client_delegate(100, 44000)
        ,ios_(ios)
        ,cache_(10)
    { }

    void append_message( buffer_type buf, const message_type &m )
    {
        m->AppendToString( buf.get( ) );
    }

    void on_message_ready( tag_type tag, buffer_type buff,
                           const_buffer_slice slice )
    {
        namespace ph = srpc::placeholders;

        srpc::shared_ptr<test::run> msg = srpc::make_shared<test::run>( );

        msg->ParseFromArray( slice.data( ), slice.size( ) );

        std::cout << msg->DebugString( )
                  << "^---- " << slice.size( )
                  << " Tag: " << tag << "\n";

        if( !buff ) {
            buff = cache_.get( );
        } else {
            buff->clear( );
        }

        buff->resize( 4 );

        buffer_slice sl = prepare_buffer( buff, 0, msg );
        sl = insert_size_prefix( buff, sl );

        cb_type::post_call_type callback =
                srpc::bind( &protocol_client::cache_back, this,
                             ph::_1, buff );

        get_transport( )->write( sl.begin( ), sl.size( ),
                                 cb_type::post( callback ) );
    }

    buffer_type unpack_message( const_buffer_slice &slice )
    {
        buffer_type r = cache_.get( );
        r->resize( slice.size( ) );
        for( size_t i=0; i<slice.size( ); i++ ) {
            (*r)[i] = slice.data( )[i] ^ 0xE4;
        }
        slice = const_buffer_slice(r->c_str( ), r->size( ));
        return r;
    }

    buffer_slice pack_message( buffer_type, buffer_slice slice )
    {
        char *p = slice.begin( );
        while( p != slice.end( ) ) {
            *p = *p ^ 0xE4;
            p++;
        }
        return slice;
    }

    void on_close( );
    void on_error( const char *mess )
    {
        std::cerr << "Proto error: " << mess << "\n";
        get_transport( )->close( );
    }

private:

    void cache_back( const error_code &err, buffer_type buf )
    {
        cache_.push( buf );
    }

    io_service &ios_;
    cache_type cache_;
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

using listener = lister<server::acceptor::async::udp>;
//using listener = lister<server::acceptor::async::tcp>;

int main( int argc, char *argv[ ] )
{
    try {

        srpc::uint16_t port = 23456;
        if( argc >= 2 ) {
            port = atoi(argv[1]);
        }

        listener::io_service ios;

        common::timers::periodical tt(ios);

//        tt.call( [&ios, &tt](...) {
//            ios.stop( );
//            tt.cancel( );
//        }, srpc::chrono::milliseconds(20000) );

        auto l = listener::create( ios, "0.0.0.0", port );

        l->subscribe_on_accept_error(
            [](const SRPC_SYSTEM::error_code &e)
            {

            } );

        l->subscribe_on_accept(
            [&ios](common::transport::interface *c,
               const std::string &addr, srpc::uint16_t svc)
            {
                std::cout << "New client " << addr << ":" << svc << "\n";

                protocol_client_sptr next =
                        srpc::make_shared<protocol_client>(srpc::ref(ios) );

                next->assign_transport( c );
                next->get_transport( )->set_delegate( next.get( ) );
                next->get_transport( )->read( );

                g_clients[next->get_transport( )] = next;
            } );

        l->start( );

        ios.run( );

        l->stop( );
        g_clients.clear( );

        google::protobuf::ShutdownProtobufLibrary( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error: " << ex.what( ) << "\n";
        return 1;
    }
    return 0;
}
