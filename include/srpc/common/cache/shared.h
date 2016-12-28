#ifndef SRPC_COMMON_CHACHE_SHARED_H
#define SRPC_COMMON_CHACHE_SHARED_H

#include "srpc/common/cache/simple.h"
#include "srpc/common/config/memory.h"

namespace srpc { namespace common { namespace cache {

    template <typename T, typename MutexType = srpc::mutex>
    class shared: public srpc::enable_shared_from_this<shared<T, MutexType> >
    {
        struct key { };
        typedef cache::simple<T, MutexType> cache_impl;

    public:

        typedef typename cache_impl::value_type         value_type;
        typedef typename cache_impl::value_shared_type  value_shared_type;
        typedef typename cache_impl::mutex_type         mutex_type;
        typedef srpc::shared_ptr<shared>                shared_type;

        shared( size_t maximum, key )
            :impl_(maximum)
        { }

        static
        shared_type create( size_t maximum = 0 )
        {
            return srpc::make_shared<shared>(maximum, key( ));
        }

        void clear( )
        {
            impl_.clear( );
        }


#if CXX11_ENABLED
        template <typename ...Args>
        value_type get( Args && ...args )
        {
            return impl_.get(std::forward<Args>(args)...);
        }
#else
        value_type get( )
        {
            return impl_.get( );
        }

        template <typename P0>
        value_type get( const P0& p0 )
        {
            return impl_.get(p0);
        }

        template <typename P0, typename P1>
        value_type get( const P0& p0, const P1& p1)
        {
            return impl_.get(p0, p1);
        }

        template <typename P0, typename P1, typename P2>
        value_type get( const P0& p0, const P1& p1, const P2& p2)
        {
            return impl_.get(p0, p1, p2);
        }
#endif
        void push( value_shared_type val )
        {
            impl_.push( val );
        }

        size_t size( ) const
        {
            return impl_.size( );
        }

    private:
        cache_impl impl_;
    };

}}}

#endif // SRPC_COMMON_CHACHE_SHARED_H
