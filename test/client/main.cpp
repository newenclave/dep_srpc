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

struct connector: public srpc::enable_shared_from_this<connector> {
    struct delegate {
        virtual ~delegate( ) { }
        virtual void on_connect( common::transport::interface * ) = 0;
        //virtual void on_disconnect( common::transport::interface * ) = 0;
        virtual void on_connect_error( const bs::error_code &err ) = 0;
        virtual void on_close( ) = 0;
    };
    virtual void open( ) = 0;
    virtual void close( ) = 0;
    virtual void connect( ) = 0;
    virtual void set_delegate( delegate * ) = 0;
    virtual ~connector( ) { }
};

class tcp_connector: public connector {

    class client_type: public common::transport::async::tcp {
    public:

        client_type(ba::io_service &ios, size_t bs)
            :common::transport::async::tcp(ios, bs)
        { }

        static
        srpc::shared_ptr<client_type> create( ba::io_service &ios, size_t bs )
        {
            return srpc::make_shared<client_type>( srpc::ref(ios), bs );
        }
    };

    void handle_connect( const bs::error_code &error,
                         srpc::weak_ptr<connector> inst )
    {
        srpc::shared_ptr<connector> lck(inst.lock( ));
        if( lck ) {
            if( !error ) {
                delegate_->on_connect( client_.get( ) );
            } else {
                delegate_->on_connect_error( error );
            }
        }
    }

public:

    typedef common::transport::async::tcp::endpoint endpoint;

    tcp_connector( ba::io_service &ios, size_t buflen )
        :ios_(ios)
        ,buflen_(buflen)
    { }

    void open( )
    { }

    void close( )
    {
        if( client_ ) {
            delegate_->on_close( );
            client_->close( );
        }
    }

    srpc::weak_ptr<connector> weak_from_this( )
    {
        return srpc::weak_ptr<connector>(shared_from_this( ));
    }

    void connect( )
    {
        namespace ph = srpc::placeholders;
        {
            srpc::shared_ptr<client_type> tmp =
                    client_type::create( ios_, buflen_ );
            tmp->set_endpoint( ep_ );
            tmp->open( );
            client_.swap( tmp );
        }
        client_->get_socket( ).async_connect( ep_,
            srpc::bind( &tcp_connector::handle_connect, this,
                        ph::_1, weak_from_this( ) ) );
    }

    void set_delegate( delegate *val )
    {
        delegate_ = val;
    }

    void set_endpoint( const endpoint &ep )
    {
        ep_ = ep;
    }

    endpoint &get_endpoint( )
    {
        return ep_;
    }

    const endpoint &get_endpoint( ) const
    {
        return ep_;
    }

private:

    ba::io_service     &ios_;
    size_t              buflen_;
    srpc::shared_ptr<client_type> client_;
    endpoint            ep_;
    delegate           *delegate_;
};

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

