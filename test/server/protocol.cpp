#include <iostream>

#include "srpc/common/queues/condition.h"
#include "srpc/common/config/atomic.h"

template <typename MessageType>
class protocol_processor {

public:

    typedef MessageType     message_type;
    typedef srpc::uint64_t  slot_key_type;
    typedef srpc::common::queues::condition<
                    slot_key_type,
                    message_type,
                    srpc::common::queues::traits::simple<message_type>
            > queue_type;

    typedef typename queue_type::result_enum result_enum;
    typedef typename queue_type::slot_ptr slot_ptr;

    protocol_processor( slot_key_type init_id )
        :next_id_(init_id)
    { }

    virtual ~protocol_processor( ) { }

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

    virtual void send_message( const message_type &msg )
    { }

    virtual void close( )
    { }

protected:

    virtual on_message( const char *, size_t )
    { }

    bool push_to_slot( slot_key_type id, const message_type &msg )
    {
        return queues_.push_to_slot( id, msg ) == result_enum::OK;
    }

private:

    srpc::atomic<slot_key_type> next_id_;
    queue_type                  queues_;
};

int main( int argc, char *argv[ ] )
{
    try {

        protocol_processor<std::string> pp(100);

    } catch ( const std::exception &ex ) {
        std::cout << "Error " << ex.what( ) << "\n";
    }
}
