#ifndef SRPC_COMMON_TIMERS_DEADLINE_H
#define SRPC_COMMON_TIMERS_DEADLINE_H

#include "srpc/common/config/chrono.h"
#include "srpc/common/config/asio.h"

namespace srpc { namespace common {  namespace timers {

    typedef SRPC_ASIO::basic_waitable_timer<chrono::steady_clock> deadline;

    typedef srpc::chrono::microseconds microseconds;
    typedef srpc::chrono::milliseconds milliseconds;
    typedef srpc::chrono::seconds      seconds;
    typedef srpc::chrono::minutes      minutes;
    typedef srpc::chrono::hours        hours;

}}}

#endif // DEADLINE_H
