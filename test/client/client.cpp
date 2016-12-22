#include <iostream>
#include <thread>

#include "srpc/common/sizepack/zigzag.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/client/connector/async/tcp.h"
#include "srpc/client/connector/async/udp.h"

#include "protocol/t.pb.h"

using namespace srpc;

using iface_ptr       = common::transport::interface *;
using client_sptr     = srpc::shared_ptr<common::transport::interface>;
using connector_sptr  = srpc::shared_ptr<client::connector::async::tcp>;
using size_policy     = common::sizepack::varint<size_t>;
using client_delegate = common::transport::delegates::message<size_policy>;


class connector: public client_delegate {

    using io_service     = common::transport::io_service;
    using error_code     = common::transport::error_code;
    using connector_type = client::connector::async::tcp;

    static const size_t max_length = client_delegate::size_policy::max_length;

    struct connector_delegate: public client::connector::interface::delegate {

        connector *parent_;

        connector_delegate( )
        { }

        void on_connect( iface_ptr i )
        {
            parent_->client_ = i->shared_from_this( );
            i->set_delegate( parent_ );
            i->read( );
        }

        void on_connect_error( const error_code &err )
        {
            std::cerr << "on_connect_error " << err.message( ) << std::endl;
        }

        void on_close( )
        {

        }
    };

    friend class connector_delegate;

    void on_message( const char *message, size_t len )
    {
        test::run r;
        r.ParseFromArray(message, len);
        std::cout << r.name( ) << std::endl;
    }

    bool validate_length( size_t len )
    {
        return len <= 44000;
    }

    void on_error( const char *message )
    {
        std::cerr << "on_error " << message << std::endl;
    }

    void on_need_read( )
    {
        client_->read( );
    }

    void on_read_error( const error_code &err )
    {
        std::cerr << "on_read_error " << err.message( ) << std::endl;
    }

    void on_write_error( const error_code &err )
    {
        std::cerr << "on_write_error " << err.message( ) << std::endl;
    }

    void on_close( )
    {

    }

public:

    connector( io_service &ios, const std::string &addr, srpc::uint16_t svc )
    {

        connector_type::endpoint ep(
                    SRPC_ASIO::ip::address::from_string(addr), svc );

        connector_        = connector_type::create( ios, 45000, ep );
        delegate_.parent_ = this;
        connector_->set_delegate( &delegate_ );
    }

    void start( )
    {
        connector_->open( );
        connector_->connect( );
    }

    void send_message( const std::string &data )
    {
        test::run msg;
        msg.set_name( data );

        typedef common::transport::interface::write_callbacks cb;
        char block[max_length];

        srpc::shared_ptr<std::string> r = get_str( );

        r->assign( msg.SerializeAsString( ) );

        size_t packed = pack_size( r->size( ), block );
        r->insert( r->begin( ), &block[0], &block[packed] );

        client_->write( &(*r)[0], r->size( ),
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

private:
    client_sptr         client_;
    connector_sptr      connector_;
    connector_delegate  delegate_;

    std::queue<srpc::shared_ptr<std::string> > cache_;
};

int main( int argc, char *argv[] )
{
    try {
        SRPC_ASIO::io_service ios;
        SRPC_ASIO::io_service::work wrk(ios);

        std::thread t([&ios]( ){ ios.run( ); });

        connector ctr(ios, "127.0.0.1", 23456);
        ctr.start( );

        while( 1 ) {
            std::string d;
            std::cin >> d;
            ctr.send_message( d );
        }

        t.join( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error " << ex.what( ) << "\n";
    }
    return 0;
}
