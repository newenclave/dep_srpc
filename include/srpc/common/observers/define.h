#ifndef SRPC_COMMON_OBSERVERS_DEFINE_H
#define SRPC_COMMON_OBSERVERS_DEFINE_H

#include "srpc/common/observers/common.h"
#include "srpc/common/observers/simple.h"
#include "srpc/common/config/mutex.h"

#define SRPC_OBSERVER_DEFINE_COMMON( Name, Sig, Visible, Mutex )            \
    public:                                                                 \
        typedef srpc::common::observers::simple<Sig, Mutex> Name##_type;    \
        typedef typename Name##_type::subscription                          \
                         Name##_subscription;                               \
        typedef typename Name##_type::slot_type                             \
                         Name##_slot_type;                                  \
        typedef typename Name##_type::scoped_subscription                   \
                         Name##_scoped_subscription;                        \
        Name##_subscription subscribe_##Name( Name##_slot_type slot )       \
        {                                                                   \
            return Name.subscribe( slot );                                  \
        }                                                                   \
    Visible:                                                                \
        Name##_type Name

#define SRPC_OBSERVER_DEFINE_SAFE( Name, Sig ) \
    SRPC_OBSERVER_DEFINE_COMMON( Name, Sig, protected, srpc::recursive_mutex )

#define SRPC_OBSERVER_DEFINE_UNSAFE( Name, Sig ) \
    SRPC_OBSERVER_DEFINE_COMMON( Name, Sig, protected, srpc::dummy_mutex )

#define SRPC_OBSERVER_DEFINE SRPC_OBSERVER_DEFINE_SAFE

#endif // SRPC_COMMON_OBSERVERS_DEFINE_H
