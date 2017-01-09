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

        simple( )
            :maximum_(0)
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

#if CXX11_ENABLED

        template <typename ...Args>
        value_type get( Args && ...args )
        {
            locker_type l(cache_lock_);
            if( cache_.empty( ) ) {
                return ValueTrait::create(std::forward<Args>(args)...);
            } else {
                value_type n = cache_.front( );
                cache_.pop( );
                return n;
            }
        }
#else

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

        template <typename P0>
        value_type get( const P0& p0 )
        {
            locker_type l(cache_lock_);
            if( cache_.empty( ) ) {
                return ValueTrait::create( p0 );
            } else {
                value_type n = cache_.front( );
                cache_.pop( );
                return n;
            }
        }

        template <typename P0, typename P1>
        value_type get( const P0& p0, const P1& p1 )
        {
            locker_type l(cache_lock_);
            if( cache_.empty( ) ) {
                return ValueTrait::create( p0, p1 );
            } else {
                value_type n = cache_.front( );
                cache_.pop( );
                return n;
            }
        }

        template <typename P0, typename P1, typename P2>
        value_type get( const P0& p0, const P1& p1, const P2& p2 )
        {
            locker_type l(cache_lock_);
            if( cache_.empty( ) ) {
                return ValueTrait::create( p0, p1, p2 );
            } else {
                value_type n = cache_.front( );
                cache_.pop( );
                return n;
            }
        }

#endif

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

        size_t                 maximum_;
        std::queue<value_type> cache_;
        mutable mutex_type     cache_lock_;
    };

}}}

#endif // SRPC_COMMON_CHACHE_SIMPLE_H
