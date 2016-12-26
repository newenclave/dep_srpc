#ifndef SRPC_COMMON_RESULT_RESULT_H
#define SRPC_COMMON_RESULT_RESULT_H

#include "srpc/common/result/traits/shared.h"
#include "srpc/common/result/traits/value.h"
#include "srpc/common/result/base.h"

namespace srpc { namespace common { namespace result {

#if CXX11_ENABLED
    template <typename T, typename E>
    using shared_result = base<T, E, traits::shared<T> >;

    template <typename T, typename E>
    using value_result = base<T, E, traits::value<T> >;
#else
    template <typename T, typename E>
    class shared_result: public base<T, E, traits::shared<T> > {
        typedef base<T, E, traits::shared<T> > parent_type;
    public:
        //// CTOR
        shared_result( )
        { }

        shared_result( const parent_type &o )
            :parent_type(o)
        { }

        shared_result( const shared_result &o )
            :parent_type(o)
        { }

        shared_result& operator  = ( const parent_type &o )
        {
            return parent_type::operator =( o );
        }

        shared_result& operator  = ( const shared_result&o )
        {
            return parent_type::operator =( o );
        }

        template <typename P0>
        explicit shared_result( const P0 &p0 )
            :parent_type(p0)
        { }

        template <typename P0, typename P1>
        explicit shared_result( const P0 &p0, const P1 &p1 )
            :parent_type(p0, p1)
        { }

        template <typename P0, typename P1, typename P2>
        explicit shared_result( const P0 &p0, const P1 &p1, const P2 &p2 )
            :parent_type(p0, p1, p2)
        { }
    };

    template <typename T, typename E>
    class value_result: public base<T, E, traits::value<T> > {
        typedef base<T, E, traits::value<T> > parent_type;
    public:
        //// CTOR
        value_result( )
        { }

        value_result( const parent_type &o )
            :parent_type(o)
        { }

        value_result( const value_result &o )
            :parent_type(o)
        { }

        value_result& operator  = ( const parent_type &o )
        {
            return parent_type::operator =( o );
        }

        value_result& operator  = ( const value_result&o )
        {
            return parent_type::operator =( o );
        }

        template <typename P0>
        explicit value_result( const P0 &p0 )
            :parent_type(p0)
        { }

        template <typename P0, typename P1>
        explicit value_result( const P0 &p0, const P1 &p1 )
            :parent_type(p0, p1)
        { }

        template <typename P0, typename P1, typename P2>
        explicit value_result( const P0 &p0, const P1 &p1, const P2 &p2 )
            :parent_type(p0, p1, p2)
        { }
    };

#endif

}}}

#endif // RESULT_H
