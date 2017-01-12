#ifndef SRPC_COMMON_OBSERVERS_SCOPED_SUBSCRIPTION_H
#define SRPC_COMMON_OBSERVERS_SCOPED_SUBSCRIPTION_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/observers/subscription.h"

namespace srpc { namespace common { namespace observers {

    class scoped_subscription {

        typedef srpc::shared_ptr<void>  void_sptr;
        typedef srpc::weak_ptr<void>    void_wptr;
        typedef srpc::function<void( )> void_call;

        static void unsubscribe_dummy( )
        { }

    public:

#if CXX11_ENABLED
        scoped_subscription( scoped_subscription &&o )
            :unsubscriber_(&scoped_subscription::unsubscribe_dummy)
        {
            unsubscriber_   = o.unsubscriber_;
            o.unsubscriber_ = &scoped_subscription::unsubscribe_dummy;
        }

        scoped_subscription &operator = ( scoped_subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.unsubscriber_ = &scoped_subscription::unsubscribe_dummy;
            return          *this;
        }

        scoped_subscription( observers::subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.unsubscriber_ = &scoped_subscription::unsubscribe_dummy;
        }

        scoped_subscription & operator = ( subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.unsubscriber_ = &scoped_subscription::unsubscribe_dummy;
            return          *this;
        }
#endif
        scoped_subscription( scoped_subscription &o )
            :unsubscriber_(&scoped_subscription::unsubscribe_dummy)
        {
            o.unsubscriber_ = &scoped_subscription::unsubscribe_dummy;
        }

        scoped_subscription &operator = ( scoped_subscription &o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.unsubscriber_ = &scoped_subscription::unsubscribe_dummy;
            return          *this;
        }

        scoped_subscription & operator = ( const subscription &o )
        {
            unsubscriber_   = o.unsubscriber_;
        }

        scoped_subscription( )
            :unsubscriber_(&scoped_subscription::unsubscribe_dummy)
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

        void swap( subscription &other )
        {
            unsubscriber_.swap( other.unsubscriber_ );
        }
    private:
        void_call   unsubscriber_;
    };

}}}

#endif // SCOPEDSUBSCRIPTION_H
