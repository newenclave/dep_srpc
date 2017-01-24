#ifndef SRPC_COMMON_TRANSPORT_DELEGATES_MESSAGE_H
#define SRPC_COMMON_TRANSPORT_DELEGATES_MESSAGE_H

#include "srpc/common/transport/interface.h"

namespace srpc { namespace common { namespace transport {

namespace delegates {

    template <typename SizePackPolicy>
    class message: public common::transport::interface::delegate {

        typedef message<SizePackPolicy> this_type;

    public:

        typedef SizePackPolicy                  size_policy;
        typedef typename size_policy::size_type size_type;

        static const size_t max_length = size_policy::max_length;
        static const size_t min_length = size_policy::min_length;

        size_t pack_size( size_type len, char *data )
        {
            return size_policy::pack( len, data );
        }

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

        void on_data( const char *data, size_t len )
        {
            //std::cout << ".";
            unpacked_.insert( unpacked_.end( ), data, data + len );

            size_t pack_len = size_policy::size_length( unpacked_.begin( ),
                                                        unpacked_.end( ) );

            while( !unpacked_.empty( )
                   && chck( pack_len )
                   && (pack_len >= size_policy::min_length) )
            {

                size_t unpacked = size_policy::unpack( unpacked_.begin( ),
                                                       unpacked_.end( ) );

                if( !validate_length( unpacked ) ) {
                    break;
                }

                if( unpacked_.size( ) >= (unpacked + pack_len) ) {

                    on_message( &unpacked_[pack_len], unpacked );

                    unpacked_.erase( unpacked_.begin( ),
                                     unpacked_.begin( ) + pack_len + unpacked );

                    pack_len = size_policy::size_length( unpacked_.begin( ),
                                                         unpacked_.end( ) );
                } else {
                    break;
                }
            }
            on_need_read( );
        }

        virtual void on_message( const char *message, size_t len ) = 0;
        virtual bool validate_length( size_t len ) = 0;
        virtual void on_error( const char *message ) = 0;
        virtual void on_need_read( ) = 0;
//        virtual void on_read_error( const error_code & ) = 0;
//        virtual void on_write_error( const error_code &) = 0;
//        virtual void on_close( ) = 0;

        std::string unpacked_;
    };
}

}}}

#endif // SRPC_COMMON_TRANSPORT_DELEGATES_MESSAGE_H
