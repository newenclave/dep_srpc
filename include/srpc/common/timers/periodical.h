#ifndef SRPC_COMMON_TIMER_PERIODICAL_H
#define SRPC_COMMON_TIMER_PERIODICAL_H

#include "srpc/common/config/asio.h"
#include "srpc/common/config/system.h"
#include "srpc/common/timers/deadline.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/config/stdint.h"
#include "srpc/common/config/mutex.h"

namespace srpc { namespace common {  namespace timers {

    class  periodical {

        typedef chrono::steady_clock::duration duration_type;
        periodical ( periodical& );
        periodical &operator = ( periodical& );

        typedef srpc::function<void (const SRPC_SYSTEM::error_code &)> hdl_type;

        void handler( const SRPC_SYSTEM::error_code &err, hdl_type userhdl )
        {
            userhdl( err );
            if( !err && enabled( ) ) {
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

        void set_enabled( bool val )
        {
            srpc::lock_guard<srpc::mutex> lck(lock_);
            enabled_ = val;
        }

    public:

        typedef SRPC_SYSTEM::error_code        error_code;

        periodical( SRPC_ASIO::io_service &ios )
            :timer_(ios)
            ,enabled_(false)
        { }

        template <typename Duration>
        void set_period( Duration dur )
        {
            duration_ = srpc::chrono::duration_cast<duration_type>(dur);
        }

        srpc::uint64_t period( ) const
        {
            return duration_.count( );
        }

        void cancel( )
        {
            srpc::lock_guard<srpc::mutex> lck(lock_);
            if( enabled_ ) {
                enabled_ = false;
                timer_.cancel( );
            }
        }

        bool enabled( ) const
        {
            srpc::lock_guard<srpc::mutex> lck(lock_);
            return enabled_;
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
        void call( Handler hdl, Duration dur )
        {
            namespace ph = srpc::placeholders;
            duration_ = srpc::chrono::duration_cast<duration_type>(dur);
            timer_.expires_from_now( duration_ );
            set_enabled( true );
            hdl_type uhdl = srpc::bind( &periodical::handler, this,
                                        ph::_1, hdl );
            timer_.async_wait( uhdl );
        }

        template <typename Handler, typename Duration>
        void call( Handler hdl, Duration dur, error_code &err )
        {
            namespace ph = srpc::placeholders;
            duration_ = srpc::chrono::duration_cast<duration_type>(dur);
            timer_.expires_from_now( duration_, err );
            if( !err ) {
                set_enabled( true );
                hdl_type uhdl = srpc::bind( &periodical::handler, this,
                                            ph::_1, hdl );
                timer_.async_wait( uhdl );
            }
        }
    private:
        deadline            timer_;
        duration_type       duration_;
        mutable srpc::mutex lock_;
        bool                enabled_;
    };

}}}

#endif // PERIODICAL_H
