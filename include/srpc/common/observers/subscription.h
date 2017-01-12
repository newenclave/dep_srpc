#ifndef SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H
#define SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/observers/scoped-subscription.h"

namespace srpc { namespace common { namespace observers {

    class scoped_subscription;
    class subscription {

        friend class srpc::common::observers::scoped_subscription;

        static void unsubscribe_dummy( )
        { }

        void reset( )
        {
            unsubscriber_ = &subscription::unsubscribe_dummy;
        }

    public:

        typedef srpc::function<void ( )> void_call;

        subscription( void_call call )
            :unsubscriber_(call)
        { }

#if CXX11_ENABLED
        subscription( subscription &&o )
            :unsubscriber_(std::move(o.unsubscriber_))
        {
            o.reset( );
        }

        subscription &operator = ( subscription &&o )
        {
            unsubscriber_ = o.unsubscriber_;
            o.reset( );
            return *this;
        }

        subscription( const subscription &o ) = default;
        subscription &operator = ( const subscription &o ) = default;
#endif
        subscription( )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        { }

        void unsubscribe(  )
        {
            unsubscriber_( );
        }

        void disconnect(  )
        {
            unsubscribe( );
        }

        void swap( subscription &other )
        {
            unsubscriber_.swap( other.unsubscriber_ );
        }
    private:
        void_call   unsubscriber_;
    };

}}}

#endif // SUBSCRIPTION_H
