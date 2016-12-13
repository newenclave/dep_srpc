#ifndef SRPC_COMMON_HASH_NONE_H
#define SRPC_COMMON_HASH_NONE_H

#include "srpc/common/hash/interface.h"

namespace srpc { namespace common { namespace hash {

    struct none: public hash::interface {
        size_t length( ) const
        {
            return 0;
        }

        std::string name( ) const
        {
            return "none";
        }

        void get( const char *, size_t, char * )
        { }

    };

}}}

#endif // NONE_H
