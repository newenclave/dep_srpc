#include <iostream>
#include <thread>

#include "srpc/common/sizepack/zigzag.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/client/connector/async/tcp.h"
#include "srpc/client/connector/async/udp.h"

#include "protocol/t.pb.h"

#include "srpc/common/protocol/noname.h"

using namespace srpc;
namespace gpb = google::protobuf;

using message_sptr = srpc::shared_ptr<gpb::Message>;

using iface_ptr       = common::transport::interface *;
using client_sptr     = srpc::shared_ptr<common::transport::interface>;
using connector_type  = client::connector::async::tcp;
using connector_sptr  = srpc::shared_ptr<connector_type>;
using size_policy     = common::sizepack::varint<size_t>;

using client_delegate = common::protocol::noname<>;

class connector: public client_delegate {

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
            std::cout << "ready!\n";
            parent_->assign_transport( i );
            i->set_delegate( parent_ );

            parent_->set_ready( true );
            parent_->ready_var_.notify_all( );

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
        :client_delegate(false, 44000)
    {

        connector_type::endpoint ep(
                    SRPC_ASIO::ip::address::from_string(addr), svc );

        connector_        = connector_type::create( ios, 45000, ep );
        delegate_.parent_ = this;
        connector_->set_delegate( &delegate_ );
    }

    static
    srpc::shared_ptr<connector> create( io_service &ios,
                                        const std::string &addr,
                                        srpc::uint16_t svc)
    {
        srpc::shared_ptr<connector> inst
                = srpc::make_shared<connector>(srpc::ref(ios), addr, svc);
        inst->init( );
        inst->track( inst );
        return inst;
    }

    void start( )
    {
        connector_->open( );
        connector_->connect( );
    }

    virtual
    service_sptr get_service( const message_type & )
    {
        std::cout << "request service!\n";
        return service_sptr( );
    }

    void execute_call( message_type msg )
    {
        std::cout << msg->DebugString( ) << "\n";
    }

    void wait_ready(  )
    {
        srpc::unique_lock<srpc::mutex> l(ready_mtx_);
        ready_var_.wait( l );
    }

private:
    client_sptr         client_;
    connector_sptr      connector_;
    connector_delegate  delegate_;

    srpc::condition_variable ready_var_;
    srpc::mutex              ready_mtx_;
};

int main( int argc, char *argv[] )
{
    try {
        SRPC_ASIO::io_service ios;
        SRPC_ASIO::io_service::work wrk(ios);

        std::thread t([&ios]( ){ ios.run( ); });

        auto ctr = connector::create( ios, "127.0.0.1", 23456);

        ctr->start( );
        ctr->wait_ready( );

        srpc::rpc::lowlevel ll;
        ll.mutable_opt( )->set_wait( true );
        ll.set_request( "!!!!!!!!!!!!!!" );
        ll.mutable_call( )->set_method_id( "call6" );

        std::string d = "!!!!!!!!!!!!!!";
        for( int i=0; i<100000; ++i ) {

            ctr->setup_message( ll, 0 );
            auto slot = ctr->add_slot( ll.id( ) );
            ctr->send_message( ll );
            srpc::shared_ptr<srpc::rpc::lowlevel> mess;
            auto res = slot->read_for( mess, srpc::chrono::seconds(1) );
            if( 0 != res ) {
                break;
            }
            ctr->erase_slot( slot );
        }
        ios.stop( );

        //ios.run( );

        t.join( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error " << ex.what( ) << "\n";
    }
    return 0;
}
