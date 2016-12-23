#ifndef CONFIG_CHRONO_H
#define CONFIG_CHRONO_H

#if CXX11_ENABLED

#include <chrono>

namespace CONFIG_TOPNAMESPACE {
    namespace chrono {
        using namespace std::chrono;
    }
}

#else

#include "boost/chrono.hpp"

namespace CONFIG_TOPNAMESPACE {
    namespace chrono {
        using namespace boost::chrono;
    }
}

#endif


#endif // CHRONO_H
