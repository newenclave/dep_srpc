#include <iostream>

#include "srpc/common/queues/condition.h"
#include "srpc/common/config/atomic.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/transport/interface.h"
#include "srpc/common/sizepack/varint.h"

using namespace srpc::common;

typedef transport::delegates::message<sizepack::varint<size_t> > mess_delegate;

template <typename MessageType>
class message_processor: public mess_delegate {

public:

    typedef transport::interface::error_code        error_code;
    typedef transport::interface *                  transport_ptr;
    typedef srpc::shared_ptr<transport::interface>  transport_sptr;

    typedef MessageType     message_type;
    typedef srpc::uint64_t  slot_key_type;
    typedef srpc::common::queues::condition<
                    slot_key_type,
                    message_type,
                    srpc::common::queues::traits::simple<message_type>
            > queue_type;

    typedef typename queue_type::result_enum result_enum;
    typedef typename queue_type::slot_ptr slot_ptr;

    message_processor( transport_ptr t, slot_key_type init_id )
        :next_id_(init_id)
    {
        transport_ = t->shared_from_this( );
    }

    virtual
    ~message_processor( ) { }

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

    transport_ptr get_transport( )
    {
        return transport_.get( );
    }

protected:

    void on_message( const char *message, size_t len )
    { }

    bool validate_length( size_t len )
    {
        return true;
    }

    void on_error( const char *message )
    {

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

    }

    void on_close( )
    {
        //transport_
    }

    bool push_to_slot( slot_key_type id, const message_type &msg )
    {
        return queues_.push_to_slot( id, msg ) == result_enum::OK;
    }

private:

    srpc::atomic<slot_key_type> next_id_;
    queue_type                  queues_;
    transport_sptr              transport_;
};

int main( int argc, char *argv[ ] )
{
    try {


    } catch ( const std::exception &ex ) {
        std::cout << "Error " << ex.what( ) << "\n";
    }
}
