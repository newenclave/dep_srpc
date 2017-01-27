#ifndef SRPC_COMMON_PROTOCOL_TRAITS_LOWLEVEL_H
#define SRPC_COMMON_PROTOCOL_TRAITS_LOWLEVEL_H

#include <string>

#include "srpc/common/protocol/lowlevel.pb.h"

#include "srpc/common/config/memory.h"
#include "srpc/common/config/stdint.h"
#include "srpc/common/config/atomic.h"

namespace srpc { namespace common { namespace protocol {

namespace traits {

    /*
        trait {
            id(mess)
            pack_message(mess, out)
            is_error(mess)
            error_message(mess)
            error_code(mess)
            parse_response(mess)
        }
    */

    struct lowlevel {

        typedef google::protobuf::Message             request_message_type;
        typedef google::protobuf::Message             response_message_type;
        typedef srpc::rpc::lowlevel                   message_type;
        typedef srpc::shared_ptr<srpc::rpc::lowlevel> shared_message_type;

        static
        void pack_message( const request_message_type &mess,
                                  std::string &out )
        {
            mess.AppendToString( &out );
        }

        static
        void parse_message( request_message_type &mess,
                            const char *data, size_t len )
        {
            mess.ParseFromArray( data, len );
        }

        static
        srpc::uint64_t id( const message_type &mess )
        {
            return mess.id( );
        }

        static
        srpc::uint64_t target_id( const message_type &mess )
        {
            return mess.target_id( );
        }

        static
        void set_id( message_type &mess, srpc::uint64_t val )
        {
            mess.set_id( val );
        }

        static
        void set_target_id( message_type &mess, srpc::uint64_t val )
        {
            mess.set_target_id( val );
        }

        static
        bool is_error( const message_type &mess )
        {
            return mess.error( ).code( ) != 0;
        }

        static
        std::string error_message( const message_type &mess )
        {
            return mess.error( ).additional( );
        }

        static
        srpc::uint32_t error_code( const message_type &mess )
        {
            return mess.error( ).code( );
        }

        static
        srpc::uint32_t call_type( const message_type &mess )
        {
            return mess.info( ).call_type( );
        }

        static
        void set_call_type( message_type &mess, srpc::uint32_t val )
        {
            mess.mutable_info( )->set_call_type( val );
        }

        static
        void parse_response( const message_type &mess,
                             response_message_type *res )
        {
            res->ParseFromString( mess.response( ) );
        }

        static
        void serialize_request( message_type &mess,
                                const response_message_type *res )
        {
            mess.set_request( res->SerializeAsString( ) );
        }

        static void set_call( message_type &mess,
                              const std::string &svc, const std::string &meth )
        {
            mess.mutable_call( )->set_service_id( svc );
            mess.mutable_call( )->set_method_id( meth );
        }

        static
        void set_wait( message_type &mess, bool value )
        {
            mess.mutable_opt( )->set_wait( value );
        }

        static
        void set_wait_response( message_type &mess, bool value )
        {
            mess.mutable_opt( )->set_accept_response( value );
        }

    };
}

}}}

#endif // LOWLEWEL_H
