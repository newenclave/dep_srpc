#include <string>
#include <iostream>

#include "options.h"

#include "srpc/client/connector/async/tcp.h"
#include "srpc/client/connector/async/udp.h"

#include "srpc/common/transport/delegates/message.h"

#include "boost/asio.hpp"

#include "async-reader.hpp"

#include <linux/if_tun.h>

namespace tunnel {
    int opentuntap( std::string &in_out, int flags, bool persis );
}

namespace tunnel { namespace client {

    using namespace srpc::client;

    namespace scomm   = srpc::common;
    namespace scnl    = srpc::client;
    namespace asio    = boost::asio;
    namespace system  = boost::system;

    using condelegate = scnl::connector::interface::delegate;
    using ctransport  = scomm::transport::interface;
    using error_code  = scnl::connector::interface::error_code;

    template<typename SP>
    using cdelegate  = scomm::transport::delegates::message<SP>;

    using transport_sptr = std::shared_ptr<ctransport>;
    using transport_ptr  = ctransport *;

    namespace tcp {
        using client_type = scnl::connector::async::tcp;
        using client_sptr = std::shared_ptr<client_type>;
        using endpoint    = client_type::endpoint;
    }

    namespace udp {
        using client_type = scnl::connector::async::udp;
        using client_sptr = std::shared_ptr<client_type>;
        using endpoint    = client_type::endpoint;
    }

    template <typename T>
    struct package_size;

    template <>
    struct package_size<scnl::connector::async::udp> {
        static const size_t value = 44 * 1024;
    };

    template <>
    struct package_size<scnl::connector::async::tcp> {
        static const size_t value = 4 * 1024;
    };

    using tuntap_stream = asio::posix::stream_descriptor;

    template <typename ClientType>
    struct connector: public condelegate  {

        using client_type = ClientType;
        using client_sptr = std::shared_ptr<client_type>;
        using endpoint    = typename client_type::endpoint;

        static const size_t package_size = package_size<client_type>::value;

        class tuntap_reader: public async_reader::point_iface<tuntap_stream>
        {

        public:

            using parent_type = async_reader::point_iface<tuntap_stream>;

            tuntap_reader( asio::io_service &ios )
                :async_reader::point_iface<tuntap_stream>(ios, package_size,
                                           parent_type::OPT_DISPATCH_READ)
            { }

            void on_read( char *data, size_t length ) override
            {
                using cb = ctransport::write_callbacks;

                auto str = std::make_shared<std::string>( 1, '\0' );

                (*str)[0] = tunnel::TAG_DATA;
                str->append( data, length );

                str = tunnel::message_packer<>::pack( str );

                auto cbs = [str]( const error_code &, size_t ) { };

                my_parent->my_transport->write( str->c_str( ),
                                                str->size( ),
                                                cb::post( cbs ) );
            }

            void on_read_error( const system::error_code &err ) override
            {
                std::cerr << "Read from device error: "
                          << err.message( ) << "\n";
            }

            void on_write_error( const system::error_code &err ) override
            {
                std::cerr << "Write to device error: "
                          << err.message( ) << "\n";
            }

            void on_write_exception(  )  override
            {
                throw;
            }

            std::string on_transform_message( std::string &message ) override
            {
                return std::move( message );
            }

            connector *my_parent = nullptr;
        };

        struct tdelegate: public cdelegate<my_size_policy> {

            void on_message( const char *message, size_t len ) override
            {
                auto cbs = [ ]( const system::error_code &err ) {
                    if( err ) {
                        std::cerr << "Device write error: "
                                  << err.message( ) << "\n";
                    }
                };

                switch ( message[0] ) {
                case tunnel::TAG_DATA:
                    my_parent->device->write_post_notify( message + 1,
                                                          len - 1, cbs );
                    break;
                default:
                    break;
                }
            }

            bool validate_length( size_t ) override
            {
                return true;
            }

            void on_error( const char *message ) override
            {
                std::cerr << "Client message error: " << message << "\n";
            }

            void on_need_read( ) override
            {
                my_parent->my_transport->read( );
            }

            void on_read_error( const error_code &err ) override
            {
                std::cerr << "Client read error: "
                          << err.message( ) << "\n";
                my_parent->ios.stop( );
            }

            void on_write_error( const error_code &err ) override
            {
                std::cerr << "Client write error: "
                          << err.message( ) << "\n";
                my_parent->ios.stop( );
            }

            void on_close( ) override
            {
                my_parent->device->close( );
            }

            connector *my_parent = nullptr;
        };

        using tdelegate_ptr = std::unique_ptr<tdelegate>;

        connector( asio::io_service &ios )
            :ios(ios)
        { }

        void connect( tunnel::options &opts )
        {
            deleg.reset( new tdelegate );
            endpoint ep( asio::ip::address::from_string( opts.address),
                                                         opts.port );

            auto flags = opts.use_tap ? IFF_TAP | IFF_NO_PI | O_NONBLOCK
                                      : IFF_TUN | IFF_NO_PI | O_NONBLOCK;

            auto dev = tunnel::opentuntap( opts.device, flags, false );
            if( dev < 0 ) {
                throw std::system_error( errno, std::system_category( ) );
            }

            auto new_inst = client_type::create( ios, package_size, ep );
            auto new_deleg = std::unique_ptr<tdelegate>( new tdelegate );

            new_deleg->my_parent = this;

            client.swap( new_inst );
            deleg.swap( new_deleg );

            client->set_delegate( this );
            client->open( );
            client->connect( );

            device = std::make_shared<tuntap_reader>( std::ref(ios) );
            device->my_parent = this;
            device->get_stream( ).assign( dev );
        }

        void on_connect( scomm::transport::interface *t ) override
        {
            using cb = ctransport::write_callbacks;
            my_transport = t->shared_from_this( );
            my_transport->set_delegate( deleg.get( ) );
            t->read( );
            device->start_read( );
            auto str = std::make_shared<std::string>( 1, tunnel::TAG_COMMAND );
            message_packer<>::pack( str );
            t->write( str->c_str( ), str->size( ), cb::post([str](...){ }) );

            std::cout << "Connect success!\n";
        }

        void on_connect_error( const error_code &err )  override
        {
            std::cerr << "Connect error: " << err.message( ) << "\n";
        }

        void on_close( ) override
        {

        }

        transport_sptr                  my_transport;
        asio::io_service               &ios;
        client_sptr                     client;
        tdelegate_ptr                   deleg;
        std::shared_ptr<tuntap_reader>  device;
    };

    int start( options &opts )
    {
        asio::io_service ios;

        if( opts.use_udp ) {
            connector<udp::client_type> cnt(ios);
            cnt.connect( opts );
            ios.run( );
        } else {
            connector<tcp::client_type> cnt(ios);
            cnt.connect( opts );
            ios.run( );
        }

    }

}}
