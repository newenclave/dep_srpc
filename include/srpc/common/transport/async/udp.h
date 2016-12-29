#ifndef TRANSPORT_ASYNC_UDP_H
#define TRANSPORT_ASYNC_UDP_H

#include "srpc/common/transport/async/base.h"
#include "srpc/common/config/asio.h"
#include "srpc/common/const_buffer.h"

#include <queue>

namespace srpc { namespace common { namespace transport {

namespace async {

    class udp: public base<SRPC_ASIO::ip::udp::socket> {

        typedef base<SRPC_ASIO::ip::udp::socket> parent_type;
        typedef udp this_type;

        typedef SRPC_ASIO::ip::udp asio_udp;

        typedef common::transport::interface::shared_type     shared_type;
        typedef common::transport::interface::weak_type       weak_type;
        typedef common::transport::interface::write_callbacks write_callbacks;

        typedef common::const_buffer<char> message_type;

        struct queue_value {
            message_type        message;
            asio_udp::endpoint  to;

            queue_value( const char *data, size_t length )
                :message(data, length)
            { }

            typedef srpc::shared_ptr<queue_value> shared_type;
            virtual void precall( ) { }
            virtual void postcall(const error_code &, size_t ) { }
            virtual ~queue_value( ) { }
            static
            shared_type create( const char *data, size_t length )
            {
                return srpc::make_shared<queue_value>( data, length );
            }
        };

        struct queue_callback: public queue_value {

            write_callbacks cbacks;

            typedef srpc::shared_ptr<queue_callback> shared_type;

            queue_callback( const char *data, size_t length )
                :queue_value(data, length)
            { }

            void precall( )
            {
                cbacks.pre_call( );
            }
            void postcall( const error_code &err, size_t len )
            {
                cbacks.post_call( err, len );
            }

            static
            shared_type create( const char *data, size_t length )
            {
                return srpc::make_shared<queue_callback>( data, length );
            }

        };

        typedef typename queue_value::shared_type  queue_value_sptr;

        typedef std::queue<queue_value_sptr> message_queue_type;

        void queue_push( const queue_value_sptr &new_mess )
        {
            write_queue_.push( new_mess );
        }

        const queue_value_sptr &queue_top( ) const
        {
            return write_queue_.front( );
        }

        void queue_pop( )
        {
            write_queue_.pop( );
        }

        bool queue_empty( ) const
        {
            return write_queue_.empty( );
        }

        //void start_read_impl_wrap(  )
        void start_read_impl( )
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

        void start_read_impl_(  )
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

        void start_read_from_impl( )
        {
            namespace ph = srpc::placeholders;
            get_socket( ).async_receive_from(
                SRPC_ASIO::buffer( &get_read_buffer( )[0],
                                    get_read_buffer( ).size( )),
                    ep_, 0,
                    get_dispatcher( ).wrap(
                        srpc::bind( &this_type::read_handler, this,
                                 ph::_1, ph::_2,
                                 this->weak_from_this( ) )
                    ) // dispatcher
            );
        }

        void write_handle( const error_code &err, size_t len,
                           shared_type )
        {
            queue_value_sptr &top(write_queue_.front( ));
            if( err ) {
                get_delegate( )->on_write_error( err );
                on_write_error( err );
            }
            top->postcall( err, len );
            write_queue_.pop( );
            if( !write_queue_.empty( ) ) {
                async_write( );
            }
        }

        void write_to_handle( const error_code &err, size_t len,
                              shared_type )
        {
            queue_value_sptr &top(write_queue_.front( ));
            if( err ) {
                get_delegate( )->on_write_error( err );
                on_write_to_error( err, top->to );
            }
            top->postcall( err, len );

            write_queue_.pop( );
            if( !write_queue_.empty( ) ) {
                async_write( );
            }
        }

        void async_write(  )
        {
            queue_value  &top(*write_queue_.front( ));
            message_type &m(top.message);
            namespace ph = srpc::placeholders;
            top.precall( );
            if( top.to.port( ) != 0 ) {
                get_socket( ).async_send_to(
                    SRPC_ASIO::buffer(m.data( ), m.size( )),
                    top.to, 0,
                    get_dispatcher( ).wrap(
                        srpc::bind( &this_type::write_to_handle, this,
                                     ph::_1, ph::_2,
                                     this->shared_from_this( ) )
                    )
                );
            } else {
                get_socket( ).async_send(
                    SRPC_ASIO::buffer(m.data( ), m.size( )),
                    0,
                    get_dispatcher( ).wrap(
                        srpc::bind( &this_type::write_handle, this,
                                     ph::_1, ph::_2,
                                     this->shared_from_this( ) )
                    )
                );
            }
        }

        void write_to_queue( queue_value_sptr val )
        {
            bool empty = write_queue_.empty( );
            write_queue_.push( val );
            if( empty ) {
                async_write( );
            }
        }

        void push_to_queue( queue_value_sptr val )
        {
            get_dispatcher( ).post(
                        srpc::bind( &udp::write_to_queue, this, val ) );
        }

    protected:
        virtual void on_read_from_error( const error_code & ) { }
        virtual void on_write_to_error( const error_code &,
                                        const asio_udp::endpoint & ) { }

    public:

        static const bool is_stream = false;
        static const bool is_safe   = false;

        typedef asio_udp::endpoint      endpoint;
        typedef parent_type::delegate   delegate;

        udp( io_service &ios, std::uint32_t buflen )
            :parent_type(ios, buflen)
        { }

        void open( )
        {
            ep_.address( ).is_v6( ) ? get_socket( ).open( asio_udp::v6( ) )
                                    : get_socket( ).open( asio_udp::v4( ) );
        }

        void write( const char *data, size_t len, write_callbacks cback )
        {
            queue_callback::shared_type t = queue_callback::create( data, len );
            t->cbacks  = cback;
            push_to_queue( t );
        }

        void write( const char *data, size_t len )
        {
            queue_value::shared_type t = queue_value::create( data, len );
            push_to_queue( t );
        }

        void write_to( const endpoint &ep,
                       const char *data, size_t len,
                       write_callbacks cback )
        {
            queue_callback::shared_type t
                    = queue_callback::create( data, len );
            t->cbacks = cback;
            t->to     = ep;
            push_to_queue( t );
        }

        void write_to( const endpoint &ep, const char *data, size_t len )
        {
            queue_value::shared_type t = queue_value::create( data, len );
            t->to = ep;
            push_to_queue( t );
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

        void read_from( const endpoint &ep )
        {
            ep_ = ep;
            start_read_from_impl( );
        }

    private:
        asio_udp::endpoint ep_;
        message_queue_type write_queue_;

    };
}

}}}

#endif // UDP_H
