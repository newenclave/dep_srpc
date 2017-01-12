#ifndef SRPC_COMMON_OBSERVERS_SCOPED_SUBSCRIPTION_H
#define SRPC_COMMON_OBSERVERS_SCOPED_SUBSCRIPTION_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/observers/subscription.h"

namespace srpc { namespace common { namespace observers {

    class scoped_subscription {

        typedef subscription::void_call void_call;

        void reset( )
        {
            unsubscriber_ = &subscription::unsubscribe_dummy;
        }

    public:

#if CXX11_ENABLED
        scoped_subscription( scoped_subscription &&o )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        {
            unsubscriber_   = o.unsubscriber_;
            o.reset( );
        }

        scoped_subscription &operator = ( scoped_subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.reset( );
            return          *this;
        }

        scoped_subscription( observers::subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.reset( );
        }

        scoped_subscription & operator = ( subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.reset( );
            return          *this;
        }
#endif
        scoped_subscription( scoped_subscription &o )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        {
            o.reset( );
        }

        scoped_subscription &operator = ( scoped_subscription &o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.reset( );
            return          *this;
        }

        scoped_subscription & operator = ( const subscription &o )
        {
            unsubscriber_   = o.unsubscriber_;
        }

        scoped_subscription( )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        { }

        ~scoped_subscription( )
        {
            unsubscribe( );
        }

        scoped_subscription( const subscription &ss )
        { }

        void unsubscribe(  )
        {
            unsubscriber_( );
        }

        void disconnect(  )
        {
            unsubscribe( );
        }

        subscription release( )
        {
            subscription tmp;
            unsubscriber_.swap( tmp.unsubscriber_ );
            return tmp;
        }

        void swap( subscription &other )
        {
            unsubscriber_.swap( other.unsubscriber_ );
        }
    private:
        void_call   unsubscriber_;
    };

}}}

#endif // SCOPEDSUBSCRIPTION_H
