#ifndef TRANSPORT_ASYNC_STREAM_H
#define TRANSPORT_ASYNC_STREAM_H

#include <queue>

#include "srpc/common/transport/async/base.h"
#include "srpc/common/buffer.h"
#include "srpc/common/const_buffer.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/config/memory.h"

namespace srpc { namespace common { namespace transport {

namespace async {

    template <typename StreamType>
    class stream: public base<StreamType> {

        typedef base<StreamType>            parent_type;
        typedef stream<StreamType>          this_type;
        typedef interface::write_callbacks  write_callbacks;

        typedef common::const_buffer<char>  message_type;

        typedef typename base<StreamType>::shared_type shared_type;
        typedef typename base<StreamType>::weak_type   weak_type;

        struct queue_value {

            typedef srpc::shared_ptr<queue_value> shared_type;

            message_type    message;

            queue_value( const char *data, size_t length )
                :message(data, length)
            { }

            virtual ~queue_value( ) { }

            virtual void precall( ) { }
            virtual void postcall(const error_code &) { }

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
            void postcall(const error_code &err)
            {
                cbacks.post_call( err );
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

    /////////////////////////////////// READ

        //void start_read_impl_wrap(  )
        void start_read_impl( )
        {
            namespace ph = srpc::placeholders;

            /// use strand::wrap for callback
            this->get_socket( ).async_read_some(
                SRPC_ASIO::buffer( &this->get_read_buffer( )[0],
                                    this->get_read_buffer( ).size( )),
                this->get_dispatcher( ).wrap(
                    srpc::bind( &this_type::read_handler, this,
                                 ph::_1, ph::_2,
                                 this->weak_from_this( ) )
                )
             );
        }

        void start_read_impl_(  )
        {
            namespace ph = srpc::placeholders;
            this->get_socket( ).async_read_some(
                SRPC_ASIO::buffer( &this->get_read_buffer( )[0],
                                    this->get_read_buffer( ).size( )),
                srpc::bind( &this_type::read_handler, this,
                             ph::_1, ph::_2, this->weak_from_this( )
                )
            );
        }

        void async_read( )
        {
            this->call_reader( );
        }
    ///////////////////////////////////

        void async_write(  )
        {
            const message_type &top( queue_top( )->message );
            async_write( top.data( ), top.size( ), 0);
        }

        void async_write( const char *data, size_t length, size_t total )
        {
            namespace ph = srpc::placeholders;
            if( this->active( ) ) {
                this->get_socket( ).async_write_some(
                        SRPC_ASIO::buffer(data, length),
                        this->get_dispatcher( ).wrap(
                            srpc::bind( &this_type::write_handler, this,
                                         ph::_1, ph::_2,
                                         length, total,
                                         this->shared_from_this( ))
                        )
                );
            }
        }

        void write_handler( const error_code &error,
                            size_t const bytes,
                            size_t const length,
                            size_t       total,
                            shared_type  /*this_inst*/ )
        {
            queue_value &top( *queue_top( ) );

            if( !error ) {

                if( bytes < length ) {

                    total += bytes;

                    const message_type &top_mess( top.message );
                    async_write( top_mess.data( ) + total,
                                 top_mess.size( ) - total, total );

                } else {

                    top.postcall( error );

                    queue_pop( );

                    if( !queue_empty( ) ) {
                        async_write(  );
                    }

                }

            } else {

                top.postcall( error );

                this->get_delegate( )->on_write_error( error );
                this->on_write_error( error );

                /// pop queue
                queue_pop( );

                if( !queue_empty( ) ) {
                    async_write(  );
                }
            }

        }

        void write_impl( const queue_value_sptr data, shared_type /*inst*/ )
        {
            const bool empty = queue_empty( );

            queue_push( data );

            if( empty ) {
                async_write(  );
            }
        }

        void post_write( const char *data, size_t len,
                         const write_callbacks &cback )
        {
            typename queue_callback::shared_type inst
                                    = queue_callback::create( data, len );

            inst->cbacks = cback;

            this->get_dispatcher( ).post(
                        srpc::bind( &this_type::write_impl, this,
                                     inst, this->shared_from_this( ) ) );
        }

        void post_write( const char *data, size_t len )
        {
            queue_value_sptr inst(queue_value::create( data, len ));
            this->get_dispatcher( ).post(
                        srpc::bind( &this_type::write_impl, this,
                                     inst, this->shared_from_this( ) ) );
        }

    public:

        typedef StreamType                      stream_type;
        typedef typename parent_type::delegate  delegate;

        stream( io_service &ios, srpc::uint32_t buflen )
            :parent_type(ios, buflen)
        { }

        ~stream( )
        {

        }

        void open( )
        {

        }

        void write( const char *data, size_t len,
                    write_callbacks cback )
        {
            post_write( data, len, cback );
        }

        void write( const char *data, size_t len )
        {
            post_write( data, len );
        }

        void read( )
        {
            async_read( );
        }

    private:

        message_queue_type write_queue_;

    };

}

}}}

#endif
