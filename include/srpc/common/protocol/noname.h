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

    class noname: public binary< srpc::shared_ptr<srpc::rpc::lowlevel>,
                                 sizepack::none,
                                 sizepack::varint<srpc::uint32_t>,
                                 srpc::uint64_t >
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

    public:

        typedef typename parent_type::message_type       message_type;
        typedef typename parent_type::tag_type           tag_type;
        typedef typename parent_type::buffer_type        buffer_type;
        typedef typename parent_type::const_buffer_slice const_buffer_slice;
        typedef typename parent_type::buffer_slice       buffer_slice;
        typedef typename parent_type::error_code         error_code;

        typedef common::cache::simple<std::string>       cache_type;
        typedef common::cache::simple<message_type>      message_cache_type;

        typedef google::protobuf::MessageLite            message_lite;

        typedef SRPC_ASIO::io_service io_service;
        typedef common::transport::interface::write_callbacks cb_type;

        typedef srpc::common::protobuf::service::shared_type service_sptr;

        typedef srpc::common::factory<std::string,
                                      service_sptr,
                                      srpc::mutex> service_factory;

        noname( bool server, size_t max_length )
            :parent_type(server ? 100 : 101, max_length)
            ,call_type_(get_call_type(server))
        { }

    protected:

        void on_message_ready( tag_type, buffer_type,
                               const_buffer_slice slice )
        {
            message_type mess = mess_cache_.get( );
            mess->ParseFromArray( slice.data( ), slice.size( ) );
            bool callback = !!( mess->info( ).call_type( ) &
                                srpc::rpc::call_info::TYPE_CALLBACK_MASK );

            srpc::uint32_t call_type = mess->info( ).call_type( )
                                & ~srpc::rpc::call_info::TYPE_CALLBACK_MASK;
            if( call_type == call_type_ ) {
                push_to_slot( mess->id( ), mess );
            } else {
                if( callback ) {
                    push_to_slot( mess->target_id( ), mess );
                } else {
                    //execute_call( mess );
                }
            }
        }

        void setup_message( message_type &mess, srpc::uint64_t target )
        {
            mess->set_id( next_id( ) );
            if( target ) {
                mess->set_target_id( target );
                mess->mutable_info( )->set_call_type( callback_type_ |
                                    srpc::rpc::call_info::TYPE_CALLBACK_MASK  );
            } else {
                mess->mutable_info( )->set_call_type( call_type_ );
            }
        }

        buffer_slice prepare_message( buffer_type buf, tag_type tag,
                                      const message_lite &mess )
        {
            typedef typename parent_type::size_policy size_policy;
            const size_t old_len = buf->size( );

            tag_policy::append( tag, *buf );

            mess.AppendToString( *buf );

            buf->resize( buf->size( ) + hash_->length( ) );

            hash_->get( buf->c_str( ) + old_len,
                        buf->size( )  - old_len - hash_->length( ),
                        &(*buf)[buf->size( )    - hash_->length( )]);

            buffer_slice res( &(*buf)[old_len], buf->size( ) - old_len );

            return pack_message( buf, res );

        }

        static void default_cb( )
        { }

        void push_cache_back( const error_code &err,
                              buffer_type buff, srpc::function<void ( )> cb )
        {
            if( !err ) {
                buffer_cache_.push( buff );
                cb( );
            }
        }

        void send_message( const google::protobuf::MessageLite &mess,
                           srpc::function<void ( )> cb )
        {
            namespace ph = srpc::placeholders;

            buffer_type  buff  = buffer_cache_.get( );
            buffer_slice slice = prepare_message( buff, 0, mess );

            cb_type::post_call_type post_callback =
                    srpc::bind( &noname::push_cache_back, this, ph::_1,
                                buff, cb );

            get_transport( )->write( slice.data( ), slice.size( ),
                                     cb_type::post( post_callback ) );

        }

        void send_message( const google::protobuf::MessageLite &mess )
        {
            send_message( mess, &noname::default_cb );
        }

    private:

        message_cache_type  mess_cache_;
        cache_type          buffer_cache_;
        srpc::uint32_t      call_type_;
    };

}}}

#endif // NONAME_H
