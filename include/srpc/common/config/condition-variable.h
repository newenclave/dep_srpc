#ifndef CONFIG_CONDITION_VARIABLE_H
#define CONFIG_CONDITION_VARIABLE_H

#if CXX11_ENABLED == 0

#include "boost/thread/condition_variable.hpp"

namespace CONFIG_TOPNAMESPACE {
    using boost::condition_variable;
}

#else

#include <condition_variable>

namespace CONFIG_TOPNAMESPACE {
    using std::condition_variable;
}

#endif

#endif // SRPC_CONDITION_VARIABLE_H
