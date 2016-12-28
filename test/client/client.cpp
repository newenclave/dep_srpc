#include <iostream>
#include <thread>

#include "srpc/common/sizepack/zigzag.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/client/connector/async/tcp.h"
#include "srpc/client/connector/async/udp.h"

#include "protocol/t.pb.h"

#include "srpc/common/protocol/binary.h"

using namespace srpc;
namespace gpb = google::protobuf;

using message_sptr = srpc::shared_ptr<gpb::Message>;

using iface_ptr       = common::transport::interface *;
using client_sptr     = srpc::shared_ptr<common::transport::interface>;
using connector_type  = client::connector::async::udp;
using connector_sptr  = srpc::shared_ptr<connector_type>;
using size_policy     = common::sizepack::varint<size_t>;

using client_delegate = common::protocol::binary<message_sptr>;

class connector: private client_delegate {

    using io_service     = common::transport::io_service;
    using error_code     = common::transport::error_code;

    static const size_t max_length = client_delegate::size_policy::max_length;

    typedef typename client_delegate::buffer_type         buffer_type;
    typedef typename client_delegate::const_buffer_slice  const_buffer_slice;
    typedef typename client_delegate::buffer_slice        buffer_slice;
    typedef typename client_delegate::tag_type            tag_type;
    typedef typename client_delegate::message_type        message_type;
    typedef typename common::transport::interface::write_callbacks cb_type;

    struct connector_delegate: public client::connector::interface::delegate {

        connector *parent_;

        connector_delegate( )
        { }

        void on_connect( iface_ptr i )
        {
            parent_->assign_transport( i );
            i->set_delegate( parent_ );
            i->read( );
        }

        void on_connect_error( const error_code &err )
        {
            std::cerr << "on_connect_error " << err.message( ) << std::endl;
            throw std::runtime_error( err.message( ) );
        }

        void on_close( )
        {
            std::cout << "Connector close\n";
        }
    };

    friend class connector_delegate;

    bool validate_length( size_t len )
    {
        //std::cout << "Validate len: " << len << "\n";
        return len <= 44000;
    }

public:

    connector( io_service &ios, const std::string &addr, srpc::uint16_t svc )
        :client_delegate(101)
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

    void append_message( buffer_type buf, const message_type &m )
    {
        m->AppendToString( buf.get( ) );
    }

    void on_message_ready( tag_type tag, buffer_type buff,
                           const_buffer_slice slice )
    {
        std::cout << "Got message " << slice.size( )
                  << " with tag " << tag
                  << " buf alocated " << (buff ? "true" : "false")
                  << "\n";
    }

    buffer_type unpack_message( const_buffer_slice &slice )
    {
        buffer_type r = get_str( );
        r->resize( slice.size( ) );
        for( size_t i=0; i<slice.size( ); i++ ) {
            (*r)[i] = slice.data( )[i] ^ 0xE4;
        }
        slice = const_buffer_slice(r->c_str( ), r->size( ));
        return r;
    }

    buffer_slice pack_message( buffer_type, buffer_slice slice )
    {
        char *p = slice.begin( );
        while( p != slice.end( ) ) {
            *p = *p ^ 0xE4;
            p++;
        }
        return slice;
    }

    void send_message( const std::string &data )
    {
        srpc::shared_ptr<test::run> msg = srpc::make_shared<test::run>( );
        buffer_type buff = get_str( );
        msg->set_name( data );

        buff->resize( 4 );

        buffer_slice slice = prepare_buffer( buff, 0, msg );
        slice = insert_size_prefix( buff, slice );

        get_transport( )->write( slice.begin( ), slice.size( ),
            cb_type::post( [this, buff](...) {
                cache_.push( buff );
            } ));
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

        std::this_thread::sleep_for( std::chrono::milliseconds(100));

        std::string test;
        for( int i=0; i<43000; i++ ) {
            test.push_back( (char)((i % 10) + '0') );
        }

        ctr.send_message( test );

        while( !std::cin.eof( ) ) {
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
