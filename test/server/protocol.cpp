#include <iostream>


#include "srpc/common/protocol/binary.h"

#include "srpc/common/result.h"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include "srpc/common/observers/simple.h"
#include "srpc/common/observers/subscription.h"
#include "srpc/common/observers/scoped-subscription.h"

#include "boost/signals2.hpp"

using namespace srpc::common;
namespace gpb = google::protobuf;
namespace bs = boost::signals2;

namespace o1 {
    using os        = observers::simple<void (int), srpc::mutex>;
    using conn      = observers::subscription;
    using scop_conn = observers::scoped_subscription;
}

namespace o2 {
    using os        = bs::signal_type <void (int),
                                bs::keywords::mutex_type<srpc::mutex> >::type;
    using conn      = bs::connection;
    using scop_conn = bs::connection;
}

namespace myos = o1;

std::atomic<std::uint64_t> gcounter(0);

int main( int argc, char *argv[ ] )
{

    myos::os o;
    auto call = [](int i) { gcounter += i; };

    for( int i=0; i<100; i++ ) {
        o.connect( call );
        o(1);
    }

    std::cout << "counter " << gcounter << "\n";

    return 0;

    try {


    } catch ( const std::exception &ex ) {
        std::cout << "Error " << ex.what( ) << "\n";
    }
}
