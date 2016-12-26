#ifndef SRPC_COMMON_OBSERVERS_SIMPLE_H
#define SRPC_COMMON_OBSERVERS_SIMPLE_H

#include "srpc/common/observers/traits/simple.h"
#include "srpc/common/observers/base.h"

namespace srpc { namespace common { namespace observers {

#if CXX11_ENABLED
    template <typename T, typename MutexType = srpc::mutex>
    using simple = base<traits::simple<T>, MutexType>;
#else
    template <typename T,
              typename MutexType = srpc::mutex>
    class simple: public base<traits::simple<T>, MutexType> {
        typedef common<traits::simple<T>, MutexType> parent_type;
    public:
        typedef typename parent_type::slot_type  slot_type;
        typedef typename parent_type::connection connection;
    };
#endif
}}}
#endif // SIMPLE_H
