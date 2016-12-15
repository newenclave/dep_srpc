#ifndef SRPC_COMMON_TIMERS_TICKS_H
#define SRPC_COMMON_TIMERS_TICKS_H

#include "srpc/common/config/stdint.h"
#include "srpc/common/config/chrono.h"

namespace srpc { namespace common {  namespace timers {

    template <typename DurationType = srpc::chrono::steady_clock::duration>
    struct ticks {
        typedef DurationType duration_type;
        static srpc::uint64_t now( )
        {
            typedef duration_type dt;
            using   srpc::chrono::duration_cast;
            using   srpc::chrono::high_resolution_clock;
            return  duration_cast<dt>( high_resolution_clock::now( )
                                      .time_since_epoch( ) ).count( );
        }
    };
}}}

#endif // TICKS_H
