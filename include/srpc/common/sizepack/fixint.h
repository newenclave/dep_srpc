#ifndef SRPC_COMMON_SIZEPACK_FIXINT_H
#define SRPC_COMMON_SIZEPACK_FIXINT_H

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common { namespace sizepack {

    template <typename SizeType>
    struct fixint {

        typedef SizeType size_type;

        static const size_t max_length = sizeof(SizeType);
        static const size_t min_length = sizeof(SizeType);

        template <typename IterT>
        static size_t size_length( IterT begin, const IterT &end )
        {
            return std::distance(begin, end) < max_length ? 0 : max_length;
        }

        static size_t packed_length( size_type input )
        {
            return max_length;
        }

        static std::string pack( size_type size )
        {
            char res[max_length];
            for( size_t current =  max_length; current > 0; --current ) {
                res[current-1]  = static_cast<char>( size & 0xFF );
                size >>= 8;
            }
            return std::string( &res[0], &res[max_length] );
        }

        static void pack( size_type size, std::string &res )
        {
            char tmp[max_length];
            for( size_t current =  max_length; current > 0; --current ) {
                tmp[current-1]  = static_cast<char>( size & 0xFF );
                size >>= 8;
            }
            res.assign( &tmp[0], &tmp[max_length] );
        }

        static size_t pack( size_type size, void *result )
        {
            srpc::uint8_t *res  = reinterpret_cast<srpc::uint8_t *>(result);
            for( size_t current = max_length; current > 0; --current ) {
                res[current-1]  = static_cast<srpc::uint8_t>( size & 0xFF );
                size >>= 8;
            }
            return max_length;
        }

        template <typename IterT>
        static size_type unpack( IterT begin, const IterT &end )
        {
            size_type  res = 0x00;
            for(size_t cur=max_length; cur>0 && begin!=end; --cur, ++begin) {
                res |= static_cast<size_type>(
                            static_cast<srpc::uint8_t>(*begin))
                                        << ((cur - 1) << 3);
            }
            return res;
        }
    };

}}}

#endif // SRPC_COMMON_SIZEPACK_FIXINT_H
