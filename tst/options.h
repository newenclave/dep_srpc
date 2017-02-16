#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <cstdint>
#include <memory>

#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"
#include "srpc/common/sizepack/none.h"

namespace tunnel {

    enum tag_type {
        TAG_DATA    = 0,
        TAG_COMMAND = 1,
    };

    struct options {
        std::string     device;
        std::string     address;
        std::uint16_t   port;
        bool            use_tap = false;
        bool            use_udp = false;
    };

    using my_size_policy = srpc::common::sizepack::varint<std::uint32_t>;

    template <typename PackPolicy = my_size_policy>
    struct message_packer {

        using buffer_type = std::shared_ptr<std::string>;
        using pack_policy = PackPolicy;

        static
        buffer_type pack( buffer_type mess )
        {
            const size_t pak = pack_policy::packed_length( mess->size( ) );
            auto old_size    = mess->size( );
            mess->insert( mess->begin( ), pak, '\0' );
            pack_policy::pack( old_size, &(*mess)[0] );
            return mess;
        }

    };

}

#endif // OPTIONS_H
