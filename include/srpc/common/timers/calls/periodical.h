#ifndef SRPC_COMMON_TIMER_PERIODICAL_H
#define SRPC_COMMON_TIMER_PERIODICAL_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/system.h"
#include "srpc/common/timers/deadline.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/config/stdint.h"

namespace srpc { namespace common {  namespace timers {

namespace calls {

    template <typename PeriodType>
    class  periodical {

        deadline    timer_;
        PeriodType  duration_;
        bool        enabled_;

        periodical ( periodical& );
        periodical &operator = ( periodical& );

        typedef srpc::function<void (const SRPC_SYSTEM::error_code &)> hdl_type;

        void handler( const SRPC_SYSTEM::error_code &err, hdl_type userhdl )
        {
            userhdl( err );
            if( !err && enabled_ ) {
                call_impl( userhdl );
            }
        }

        void call_impl( hdl_type &hdl )
        {
            namespace ph = srpc::placeholders;
            SRPC_SYSTEM::error_code err;
            timer_.expires_from_now( duration_, err );
            if( !err ) {
                hdl_type uhdl = srpc::bind( &periodical::handler, this,
                                            ph::_1, hdl );
                timer_.async_wait( uhdl );
            } else {
                hdl( err );
            }
        }

    public:

        typedef PeriodType period_type;
        typedef SRPC_SYSTEM::error_code error_code;

        periodical( SRPC_ASIO::io_service &ios, srpc::uint64_t period )
            :timer_(ios)
            ,duration_(period_type(period))
            ,enabled_(false)
        { }

        void set_period( srpc::uint64_t val )
        {
            duration_ = period_type(val);
        }

        srpc::uint64_t period( ) const
        {
            return duration_.count( );
        }

        void cancel( )
        {
            if( enabled_ ) {
                enabled_ = false;
                timer_.cancel( );
            }
        }

        deadline &timer( )
        {
            return timer_;
        }

        const deadline &timer( ) const
        {
            return timer_;
        }

        template <typename Handler>
        void call( Handler hdl )
        {
            namespace ph = srpc::placeholders;
            timer_.expires_from_now( duration_ );
            enabled_ = true;
            hdl_type uhdl = srpc::bind( &periodical::handler, this,
                                        ph::_1, hdl );
            timer_.async_wait( uhdl );
        }

        template <typename Handler>
        void call( Handler hdl, error_code &err )
        {
            namespace ph = srpc::placeholders;
            timer_.expires_from_now( duration_, err );
            if( !err ) {
                enabled_ = true;
                hdl_type uhdl = srpc::bind( &periodical::handler, this,
                                            ph::_1, hdl );
                timer_.async_wait( uhdl );
            }
        }

    };
}

}}}

#endif // PERIODICAL_H
