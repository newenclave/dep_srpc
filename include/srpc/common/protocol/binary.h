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

    template <typename MessageType,
              typename SizePolicy  = sizepack::varint<srpc::uint32_t>,
              typename QueueIdType = srpc::uint64_t,
              typename TagPolicy   = sizepack::none>
    class binary: public transport::delegates::message<SizePolicy> {

        typedef transport::delegates::message<SizePolicy> parent_type;

    protected:

        typedef srpc::shared_ptr<std::string>           buffer_type;

    public:

        typedef transport::interface::error_code        error_code;
        typedef common::buffer<char>                    buffer_slice;
        typedef common::const_buffer<char>              const_buffer_slice;

        typedef transport::interface *                  transport_ptr;
        typedef srpc::weak_ptr<transport::interface>    transport_wptr;
        typedef srpc::shared_ptr<transport::interface>  transport_sptr;

        typedef SizePolicy                              size_policy;
        typedef TagPolicy                               tag_policy;
        typedef typename tag_policy::size_type          tag_type;
        typedef MessageType                             message_type;
        typedef QueueIdType                             key_type;

        typedef srpc::common::queues::condition<
                        key_type,
                        message_type,
                        queues::traits::simple<message_type>
                > queue_type;

        typedef typename queue_type::result_enum        result_enum;
        typedef typename queue_type::slot_ptr           slot_ptr;

        binary( key_type init_id )
            :next_id_(init_id)
            ,hash_(new hash::crc32)
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
        }

        transport_ptr get_transport( )
        {
            return transport_.get( );
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

        virtual const_buffer_slice pack_message( buffer_type,
                                                 const_buffer_slice slice )
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
                              const_buffer_slice(m, (end - m) - hash_size) );
        }

        const_buffer_slice prepare_buffer( buffer_type buf, tag_type tag,
                                           const message_type &mess,
                                           size_t *length_size = NULL )
        {
            typedef typename parent_type::size_policy size_policy;

            buf->resize( buf->size( ) + size_policy::max_length );

            const size_t old_len = buf->size( );

            tag_policy::append( tag, *buf );

            append_message( buf, mess );

            buf->resize( buf->size( ) + hash_->length( ) );

            hash_->get( buf->c_str( ) + old_len,
                        buf->size( ) - old_len - hash_->length( ),
                        &(*buf)[buf->size( ) - hash_->length( )]);

            size_t packed = 0;

            if( length_size ) {
                packed = size_policy::packed_length( buf->size( ) - old_len );
                size_policy::pack( buf->size( ) - old_len,
                                &(*buf)[old_len - packed]);
                *length_size = packed;
            }

            const_buffer_slice res( buf->c_str( ) + old_len - packed,
                                    buf->size( )  - old_len + packed );

            return pack_message( buf, res );

        }

        virtual void on_message_ready( tag_type, buffer_type,
                                       const_buffer_slice )
        { }

        virtual buffer_type buffer_alloc(  )
        {
            return srpc::make_shared<std::string>( );
        }

        virtual void buffer_free( buffer_type )
        { }

        virtual void append_message( buffer_type, const message_type & )
        { }

        bool validate_length( size_t )
        {
            return true;
        }

        virtual void on_error( const char * )
        {
            transport_->close( );
        }

        void on_need_read( )
        {
            transport_->read( );
        }

        void on_read_error( const error_code & )
        {
            transport_->close( );
        }

        void on_write_error( const error_code &)
        {
            transport_->close( );
        }

        void on_close( )
        { }

        bool push_to_slot( key_type id, const message_type &msg )
        {
            return queues_.push_to_slot( id, msg ) == result_enum::OK;
        }

    private:

        srpc::atomic<key_type>  next_id_;
        queue_type              queues_;
        transport_sptr          transport_;
        hash::interface_uptr    hash_;

    };

}}}

#endif // DEFAULT_H