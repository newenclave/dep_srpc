#include "srpc/server/listener/interface.h"
#include "srpc/common/config/asio.h"


namespace srpc { namespace server { namespace listener {

namespace {

    namespace ba = SRPC_ASIO;

    class impl: public interface {

        typedef ba::ip::tcp::endpoint endpoint;

    public:

        static endpoint epcreate( const std::string &addr, srpc::uint16_t port )
        {
            return endpoint( ba::ip::address::from_string(addr), port );
        }

        impl( const std::string &addr, srpc::uint16_t port, bool tcp_nowait )
            :ep_(epcreate(addr, port))
            ,nowait_(tcp_nowait)
        { }

        void start( )
        { }

        void stop(  )
        { }

        endpoint ep_;
        bool     nowait_;
    };
}

    namespace tcp {
        interface::shared_type create( const std::string &addr,
                                       srpc::uint16_t port, bool tcp_nowait )
        {
            return srpc::make_shared<impl>( addr, port, tcp_nowait );
        }
    }

}}}
