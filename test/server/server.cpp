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
#include "srpc/common/timers/once.h"

#include "srpc/common/observers/simple.h"
#include "srpc/server/acceptor/async/tcp.h"
#include "srpc/server/acceptor/async/udp.h"
#include "srpc/common/transport/delegates/message.h"

#include "protocol/t.pb.h"

using namespace srpc;

using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::transport::delegates::message<size_policy>;

namespace gpb = google::protobuf;

class protocol_client: public client_delegate {

    using iface_ptr   = common::transport::interface *;
    using client_sptr = srpc::shared_ptr<common::transport::interface>;
    using error_code  = common::transport::error_code;

    static const size_t max_length = client_delegate::size_policy::max_length;

public:

    protocol_client( iface_ptr iface )
        :client_(iface->shared_from_this( ))
    { }

private:

    void on_message( const char *message, size_t len )
    {
        process_call( message, len );
    }

    void on_need_read( )
    {
        client_->read( );
    }

    bool validate_length( size_t len )
    {
        return (len < 44000);
    }

    void on_error( const char *message )
    {
        std::cerr << "on_error " << message << "\n";
    }

    void on_read_error( const error_code &e )
    {
        std::cerr << "on_read_error " << e.message( ) << "\n";
        client_->close( );
    }

    void on_write_error( const error_code &e)
    {
        std::cerr << "on_write_error " << e.message( ) << "\n";
        client_->close( );
    }

    void on_close( );

    void send_message( const gpb::MessageLite &msg )
    {
        typedef common::transport::interface::write_callbacks cb;
        char block[max_length];

        srpc::shared_ptr<std::string> r(srpc::make_shared<std::string>( ));

        r->assign( msg.SerializeAsString( ) );

        size_t packed = pack_size( r->size( ), block );
        r->insert( r->begin( ), &block[0], &block[packed] );

        client_->write( &(*r)[0], r->size( ),
                cb::post( [r, this](...) {  } ) );
    }

    void process_call( const char *message, size_t len )
    {
        test::run t;
        t.ParseFromArray( message, len );

        test::run r;
        std::string *name = r.mutable_name( );

        size_t l = len = t.name( ).size( );

        while( len-- ) {
            name->push_back( t.name( )[len] );
        }
        send_message( r );
    }

public:

    client_sptr client_;
};

using protocol_client_sptr = srpc::shared_ptr<protocol_client>;
std::map<common::transport::interface *, protocol_client_sptr> g_clients;

void protocol_client::on_close( )
{
    g_clients.erase( client_.get( ) );
}


struct listener {

    using size_policy = common::sizepack::varint<size_t>;
    using error_code  = common::transport::error_code;
    using io_service  = common::transport::io_service;

private:

    using tcp = server::acceptor::async::tcp;
    using udp = server::acceptor::async::udp;

    using acceptro_type = udp;

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
                        srpc::make_shared<protocol_client>(c);

                next->client_ = c->shared_from_this( );

                next->client_->set_delegate( next.get( ) );
                next->client_->read( );

                g_clients.insert( std::make_pair(c, next) );

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
        acceptro_type::endpoint ep(
                    SRPC_ASIO::ip::address::from_string(addr), svc);

        impl_ = srpc::make_shared<impl>( );
        impl_->acceptor_ = acceptro_type::create( ios, 45000, ep );
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
        listener l(ios, "0.0.0.0", 23456);
        l.start( );
        ios.run( );
    } catch( const std::exception &ex ) {
        std::cerr << "Error: " << ex.what( ) << "\n";
        return 1;
    }
    return 0;
}
