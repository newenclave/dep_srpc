#ifndef SRPC_COMMON_RESULT_TRAIT_SHARED_H
#define SRPC_COMMON_RESULT_TRAIT_SHARED_H

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"

namespace srpc { namespace common { namespace result {

namespace traits {

    template <typename T>
    struct shared {

        typedef srpc::shared_ptr<T> value_type;
        static
        void copy( value_type &v, const value_type &from )
        {
            v = from;
        }

#if CXX11_ENABLED
        template <typename ...Args>
        static
        value_type create( Args&&...args )
        {
            return srpc::make_shared<T>( std::forward<Args>( args )... );
        }
#else
        static
        value_type create(  )
        {
            return srpc::make_shared<T>( );
        }

        template <typename P0>
        static
        value_type create( const P0 &p0 )
        {
            return srpc::make_shared<T>( srpc::ref(p0) );
        }

        template <typename P0, typename P1>
        static
        value_type create( const P0 &p0, const P1 &p1 )
        {
            return srpc::make_shared<T>( srpc::ref(p0), srpc::ref(p1) );
        }

        template <typename P0, typename P1, typename P2>
        static
        value_type create( const P0 &p0, const P1 &p1, const P2 &p2 )
        {
            return srpc::make_shared<T>( srpc::ref(p0), srpc::ref(p1),
                                         srpc::ref(p2) );
        }
#endif
        static
        void destroy( value_type & )
        { }

        static
        T &get_value( value_type &v )
        {
            return *v;
        }

        static
        const T &get_value( value_type const &v )
        {
            return *v;
        }
    };
}

}}}

#endif // SRPC_COMMON_RESULT_TRAIT_SHARED_H
