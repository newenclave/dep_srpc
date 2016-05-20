#include <iostream>
#include "protocol/t.pb.h"

int main( )
{
    test::request_message rm;
    rm.set_name( "asdasd" );
    std::cout << rm.DebugString( ) << "\n";
    return 0;
}

