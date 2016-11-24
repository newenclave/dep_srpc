#include <iostream>
#include "protocol/t.pb.h"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include "transport-iface.h"

#include <memory>
#include <queue>

namespace ba = boost::asio;
namespace bs = boost::system;

using namespace srpc;

template <typename StreamType>
class common_transport: public common::transport_iface {

    typedef common_transport<StreamType> this_type;
    typedef void (this_type::*call_impl)( );

    static
    call_impl get_read_dispatch( std::uint32_t opts )
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
            stream_.close( );
            active_ = false;
        }
    }

protected:

    void read_handler( const boost::system::error_code &error,
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

    enum options {
        OPT_NONE          = 0x00,
        OPT_DISPATCH_READ = 0x01,
    };

    common_transport( ba::io_service &ios )
        :stream_(ios)
        ,dispatcher_(ios)
        ,read_buffer_(4096)
        ,read_impl_(get_read_dispatch(OPT_DISPATCH_READ))
    { }

    common_transport( ba::io_service &ios, std::uint32_t opts )
        :stream_(ios)
        ,dispatcher_(ios)
        ,read_buffer_(4096)
        ,read_impl_(get_read_dispatch(opts))
    { }

    void resize_buffers( size_t len )
    {
        read_buffer_.resize( len );
        set_buffers( len );
    }

    void close( )
    {
        dispatcher_.post( std::bind(&this_type::close_impl, this,
                           weak_type(this->shared_from_this( ) ) ) );
    }

    ba::io_service &get_io_service( )
    {
        return stream_.get_io_service( );
    }

    ba::io_service::strand &get_dispatcher( )
    {
        return dispatcher_;
    }

    StreamType &get_stream( )
    {
        return stream_;
    }

    void set_delegate( delegate *val )
    {
        delegate_ = val;
    }

private:

    StreamType stream_;
    ba::io_service::strand dispatcher_;
    common::transport_iface::delegate *delegate_;
    std::vector<char>  read_buffer_;
    call_impl          read_impl_;
    bool               active_;
};

template <typename StreamType>
class stream_transport: public common_transport<StreamType> {

    typedef common_transport<StreamType> parent_type;
    typedef stream_transport<StreamType> this_type;

    typedef common::const_buffer<char>   message_type;

    typedef typename common_transport<StreamType>::shared_type shared_type;
    typedef typename common_transport<StreamType>::weak_type   weak_type;

    struct queue_value {

        typedef std::shared_ptr<queue_value> shared_type;

        message_type                message_;
        common::transport_callbacks cbacks_;

        queue_value( const char *data, size_t length )
            :message_(data, length)
        { }

        static
        shared_type create( const char *data, size_t length )
        {
            return std::make_shared<queue_value>( data, length );
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

    void start_read_impl_wrap(  )
    {
        namespace ph = std::placeholders;

        /// use strand::wrap for callback
        this->get_stream( ).async_read_some(
            boost::asio::buffer(&this->get_read_buffer( )[0],
                                 this->get_read_buffer( ).size( )),
            this->get_dispatcher( ).wrap(
                std::bind( &this_type::read_handler, this,
                    ph::_1, ph::_2,
                    weak_type(this->shared_from_this( )) )
            )
         );
    }

    void start_read_impl(  )
    {
        namespace ph = std::placeholders;
        this->get_stream( ).async_read_some(
            boost::asio::buffer(&this->get_read_buffer( )[0],
                                 this->get_read_buffer( ).size( )),
            std::bind( &this_type::read_handler, this,
                ph::_1, ph::_2,
                weak_type(this->shared_from_this( ))
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
        const message_type &top( queue_top( )->message_ );
        async_write( top.data( ), top.size( ), 0);
    }

    void async_write( const char *data, size_t length, size_t total )
    {
        namespace ph = std::placeholders;
        this->get_stream( ).async_write_some(
                boost::asio::buffer(data, length),
                this->get_dispatcher( ).wrap(
                    std::bind( &this_type::write_handler, this,
                                ph::_1, ph::_2,
                                length, total,
                                this->shared_from_this( ))
                )
        );
    }

    void write_handler( const boost::system::error_code &error,
                        size_t const bytes,
                        size_t const length,
                        size_t       total,
                        shared_type  /*this_inst*/ )
    {
        queue_value &top( *queue_top( ) );

        if( !error ) {

            if( bytes < length ) {

                total += bytes;

                const message_type &top_mess( top.message_ );
                async_write( top_mess.data( ) + total,
                             top_mess.size( ) - total, total );

            } else {

                top.cbacks_.post_call( error );

                queue_pop( );

                if( !queue_empty( ) ) {
                    async_write(  );
                }

            }

        } else {

            top.cbacks_.post_call( error );
            this->get_delegate( )->on_write_error( error );

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
                     const common::transport_callbacks &cback )
    {
        queue_value_sptr inst(queue_value::create( data, len ));
        inst->cbacks_ = cback;

        this->get_dispatcher( ).post(
                    std::bind( &this_type::write_impl, this,
                                inst, this->shared_from_this( ) ) );
    }

public:

    typedef StreamType stream_type;

    stream_transport(ba::io_service &ios, std::uint32_t opts)
        :parent_type(ios, opts)
    { }

    stream_transport( ba::io_service &ios )
        :parent_type(ios)
    { }

    ~stream_transport( )
    {

    }

    void open( )
    {
    }

    void write( const char *data, size_t len,
                common::transport_callbacks cback )
    {
        post_write( data, len, cback );
    }

    void write( const char *data, size_t len )
    {
        write( data, len, common::transport_callbacks( ) );
    }

    void read( )
    {
        async_read( );
    }

private:

    message_queue_type write_queue_;

};

using tcp_transport = stream_transport<ba::ip::tcp::socket>;

class udp_transport: public common_transport<ba::ip::udp::socket> {

    typedef common_transport<ba::ip::udp::socket> parent_type;
    typedef udp_transport this_type;

    typedef common::transport_iface::shared_type shared_type;
    typedef common::transport_iface::weak_type   weak_type;

    void read_handler0( const bs::error_code &error,
                        size_t const bytes, weak_type inst )
    {
        shared_type lck(inst.lock( ));
        if( lck ) {
            if( !error ) {
                get_delegate( )->on_data( &get_read_buffer( )[0], bytes );
            } else {
                get_delegate( )->on_read_error( error );
            }
        }
    }

    void start_read_impl_wrap(  )
    {
        namespace ph = std::placeholders;

        /// use strand::wrap for callback
        get_stream( ).async_receive(
            boost::asio::buffer(&get_read_buffer( )[0],
                                 get_read_buffer( ).size( )),
            get_dispatcher( ).wrap(
                std::bind( &this_type::read_handler, this,
                            ph::_1, ph::_2,
                            weak_type(this->shared_from_this( )) )
            )
         );
    }

    void start_read_impl(  )
    {
        namespace ph = std::placeholders;
        get_stream( ).async_receive(
            boost::asio::buffer(&get_read_buffer( )[0],
                                 get_read_buffer( ).size( )),
            std::bind( &this_type::read_handler, this,
                        ph::_1, ph::_2,
                        weak_type(this->shared_from_this( ))
            )
        );
    }

    void write_handle( const bs::error_code &err, size_t len,
                       common::transport_callbacks cbacks,
                       shared_type )
    {
        //std::cout << "Udp write: " << len << "bytes\n";
        if( err ) {
            get_delegate( )->on_write_error( err );
        } else {
            cbacks.post_call( err );
        }
    }

public:

    udp_transport( ba::io_service &ios, const ba::ip::udp::endpoint &ep )
        :parent_type(ios)
        ,ep_(ep)
    { }

    void close( )
    {

    }

    void open( )
    {
        ep_.address( ).is_v4( ) ? get_stream( ).open( ba::ip::udp::v4( ) )
                                : get_stream( ).open( ba::ip::udp::v6( ) );
        get_stream( ).connect( ep_ );
        ep_ = get_stream( ).local_endpoint( );
    }

    void write( const char *data, size_t len,
                common::transport_callbacks cback )
    {
        namespace ph = std::placeholders;
        //std::cout << "Udp send: " << len << "bytes\n";
        cback.pre_call( );
        get_stream( ).async_send( ba::buffer(data, len), 0,
            get_dispatcher( ).wrap(
                std::bind( &this_type::write_handle, this,
                           ph::_1, ph::_2, cback,
                           this->shared_from_this( ) )
            )
            );
    }

    void write( const char *data, size_t len )
    {
        write( data, len, common::transport_callbacks( ) );
    }

    void read( )
    {
        call_reader( );
    }

private:

    ba::ip::udp::endpoint ep_;
};

struct m_delegate: public common::transport_iface::delegate {

    common::transport_iface::shared_type parent_;
    int count = 0;

    m_delegate(common::transport_iface::shared_type parent)
        :parent_(parent)
    { }

    void on_read_error( const boost::system::error_code &err )
    {
        std::cout << "Read error: " << err.message( ) << "\n";
    }

    void on_write_error( const boost::system::error_code &err)
    {
        std::cout << "Write error: " << err.message( ) << "\n";
    }

    void on_data( const char *data, size_t len )
    {
        using cb = common::transport_callbacks;

        //std::cout << "data: " << std::string(data, len) << std::endl;

        if( count++ > 1000000 ) {
            parent_->write( data, len,
                        cb::post( [this]( ... ) { parent_->close( ); } ) );
        } else {
            std::shared_ptr<std::string> d = std::make_shared<std::string>( );
            (*d) += (boost::lexical_cast<std::string>(count)) + "\n";
            //parent_->write( data, len, cb( ) );
            parent_->write( d->c_str( ), d->length( ),
                            cb::post( [this, d]( ... ) {
                                if( count % 10000 == 0 ) {
                                    std::cout << d->c_str() << "\n";
                                }
                            } ) );
            parent_->read( );
        }
    }

    void on_close( )
    {
        std::cout << "Close\n";
    }
};



int main( )
{
    try {

        std::cout << sizeof(std::function<void( )>) << "\n\n";

        ba::io_service ios;
        ba::ip::tcp::endpoint ep(ba::ip::address::from_string("127.0.0.1"), 2356);

        auto t = std::make_shared<tcp_transport>(std::ref(ios) );

        t->get_stream( ).open( ba::ip::tcp::v4( ) );
        t->get_stream( ).connect( ep );

        m_delegate del( t );
        t->set_delegate( &del );
        t->open( );
        t->write( "Hello!\n", 7 );
        t->read( );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Errr: " << ex.what( ) << "\n";
    }

    return 0;
}

