#ifndef TRANSPORT_ASYNC_BASE_H
#define TRANSPORT_ASYNC_BASE_H

#include "srpc/common/transport/interface.h"
#include "srpc/common/transport/types.h"

namespace srpc { namespace common { namespace transport {

namespace async {

    template <typename SocketType>
    class base: public interface {

        typedef interface::delegate        delegate;
        typedef base<SocketType>           this_type;
        typedef interface::write_callbacks write_callbacks;

        typedef void (this_type::*call_impl)( );

        static
        call_impl get_read_dispatch( srpc::uint32_t opts )
        {
            return ( opts & OPT_DISPATCH_READ )
                   ? &this_type::start_read_impl_wrap
                   : &this_type::start_read_impl;
        }

        virtual void start_read_impl_wrap(  )
        { }

        virtual void start_read_impl(  )
        { }

        virtual void set_buffers( size_t )
        { }

        void close_impl( weak_type inst )
        {
            shared_type lck(inst.lock( ));
            if( lck && active_ ) {
                delegate_->on_close( );
                socket_.close( );
                active_ = false;
            }
        }

    protected:

        void read_handler( const error_code &error,
                           size_t const bytes, weak_type inst )
        {
            shared_type lck(inst.lock( ));
            if( lck ) {
                if( !error ) {
                    delegate_->on_data( &read_buffer_[0], bytes );
                } else {
                    delegate_->on_read_error( error );
                }
            }
        }

        void call_reader( )
        {
            (this->*read_impl_)( );
        }

        std::vector<char> &get_read_buffer( )
        {
            return read_buffer_;
        }

        delegate* get_delegate(  )
        {
            return delegate_;
        }

    public:

        typedef SocketType socket_type;

        enum options {
            OPT_NONE          = 0x00,
            OPT_DISPATCH_READ = 0x01,
        };

        base( io_service &ios, srpc::uint32_t buf_len )
            :socket_(ios)
            ,dispatcher_(ios)
            ,read_buffer_(buf_len)
            ,read_impl_(get_read_dispatch(OPT_DISPATCH_READ))
            ,active_(true)
        { }

        base( io_service &ios, srpc::uint32_t buf_len, srpc::uint32_t opts )
            :socket_(ios)
            ,dispatcher_(ios)
            ,read_buffer_(buf_len)
            ,read_impl_(get_read_dispatch(opts))
            ,active_(true)
        { }

        virtual ~base( ) { }

        void resize_buffer( size_t len )
        {
            read_buffer_.resize( len );
            set_buffers( len );
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

    private:

        socket_type          socket_;
        io_service::strand   dispatcher_;
        delegate            *delegate_;
        std::vector<char>    read_buffer_;
        call_impl            read_impl_;
        bool                 active_;
    };

}}}}

#endif // TRANSPORT_ASYNC_BASE_H
