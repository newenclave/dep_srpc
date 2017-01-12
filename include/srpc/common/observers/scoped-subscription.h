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
            void_call tmp = &subscription::unsubscribe_dummy;
            unsubscriber_.swap(tmp);
            tmp( );
        }

    public:

        /// C-tors
#if CXX11_ENABLED
        scoped_subscription( scoped_subscription &&o )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        {
            unsubscriber_.swap(o.unsubscriber_);
            o.reset( );
        }

        scoped_subscription &operator = ( scoped_subscription &&o )
        {
            if( this != &o ) {
                unsubscriber_.swap(o.unsubscriber_);
                o.reset( );
            }
            return *this;
        }

        scoped_subscription( observers::subscription &&o )
            :unsubscriber_(std::move(o.unsubscriber_))
        { }

        scoped_subscription & operator = ( subscription &&o )
        {
            unsubscribe( );
            unsubscriber_.swap(o.unsubscriber_);
            return *this;
        }
#endif
        scoped_subscription( scoped_subscription &o )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        {
            o.reset( );
        }

        scoped_subscription &operator = ( scoped_subscription &o )
        {
            if( this != &o ) {
                unsubscribe( );
                unsubscriber_   = o.unsubscriber_;
                o.reset( );
            }
            return *this;
        }

        scoped_subscription & operator = ( const subscription &o )
        {
            unsubscribe( );
            unsubscriber_ = o.unsubscriber_;
            return *this;
        }

        scoped_subscription( )
            :unsubscriber_(&subscription::unsubscribe_dummy)
        { }

        scoped_subscription( const subscription &o )
            :unsubscriber_(o.unsubscriber_)
        { }

        /// D-tor
        ~scoped_subscription( )
        {
            unsubscribe( );
        }

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

        void swap( scoped_subscription &other )
        {
            unsubscriber_.swap( other.unsubscriber_ );
        }

    private:
        void_call   unsubscriber_;
    };

}}}

#endif // SCOPEDSUBSCRIPTION_H
