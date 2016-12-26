#ifndef SRPC_COMMON_SIZEPACK_VARINT_H
#define SRPC_COMMON_SIZEPACK_VARINT_H

#include <string>
#include <algorithm>

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common { namespace sizepack {

    template <typename SizeType>
    struct varint {

        typedef SizeType size_type;

        static const size_t max_length = (sizeof(size_type) * 8) / 7 + 1;
        static const size_t min_length = 1;

        static
        bool valid_length( size_t len )
        {
            return (len >= min_length) && (len <= max_length);
        }

        template <typename IterT>
        static size_t size_length( IterT begin, const IterT &end )
        {
            size_t        length = 0x00;
            srpc::uint8_t last   = 0x80;

            for( ;(begin != end) && (last & 0x80); ++begin ) {
                ++length;
                last = static_cast<srpc::uint8_t>(*begin);
            }
            return (last & 0x80) ? 0 : length;
        }

        template <typename IterT>
        static bool valid_packed( const IterT &begin, const IterT &end )
        {
            return valid_length( size_length(begin, end) );
        }

        static size_t packed_length( size_type input )
        {
            size_t res = 0;
            while( input ) ++res, input >>= 7;
            return res;
        }

        static std::string pack( size_type size )
        {
            std::string res;
            for( ; size > 0x7F; size >>= 7 ) {
                res.push_back(static_cast<char>((size & 0x7F) | 0x80));
            }
            res.push_back(static_cast<char>(size));
            return res;
        }

        static void pack( size_type size, std::string &res )
        {
            std::string tmp;
            append( size, tmp );
            res.swap(tmp);
        }

        static void append( size_type size, std::string &res )
        {
            for( ; size > 0x7F; size >>= 7 ) {
                res.push_back(static_cast<char>((size & 0x7F) | 0x80));
            }
            res.push_back(static_cast<char>(size));
        }

        static size_t pack( size_type size, void *result )
        {
            size_t index = 0;

            srpc::uint8_t *res = reinterpret_cast<srpc::uint8_t *>(result);

            for( ; size > 0x7F; size >>= 7 ) {
                res[index++] = (static_cast<char>((size & 0x7F) | 0x80));
            }
            res[index++] = (static_cast<char>(size));
            return index;
        }

        template <typename IterT>
        static size_type unpack( IterT begin, const IterT &end )
        {
            size_type      res   = 0x00;
            srpc::uint32_t shift = 0x00;
            srpc::uint8_t  last  = 0x80;

            for( ; (begin != end) && (last & 0x80); ++begin, shift += 7 ) {
                last = (*begin);
                res |= ( static_cast<size_type>(last & 0x7F) << shift);
            }
            return res;
        }
    };

}}}

#endif // SRPC_COMMON_SIZEPACK_VARINT_H
