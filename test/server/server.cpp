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
                                    common::sizepack::none >;

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
        srpc::shared_ptr<test::run> msg = srpc::make_shared<test::run>( );

        msg->ParseFromArray( slice.data( ), slice.size( ) );

        std::cout << msg->DebugString( )
                  << "^---- " << slice.size( )
                  << " Tag: " << tag << "\n";

        if( !buff ) {
            buff = get_str( );
        } else {
            buff->clear( );
        }

        buff->resize( 4 );

        buffer_slice sl = prepare_buffer( buff, 0, msg );
        sl = insert_size_prefix( buff, sl );

        get_transport( )->write( sl.begin( ), sl.size( ),
            cb_type::post([this, buff](...) {
                cache_.push( buff );
            } ) );
    }

    buffer_type unpack_message( const_buffer_slice &slice )
    {
        buffer_type r = get_str( );
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

    buffer_type get_str( )
    {
        return cache_.get( );
    }

private:
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


struct listener {

    using size_policy = common::sizepack::varint<size_t>;
    using error_code  = common::transport::error_code;
    using io_service  = common::transport::io_service;

private:

    using tcp = server::acceptor::async::tcp;
    using udp = server::acceptor::async::udp;

    using acceptor_type = udp;

    struct message_delegate;
    struct impl;
    struct accept_delegate;

    using iface_ptr     = common::transport::interface *;
    using client_sptr   = srpc::shared_ptr<common::transport::interface>;
    using acceptor_sptr = srpc::shared_ptr<server::acceptor::interface>;
    using impl_sptr     = srpc::shared_ptr<impl>;

    using message_delegate_sptr = srpc::shared_ptr<message_delegate>;

    struct impl: public srpc::enable_shared_from_this<impl> {

        srpc::unique_ptr<accept_delegate>   deleg_;
        acceptor_sptr                       acceptor_;
        io_service                         &ios_;
        impl( io_service &ios )
            :ios_(ios)
        { }
    };

    typedef common::transport::delegates::message<size_policy> client_delegate;

    struct accept_delegate: public server::acceptor::interface::delegate {

        accept_delegate( srpc::shared_ptr<impl> lst )
            :lst_(lst)
        { }

        void on_accept_client( common::transport::interface *c,
                               const std::string &addr, srpc::uint16_t svc )
        {

            srpc::shared_ptr<impl> lck(lst_.lock( ));

            if( lck ) {

                std::cout << "New client " << addr << ":" << svc << "\n";

                protocol_client_sptr next =
                        srpc::make_shared<protocol_client>( lck->ios_ );

                next->assign_transport( c );

                next->get_transport( )->set_delegate( next.get( ) );
                next->get_transport( )->read( );

                g_clients[next->get_transport( )] = next;
                //lck->on_connect( next->client_.get( ), next );

                lck->acceptor_->start_accept( );
            }
        }

        void on_accept_error( const error_code &e )
        {
            std::cerr << "on_accept_error " << e.message( ) << "\n";
        }

        void on_close( )
        {

        }

        srpc::weak_ptr<impl> lst_;
    };


    friend class accept_delegate;
    friend class message_delegate;

public:

    ~listener( )
    {
        impl_->acceptor_.reset( );
    }

    listener( io_service &ios, const std::string &addr, srpc::uint16_t svc )
    {
        acceptor_type::endpoint ep(
                    SRPC_ASIO::ip::address::from_string(addr), svc);

        impl_ = srpc::make_shared<impl>( ios );
        impl_->acceptor_ = acceptor_type::create( ios, 45000, ep );
        impl_->deleg_.reset( new accept_delegate(impl_) );
        impl_->acceptor_->set_delegate( impl_->deleg_.get( ) );
    }

    void start( )
    {
        impl_->acceptor_->open( );
        impl_->acceptor_->start_accept( );
    }

    void stop( )
    {
        impl_->acceptor_->close( );
    }

private:

    impl_sptr impl_;
};

int main( int argc, char *argv[ ] )
{
    try {

        listener::io_service ios;

        common::timers::periodical tt(ios);

        tt.call( [&ios, &tt](...) {
            ios.stop( );
            tt.cancel( );
        }, srpc::chrono::milliseconds(20000) );

        listener l(ios, "0.0.0.0", 23456);

        l.start( );
        ios.run( );
        l.stop( );

        g_clients.clear( );

        google::protobuf::ShutdownProtobufLibrary( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error: " << ex.what( ) << "\n";
        return 1;
    }
    return 0;
}
