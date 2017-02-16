#include <string>
#include <iostream>

#include "options.h"

#include "srpc/server/acceptor/async/tcp.h"
#include "srpc/server/acceptor/async/udp.h"

#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/timers/periodical.h"

#include "boost/asio.hpp"

#include "async-reader.hpp"

#include <linux/if_tun.h>

namespace tunnel {
    int opentuntap( std::string &in_out, int flags, bool persis );
}

namespace tunnel { namespace server {

    using namespace srpc::server;

    namespace scomm     = srpc::common;
    namespace ssrv      = srpc::server;

    namespace asio      = boost::asio;
    namespace system    = boost::system;
    namespace ip        = asio::ip;

    using adelegate     = acceptor::interface::delegate;

    namespace tcp {
        using listener      = ssrv::acceptor::async::tcp;
        using endpoint      = listener::endpoint;
    }

    namespace udp {
        using listener      = ssrv::acceptor::async::udp;
        using endpoint      = listener::endpoint;
    }

    template <typename T>
    struct package_size;

    template <>
    struct package_size<ssrv::acceptor::async::udp> {
        static const size_t value = 44 * 1024;
    };

    template <>
    struct package_size<ssrv::acceptor::async::tcp> {
        static const size_t value = 4 * 1024;
    };

    using ctransport    = scomm::transport::interface;
    using tuntap_stream = asio::posix::stream_descriptor;

    template<typename SP>
    using cdelegate  = scomm::transport::delegates::message<SP>;

    using transport_sptr = std::shared_ptr<ctransport>;
    using transport_ptr  = ctransport *;

    template <typename ListenerType>
    struct acceptor {

        using listener      = ListenerType;
        using endpoint      = typename listener::endpoint;
        using error_code    = typename listener::error_code;

        static const size_t package_size = package_size<listener>::value;

        class tuntap_reader: public async_reader::point_iface<tuntap_stream>
        {

        public:

            using parent_type = async_reader::point_iface<tuntap_stream>;
            using delegate = typename acceptor<listener>::acceptor_delegate;

            tuntap_reader( asio::io_service &ios )
                :async_reader::point_iface<tuntap_stream>(ios, package_size,
                                           parent_type::OPT_DISPATCH_READ)
            { }

            void on_read( char *data, size_t length )
            {
                my_acceptor->write_to_clients( data, length );
            }

            void on_read_error( const system::error_code &err ) override
            {
                std::cerr << "Device read error: "
                          << err.message( ) << "\n";
            }

            void on_write_error( const system::error_code &err ) override
            {
                std::cerr << "Device write error: "
                          << err.message( ) << "\n";
            }

            void on_write_exception(  ) override
            {
                throw;
            }

            std::string on_transform_message( std::string &message ) override
            {
                return std::move( message );
            }

            delegate *my_acceptor = nullptr;
        };

        struct client_delegate: cdelegate<my_size_policy> {

            using cb = ctransport::write_callbacks;
            using parent_type = cdelegate<my_size_policy>;
            using delegate = typename acceptor<listener>::acceptor_delegate;

            client_delegate( asio::io_service &ios )
                :keepalive(ios)
            {

            }

            void on_message( const char *message, size_t len ) override
            {
                switch (message[0]) {
                    case tunnel::TAG_DATA:
                        my_acceptor->write_to_device( message + 1, len - 1 );
                    break;
                    default:
                    break;
                }
            }

            bool validate_length( size_t len ) override
            {
                return true;
            }

            void on_error( const char *message ) override
            {
                std::cerr << "Client stream error: " << message << "\n";
                my_transport->close( );
            }

            void on_need_read( ) override
            {
                my_transport->read( );
            }

            void on_read_error( const error_code &err ) override
            {
                std::cerr << "Client read error: " << err.message( ) << "\n";
                my_transport->close( );
            }

            void on_write_error( const error_code &err ) override
            {
                std::cerr << "Client write error: " << err.message( ) << "\n";
                my_transport->close( );
            }

            void on_close( ) override
            {
                my_acceptor->remove_client( this );
                std::cout << "Client has quited.\n";
            }

            delegate                   *my_acceptor  = nullptr;
            transport_ptr               my_transport = nullptr;
            scomm::timers::periodical   keepalive;
            bool                        shot = false;
        };

        using cdelegate_sptr = std::shared_ptr<client_delegate>;
        using client_pair    = std::pair<cdelegate_sptr, transport_sptr>;

        using client_map = std::map<client_delegate *, client_pair>;

        struct acceptor_delegate: adelegate {

            using key_type = typename client_map::key_type;

            acceptor_delegate( asio::io_service &ios )
                :device(std::make_shared<tuntap_reader>(std::ref(ios)))
            {
                device->my_acceptor = this;
            }

            void on_accept_client( scomm::transport::interface *t,
                                   const std::string &addr,
                                   srpc::uint16_t port )
            {
                std::cout << "Accept new client from " << addr << ":"
                          << port << "\n";

                auto deleg = std::make_shared<client_delegate>(
                                                    device->get_io_service( ) );

                deleg->my_acceptor  = this;
                deleg->my_transport = t;

                auto clnt = t->shared_from_this( );

                clnt->set_delegate( deleg.get( ) );

                auto cpair = std::make_pair( deleg, clnt );
                all_clients[deleg.get( )] = cpair;

                t->read( );

                my_listener->start_accept( );

            }

            void on_accept_error( const error_code &err )
            {
                std::cout << "Accept error: " << err.message( ) << "\n";
            }

            void on_close( )
            {
                for( auto &f: all_clients ) {
                    f.second.second.reset( );
                    f.second.first.reset( );
                }
                all_clients.clear( );
            }

            void remove_client( key_type k )
            {
                auto f = all_clients.find( k );
                if( f != all_clients.end( ) ) {
                    f->second.second.reset( );
                    f->second.first.reset( );
                    all_clients.erase( f );
                }
            }

            void write_to_device( const char *data, size_t len )
            {
                static const auto cbs = [this]( const system::error_code &err )
                {
                    if( err ) {
                        std::cerr << "Device write error: "
                                  << err.message( ) << "\n";
                    }
                };

                device->write_post_notify( data, len, cbs );
            }

            void write_to_clients( const char *data, size_t len )
            {
                using cb = ctransport::write_callbacks;

                auto str = std::make_shared<std::string>( 1, '\0' );

                (*str)[0] = tunnel::TAG_DATA;

                str->append(data, len);

                str = tunnel::message_packer<>::pack( str );

                auto cbs = [this, str]( const error_code &, size_t ) { };

                for( auto &f: all_clients ) {
                    f.second.second->write( str->c_str( ), str->size( ),
                                            cb::post( cbs ) );
                }
            }

            client_map                       all_clients;
            listener                        *my_listener = nullptr;
            std::shared_ptr<tuntap_reader>   device;
        };

        static
        int start( options &opts )
        {
            asio::io_service ios;
            acceptor_delegate deleg(ios);

            auto flags = opts.use_tap ? IFF_TAP | IFF_NO_PI | O_NONBLOCK
                                      : IFF_TUN | IFF_NO_PI | O_NONBLOCK;

            auto dev = opentuntap( opts.device, flags, false );

            if( dev < 0 ) {
                throw std::system_error( errno, std::system_category( ) );
            }

            endpoint ep( ip::address::from_string(opts.address), opts.port);
            auto lst = listener::create( ios, package_size, ep );

            deleg.my_listener = lst.get( );
            deleg.device->get_stream( ).assign( dev );

            lst->set_delegate( &deleg );
            lst->open( );
            lst->bind( );
            lst->start_accept( );
            deleg.device->start_read( );
            ios.run( );
            return 0;
        }

    };

    int start( options &opts )
    {
        if( opts.use_udp ) {
            return acceptor<udp::listener>::start( opts );
        } else {
            return acceptor<tcp::listener>::start( opts );
        }

        return 0;
    }

}}
