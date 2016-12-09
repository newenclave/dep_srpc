#ifndef CONFIG_ASIO_FORWARD_H
#define CONFIG_ASIO_FORWARD_H

#ifdef ASIO_STANALONE
#define SRPC_ASIO_FORWARD( x ) namespace asio { x }
#define SRPC_ASIO ::asio
#else
#define SRPC_ASIO_FORWARD( x ) namespace boost { namespace asio { x } }
#define SRPC_ASIO ::boost::asio
#endif

#endif // CONFIG_ASIO_FORWARD_H

