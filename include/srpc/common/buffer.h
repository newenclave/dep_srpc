#ifndef SRPC_BUFFER_H
#define SRPC_BUFFER_H

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common {

    template <typename T>
    class buffer {

    public:

        typedef T value_type;

        buffer( )
            :data_(NULL)
            ,length_(0)
        { }

        buffer( value_type *val, size_t len )
            :data_(val)
            ,length_(len)
        { }

        buffer( const buffer &other )
            :data_(other.data_)
            ,length_(other.length_)
        { }

        value_type *data( )
        {
            return data_;
        }

        const value_type *data( ) const
        {
            return data_;
        }

        size_t size( ) const
        {
            return length_;
        }

        const value_type *begin( ) const
        {
            return data_;
        }

        const value_type *end( ) const
        {
            return data_ + length_;
        }

        value_type *begin( )
        {
            return data_;
        }

        value_type *end( )
        {
            return data_ + length_;
        }

    public:

        value_type *data_;
        size_t      length_;
    };
}}

#endif // SRPC_BUFFER_H
