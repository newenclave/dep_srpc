#ifndef SRPC_COMMON_TRANSPORT_DELEGATES_STREAM_H
#define SRPC_COMMON_TRANSPORT_DELEGATES_STREAM_H

#include "srpc/common/transport/interface.h"
#include "srpc/common/const_buffer.h"

namespace srpc { namespace common { namespace transport {

namespace delegates {

    template <typename SizePackPolicy>
    class stream: public common::transport::interface::delegate {

        typedef stream<SizePackPolicy> this_type;
        typedef srpc::common::const_buffer<char> buffer_type;

    public:

        typedef SizePackPolicy                  size_policy;
        typedef typename size_policy::size_type size_type;

        static const size_t max_length = size_policy::max_length;
        static const size_t min_length = size_policy::min_length;

        stream( )
            :cursor_(0)
        { }

        size_t pack_size( size_type len, char *data )
        {
            return size_policy::pack( len, data );
        }

    private:

        buffer_type update_cursor( const char *data, size_t len )
        {
            if( cursor_ <= len ) {
                on_stream_update( data, cursor_ );
                len     -= cursor_;
                data    += cursor_;
                cursor_  = 0;
                on_stream_end( );
            } else {
                on_stream_update( data, len );
                data    += len;
                cursor_ -= len;
                len = 0;
            }
            return buffer_type( data, len );
        }

        buffer_type start_new_stream( const char *data, size_t len )
        {
            static const size_t max_len = size_policy::max_length + 1;
            static const size_t min_len = size_policy::min_length;

            size_t old_packed = tmp_.size( );
            size_t minimum    = ((len < max_len) ? len : max_len);

            tmp_.insert( tmp_.end( ), data, data + minimum );

            size_t packed = size_policy::size_length( tmp_.begin( ),
                                                      tmp_.end( ) );

            if( packed >= min_len ) {
                typename size_policy::size_type unpack_size
                        = size_policy::unpack( tmp_.begin( ), tmp_.end( ) );

                if( !validate_length( unpack_size ) ) {
                    tmp_.resize( old_packed );
                    return buffer_type( );
                }

                size_t used = (minimum - (tmp_.size( ) - packed));

                cursor_ = unpack_size;
                tmp_.resize( 0 );

                on_stream_begin( unpack_size );

                return buffer_type( data + used, len - used );

            } else if( tmp_.size( ) >= max_len ) {
                on_error( "Bad serialized data; prefix > max_length." );
                tmp_.resize( old_packed );
                return buffer_type( );
            } else {
                return buffer_type( data + minimum, len - minimum );
            }

            return buffer_type( );
        }

        void on_data( const char *data, size_t len )
        {
            buffer_type buff(data, len);
            do {
                if( cursor_ ) {
                    buff = update_cursor( buff.data( ), buff.size( ) );
                }
                if( buff.size( ) ) {
                    buff = start_new_stream( buff.data( ), buff.size( ) );
                }
            } while( buff.size( ) );
            on_need_read( );
        }

        virtual void on_stream_begin( size_t len ) = 0;
        virtual void on_stream_update( const char *part, size_t len ) = 0;
        virtual void on_stream_end( ) = 0;
        virtual void on_need_read( ) = 0;
        virtual void on_error( const char *message ) = 0;

        virtual bool validate_length( size_t len ) = 0;
//        virtual void on_read_error( const error_code & ) = 0;
//        virtual void on_write_error( const error_code &) = 0;
//        virtual void on_close( ) = 0;

        std::string tmp_;
        size_t      cursor_;

    };
}


}}}
#endif // STREAM_H
