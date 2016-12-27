#include <iostream>


#include "srpc/common/protocol/binary.h"

#include "srpc/common/result.h"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

using namespace srpc::common;
namespace gpb = google::protobuf;


int mainp( int argc, char *argv[ ] )
{

    try {


    } catch ( const std::exception &ex ) {
        std::cout << "Error " << ex.what( ) << "\n";
    }
}
