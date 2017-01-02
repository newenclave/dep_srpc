#ifndef SRPC_COMMON_PROTOCOL_NONAME_H
#define SRPC_COMMON_PROTOCOL_NONAME_H

#include "srpc/common/protocol/binary.h"
#include "srpc/common/protocol/lowlevel.pb.h"

namespace srpc { namespace common { namespace protocol {

    class noname: public binary<srpc::shared_ptr<srpc::rpc::lowlevel>,
                                sizepack::none,
                                sizepack::varint<srpc::uint32_t>,
                                srpc::uint64_t>
    {
        typedef binary< srpc::shared_ptr<srpc::rpc::lowlevel>,
                        sizepack::none,
                        sizepack::varint<srpc::uint32_t>,
                        srpc::uint64_t > parent_type;
    public:
        noname( bool server, size_t max_length )
            :parent_type(server ? 100 : 101, max_length)
        { }
    protected:

        void append_message( buffer_type buff, const message_type &mess )
        {
            mess->AppendToString( *buff );
        }

    };

}}}

#endif // NONAME_H
