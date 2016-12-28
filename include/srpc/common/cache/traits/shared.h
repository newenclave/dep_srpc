#ifndef SRPC_COMMON_CHACHE_TRAITS_SHARED_H
#define SRPC_COMMON_CHACHE_TRAITS_SHARED_H

#include "srpc/common/config/memory.h"

namespace srpc { namespace common { namespace cache {

namespace traits {

    template <typename T>
    struct shared {
        typedef srpc::shared_ptr<T> value_type;

        static
        value_type create( )
        {
            return srpc::make_shared<T>( );
        }

        static
        void destroy( value_type )
        {  }

    };
}

}}}

#endif // SHARED_H
