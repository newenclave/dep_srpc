#ifndef SRPC_COMMON_TIMERS_TYPES_H
#define SRPC_COMMON_TIMERS_TYPES_H

#include "srpc/common/config/chrono.h"

namespace srpc { namespace common {  namespace timers {
    typedef srpc::chrono::nanoseconds   nanoseconds;
    typedef srpc::chrono::microseconds  microseconds;
    typedef srpc::chrono::milliseconds  milliseconds;
    typedef srpc::chrono::seconds       seconds;
    typedef srpc::chrono::minutes       minutes;
    typedef srpc::chrono::hours         hours;
}}}

#endif // TYPES_H
