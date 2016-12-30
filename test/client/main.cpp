#include <iostream>
#include "protocol/t.pb.h"

#include "boost/lexical_cast.hpp"

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"
#include "srpc/common/sizepack/none.h"

#include "srpc/common/transport/async/stream.h"
#include "srpc/common/transport/async/tcp.h"
#include "srpc/common/transport/async/udp.h"

#include "srpc/client/connector/async/tcp.h"
#include "srpc/client/connector/async/udp.h"

#include "srpc/common/queues/condition.h"

#include <memory>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace ba = SRPC_ASIO;
namespace bs = SRPC_SYSTEM;

using namespace srpc;

template <typename StreamType>
using common_transport = common::transport::async::base<StreamType>;

template <typename StreamType>
using stream_transport = common::transport::async::stream<StreamType>;

using tcp_transport = common::transport::async::tcp;
using udp_transport = common::transport::async::udp;

template <typename SizePack>
using delegate_message = common::transport::delegates::message<SizePack>;

using size_pack_policy = common::sizepack::varint<std::uint16_t>;

class mess_delegate: public delegate_message<size_pack_policy> {

public:

    srpc::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    int cnt = 0;

    //mess_delegate::pack_context ctx;

    mess_delegate( int c )
        :cnt(c)
    { }

    void on_need_read( )
    {
        parent_->read( );
    }

    void on_message( const char *message, size_t len )
    {
//        auto ctx = std::make_shared<mess_delegate::pack_context>( );
//        if( cnt-- > 0 ) {

//            if( 0 == cnt % 10000 ) {
//                std::cout << cnt << std::endl;
//            }

//            pack_begin( *ctx, 10 * 100 );

//            for( int i=0; i<100; i++ )
//                pack_update( *ctx, "??????????", 10 );
//            pack_end( *ctx );

//            parent_->write( ctx->data( ).c_str( ), ctx->data( ).size( ),
//            cb::post([ctx]( const bs::error_code &err, size_t len )
//            {
////                if( err ) {
////                    throw SRPC_SYSTEM::system_error( err );
////                };
////                std::cout << "sent " << len << " bytes from "
////                            << ctx->data( ).size( )
////                          << " error " << err.message( ) << "\n";
//            } ));
//        } else {
//            parent_->close( );
//        }
    }

    bool validate_length( size_t len )
    {
        return true;
    }

    void on_error( const char *message )
    {
        std::cout << "on error: " << message << "\n";
    }

    void on_read_error( const SRPC_SYSTEM::error_code &err )
    {
        std::cout << "on read error: " << err.message( ) << "\n";
        parent_->close( );
    }

    void on_write_error( const SRPC_SYSTEM::error_code &err)
    {
        std::cout << "on write error: " << err.message( ) << "\n";
        parent_->close( );
    }

    void on_close( )
    {
        std::cout << "on close\n";
    }


};

struct udp_echo_delegate final: public common::transport::async::udp::delegate {

    std::shared_ptr<common::transport::interface> parent_;
    using cb = common::transport::interface::write_callbacks;
    int cnt = 0;

    udp_echo_delegate( int count )
        :cnt(count)
    {

    }

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
        // std::cout << std::string(data, len);
        std::shared_ptr<std::string> echo
                = std::make_shared<std::string>(data, len);
        //std::cin >> *echo;
        if( cnt-- > 0 ) {
            if( 0 == cnt % 10000 ) {
                std::cout << cnt << std::endl;
            }
            parent_->write( echo->c_str( ), echo->size( ),
                            cb::post([echo](...){ }) );
            parent_->read( );
        } else {
            parent_->close( );
        }
    }

    void on_close( )
    {

    }
};

using connector = client::connector::interface;
using tcp_connector = client::connector::async::tcp;

struct connector_delegate: public connector::delegate {

    //udp_echo_delegate echo_;
    mess_delegate echo_;

    connector_delegate( )
        :echo_(1000000)
    { }

    void on_connect( common::transport::interface *c )
    {
        std::cout << "On connect!!\n";
        echo_.parent_ = c->shared_from_this( );
        c->set_delegate( &echo_ );
        echo_.on_message( "?", 1 );
        //c->write( "1", 1 );
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

int main_( int argc, char *argv[] )
{
    try {

        const char *host = argc > 1 ? argv[1] : "127.0.0.1";

        std::cout << sizeof(std::function<void( )>) << "\n\n";
        using transtort_type      = udp_transport;
        using transtort_delegate  = udp_echo_delegate;


        std::cout << "host: " << host << "\n";

        ba::io_service ios;
        transtort_type::endpoint ep(ba::ip::address::from_string(host), 12347);

        connector_delegate deleg;
        auto uc = client::connector::async::udp::create(std::ref(ios), 4096, ep);

        uc->set_delegate( &deleg );
        uc->connect( );
        uc->transport( )->resize_buffer(12000);

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

