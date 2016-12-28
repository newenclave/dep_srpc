#ifndef SRPC_COMMON_CHACHE_SIMPLE_H
#define SRPC_COMMON_CHACHE_SIMPLE_H

#include <queue>
#include "srpc/common/config/mutex.h"
#include "srpc/common/cache/traits/shared.h"
#include "srpc/common/cache/traits/raw.h"

namespace srpc { namespace common { namespace cache {

    template <typename T,
              typename MutexType  = srpc::mutex,
              typename ValueTrait = traits::shared<T> >
    class simple
    {
        typedef srpc::lock_guard<MutexType> locker_type;

        void clear_unsafe( )
        {
            while( !cache_.empty( ) ) {
                ValueTrait::destroy( cache_.front( ) );
                cache_.pop( );
            }
        }

    public:

        typedef typename ValueTrait::value_type value_type;
        typedef MutexType                       mutex_type;

        simple( size_t maximum )
            :maximum_(maximum)
        { }

        ~simple( )
        {
            clear_unsafe( );
        }

        void clear( )
        {
            locker_type l(cache_lock_);
            clear_unsafe( );
        }

        value_type get( )
        {
            locker_type l(cache_lock_);
            if( cache_.empty( ) ) {
                return ValueTrait::create( );
            } else {
                value_type n = cache_.front( );
                cache_.pop( );
                return n;
            }
        }

        void push( value_type val )
        {
            locker_type l(cache_lock_);
            if( (maximum_ > 0) && (cache_.size( ) < maximum_) ) {
                cache_.push( val );
            }
        }

        size_t size( ) const
        {
            locker_type l(cache_lock_);
            return cache_.size( );
        }

    private:

        std::queue<value_type> cache_;
        mutable mutex_type     cache_lock_;
        size_t                 maximum_;
    };

}}}

#endif // SRPC_COMMON_CHACHE_SIMPLE_H
