#ifndef SRPC_COMMON_SIZEPACK_TYPES_H
#define SRPC_COMMON_SIZEPACK_TYPES_H

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common { namespace sizepack {

    template <int BITS>
    struct types;

    template<>
    struct types<sizeof(srpc::uint8_t)> {
        typedef srpc::int8_t        signed_type;
        typedef srpc::uint8_t       unsigned_type;
        static const unsigned_type  top_bit         = 0x80;
        static const unsigned_type  top_bit_shift   = 7;

    };

    template<>
    struct types<sizeof(srpc::uint16_t)> {
        typedef srpc::int16_t       signed_type;
        typedef srpc::uint16_t      unsigned_type;
        static const unsigned_type  top_bit         = 0x8000;
        static const unsigned_type  top_bit_shift   = 15;
    };

    template<>
    struct types<sizeof(srpc::uint32_t)> {
        typedef srpc::int32_t       signed_type;
        typedef srpc::uint32_t      unsigned_type;
        static const unsigned_type  top_bit         = 0x80000000;
        static const unsigned_type  top_bit_shift   = 31;
    };

    template<>
    struct types<sizeof(srpc::uint64_t)> {
        typedef srpc::int64_t       signed_type;
        typedef srpc::uint64_t      unsigned_type;
        static const unsigned_type  top_bit         = 0x8000000000000000ul;
        static const unsigned_type  top_bit_shift   = 63;
    };

}}}

#endif // TYPES_H
