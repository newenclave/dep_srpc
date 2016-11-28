#ifndef SRPC_BUFFER_H
#define SRPC_BUFFER_H

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common {

    template <typename T>
    class buffer {

        T       *data_;
        size_t   length_;

    public:

        typedef T value_type;

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
    };
}}

#endif // SRPC_BUFFER_H
