#ifndef SRPC_COMMON_HASH_INTERFACE_H
#define SRPC_COMMON_HASH_INTERFACE_H

#include <string>
#include "srpc/common/config/stdint.h"
#include "srpc/common/config/memory.h"

namespace srpc { namespace common { namespace hash {

    struct interface {
        virtual ~interface( ) { }
        virtual size_t length( ) const = 0;
        virtual void get(   const char *data, size_t len, char *out ) = 0;
        virtual bool check( const char *data, size_t len, const char *res ) = 0;
        virtual std::string name( ) const = 0;
    };

    typedef interface *                 interface_ptr;
    typedef srpc::shared_ptr<interface> interface_sptr;
    typedef srpc::unique_ptr<interface> interface_uptr;

}}}


#endif // INTERFACE_H
