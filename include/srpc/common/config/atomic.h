#ifndef CONFIG_ATOMIC_H
#define CONFIG_ATOMIC_H

#if CXX11_ENABLED == 0

#include "boost/atomic.hpp"

namespace CONFIG_TOPNAMESPACE {
    using boost::atomic;
}

#else

#include <atomic>

namespace CONFIG_TOPNAMESPACE {
    using std::atomic;
}

#endif

#endif // ATOMIC_H
