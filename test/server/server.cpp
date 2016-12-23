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

#include "srpc/common/observers/define.h"

#include "protocol/t.pb.h"

using namespace srpc;

using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::transport::delegates::message<size_policy>;

namespace gpb = google::protobuf;

class protocol_client: public client_delegate {

    using iface_ptr   = common::transport::interface *;
    using client_sptr = srpc::shared_ptr<common::transport::interface>;
    using error_code  = common::transport::error_code;
    using timer       = common::timers::periodical;
    using ticks       = common::timers::ticks<srpc::chrono::milliseconds>;
    using io_service  = common::transport::io_service;

    static const size_t max_length = client_delegate::size_policy::max_length;

public:

    protocol_client( io_service &ios, iface_ptr iface )
        :client_(iface->shared_from_this( ))
        ,keep_alive_(ios)
    {
        last_message_ = ticks::now( );
        keep_alive_.call(
            [this]( const error_code &e )
            {
                if( !e ) {
                    std::uint32_t now = ticks::now( );
                    if( now - last_message_ > 1000 ) {
                        client_->close( );
                    }
                }
            }, srpc::chrono::milliseconds(1000) );
    }

private:

    void on_message( const char *message, size_t len )
    {
        last_message_ = ticks::now( );
        std::cout << "rcv message " << len << "\n";
        process_call( message, len );
    }

    void on_need_read( )
    {
        client_->read( );
    }

    bool validate_length( size_t len )
    {
        if( len >= 44000 ) {
            client_->close( );
        }

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

        srpc::shared_ptr<std::string> r = get_str( );
        r->resize( max_length );
        msg.AppendToString( r.get( ) );

        size_t packed = pack_size( r->size( ) - max_length, block );
        std::copy( &block[0], &block[packed],
                    r->begin( ) + (max_length - packed) );

        client_->write( &(*r)[max_length - packed],
                        r->size( ) - max_length + packed,
            cb::post( [r, this](...)
            {
                if( cache_.size( ) < 10 ) {
                    cache_.push( r );
                }
            } ) );
    }

    srpc::shared_ptr<std::string> get_str( )
    {
        if(cache_.empty( )) {
            return srpc::make_shared<std::string>( );
        } else {
            srpc::shared_ptr<std::string> n = cache_.front( );
            cache_.pop( );
            return n;
        }
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

    client_sptr     client_;
    timer           keep_alive_;
    srpc::uint32_t  last_message_;

    std::queue<srpc::shared_ptr<std::string> > cache_;

};

using protocol_client_sptr = srpc::shared_ptr<protocol_client>;
std::map<common::transport::interface *, protocol_client_sptr> g_clients;

void protocol_client::on_close( )
{
    std::cout << "Erase client " << client_.get( ) << std::endl;
    g_clients.erase( client_.get( ) );
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
                        srpc::make_shared<protocol_client>( lck->ios_, c );

                next->client_ = c->shared_from_this( );

                next->client_->set_delegate( next.get( ) );
                next->client_->read( );

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

        tt.call( [&ios](...) {
            ios.stop( );
        }, srpc::chrono::milliseconds(10000) );

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
