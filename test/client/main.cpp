#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"

#include "srpc/client/connector/async/tcp.h"

#include <memory>
#include <queue>

namespace ba = boost::asio;
namespace bs = boost::system;

using namespace srpc;

template <typename StreamType>
using common_transport = common::transport::async::base<StreamType>;

template <typename StreamType>
using stream_transport = common::transport::async::stream<StreamType>;

using tcp_transport = common::transport::async::tcp;
using udp_transport = common::transport::async::udp;

struct tcp_echo_delegate final: public common::transport::interface::delegate {

    using cb = common::transport::interface::write_callbacks;

    std::shared_ptr<common::transport::interface> parent_;

    void on_read_error( const bs::error_code &err )
    {
        std::cout << "Read error " << err.message( ) << "\n";
        parent_->close( );
    }

    void on_write_error( const bs::error_code &)
    {

    }

    void on_data( const char *data, size_t len )
    {
        std::shared_ptr<std::string> echo
                = std::make_shared<std::string>( data, len );
        std::cout << std::string(data, len);
        std::cin >> *echo;
        parent_->write( echo->c_str( ), echo->size( ),
                        cb::post([echo](...){ }) );
        parent_->read( );
    }

    void on_close( )
    {
        parent_.reset( );
    }

    void start_read( )
    {
        parent_->read( );
    }

};

struct udp_echo_delegate final: public common::transport::async::udp::delegate {

    std::shared_ptr<udp_transport> parent_;
    using cb = common::transport::interface::write_callbacks;

    void on_read_error( const bs::error_code &ex )
    {
        std::cout << "Read error! " << ex.message( ) << "\n";
        parent_->close( );
    }

    void on_write_error( const bs::error_code &ex )
    {
        std::cout << "Write error! " << ex.message( ) << "\n";
    }

    void on_data( const char *data, size_t len )
    {
        std::cout << std::string(data, len);
        std::shared_ptr<std::string> echo = std::make_shared<std::string>( );
        std::cin >> *echo;
        parent_->write( echo->c_str( ), echo->size( ),
                        cb::post([echo](...){ }) );
        parent_->read( );
    }

    void on_close( )
    {

    }
};

using connector = client::connector::interface;
using tcp_connector = client::connector::async::tcp;

struct connector_delegate: public connector::delegate {

    tcp_echo_delegate echo_;

    void on_connect( common::transport::interface *c )
    {
        std::cout << "On connect!!\n";
        echo_.parent_ = c->shared_from_this( );
        c->set_delegate( &echo_ );
        c->write( "1", 1 );
        c->read( );
    }

    void on_connect_error( const bs::error_code &err )
    {
        std::cerr << "Error " << err.message( ) << "\n";
    }

    void on_close( )
    {
        std::cerr << "Close\n";
    }
};

int main( )
{
    try {

        std::cout << sizeof(std::function<void( )>) << "\n\n";
        using transtort_type      = tcp_transport;
        using transtort_delegate  = udp_echo_delegate;

        ba::io_service ios;
        transtort_type::endpoint ep(ba::ip::address::from_string("127.0.0.1"), 2356);

        connector_delegate deleg;

        auto t = std::make_shared<tcp_connector>(std::ref(ios), 4096);
        t->set_delegate( &deleg );
        t->set_endpoint( ep );
        t->connect( );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

