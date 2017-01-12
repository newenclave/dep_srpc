#ifndef SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H
#define SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/observers/scoped-subscription.h"

namespace srpc { namespace common { namespace observers {

    class scoped_subscription;
    class subscription {

        friend class srpc::common::observers::scoped_subscription;

        typedef srpc::shared_ptr<void>  void_sptr;
        typedef srpc::weak_ptr<void>    void_wptr;
        typedef srpc::function<void( )> void_call;

        template <typename T, typename KeyType>
        static
        void unsubscribe_impl( void_wptr parent, KeyType key )
        {
            void_sptr lock(parent.lock( ));
            if( lock ) {
                reinterpret_cast<T *>(lock.get( ))->unsubscribe( key );
            }
        }

        static void unsubscribe_dummy( )
        { }

    public:

        typedef size_t key_type;

        template<typename T, typename KeyType>
        subscription( srpc::shared_ptr<T> &parent, KeyType key )
        {
            unsubscriber_ =
                srpc::bind( &subscription::unsubscribe_impl<T, KeyType>,
                             parent, key );
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
