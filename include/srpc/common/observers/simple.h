#ifndef SRPC_COMMON_OBSERVERS_SIMPLE_H
#define SRPC_COMMON_OBSERVERS_SIMPLE_H

#include "srpc/common/observers/traits/simple.h"
#include "srpc/common/observers/common.h"

namespace srpc { namespace common { namespace observers {

#if CXX11_ENABLED == 1
    template <typename T,
              typename MutexType = srpc::recursive_mutex>
    class simple: public common<traits::simple<T>, MutexType> {
        typedef common<traits::simple<T>, MutexType> parent_type;
    public:
        typedef typename parent_type::slot_type  slot_type;
        typedef typename parent_type::connection connection;
    };
#else
    template <typename T, typename MutexType = srpc::recursive_mutex>
    using simple = common<traits::simple<T>, MutexType>;
#endif
}}}
#endif // SIMPLE_H
