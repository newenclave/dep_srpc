#ifndef CONFIG_ASIO_H
#define CONFIG_ASIO_H

#ifdef ASIO_STANDALONE
#include "asio.hpp"
#else
#include "boost/asio.hpp"
#endif

#include "asio-forward.h"

#endif // CONFIG_ASIO_H

