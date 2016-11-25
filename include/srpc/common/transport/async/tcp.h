#ifndef TRANSPORT_ASYNC_TCP_H
#define TRANSPORT_ASYNC_TCP_H

#include "srpc/common/transport/async/stream.h"
#include "srpc/common/config/asio.h"

namespace srpc { namespace common { namespace transport {

namespace async {

    class tcp: public stream<SRPC_ASIO::ip::tcp::socket> {

        typedef stream<SRPC_ASIO::ip::tcp::socket> parent_type;
        typedef SRPC_ASIO::ip::tcp asio_tcp;

        void set_buffers( size_t len )
        {
            SRPC_ASIO::socket_base::send_buffer_size snd( len );
            SRPC_ASIO::socket_base::send_buffer_size rcv( len );
            get_socket( ).set_option(snd);
            get_socket( ).set_option(rcv);
        }

    public:

        typedef asio_tcp::endpoint endpoint;
        typedef parent_type::delegate delegate;

        tcp(io_service &ios, srpc::uint32_t buflen )
            :parent_type(ios, buflen)
        { }

        void open( )
        {
            ep_.address( ).is_v6( ) ? get_socket( ).open( asio_tcp::v4( ) )
                                    : get_socket( ).open( asio_tcp::v6( ) );
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

    private:

        asio_tcp::endpoint ep_;

    };
}

}}}

#endif // TCP_H
