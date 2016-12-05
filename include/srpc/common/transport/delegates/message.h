#ifndef SRPC_COMMON_TRANSPORT_DELEGATES_MESSAGE_H
#define SRPC_COMMON_TRANSPORT_DELEGATES_MESSAGE_H

#include "srpc/common/transport/interface.h"

namespace srpc { namespace common { namespace transport {

namespace delegates {

    template <typename SizePackPolicy>
    class message: public common::transport::interface::delegate {

        typedef message<SizePackPolicy> this_type;


    public:

        typedef SizePackPolicy size_policy;
        typedef typename size_policy::size_type size_type;

        class pack_context {
            friend class message<SizePackPolicy>;
            std::string data_;
        public:
            std::string &data( )
            {
                return data_;
            }

            void clear( )
            {
                data_.clear( );
            }
        };

        void pack_begin( pack_context &ctx, size_t len )
        {
            static const size_t max = size_policy::max_length;
            size_t old_size = ctx.data_.size( );
            ctx.data_.resize( old_size + max + 1 );
            size_t pack_size = size_policy::pack( len, &ctx.data_[old_size] );
            ctx.data_.resize( old_size + pack_size );
        }

        void pack_update( pack_context &ctx, const char *data, size_t len )
        {
            ctx.data_.insert( ctx.data_.end( ), data, data + len );
        }

        void pack_end( pack_context &ctx )
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

        void on_data( const char *data, size_t len )
        {
            unpacked_.insert( unpacked_.end( ), data, data + len );

            size_t pack_len = size_policy::size_length( unpacked_.begin( ),
                                                        unpacked_.end( ) );

            while( chck( pack_len ) && (pack_len >= size_policy::min_length) ) {

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
        }

        virtual void on_message( const char *message, size_t len ) = 0;
        virtual bool validate_length( size_t len ) = 0;
        virtual void on_error( const char *message ) = 0;
//        virtual void on_read_error( const error_code & ) = 0;
//        virtual void on_write_error( const error_code &) = 0;
//        virtual void on_close( ) = 0;

        std::string unpacked_;
    };
}

}}}

#endif // SRPC_COMMON_TRANSPORT_DELEGATES_MESSAGE_H
