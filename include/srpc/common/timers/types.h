#ifndef SRPC_COMMON_TIMERS_TYPES_H
#define SRPC_COMMON_TIMERS_TYPES_H

#ifdef ASIO_STANALONE
#include "asio/basic_waitable_timer.hpp"
#else
#include "boost/asio/basic_deadline_timer.hpp"
#endif

#include "srpc/common/config/chrono.h"

namespace srpc { namespace common {  namespace timers {
#ifdef ASIO_STANALONE
    typedef srpc::chrono::nanoseconds   nanoseconds;
    typedef srpc::chrono::microseconds  microseconds;
    typedef srpc::chrono::milliseconds  milliseconds;
    typedef srpc::chrono::seconds       seconds;
    typedef srpc::chrono::minutes       minutes;
    typedef srpc::chrono::hours         hours;
#else

#endif

}}}

#endif // TYPES_H
