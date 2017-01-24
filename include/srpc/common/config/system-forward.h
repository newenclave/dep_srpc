#ifndef CONFIG_SYSTEM_FORWARD_H
#define CONFIG_SYSTEM_FORWARD_H

#ifdef ASIO_STANDALONE
#define SRPC_SYSTEM_FORWARD( x ) namespace asio { x } }
#define SRPC_SYSTEM ::asio
#else
#define SRPC_SYSTEM_FORWARD( x ) namespace boost { namespace system { x } }
#define SRPC_SYSTEM ::boost::system
#endif

#endif // CONFIG_SYSTEM_FORWARD_H

