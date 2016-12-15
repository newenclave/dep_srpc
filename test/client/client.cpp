#include <iostream>

#include "srpc/common/sizepack/zigzag.h"

using namespace srpc;

int main( int argc, char *argv[] )
{
    using zigzag = common::sizepack::zigzag<srpc::uint16_t>;

    for( srpc::int16_t i = -100; i<=100; i++ ) {
        srpc::uint16_t pack   = zigzag::to_unsigned( i );
        srpc::int16_t  unpack = zigzag::to_signed( pack );

        std::cout << i << " = " << pack << " ";
        std::cout << unpack << std::endl;

        if( i != unpack ) {
            std::cerr << i << " != " << unpack << "\n";
            std::terminate( );
        }
    }

    return 0;
}
