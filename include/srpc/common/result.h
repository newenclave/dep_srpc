#ifndef SRPC_COMMON_RESULT_H
#define SRPC_COMMON_RESULT_H

#include <map>

namespace srpc { namespace common {

    //// very simple result pair
    template<typename T, typename E>
    class result {

        struct ok_type {
            explicit ok_type( const T &val )
                :value(val)
            { }
            const T &value;
        };

        struct fail_type {
            explicit fail_type( const E &val )
                :value(val)
            { }
            const E &value;
        };

        explicit result( const ok_type &val )
            :value_(std::make_pair(val.value, E( )))
        { }

        explicit result( const fail_type &err )
            :value_(std::make_pair(T( ), err.value))
        { }

    public:

        typedef T value_type;
        typedef E error_type;

        operator bool ( ) const
        {
            return !value_.second;
        }

        explicit result( const value_type &val )
            :value_(std::make_pair(val, error_type( )))
        { }

#if CXX11_ENABLED

        explicit result( T &&val )
            :value_(std::make_pair(std::move(val), E( )))
        { }

        result              (                 ) = default;
        result              (       result && ) = default;
        result & operator = ( const result  & ) = default;
        result & operator = (       result && ) = default;

        static
        result ok( value_type &&val )
        {
            result res(std::move(val));
            return std::move(res);
        }
#else
#endif

        static
        result ok( const value_type &val )
        {
            return result(ok_type(val));
        }

        static
        result fail( const error_type &val )
        {
            return result(fail_type(val));
        }

        value_type &operator *( )
        {
            return value_.first;
        }

        const value_type &operator *( ) const
        {
            return value_.first;
        }

        const value_type *operator -> ( ) const
        {
            return &value_.first;
        }

        value_type *operator -> ( )
        {
            return &value_.first;
        }

        const error_type &error( ) const
        {
            return value_.second;
        }

        void swap( result &other )
        {
            value_.swap( other );
        }

    private:
        std::pair<value_type, error_type> value_;
    };

}}

#endif // RESULT_H
