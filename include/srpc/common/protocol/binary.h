#ifndef SRPC_COMMON_PROTOCOL_DEFAULT_H
#define SRPC_COMMON_PROTOCOL_DEFAULT_H

#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"
#include "srpc/common/sizepack/none.h"
#include "srpc/common/queues/condition.h"
#include "srpc/common/hash/crc32.h"

#include "srpc/common/transport/interface.h"
#include "srpc/common/buffer.h"
#include "srpc/common/const_buffer.h"

#include "srpc/common/config/memory.h"
#include "srpc/common/config/stdint.h"
#include "srpc/common/config/atomic.h"

namespace srpc { namespace common { namespace protocol {

    template < typename MessageType,
               typename TagPolicy   = sizepack::none,
               typename SizePolicy  = sizepack::varint<srpc::uint32_t>,
               typename QueueIdType = srpc::uint64_t >
    class binary: public transport::delegates::message<SizePolicy> {

        typedef transport::delegates::message<SizePolicy>  parent_type;

    public:

        typedef srpc::shared_ptr<std::string>              buffer_type;
        typedef transport::interface::error_code           error_code;
        typedef common::buffer<char>                       buffer_slice;
        typedef common::const_buffer<char>                 const_buffer_slice;

        typedef transport::interface *                     transport_ptr;
        typedef srpc::weak_ptr<transport::interface>       transport_wptr;
        typedef srpc::shared_ptr<transport::interface>     transport_sptr;

        typedef SizePolicy                                 size_policy;
        typedef TagPolicy                                  tag_policy;
        typedef typename tag_policy::size_type             tag_type;
        typedef MessageType                                message_type;
        typedef QueueIdType                                key_type;

        typedef srpc::common::queues::condition<
                        key_type,
                        message_type,
                        queues::traits::simple<message_type>
                > queue_type;

        typedef typename queue_type::result_enum           result_enum;
        typedef typename queue_type::slot_ptr              slot_ptr;

        binary( key_type init_id, size_t max_length )
            :next_id_(init_id)
            ,hash_(new hash::crc32)
            ,max_length_(max_length)
            ,transport_(nullptr)
        { }

        virtual ~binary( )
        { }

        key_type next_id( )
        {
            srpc::uint64_t n = (next_id_ += 2);
            return n - 2;
        }

        slot_ptr add_slot( key_type id )
        {
            return queues_.add_slot( id );
        }

        void erase_slot( slot_ptr slot )
        {
            queues_.erase_slot( slot );
        }

        void assign_transport( transport_ptr t )
        {
            transport_ = t->shared_from_this( );
            transport_->resize_buffer( max_length_ );
        }

        transport_ptr get_transport( )
        {
            return transport_.get( );
        }

        void set_max_length( size_t len )
        {
            max_length_ = len;
        }

        void max_length( ) const
        {
            return max_length_;
        }

        hash::interface_ptr hash( )
        {
            return hash_.get( );
        }

        const hash::interface_ptr hash( ) const
        {
            return hash_.get( );
        }

    protected:

        template <typename Policy, typename Itr>
        static
        bool get_next_item( Itr &begin, const Itr &end,
                            typename Policy::size_type *res )
        {
            typedef Policy size_policy;

            size_t len = size_policy::size_length(begin, end);

            if( size_policy::valid_length(len) ) {
                *res = size_policy::unpack(begin, begin + len);
                begin += len;
                return true;
            } else {
                return false;
            }
        }

        virtual buffer_type unpack_message( const_buffer_slice & )
        {
            return buffer_type( );
        }

        virtual buffer_slice pack_message( buffer_type, buffer_slice slice )
        {
            return slice;
        }

        void on_message( const char *m, size_t len )
        {
            typedef typename parent_type::size_policy size_policy;

            const_buffer_slice slice( m, len );
            buffer_type unpacked = unpack_message( slice );

            m   = slice.data( );
            len = slice.size( );

            const char *end        = m + len;
            const size_t hash_size = hash_->length( );

            size_t mess_len = end - m;

            if( mess_len >= hash_size ) {
                const char * checksum = m + mess_len - hash_size;
                if( !hash_->check( m, mess_len - hash_size, checksum ) ) {
                    on_error( "Checksum. Bad serialized block." );
                    return;
                }
            } else {
                on_error( "Bad length. Bad serialized block." );
                return;
            }

            tag_type tag = tag_type( );

            if( !get_next_item<tag_policy>( m, end, &tag ) ) {
                on_error( "Tag. Bad serialized block." );
                return;
            }

            on_message_ready( tag, unpacked,
                              const_buffer_slice(m, mess_len - hash_size) );
        }

        static
        buffer_slice insert_prefix( buffer_type buf, buffer_slice dst,
                                    const_buffer_slice src )
        {
            const size_t cap = dst.data( ) - buf->c_str( );
            const size_t pak = src.size( );

            if( cap >= pak ) {
                std::copy( src.begin( ), src.end( ), &(*buf)[cap - pak] );
                return buffer_slice( &(*buf)[cap - pak], src.size( ) + pak );
            } else {
                buf->insert( buf->begin( ), src.begin( ), src.end( ) );
                return buffer_slice( &(*buf)[0], src.size( ) + pak );
            }
        }

        static
        buffer_slice insert_size_prefix( buffer_type buf, buffer_slice slice )
        {
            typedef typename parent_type::size_policy size_policy;
            const size_t cap = slice.data( ) - buf->c_str( );
            const size_t pak = size_policy::packed_length( slice.size( ) );
            if( cap >= pak ) {
                size_policy::pack( slice.size( ), slice.data( ) - pak );
                return buffer_slice( slice.data( ) - pak,
                                     slice.size( ) + pak );
            } else {
                buf->insert( buf->begin( ), pak - cap, '\0' );
                size_policy::pack( slice.size( ), &(*buf)[0] );
                return buffer_slice( &(*buf)[0], slice.size( ) + pak );
            }
        }

        buffer_slice prepare_buffer( buffer_type buf, tag_type tag,
                                     const message_type &mess )
        {
            typedef typename parent_type::size_policy size_policy;

            //buf->resize( buf->size( ) + size_policy::max_length );

            const size_t old_len = buf->size( );

            tag_policy::append( tag, *buf );

            append_message( buf, mess );

            buf->resize( buf->size( ) + hash_->length( ) );

            hash_->get( buf->c_str( ) + old_len,
                        buf->size( )  - old_len - hash_->length( ),
                        &(*buf)[buf->size( )    - hash_->length( )]);

            buffer_slice res( &(*buf)[0]   + old_len,
                              buf->size( ) - old_len );

            return pack_message( buf, res );

        }

        virtual void on_message_ready( tag_type, buffer_type,
                                       const_buffer_slice )
        { }

        virtual void append_message( buffer_type, const message_type & )
        { }

        virtual bool validate_length( size_t len )
        {
            return len <= max_length_;
        }

        virtual void on_error( const char *mess )
        {
            transport_->close( );
        }

        virtual void on_need_read( )
        {
            transport_->read( );
        }

        virtual void on_read_error( const error_code & )
        {
            transport_->close( );
        }

        virtual void on_write_error( const error_code &)
        {
            transport_->close( );
        }

        virtual void on_close( )
        {
            //// ?
            /// transport_.reset( );
        }

        bool push_to_slot( key_type id, const message_type &msg )
        {
            return queues_.push_to_slot( id, msg ) == result_enum::OK;
        }

        queue_type &message_queue( )
        {
            return queues_;
        }

        const queue_type &message_queue( ) const
        {
            return queues_;
        }

    private:

        srpc::atomic<key_type>  next_id_;
        queue_type              queues_;
        hash::interface_uptr    hash_;
        size_t                  max_length_;
        transport_sptr          transport_;

    };

}}}

#endif // DEFAULT_H
