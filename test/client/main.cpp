#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"

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


template <typename ParentType>
struct m_delegate: public common::transport::interface::delegate {

    std::shared_ptr<ParentType> parent_;
    int count = 0;

    m_delegate(std::shared_ptr<ParentType> parent)
        :parent_(parent)
    { }

    void on_read_error( const boost::system::error_code &err )
    {
        std::cout << "Read error: " << err.message( ) << "\n";
    }

    void on_write_error( const boost::system::error_code &err)
    {
        std::cout << "Write error: " << err.message( ) << "\n";
    }

    void on_data( const char *data, size_t len )
    {
        typedef common::transport::interface::write_callbacks cb;

        //std::cout << "data: " << std::string(data, len) << std::endl;

        if( count++ > 10000 ) {
            parent_->write( data, len,
                        cb::post( [this]( ... ) { parent_->close( ); } ) );
        } else {
            std::shared_ptr<std::string> d = std::make_shared<std::string>( );
            (*d) += (boost::lexical_cast<std::string>(count)) + "\n";
            //parent_->write( data, len, cb( ) );
            parent_->write( d->c_str( ), d->length( ),
                            cb::post( [this, d]( ... ) {
                                if( count % 10000 == 0 ) {
                                    std::cout << d->c_str( ) << "\n";
                                }
                            } ) );
            parent_->read( );
        }
    }

    void on_close( )
    {
        std::cout << "Close\n";
    }
};

struct tcp_echo_delegate final: public common::transport::interface::delegate {
    using cb = common::transport::interface::write_callbacks;

    std::shared_ptr<tcp_transport> parent_;

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
        parent_->write( echo->c_str( ), echo->size( ),
                        cb::post([echo](...){ }) );
    }

    void on_close( )
    {

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

    void on_write_error( const bs::error_code &)
    {

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

int main( )
{
    try {


        std::cout << sizeof(std::function<void( )>) << "\n\n";
        using transtort_type      = udp_transport;
        using transtort_delegate  = udp_echo_delegate;

        ba::io_service ios;
        transtort_type::endpoint ep(ba::ip::address::from_string("127.0.0.1"), 2356);

        auto t = std::make_shared<transtort_type>(std::ref(ios), 4096);

        t->set_endpoint( ep );
        t->open( );
        t->get_socket( ).connect( ep );

        t->resize_buffer( 44000 );

        transtort_delegate del;
        del.parent_ = t;
        t->set_delegate( &del );
        t->write( "Hi!", 3 );
        t->read( );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

