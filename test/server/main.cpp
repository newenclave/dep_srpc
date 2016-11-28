#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"
#include "srpc/server/acceptor/interface.h"
#include "srpc/server/acceptor/async/tcp.h"

#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"


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

std::set< std::shared_ptr<common::transport::interface> > gclients;

struct tcp_echo_delegate final: public common::transport::interface::delegate {
    using cb = common::transport::interface::write_callbacks;

    std::shared_ptr<common::transport::interface> parent_;

    void on_read_error( const bs::error_code & )
    {
        std::cout << "Read error!\n";
        gclients.erase( parent_ );
        parent_->close( );
        parent_.reset( );
    }

    void on_write_error( const bs::error_code &)
    {

    }

    void on_data( const char *data, size_t len )
    {
        std::shared_ptr<std::string> echo
                = std::make_shared<std::string>( data, len );
        parent_->write( echo->c_str( ), echo->size( ),
                        cb::post([echo](...){ }) );
        start_read( );
    }

    void on_close( )
    {
        std::cout << "Close client\n";
    }

    void start_read( )
    {
        parent_->read( );
    }

};

struct udp_echo_delegate final: public common::transport::async::udp::delegate {

    std::shared_ptr<udp_transport> parent_;
    using cb = common::transport::interface::write_callbacks;

    void on_read_error( const bs::error_code & )
    {

    }

    void on_write_error( const bs::error_code &)
    {

    }

    void on_data( const char *data, size_t len )
    {
        std::shared_ptr<std::string> echo
                = std::make_shared<std::string>( data, len );

        parent_->write_to( parent_->get_endpoint( ),
                           echo->c_str( ), echo->size( ),
                           cb::post([echo](...){ }) );
        start_read( );
    }

    void on_close( )
    {
        std::cout << "close\n";
    }

    void start_read( )
    {
        namespace ph = std::placeholders;
        ba::ip::udp::endpoint ep ;
        parent_->read_from( ep );
    }
};

using acceptor = server::acceptor::interface;
using tcp_acceptor = server::acceptor::async::tcp;

struct acceptor_del: public acceptor::delegate {

    tcp_echo_delegate               delegate_;
    std::shared_ptr<tcp_acceptor>   acceptor_;

    void on_disconnect_client( common::transport::interface * )
    {

    }

    void on_accept_client( common::transport::interface *c )
    {
        gclients.insert( c->shared_from_this( ) );
        c->set_delegate( &delegate_ );
        delegate_.parent_ = c->shared_from_this( );
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

int main( )
{
    try {

        common::sizepack::varint<unsigned> pak;
        auto p = pak.pack( 123445 );

        std::cout << p << "\n";

        using transtort_type      = tcp_transport;
        using transtort_delegate  = udp_echo_delegate;

        ba::io_service ios;
        transtort_type::endpoint ep(ba::ip::address::from_string("0.0.0.0"), 2356);
        acceptor_del deleg;
        auto t = std::make_shared<tcp_acceptor>(std::ref(ios), 4096);
        t->set_delegate( &deleg );
        deleg.acceptor_ = t;
        t->open( );
        t->bind( ep, true );
        t->listen( 5 );
        t->start_accept( );
        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

