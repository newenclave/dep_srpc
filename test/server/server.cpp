#include <iostream>
#include <list>
#include <set>
#include <atomic>
#include <thread>

#include "boost/asio.hpp"

#include "srpc/common/config/memory.h"
#include "srpc/common/config/mutex.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"

#include "srpc/common/observers/simple.h"

#include "boost/signals2.hpp"

using namespace srpc;

using size_policy = common::sizepack::varint<size_t>;

struct keeper: public srpc::enable_shared_from_this<keeper> {

};

namespace rrr {
    template <typename T>
    struct result;

    template <typename T>
    struct result<T()> {
        typedef T type;
    };

    template <typename T, typename P0>
    struct result<T(P0)> {
        typedef T type;
    };

    template <typename T, typename P0, typename P1>
    struct result<T(P0, P1)> {
        typedef T type;
    };

    template <typename T, typename P0,
                          typename P1,
                          typename P2>
    struct result<T(P0, P1, P2)> {
        typedef T type;
    };
}

std::atomic<std::uint32_t> gcounter {0};

namespace sig {

    using vtype  = common::observers::simple<void (int)>;
    ///using vtype  = common::observers::simple<void (int), srpc::dummy_mutex>;
    using scoped_connection = vtype::scoped_subscription;

}

namespace bsig {

//    using vtype = boost::signals2::signal_type<void (int),
//                  boost::signals2::keywords::mutex_type<
//                                       srpc::dummy_mutex> >::type;

    using vtype = boost::signals2::signal_type<void (int)>::type;
    using scoped_connection = boost::signals2::scoped_connection;

}

namespace my = bsig;

my::vtype s;

void sleep_thread( )
{
//    for( int i=0; i<100; i++ ) {
//        my::scoped_connection c = s.connect([](int){ gcounter++; });
//        s( i );
//    }
//    for( int i = 1; i<5; i++ ) {
//        auto c = s.connect([](...){ gcounter++; });
//        std::this_thread::sleep_for( std::chrono::milliseconds(100) );
//        s.disconnect( c );
//    }
}

int main( int argc, char *argv[] )
{

//    std::vector<std::shared_ptr<my::vtype> > v;

//    for( int i=0; i<10000; i++ ) {
//        v.push_back(std::make_shared<my::vtype>( ));
//        v.back( )->connect( [ ](int){ gcounter++; } );
//        v.back( )->connect( [ ](int){ gcounter++; } );
//        (*v.back( ))( 1 );
//    }
//    std::cout << gcounter << "\n";

//    return 0;
    auto lambda2 = []( int i ){
        //std::cout << "!\n";
        gcounter += i;
    };

    auto lambda = [lambda2]( int i ){
        gcounter += i;
        s.connect( lambda2 );
    };

    s.connect( lambda );

    for( int i = 0; i<100; i++ ) {
        s.connect( lambda );
    }

    std::thread r(sleep_thread);

    for( int i = 0; i<600; i++ ) {
        s( 1 );
        s.connect( lambda );
        //std::cout << i << "\n";
    }

    r.join( );

//    for( int i = 0; i<30000000; i++ ) {
//        lambda( 1 );
//    }

    //s.disconnect_all( );
    std::cout << gcounter << "\n";

    return 0;
}
