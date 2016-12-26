#ifndef SRPC_COMMON_RESULT_BASE_H
#define SRPC_COMMON_RESULT_BASE_H

#include "srpc/common/config/memory.h"

namespace srpc { namespace common { namespace result {


    template <typename T, typename E, typename Trait>
    class base {

        typedef typename Trait::value_type value_container;

    public:

        typedef T value_type;
        typedef E error_type;

        base( )
            :value_(Trait::create( ))
        { }

        base( const base &other )
            :value_(Trait::create( ))
        {
            Trait::copy( value_, other.value_ );
            error_ = other.error_;
        }

        base &operator = ( const base &other )
        {
            error_ = other.error_;
            Trait::copy( value_, other.value_ );
            return *this;
        }

        ~base( )
        {
            Trait::destroy( value_ );
        }

        void swap( base &other )
        {
            std::swap( value_, other.value_ );
            std::swap( error_, other.error_ );
        }

        const E *error( ) const
        {
            return error_.get( );
        }

        operator bool ( ) const
        {
            return (error_.get( ) == NULL);
        }

        value_type &operator *( )
        {
            return Trait::get_value(value_);
        }

        const value_type &operator *( ) const
        {
            return Trait::get_value(value_);
        }

        value_type *operator -> ( )
        {
            return &Trait::get_value(value_);
        }

        const value_type *operator -> ( ) const
        {
            return &Trait::get_value(value_);
        }

#if CXX11_ENABLED

        template <typename ...Args>
        explicit base( Args&& ...args )
            :value_(Trait::create(std::forward<Args>(args)...))
        { }

        explicit base( base &&other )
            :value_(std::move(other.value_))
            ,error_(std::move(other.error_))
        { }

        base &operator = ( base &&other )
        {
            error_ = std::move(other.error_);
            value_ = std::move(other.value_);
            return *this;
        }

        template <typename ...Args>
        static
        base ok( Args&& ...args )
        {
            return base( std::forward<Args>(args)... );
        }

        template <typename ...Args>
        static
        base fail( Args&& ...args )
        {
            base res;
            res.error_ = std::make_shared<E>( std::forward<Args>(args)... );
            return std::move( res );
        }
#else
        //// CTOR
        template <typename P0>
        explicit base( const P0 &p0 )
            :value_(Trait::create(p0))
        { }

        template <typename P0, typename P1>
        explicit base( const P0 &p0, const P1 &p1 )
            :value_(Trait::create(p0, p1))
        { }

        template <typename P0, typename P1, typename P2>
        explicit base( const P0 &p0, const P1 &p1, const P2 &p2 )
            :value_(Trait::create(p0, p1, p2))
        { }

        //// OK ctor
        static
        base ok( )
        {
            return base( );
        }

        template <typename P0>
        static
        base ok( const P0 &p0 )
        {
            return base(p0);
        }

        template <typename P0, typename P1>
        static
        base ok( const P0 &p0, const P1 &p1 )
        {
            return base(p0, p1 );
        }

        template <typename P0, typename P1, typename P2>
        static
        base ok( const P0 &p0, const P1 &p1, const P2 &p2 )
        {
            return base(p0, p1, p2 );
        }

        //// FAIL ctor
        static
        base fail( )
        {
            base res;
            res.error_ = srpc::make_shared<E>( );
            return res;
        }

        template <typename P0>
        static
        base fail( const P0 &p0 )
        {
            base res;
            res.error_ = srpc::make_shared<E>( p0 );
            return res;
        }

        template <typename P0, typename P1>
        static
        base fail( const P0 &p0, const P1 &p1 )
        {
            base res;
            res.error_ = srpc::make_shared<E>( p0, p1 );
            return res;
        }

        template <typename P0, typename P1, typename P2>
        static
        base fail( const P0 &p0, const P1 &p1, const P2 &p2 )
        {
            base res;
            res.error_ = srpc::make_shared<E>( p0, p1, p2 );
            return res;
        }

#endif

    private:
        value_container value_;
        srpc::shared_ptr<E const> error_;
    };

}}}

#endif // SRPC_COMMON_RESULT_H
