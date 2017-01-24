#include <iostream>
#include <thread>


#include "srpc/common/protocol/binary.h"

#include "srpc/common/result.h"

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"

#include "srpc/common/observers/simple.h"
#include "srpc/common/observers/subscription.h"
#include "srpc/common/observers/scoped-subscription.h"
#include "srpc/common/config/mutex.h"

#include "boost/signals2.hpp"

using namespace srpc::common;
namespace gpb = google::protobuf;
namespace bs = boost::signals2;

using my_mutex = srpc::mutex;

namespace o1 {
    std::string callname = "my observer";
    using os        = observers::simple<void (int), my_mutex>;
    using conn      = observers::subscription;
    using scop_conn = observers::scoped_subscription;
}

namespace o2 {
    std::string callname = "boost signals";
    using os        = bs::signal_type <void (int),
                                bs::keywords::mutex_type<my_mutex> >::type;
    using conn      = bs::connection;
    using scop_conn = bs::scoped_connection;
}

namespace myos = o2;

std::atomic<std::uint64_t> gcounter(0);
myos::os o;

const auto factor  = 2000;
const auto threads = 10;

void vvv(  )
{
    auto call = [](int i) { gcounter += i; };
    for( auto i=0; i<factor; i++ ) {
        o.connect( call );
        myos::scop_conn scon(o.connect( call ));
        o(1);
    }
}

int mainp( int argc, char *argv[ ] )
{

    std::vector<std::thread> tds;

    for( auto i = 0; i < threads; i++ ) {
        tds.emplace_back( vvv );
    }

    vvv(  );

    for( auto &a: tds ) {
        if( a.joinable( ) ) {
            a.join( );
        }
    }

    std::cout << "counter " << gcounter << " for "
              << myos::callname << "\n";

    return 0;
}
