#ifndef SRPC_COMMON_TIMER_TRAITS_MONO_H
#define SRPC_COMMON_TIMER_TRAITS_MONO_H

#include "srpc/common/config/chrono.h"

#ifndef ASIO_STANALONE
#include "boost/date_time/posix_time/posix_time.hpp"
#endif
namespace srpc { namespace common {  namespace timers {

namespace traits {

#ifndef ASIO_STANALONE
    struct monotonic {

        typedef srpc::chrono::milliseconds milliseconds;
        typedef srpc::chrono::microseconds microseconds;
        typedef srpc::chrono::seconds seconds;
        typedef srpc::chrono::minutes minutes;
        typedef srpc::chrono::hours hours;

        typedef srpc::chrono::steady_clock::time_point  time_type;
        typedef boost::posix_time::time_duration        duration_type;

        static time_type now( )
        {
            return srpc::chrono::steady_clock::now( );
        }

        static time_type add( const time_type& time,
                              const duration_type& duration )
        {
            return time + microseconds( duration.total_microseconds( ) );
        }

        static duration_type subtract( const time_type& timeLhs,
                                       const time_type& timeRhs )
        {
          microseconds duration_us (
                srpc::chrono::duration_cast<microseconds>(timeLhs - timeRhs));
          return boost::posix_time::microseconds( duration_us.count( ) );
        }

        static bool less_than( const time_type& timeLhs,
                               const time_type& timeRhs )
        {
            return timeLhs < timeRhs;
        }

        static duration_type to_posix_duration( const duration_type& duration )
        {
            return duration;
        }
    };
#endif
}

}}}

#endif // SRPC_COMMON_TIMER_TRAITS_MONO_H
