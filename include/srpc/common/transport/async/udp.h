#ifndef TRANSPORT_ASYNC_UDP_H
#define TRANSPORT_ASYNC_UDP_H

#include "srpc/common/transport/async/base.h"
#include "srpc/common/config/asio.h"


namespace srpc { namespace common { namespace transport {

namespace async {

    class udp: public base<SRPC_ASIO::ip::udp::socket> {

        typedef base<SRPC_ASIO::ip::udp::socket> parent_type;
        typedef udp this_type;

        typedef SRPC_ASIO::ip::udp asio_udp;

        typedef common::transport::interface::shared_type     shared_type;
        typedef common::transport::interface::weak_type       weak_type;
        typedef common::transport::interface::write_callbacks write_callbacks;

        void start_read_impl_wrap(  )
        {
            namespace ph = srpc::placeholders;

            /// use strand::wrap for callback
            get_socket( ).async_receive(
                SRPC_ASIO::buffer( &get_read_buffer( )[0],
                                    get_read_buffer( ).size( )),
                get_dispatcher( ).wrap(
                    srpc::bind( &this_type::read_handler, this,
                                 ph::_1, ph::_2,
                                 this->weak_from_this( ) )
                ) // dispatcher
             );
        }

        void start_read_impl(  )
        {
            namespace ph = srpc::placeholders;
            get_socket( ).async_receive(
                SRPC_ASIO::buffer( &get_read_buffer( )[0],
                                    get_read_buffer( ).size( )),
                srpc::bind( &this_type::read_handler, this,
                             ph::_1, ph::_2,
                             this->weak_from_this( ) )
            );
        }

        void write_handle( const error_code &err, size_t /*len*/,
                           write_callbacks cbacks,
                           shared_type )
        {
            if( err ) {
                get_delegate( )->on_write_error( err );
            } else {
                cbacks.post_call( err );
            }
        }

        void write_handle_empty( const error_code &err, size_t /*len*/,
                                 shared_type )
        {
            if( err ) {
                get_delegate( )->on_write_error( err );
            }
        }

    public:

        typedef asio_udp::endpoint endpoint;

        udp( io_service &ios, std::uint32_t buflen )
            :parent_type(ios, buflen)
        { }

        udp( io_service &ios, std::uint32_t buflen, std::uint32_t opts )
            :parent_type(ios, buflen, opts)
        { }

        void open( )
        {
            ep_.address( ).is_v4( ) ? get_socket( ).open( asio_udp::v4( ) )
                                    : get_socket( ).open( asio_udp::v6( ) );
        }

        void write( const char *data, size_t len, write_callbacks cback )
        {
            namespace ph = srpc::placeholders;
            cback.pre_call( );
            get_socket( ).async_send( SRPC_ASIO::buffer(data, len), 0,
                get_dispatcher( ).wrap(
                    srpc::bind( &this_type::write_handle, this,
                                 ph::_1, ph::_2, cback,
                                 this->shared_from_this( ) )
                )
            );
        }

        void write( const char *data, size_t len )
        {
            namespace ph = srpc::placeholders;
            get_socket( ).async_send( SRPC_ASIO::buffer(data, len), 0,
                get_dispatcher( ).wrap(
                    srpc::bind( &this_type::write_handle_empty, this,
                                 ph::_1, ph::_2,
                                 this->shared_from_this( ) )
                )
            );
        }

        void set_endpoint( const endpoint &val )
        {
            ep_ = val;
        }

        endpoint &get_endpoint( )
        {
            return ep_;
        }

        const endpoint &get_endpoint( ) const
        {
            return ep_;
        }

        void read( )
        {
            call_reader( );
        }

    private:

        asio_udp::endpoint ep_;
    };
}

}}}

#endif // UDP_H
