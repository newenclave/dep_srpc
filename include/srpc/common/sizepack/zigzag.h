#ifndef SRPC_COMMON_SIZEPACK_ZIGZAG_H
#define SRPC_COMMON_SIZEPACK_ZIGZAG_H

#include "srpc/common/sizepack/types.h"

namespace srpc { namespace common { namespace sizepack {


    template <typename SignType>
    struct zigzag {

        typedef SignType value_type;
        typedef types<sizeof(value_type)> type_trait;
        typedef typename type_trait::signed_type   signed_type;
        typedef typename type_trait::unsigned_type unsigned_type;

        static
        unsigned_type to_unsigned( signed_type val )
        {
            unsigned_type p = *reinterpret_cast<unsigned_type *>(&val);
            p = (p & type_trait::top_bit) ? ((~(p << 1)) | 1) : (p << 1);
            return p;
        }

        static
        signed_type to_signed( unsigned_type val )
        {
            val = (val & 1) ? ~(val >> 1) : (val >> 1);
            return *reinterpret_cast<signed_type *>(&val);
        }

    };

}}}

#endif // ZIGZAG_H
