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

        typedef srpc::function<void (void)> empty_call;

        static
        srpc::uint32_t get_call_type( bool server )
        {
            return server ? srpc::rpc::call_info::TYPE_SERVER_CALL
                          : srpc::rpc::call_info::TYPE_CLIENT_CALL;
        }

        struct call_keeper {

        };

        static void default_on_ready( bool )
        { }

    public:

        typedef srpc::function<void (bool)> on_ready_type;

        typedef srpc::rpc::lowlevel                      lowlevel_message_type;
        typedef typename parent_type::message_type       message_type;
        typedef typename parent_type::tag_type           tag_type;
        typedef typename parent_type::buffer_type        buffer_type;
        typedef typename parent_type::const_buffer_slice const_buffer_slice;
        typedef typename parent_type::buffer_slice       buffer_slice;
        typedef typename parent_type::error_code         error_code;

        typedef common::cache::simple<std::string>          cache_type;
        typedef common::cache::simple<srpc::rpc::lowlevel>  message_cache_type;

        typedef google::protobuf::MessageLite            message_lite;

        typedef SRPC_ASIO::io_service io_service;
        typedef common::transport::interface::write_callbacks cb_type;

        typedef srpc::common::protobuf::service::shared_type service_sptr;

        typedef srpc::common::factory< std::string,
                                       service_sptr,
                                       srpc::mutex > service_factory;

        noname( bool server, size_t max_length )
            :parent_type(server ? 100 : 101, max_length)
            ,call_type_(get_call_type(server))
            ,on_ready_(&noname::default_on_ready)
            ,ready_(false)
        {
            set_ready( true );
        }

        void setup_message( lowlevel_message_type &mess, srpc::uint64_t target )
        {
            mess.set_id( next_id( ) );
            if( target ) {
                mess.set_target_id( target );
                mess.mutable_info( )
                    ->set_call_type( call_type_
                                   | srpc::rpc::call_info::TYPE_CALLBACK_MASK );
            } else {
                mess.mutable_info( )->set_call_type( call_type_ );
            }
        }

        bool send_message( const message_lite &mess, empty_call cb )
        {
            namespace ph = srpc::placeholders;

            if( !ready( ) ) {
                return false;
            }

            buffer_type  buff  = buffer_cache_.get( );
            buffer_slice slice = prepare_message( buff, 0, mess );

            cb_type::post_call_type post_callback =
                    srpc::bind( &noname::push_cache_back, this, ph::_1,
                                buff, cb );

            get_transport( )->write( slice.data( ), slice.size( ),
                                     cb_type::post( post_callback ) );
            return true;
        }

        bool send_message( const message_lite &mess )
        {
            return send_message( mess, &noname::default_cb );
        }

        void assign_on_ready( on_ready_type value )
        {
            if( value ) {
                on_ready_ = value;
            } else {
                on_ready_ = &noname::default_on_ready;
            }
        }

        bool ready(  )
        {
            return ready_;
        }

    protected:

        void set_ready( bool value )
        {
            ready_ = value;
            on_ready_(ready_);
        }

        void push_message_to_cache( message_type msg )
        {
            mess_cache_.push( msg );
        }

        void clear_message( message_type &msg )
        {
            msg->clear_call( );
            msg->clear_opt( );
            msg->clear_request( );
            msg->clear_response( );
        }

        void execute_default( message_type msg )
        {
            using namespace srpc::rpc::errors;
            namespace gpb = google::protobuf;
            typedef srpc::common::protobuf::service::method_type method_type;
            service_sptr svc = get_service( msg );
            if( !svc ) {
                clear_message( msg );
                msg->mutable_error( )->set_code( ERR_NOT_FOUND );
                msg->mutable_error( )->set_category( CATEGORY_PROTOCOL );
                msg->mutable_error( )->set_additional( "Service not found");
                send_message( *msg );
                return;
            }

            const method_type *call =
                    svc->find_method( msg->call( ).method_id( ) );

            if( !call ) {
                clear_message( msg );
                msg->mutable_error( )->set_code( ERR_NO_FUNC );
                msg->mutable_error( )->set_category( CATEGORY_PROTOCOL );
                msg->mutable_error( )->set_additional( "Method not found");
                send_message( *msg );
                return;
            }

            gpb::Message *req = svc->get( )
                    ->GetRequestPrototype( call ).New( );

            gpb::Message *res = svc->get( )
                    ->GetResponsePrototype( call ).New( );;

            req->ParseFromString( msg->request( ) );

            svc->call( call, NULL, req, res, NULL );
            msg->clear_call( );
            msg->clear_opt( );
            msg->clear_request( );
            res->AppendPartialToString(msg->mutable_response( ));

            send_message( *msg );
            mess_cache_.push( msg );
        }

        virtual void execute_call( message_type msg )
        {

            using namespace srpc::rpc::errors;
            namespace gpb = google::protobuf;

            typedef srpc::common::protobuf::service::method_type method_type;

            try {
                execute_default( msg );
            } catch( const std::exception &ex ) { /// std error

            } catch( ... ) { /// all error

            }
        }

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
                    execute_call( mess );
                }
            }
        }

        void append_message( buffer_type buff, const message_type &mess )
        {
            mess->AppendToString( buff.get( ) );
        }

        buffer_slice prepare_message( buffer_type buf, tag_type tag,
                                      const message_lite &mess )
        {
            typedef typename parent_type::size_policy size_policy;

            buf->resize( size_policy::max_length );

            const size_t old_len    = buf->size( );
            const size_t hash_size  = hash( )->length( );

            tag_policy::append( tag, *buf );

            mess.AppendToString( buf.get( ) );

            buf->resize( buf->size( ) + hash_size );

            hash( )->get( buf->c_str( ) + old_len,
                          buf->size( )  - old_len - hash_size,
                          &(*buf)[buf->size( )    - hash_size]);

            buffer_slice res( &(*buf)[old_len],
                              buf->size( ) - old_len );

            buffer_slice packed = pack_message( buf, res );

            return insert_size_prefix( buf, packed );
        }

        static void default_cb( )
        { }

        void push_cache_back( const error_code &err,
                              buffer_type buff, empty_call cb )
        {
            if( !err ) {
                buffer_cache_.push( buff );
                cb( );
            }
        }

        virtual
        service_sptr get_service( const message_type & )
        {
            return service_sptr( );
        }

    private:

        srpc::uint32_t      call_type_;
        message_cache_type  mess_cache_;
        cache_type          buffer_cache_;
        on_ready_type       on_ready_;
        bool                ready_;
    };

}}}

#endif // NONAME_H
