#ifndef SRPC_COMMON_HASH_FACTORY_H
#define SRPC_COMMON_HASH_FACTORY_H

#include <map>

#include "srpc/common/hash/interface.h"
#include "srpc/common/config/mutex.h"
#include "srpc/common/config/functional.h"

namespace srpc { namespace common { namespace hash {
    class factory {
    public:
        srpc::function<hash::interface *( )> producer_type;
    private:
        std::map<std::string, creator_type> map_;
    };
}}}

#endif // FACTORY_H
