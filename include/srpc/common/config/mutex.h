#ifndef CONFIG_MUTEX_H
#define CONFIG_MUTEX_H

#if CXX11_ENABLED == 0

#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"

namespace CONFIG_TOPNAMESPACE {
    using boost::mutex;
    using boost::unique_lock;
    using boost::lock_guard;
}

#else

#include <mutex>

namespace CONFIG_TOPNAMESPACE {
    using std::mutex;
    using std::unique_lock;
    using std::lock_guard;
}

#endif

#endif // CONFIG_MUTEX_H
