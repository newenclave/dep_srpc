#ifndef SRPC_COMMON_CHACHE_SIMPLE_H
#define SRPC_COMMON_CHACHE_SIMPLE_H

#include <queue>
#include "srpc/common/config/memory.h"
#include "srpc/common/config/mutex.h"

namespace srpc { namespace common { namespace cache {

    template <typename T, typename MutexType = srpc::mutex>
    class simple
    {
        typedef srpc::lock_guard<MutexType> locker_type;

    public:

        typedef T                            value_type;
        typedef srpc::shared_ptr<value_type> value_shared_type;
        typedef MutexType                    mutex_type;

        simple( size_t maximum )
            :maximum_(maximum)
        { }

        value_shared_type get( )
        {
            locker_type l(cache_lock_);
            if( cache_.empty( ) ) {
                return srpc::make_shared<value_type>( );
            } else {
                value_shared_type n = cache_.front( );
                cache_.pop( );
                return n;
            }
        }

        void push( value_shared_type val )
        {
            locker_type l(cache_lock_);
            if( (maximum_ > 0) && (cache_.size( ) < maximum_) ) {
                std::cout << "Push value\n";
                cache_.push( val );
            }
        }

        size_t size( ) const
        {
            locker_type l(cache_lock_);
            return cache_.size( );
        }

    private:

        std::queue<value_shared_type> cache_;
        mutable mutex_type            cache_lock_;
        size_t                        maximum_;
    };

}}}

#endif // SRPC_COMMON_CHACHE_SHARED_H
