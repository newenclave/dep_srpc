#ifndef SRPC_CONST_BUFFER_H
#define SRPC_CONST_BUFFER_H

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common {

    template <typename T>
    class const_buffer {

        const T *data_;
        size_t   length_;

    public:

        typedef T value_type;

        const_buffer( const value_type *val, size_t len )
            :data_(val)
            ,length_(len)
        { }

        const_buffer( const const_buffer &other )
            :data_(other.data_)
            ,length_(other.length_)
        { }

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

#endif // CONST_BUFFER_H
