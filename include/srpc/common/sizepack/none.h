#ifndef SRPC_COMMON_SIZEPACK_NONE_H
#define SRPC_COMMON_SIZEPACK_NONE_H

#include <string>
#include <algorithm>

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common { namespace sizepack {

    struct none {

        typedef size_t size_type;

        static const size_t max_length = 0;
        static const size_t min_length = 0;

        static
        bool valid_length( size_t )
        {
            return true;
        }

        template <typename IterT>
        static size_t size_length( IterT, const IterT & )
        {
            return 0;
        }

        static size_t packed_length( size_type )
        {
            return max_length;
        }

        static std::string pack( size_type )
        {
            return std::string( );
        }

        static void pack( size_type, std::string & )
        { }

        static void append( size_type, std::string & )
        { }

        static size_t pack( size_type, void * )
        {
            return max_length;
        }

        template <typename IterT>
        static size_type unpack( const IterT &begin, const IterT &end )
        {
            return std::distance( begin, end );
        }
    };

}}}


#endif // SRPC_COMMON_SIZEPACK_NONE_H
