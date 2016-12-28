#ifndef SRPC_CONST_BUFFER_H
#define SRPC_CONST_BUFFER_H

#include "srpc/common/config/stdint.h"
#include "srpc/common/buffer.h"

namespace srpc { namespace common {

    template <typename T>
    class const_buffer {

    public:

        typedef T value_type;

        const_buffer( )
            :data_(NULL)
            ,length_(0)
        { }

        const_buffer( const value_type *val, size_t len )
            :data_(val)
            ,length_(len)
        { }

        const_buffer( const const_buffer &other )
            :data_(other.data_)
            ,length_(other.length_)
        { }

        const_buffer( const buffer<value_type> &other )
            :data_(other.data( ))
            ,length_(other.size( ))
        { }

        const value_type& operator []( size_t id ) const
        {
            return data_[id];
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

    private:

        const value_type *data_;
        size_t            length_;
    };
}}

#endif // CONST_BUFFER_H
