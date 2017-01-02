#ifndef SRPC_COMMON_PROTOCOL_NONAME_H
#define SRPC_COMMON_PROTOCOL_NONAME_H

#include "srpc/common/protocol/binary.h"
#include "srpc/common/protocol/lowlevel.pb.h"
#include "srpc/common/factory.h"
#include "srpc/common/cache/simple.h"

#include "srpc/common/protobuf/service.h"
#include "srpc/common/protobuf/channel.h"
#include "srpc/common/protobuf/controller.h"
//#include "srpc/common/protobuf/stub.h"

#include "srpc/common/config/asio.h"

namespace srpc { namespace common { namespace protocol {

    class noname: public binary<srpc::shared_ptr<srpc::rpc::lowlevel>,
                                sizepack::none,
                                sizepack::varint<srpc::uint32_t>,
                                srpc::uint64_t>
    {
        typedef binary< srpc::shared_ptr<srpc::rpc::lowlevel>,
                        sizepack::none,
                        sizepack::varint<srpc::uint32_t>,
                        srpc::uint64_t > parent_type;

        static
        srpc::uint32_t get_call_type( bool server )
        {
            return server ? srpc::rpc::call_info::TYPE_SERVER_CALL
                          : srpc::rpc::call_info::TYPE_CLIENT_CALL;
        }

        static
        srpc::uint32_t get_callback_type( bool server )
        {
            return server ? srpc::rpc::call_info::TYPE_SERVER_CALLBACK
                          : srpc::rpc::call_info::TYPE_CLIENT_CALLBACK;
        }

    public:

        typedef typename parent_type::message_type       message_type;
        typedef typename parent_type::tag_type           tag_type;
        typedef typename parent_type::buffer_type        buffer_type;
        typedef typename parent_type::const_buffer_slice const_buffer_slice;
        typedef typename parent_type::buffer_slice       buffer_slice;

        typedef common::cache::simple<std::string>       cache_type;
        typedef common::cache::simple<message_type>      message_cache_type;

        typedef SRPC_ASIO::io_service io_service;
        typedef common::transport::interface::write_callbacks cb_type;

        typedef srpc::common::protobuf::service::shared_type service_sptr;

        typedef srpc::common::factory<std::string,
                                      service_sptr,
                                      srpc::mutex> service_factory;

        noname( bool server, size_t max_length )
            :parent_type(server ? 100 : 101, max_length)
            ,call_type_(get_call_type(server))
            ,callback_type_(get_callback_type(server))
        { }

    protected:

        void append_message( buffer_type buff, const message_type &mess )
        {
            mess->AppendToString( *buff );
        }

        void on_message_ready( tag_type, buffer_type,
                               const_buffer_slice slice )
        {
            message_type mess = mess_cache_.get( );
            mess->ParseFromArray( slice.data( ), slice.size( ) );
        }

    private:
        service_factory     factory_;
        message_cache_type  mess_cache_;
        srpc::uint32_t      call_type_;
        srpc::uint32_t      callback_type_;
    };

}}}

#endif // NONAME_H
