#ifndef TRANSPORT_ASYNC_BASE_H
#define TRANSPORT_ASYNC_BASE_H

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/types.h"

namespace srpc { namespace common { namespace transport {

namespace async {

    template <typename SocketType>
    class base: public interface {

        typedef base<SocketType>           this_type;
        typedef interface::write_callbacks write_callbacks;

        virtual void start_read_impl(  ) { }

        virtual void set_buffers( size_t )
        { }

        void close_impl( weak_type inst )
        {
            shared_type lck(inst.lock( ));
            if( lck ) {
                close_unsafe( );
            }
        }

    protected:

        virtual void on_read_error( const error_code & ) { }
        virtual void on_write_error( const error_code & ) { }
        virtual void on_close( ) { }

        void close_unsafe( )
        {
            if( active_ ) {
                get_delegate( )->on_close( );
                on_close( );
                socket_.close( );
                active_ = false;
            }
        }

        void read_handler( const error_code &error,
                           size_t const bytes, weak_type inst )
        {
            shared_type lck(inst.lock( ));
            if( lck ) {
                if( !error ) {
                    get_delegate( )->on_data( &read_buffer_[0], bytes );
                } else {
                    get_delegate( )->on_read_error( error );
                    on_read_error( error );
                }
            }
        }

        void call_reader( )
        {
            start_read_impl( );
        }

        delegate* get_delegate(  )
        {
            return delegate_;
        }

    public:

        typedef SocketType                               socket_type;
        typedef interface::delegate                      delegate;
        typedef typename socket_type::native_handle_type native_handle_type;

        base( io_service &ios, srpc::uint32_t buf_len )
            :socket_(ios)
            ,dispatcher_(ios)
            ,read_buffer_(buf_len)
            ,active_(true)
        { }

        virtual ~base( ) { }

        void resize_buffer( size_t len )
        {
            read_buffer_.resize( len );
            set_buffers( len );
        }

        std::vector<char> &get_read_buffer( )
        {
            return read_buffer_;
        }

        const std::vector<char> &get_read_buffer( ) const
        {
            return read_buffer_;
        }

        void close( )
        {
            dispatcher_.post( srpc::bind(&this_type::close_impl, this,
                              weak_type(this->shared_from_this( ) ) ) );
        }

        io_service &get_io_service( )
        {
            return socket_.get_io_service( );
        }

        io_service::strand &get_dispatcher( )
        {
            return dispatcher_;
        }

        socket_type &get_socket( )
        {
            return socket_;
        }

        void set_delegate( delegate *val )
        {
            delegate_ = val;
        }

        bool active( ) const
        {
            return active_;
        }

        srpc::handle_type native_handle( )
        {
            return socket_.native_handle( );
        }

    private:

        socket_type          socket_;
        io_service::strand   dispatcher_;
        delegate            *delegate_;
        std::vector<char>    read_buffer_;
        bool                 active_;
    };

}}}}

#endif // TRANSPORT_ASYNC_BASE_H
