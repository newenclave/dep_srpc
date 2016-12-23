#ifndef CONFIG_ATOMIC_H
#define CONFIG_ATOMIC_H

#if CXX11_ENABLED

#include <atomic>

namespace CONFIG_TOPNAMESPACE {
    using std::atomic;
}


#else

#include "boost/atomic.hpp"

namespace CONFIG_TOPNAMESPACE {
    using boost::atomic;
}

#endif

#endif // ATOMIC_H
