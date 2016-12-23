#ifndef CONFIG_MUTEX_H
#define CONFIG_MUTEX_H

#if CXX11_ENABLED

#include <mutex>

namespace CONFIG_TOPNAMESPACE {
    using std::mutex;
    using std::recursive_mutex;
    using std::unique_lock;
    using std::lock_guard;
}

#else

#include "boost/thread/mutex.hpp"
#include "boost/thread/recursive_mutex.hpp"
#include "boost/thread/locks.hpp"

namespace CONFIG_TOPNAMESPACE {
    using boost::mutex;
    using boost::recursive_mutex;
    using boost::unique_lock;
    using boost::lock_guard;

}

#endif

namespace CONFIG_TOPNAMESPACE {
    struct dummy_mutex {
        void lock( ) { }
        void unlock( ) { }
        bool try_lock( ) { return true; }
    };
}

#endif // CONFIG_MUTEX_H
