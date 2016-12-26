#ifndef SRPC_COMMON_RESULT_TRAIT_VALUE_H
#define SRPC_COMMON_RESULT_TRAIT_VALUE_H

namespace srpc { namespace common { namespace result {

namespace traits {
    template <typename T>
    struct value {
        typedef T value_type;

        static
        void copy( value_type &v, const value_type &from )
        {
            v = from;
        }

        static
        value_type create(  )
        {
            return value_type( );
        }

#if CXX11_ENABLED
        template <typename ...Args>
        static
        value_type create( Args&&...args )
        {
            return std::move(value_type(std::forward<Args>( args )... ));
        }
#else
        static
        value_type create( )
        {
            return value_type( );
        }

        template <typename P0>
        static
        value_type create( const P0 &p0 )
        {
            return value_type( p0 );
        }

        template <typename P0, typename P1>
        static
        value_type create( const P0 &p0, const P1 &p1 )
        {
            return value_type( p0, p1 );
        }

        template <typename P0, typename P1, typename P2>
        static
        value_type create( const P0 &p0, const P1 &p1, const P2 &p2 )
        {
            return value_type( p0, p1, p2 );
        }

#endif
        static
        void destroy( value_type & )
        { }

        static
        T &get_value( value_type &v )
        {
            return v;
        }

        static
        const T &get_value( value_type const &v )
        {
            return v;
        }

    };
}

}}}

#endif // VALUE_H
