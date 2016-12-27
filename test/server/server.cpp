#include <iostream>
#include <list>
#include <set>
#include <atomic>
#include <thread>

#include "boost/asio.hpp"

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

#include "srpc/common/protocol/binary.h"

#include "srpc/common/observers/define.h"

#include "protocol/t.pb.h"

using namespace srpc;
namespace gdb = google::protobuf;

using message_sptr = srpc::shared_ptr<gdb::Message>;

using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::protocol::binary<message_sptr>;

class protocol_client: public client_delegate {

    typedef typename client_delegate::tag_type tag_type;
    typedef typename client_delegate::buffer_type buffer_type;
    typedef typename client_delegate::const_buffer_slice const_buffer_slice;
    typedef SRPC_ASIO::io_service io_service;

public:

    protocol_client( io_service &ios )
        :client_delegate(100)
        ,ios_(ios)
    { }

    void append_message( buffer_type buf, const message_type &m )
    {
        m->AppendToString( buf.get( ) );
    }

    void on_message_ready( tag_type tag, buffer_type buff,
                           const_buffer_slice slice )
    {
        test::run msg;
        msg.ParseFromArray( slice.data( ), slice.size( ) );
        std::cout << msg.DebugString( );
    }

    void on_close( );
    void on_error( const char *mess )
    {
        std::cerr << "Proto error: " << mess << "\n";
        get_transport( )->close( );
    }

private:
    io_service &ios_;
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

    using acceptor_type = tcp;

    struct message_delegate;
    struct impl;
    struct accept_delegate;

    using iface_ptr     = common::transport::interface *;
    using client_sptr   = srpc::shared_ptr<common::transport::interface>;
    using acceptor_sptr = srpc::shared_ptr<server::acceptor::interface>;
    using impl_sptr     = srpc::shared_ptr<impl>;

    using message_delegate_sptr = srpc::shared_ptr<message_delegate>;

    struct impl: public srpc::enable_shared_from_this<impl> {

        acceptor_sptr                               acceptor_;
        srpc::unique_ptr<accept_delegate>           deleg_;
        io_service                                 &ios_;
        impl( io_service &ios )
            :ios_(ios)
        { }
    };

    using client_delegate = common::transport::delegates::message<size_policy>;

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

private:

    impl_sptr impl_;
};


int main( int argc, char *argv[ ] )
{

    try {

        listener::io_service ios;

        common::timers::periodical tt(ios);

//        tt.call( [&ios](...) {
//            ios.stop( );
//        }, srpc::chrono::milliseconds(10000) );

        listener l(ios, "0.0.0.0", 23456);

        l.start( );
        ios.run( );

        google::protobuf::ShutdownProtobufLibrary( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error: " << ex.what( ) << "\n";
        return 1;
    }
    return 0;
}
