#include <stdio.h>

#include <linux/if_tun.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>

#include <stdlib.h>
#include <string.h>
#include <string>

#define TUNTAP_DEVICE_PATH "/dev/net/tun"
#define TUNTAP_DEVICE_NAME "tun666"
#define TUNTAP_DEVICE_FAGS IFF_TUN | IFF_NO_PI

namespace tunnel {

    int opentuntap( std::string &in_out, int flags, bool persis )
    {
        const char *clonedev = TUNTAP_DEVICE_PATH;

        struct ifreq ifr;
        struct ifreq ifr_out;
        int fd = -1;

        if( (fd = open(clonedev , O_RDWR)) < 0 ) {
            return fd;
        }

        memset( &ifr, 0, sizeof(ifr) );
        memset( &ifr_out, 0, sizeof(ifr) );

        ifr.ifr_flags = flags;

        strncpy( ifr.ifr_name, in_out.c_str( ), IFNAMSIZ );

        if( ioctl( fd, TUNSETIFF, static_cast<void *>(&ifr) ) < 0 ) {
            close( fd );
            return -1;
        }

        ioctl( fd, TUNGETIFF, static_cast<void *>(&ifr_out) );
        in_out.assign( ifr_out.ifr_name );

        flags = fcntl( fd, F_GETFL, 0 );
        if( flags < 0 ) {
            close( fd );
            return -1;
        }

        if( fcntl( fd, F_SETFL, flags ) < 0) {
            close( fd );
            return -1;
        }

        if( persis ) {
            ioctl( fd, TUNSETPERSIST, 1 );
        }

        return fd;
    }
}
