#ifndef SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H
#define SRPC_COMMON_OBSERVERS_SUBSCRIPTION_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/stdint.h"
#include "srpc/common/observers/scoped-subscription.h"

namespace srpc { namespace common { namespace observers {

    class scoped_subscription;
    class subscription {

        friend class srpc::common::observers::scoped_subscription;

        static void unsubscribe_dummy( )
        { }

        void reset( )
        {
            unsubscriber_.reset( );
        }

    public:

        struct unsubscriber {
            virtual srpc::uintptr_t data( ) { return 0; }
            virtual ~unsubscriber( ) { }
            virtual void run( ) = 0;
        };

        typedef srpc::shared_ptr<unsubscriber> unsubscriber_sptr;

        subscription( unsubscriber_sptr call )
            :unsubscriber_(call)
        { }

#if CXX11_ENABLED
        subscription( subscription &&o )
            :unsubscriber_(std::move(o.unsubscriber_))
        { }

        subscription &operator = ( subscription &&o )
        {
            unsubscriber_ = o.unsubscriber_;
            return *this;
        }

        subscription( const subscription &o ) = default;
        subscription &operator = ( const subscription &o ) = default;
#endif
        subscription( )
        { }

        void unsubscribe(  )
        {
            if( unsubscriber_ ) {
                unsubscriber_->run( );
            }
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
        unsubscriber_sptr unsubscriber_;
    };

}}}

#endif // SUBSCRIPTION_H
