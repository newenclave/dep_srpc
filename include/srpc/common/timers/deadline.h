#ifndef SRPC_COMMON_TIMERS_DEADLINE_H
#define SRPC_COMMON_TIMERS_DEADLINE_H

#include "srpc/common/config/chrono.h"

#ifdef ASIO_STANALONE
#include "asio/basic_waitable_timer.hpp"
#else
#include "srpc/common/timers/traits/monotonic.h"
#include "boost/asio/basic_deadline_timer.hpp"
#endif

namespace srpc { namespace common {  namespace timers {

#ifdef ASIO_STANALONE
    typedef asio::basic_waitable_timer<srpc::chrono::steady_clock> deadline;
    typedef srpc::chrono::microseconds deadline_microseconds;
    typedef srpc::chrono::milliseconds deadline_milliseconds;
    typedef srpc::chrono::seconds      deadline_seconds;
    typedef srpc::chrono::minutes      deadline_minutes;
    typedef srpc::chrono::hours        deadline_hours;
#else
    typedef boost::asio::basic_deadline_timer< srpc::chrono::steady_clock,
                                               traits::monotonic> deadline;
    typedef boost::posix_time::microseconds deadline_microseconds;
    typedef boost::posix_time::milliseconds deadline_milliseconds;
    typedef boost::posix_time::seconds      deadline_seconds;
    typedef boost::posix_time::minutes      deadline_minutes;
    typedef boost::posix_time::hours        deadline_hours;
#endif

}}}

#endif // DEADLINE_H
