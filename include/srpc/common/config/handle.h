#ifndef CONFIG_HANDLE_H
#define CONFIG_HANDLE_H


#if 0

#ifdef _WIN32

namespace CONFIG_TOPNAMESPACE {
    typedef HANDLE handle_type;
    static const handle_type invalid_handle_value = INVALID_HANDLE_VALUE;
}

#else

namespace CONFIG_TOPNAMESPACE {
    typedef int handle_type;
    static const handle_type invalid_handle_value = -1;
}

#endif

#else

#include "srpc/common/config/stdint.h"

namespace CONFIG_TOPNAMESPACE {
    typedef srpc::uintptr_t handle_type;
    static const handle_type invalid_handle_value = ((handle_type)-1);
}

#endif

#endif // HANDLE_H
