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

struct m_delegate: public common::transport::interface::delegate {

    common::transport::interface::shared_type parent_;
    int count = 0;

    m_delegate(common::transport::interface::shared_type parent)
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

        if( count++ > 1000000 ) {
            parent_->write( data, len,
                        cb::post( [this]( ... ) { parent_->close( ); } ) );
        } else {
            std::shared_ptr<std::string> d = std::make_shared<std::string>( );
            (*d) += (boost::lexical_cast<std::string>(count)) + "\n";
            //parent_->write( data, len, cb( ) );
            parent_->write( d->c_str( ), d->length( ),
                            cb::post( [this, d]( ... ) {
                                if( count % 10000 == 0 ) {
                                    std::cout << d->c_str() << "\n";
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



int main( )
{
    try {

        std::cout << sizeof(std::function<void( )>) << "\n\n";

        ba::io_service ios;
        ba::ip::udp::endpoint ep(ba::ip::address::from_string("127.0.0.1"), 2356);

        auto t = std::make_shared<udp_transport>(std::ref(ios), 4096, 0);

        t->set_endpoint( ep );
        t->open( );

        t->resize_buffer( 44000 );
        t->get_socket( ).connect( t->get_endpoint( ) );

        m_delegate del( t );
        t->set_delegate( &del );
        t->write( "Hello!\n", 7 );
        t->read( );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

