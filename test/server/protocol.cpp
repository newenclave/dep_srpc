#include <iostream>

#include "srpc/common/queues/condition.h"
#include "srpc/common/config/atomic.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/transport/interface.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"
#include "srpc/common/const_buffer.h"
#include "srpc/common/hash/interface.h"
#include "srpc/common/hash/crc32.h"

#include "srpc/common/result.h"

using namespace srpc::common;

typedef transport::delegates::message<sizepack::varint<size_t> > mess_delegate;

typedef srpc::shared_ptr<std::string> send_buffer_type;

template <typename MessageType>
class message_processor: public mess_delegate {

    typedef message_processor<MessageType>        this_type;
    typedef transport::interface::write_callbacks cb_type;

protected:

public:

    typedef transport::interface::error_code        error_code;
    typedef transport::interface *                  transport_ptr;
    typedef srpc::shared_ptr<transport::interface>  transport_sptr;
    typedef srpc::common::const_buffer<char>        const_buffer;

    typedef MessageType     message_type;
    typedef srpc::uint64_t  slot_key_type;
    typedef srpc::common::queues::condition<
                    slot_key_type,
                    message_type,
                    queues::traits::simple<message_type>
            > queue_type;

    typedef typename queue_type::result_enum result_enum;
    typedef typename queue_type::slot_ptr slot_ptr;

    message_processor( transport_ptr t, slot_key_type init_id )
        :next_id_(init_id)
        ,hash_(new hash::crc32)
    {
        //transport_ = t->shared_from_this( );
        //transport_->read( );
    }

    virtual ~message_processor( ) { }

    slot_key_type next_id( )
    {
        srpc::uint64_t n = (next_id_ += 2);
        return n - 2;
    }

    slot_ptr add_slot( slot_key_type id )
    {
        return queues_.add_slot( id );
    }

    void erase_slot( slot_ptr slot )
    {
        queues_.erase_slot( slot );
    }

protected:
public:
    virtual send_buffer_type buffer_alloc(  )
    {
        return srpc::make_shared<std::string>( );
    }

    transport_ptr get_transport( )
    {
        return transport_.get( );
    }

    virtual void append_message( send_buffer_type b, const message_type &m )
    {
        b->append( m );
    }

    template <typename Itr>
    bool get_next_item( Itr &begin, const Itr &end, srpc::uint64_t *res )
    {
        typedef mess_delegate::size_policy size_policy;

        size_t len;

        len = size_policy::size_length(begin, end);
        if( size_policy::valid_length(len) ) {

            *res = size_policy::unpack(begin, end);
            begin += len;
            return true;
        }
        return false;
    }

    void on_message( const char *m, size_t len )
    {
        typedef mess_delegate::size_policy size_policy;

        const char *end = m + len;

        srpc::uint64_t t1;
        srpc::uint64_t t2;

        if( !get_next_item( m, end, &t1 ) ) {
            on_error( "Tag1. Bad serialized block." );
            return;
        }

        if( !get_next_item(m, end, &t2 ) ) {
            on_error( "Tag2. Bad serialized block." );
            return;
        }

        on_message( t1, t2, const_buffer(m, end - m) );
    }

    virtual void on_message( srpc::uint64_t tag1, srpc::uint64_t tag2,
                             const_buffer message )
    {

    }

    const_buffer pack_message( send_buffer_type buf,
                               srpc::uint64_t tag1, srpc::uint64_t tag2,
                               const message_type &mess )
    {
        typedef mess_delegate::size_policy size_policy;

        const size_t size_max = size_policy::max_length;

        buf->resize( size_policy::max_length );

        size_policy::append( tag1, *buf );
        size_policy::append( tag2, *buf );

        append_message( buf, mess );
        buf->resize( buf->size( ) + hash_->length( ) );
        hash_->get( buf->c_str( ) + size_max,
                    buf->size( )  - size_max - hash_->length( ),
                    &(*buf)[buf->size( ) - hash_->length( )]);

        size_t packed = size_policy::packed_length( buf->size( ) - size_max );
        size_policy::pack( buf->size( ) - size_max,
                          &(*buf)[size_max - packed]);

        return const_buffer( buf->c_str( ) + size_max - packed,
                             buf->size( ) - size_max + packed );

    }

    bool validate_length( size_t len )
    {
        return true;
    }

    void on_error( const char * /*message*/ )
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
    {
        // transport_.reset( );
    }

    bool push_to_slot( slot_key_type id, const message_type &msg )
    {
        return queues_.push_to_slot( id, msg ) == result_enum::OK;
    }

private:

    srpc::atomic<slot_key_type> next_id_;
    queue_type                  queues_;
    transport_sptr              transport_;
    hash::interface_uptr        hash_;
};

using sizepol = sizepack::varint<size_t>;
using sizepol0 = sizepack::fixint<size_t>;

std::string pack( const std::string &data, size_t tag1, size_t tag2 )
{
    size_t size_max = sizepol::max_length;

    std::string res;
    res.resize( size_max );
    sizepol::append( tag1, res );
    sizepol::append( tag2, res );
    res.append( data.begin( ), data.end( ) );

    size_t packed = sizepol::packed_length( res.size( ) - size_max );
    sizepol::pack(res.size( ) - size_max, &res[size_max - packed]);

    size_t full_size = res.size( ) - size_max + packed;

    char *begin = &res[size_max - packed];
    std::string tmp(begin, full_size);

    for( auto &d: tmp ) {
        std::cout << std::hex
                  << (std::uint32_t)(unsigned char)(d)
                  << " ";
    }

    std::cout << "\n";
    return tmp;
}

using uint64_result = result<srpc::uint64_t, bool>;

template <typename Itr>
uint64_result get_next( Itr &begin, const Itr &end )
{
    typedef mess_delegate::size_policy size_policy;

    size_t len;
    srpc::uint64_t res;
    len = size_policy::size_length(begin, end);
    if( size_policy::valid_length(len) ) {

        res = size_policy::unpack(begin, end);
        begin += len;

        return uint64_result::ok(res);
    }

    return uint64_result::fail(false);
}

void unpack( const char *m, size_t len )
{
    typedef mess_delegate::size_policy size_policy;

    const char *end = m + len;

    uint64_result t1 = get_next(m, end);
    if( !t1 ) {
        std::cout << "Bad serialized block\n";
        return;
    }

    uint64_result t2 = get_next(m, end);
    if( !t2 ) {
        std::cout << "Bad serialized block\n";
        return;
    }

    std::cout << std::dec << "t1 " << *t1 << " t2 " << *t2 << "\n";
    std::cout << std::string(m, end) << "\n";
}

int main( int argc, char *argv[ ] )
{

    try {

        message_processor<std::string> msg(NULL, 100);

        auto b = std::make_shared<std::string>( );

        auto rr = msg.pack_message( b, 1, 1, "1234567980" );

        std::cout << rr.size( ) << "\n";

    } catch ( const std::exception &ex ) {
        std::cout << "Error " << ex.what( ) << "\n";
    }
}
