#ifndef SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H
#define SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/observers/scoped-subscription.h"

namespace srpc { namespace common { namespace observers {

    class scoped_subscription;
    class subscription {

        friend class srpc::common::observers::scoped_subscription;

        typedef srpc::function<void( )> void_call;

        template <typename T, typename KeyType>
        static
        void unsubscribe_impl( srpc::weak_ptr<T> parent, KeyType key )
        {
            srpc::shared_ptr<T> lock(parent.lock( ));
            if( lock ) {
                lock->unsubscribe( key );
            }
        }

        static void unsubscribe_dummy( )
        { }

    public:

        template<typename T, typename KeyType>
        subscription( srpc::shared_ptr<T> &parent, KeyType key )
        {
            unsubscriber_ =
                srpc::bind( &subscription::unsubscribe_impl<T, KeyType>,
                             srpc::weak_ptr<T>(parent), key );
        }

#if CXX11_ENABLED
        subscription( subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            unsubscriber_   = &subscription::unsubscribe_dummy;

        }

        subscription &operator = ( subscription &&o )
        {
            unsubscriber_   = o.unsubscriber_;
            o.unsubscriber_ = &subscription::unsubscribe_dummy;
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
