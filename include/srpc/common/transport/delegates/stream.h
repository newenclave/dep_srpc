#ifndef SRPC_COMMON_TRANSPORT_DELEGATES_STREAM_H
#define SRPC_COMMON_TRANSPORT_DELEGATES_STREAM_H

#include "srpc/common/transport/interface.h"

namespace srpc { namespace common { namespace transport {

namespace delegates {

    template <typename SizePackPolicy>
    class stream: public common::transport::interface::delegate {

        typedef stream<SizePackPolicy> this_type;
    public:
        typedef SizePackPolicy size_policy;
        typedef typename size_policy::size_type size_type;

        stream( )
            :cursor_(0)
        { }

    private:

        bool chck( size_t len )
        {
            if( (unpacked_.size( ) > size_policy::max_length)
                           && (len < size_policy::min_length) )
            {
                on_error( "Bad serialized data; prefix > max_length." );
                return false;
            }
            return true;
        }

//        void start_stream( const char *data, size_t len )
//        {
//            size_t pack_len = size_policy::size_length( data, data + len );
//            while( (len > 0) && chck( pack_len )
//                   && (pack_len >= size_policy::min_length) )
//            {
//                size_t unpacked = size_policy::unpack( data, data + len );
//                if( !validate_length( unpacked ) ) {
//                    break;
//                }
//                data += pack_len;
//                len  -= pack_len;
//                on_stream_begin( unpacked );
//                if( len < unpacked ) {

//                } else {
//                    cursor_ = len - unpacked;
//                }
//            }
//        }

//        void on_data( const char *data, size_t len )
//        {
//            if( cursor_ > 0 ) {
//                if( len < cursor_ ) {
//                    on_stream_update( data, len );
//                    cursor_ -= len;
//                } else {
//                    on_stream_update( data, cursor_ );
//                    data += cursor_;
//                    cursor_ = 0;
//                }
//            }

//        }

        virtual void on_stream_begin( size_t len ) = 0;
        virtual void on_stream_update( const char *part, size_t len ) = 0;
        virtual void on_stream_end( ) = 0;
        virtual bool validate_length( size_t len ) = 0;
        virtual void on_error( const char *message ) = 0;
//        virtual void on_read_error( const error_code & ) = 0;
//        virtual void on_write_error( const error_code &) = 0;
//        virtual void on_close( ) = 0;

        std::string unpacked_;
        size_t      cursor_;

    };
}


}}}
#endif // STREAM_H
