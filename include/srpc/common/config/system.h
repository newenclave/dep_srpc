#ifndef CONFIG_SYSTEM_H
#define CONFIG_SYSTEM_H

#ifdef ASIO_STANDALONE
#include "asio/system_error.hpp"
#include "asio/error_code.hpp"
#else
#include "boost/system/error_code.hpp"
#include "boost/system/system_error.hpp"
#endif

#include "system-forward.h"

#endif // SYSTEM_H
