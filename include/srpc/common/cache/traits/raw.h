#ifndef SRPC_COMMON_CHACHE_TRAITS_RAW_H
#define SRPC_COMMON_CHACHE_TRAITS_RAW_H

namespace srpc { namespace common { namespace cache {

namespace traits {

    template <typename T>
    struct raw {
        typedef T * value_type;

#if CXX11_ENABLED
        template <typename ...Args>
        static
        value_type create( Args && ...args )
        {
            return new T(std::forward<Args>(args)...);
        }
#else
        static
        value_type create( )
        {
            return new T;
        }

        template <typename P0>
        static
        value_type create( const P0& p0 )
        {
            return new T(p0);
        }

        template <typename P0, typename P1>
        static
        value_type create( const P0& p0, const P1& p1 )
        {
            return new T(p0, p1);
        }

        template <typename P0, typename P1, typename P2>
        static
        value_type create( const P0& p0, const P1& p1, const P2& p2 )
        {
            return new T(p0, p1, p2);
        }
#endif

        static
        void destroy( value_type val )
        {
            delete val;
        }
    };
}

}}}


#endif // RAW_H
