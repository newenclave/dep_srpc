#ifndef SRPC_COMMON_TIMER_ONCE_H
#define SRPC_COMMON_TIMER_ONCE_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/system.h"
#include "srpc/common/timers/deadline.h"

namespace srpc { namespace common {  namespace timers {

namespace calls {

    class once { // !periodical

        deadline timer_;
        once( const once & );
        once &operator = ( const once & );

    public:

        typedef SRPC_SYSTEM::error_code error_code;

        once( SRPC_ASIO::io_service &ios )
            :timer_(ios)
        { }

        void cancel( )
        {
            timer_.cancel( );
        }

        deadline &timer( )
        {
            return timer_;
        }

        const deadline &timer( ) const
        {
            return timer_;
        }

        template <typename Handler, typename Duration>
        void call( Handler hdl, const Duration &dur )
        {
            timer_.expires_from_now( dur );
            timer_.async_wait( hdl );
        }

        template <typename Handler, typename Duration>
        void call( Handler hdl, const Duration &dur, error_code &err )
        {
            timer_.expires_from_now( dur, err );
            timer_.async_wait( hdl );
        }

    };
}

}}}

#endif // ONCE_H
