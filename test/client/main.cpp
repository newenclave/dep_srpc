#include <iostream>
#include "protocol/t.pb.h"
#include "boost/asio.hpp"

int main( )
{
    test::request_message rm;
    rm.set_name( "asdasd" );
    std::cout << rm.DebugString( ) << "\n";
    return 0;
}

