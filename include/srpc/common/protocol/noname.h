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

    template <typename SizePackPolicy = sizepack::varint<srpc::uint32_t> >
    class noname: public binary< srpc::shared_ptr<srpc::rpc::lowlevel>,
                                 sizepack::none, SizePackPolicy,
                                 srpc::uint64_t >,
                  public srpc::enable_shared_from_this<noname<SizePackPolicy> >
    {

        typedef binary< srpc::shared_ptr<srpc::rpc::lowlevel>,
                        sizepack::none, SizePackPolicy,
                        srpc::uint64_t > parent_type;

        typedef noname<SizePackPolicy> this_type;

        typedef srpc::function<void (void)> empty_call;

        static
        srpc::uint32_t get_call_type( bool server )
        {
            return server ? srpc::rpc::call_info::TYPE_SERVER_CALL
                          : srpc::rpc::call_info::TYPE_CLIENT_CALL;
        }

        struct call_keeper: enable_shared_from_this<call_keeper> {

            srpc::unique_ptr<protobuf::controller>       controller;
            srpc::unique_ptr<google::protobuf::Message>  request;
            srpc::unique_ptr<google::protobuf::Message>  response;
            srpc::unique_ptr<google::protobuf::Closure>  closure;
            srpc::function<void ( )>                     done;
            srpc::shared_ptr<srpc::rpc::lowlevel>        message;
            static
            srpc::shared_ptr<call_keeper> create( )
            {
                return srpc::make_shared<call_keeper>( );
            }
        };

        typedef srpc::shared_ptr<call_keeper>   call_sptr;
        typedef srpc::weak_ptr<call_keeper>     call_wptr;
        typedef std::map<srpc::uint64_t, call_sptr> call_map;

        static void default_on_ready( bool )
        { }

        static void default_write_cb( )
        { }

    protected:
        struct key { };
    public:

        typedef SizePackPolicy sizepack_polisy;
        typedef srpc::function<void (bool)> on_ready_type;

        typedef srpc::rpc::lowlevel                      lowlevel_message_type;
        typedef typename parent_type::message_type       message_type;
        typedef typename parent_type::tag_type           tag_type;
        typedef typename parent_type::tag_policy         tag_policy;
        typedef typename parent_type::buffer_type        buffer_type;
        typedef typename parent_type::const_buffer_slice const_buffer_slice;
        typedef typename parent_type::buffer_slice       buffer_slice;
        typedef typename parent_type::error_code         error_code;
        typedef google::protobuf::MessageLite            message_lite;

        typedef SRPC_ASIO::io_service io_service;
        typedef common::transport::interface::write_callbacks   cb_type;
        typedef srpc::common::protobuf::service::shared_type    service_sptr;

        typedef srpc::common::factory< std::string,
                                       service_sptr,
                                       srpc::mutex > service_factory;

        typedef common::cache::simple<std::string>          cache_type;
        typedef common::cache::simple<srpc::rpc::lowlevel>  message_cache_type;

        noname( bool server, size_t max_length )
            :parent_type(server ? 100 : 101, max_length)
            ,call_type_(get_call_type(server))
            ,on_ready_(&noname::default_on_ready)
            ,ready_(false)
        {
            init( );
        }

        void init( )
        {
            set_ready( true );
        }

        void setup_message( lowlevel_message_type &mess, srpc::uint64_t target )
        {
            mess.set_id( this->next_id( ) );
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

            this->get_transport( )->write( slice.data( ), slice.size( ),
                                           cb_type::post( post_callback ) );
            return true;
        }

        bool send_message( const message_lite &mess )
        {
            return send_message( mess, &noname::default_write_cb );
        }

        void assign_on_ready( on_ready_type value )
        {
            if( value ) {
                on_ready_ = value;
            } else {
                on_ready_ = &noname::default_on_ready;
            }
        }

        bool ready(  ) const
        {
            return ready_;
        }

        void track( srpc::shared_ptr<void> t )
        {
            track_ = t;
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

        void set_message_error( message_type &msg, int code,
                                const std::string &add )
        {
            using namespace srpc::rpc::errors;
            clear_message( msg );
            msg->mutable_error( )->set_code( code );
            msg->mutable_error( )->set_category( CATEGORY_PROTOCOL );
            msg->mutable_error( )->set_additional( add );
        }

        srpc::function<void ( )> create_cb( call_sptr c, bool wait )
        {
            srpc::function<void ()> call =
                    srpc::bind( &this_type::call_closure, this,
                                call_wptr(c), wait );
            return call;
        }

        void remove_call( call_sptr call )
        {
            srpc::lock_guard<srpc::mutex> lg(calls_lock_);
            calls_.erase( call->message->id( ) );
        }

        void call_closure( call_wptr call, bool wait )
        {
            using namespace srpc::rpc::errors;
            call_sptr lock( call.lock( ) );
            if( lock ) {

                remove_call( lock );

                if( wait ) {
                    if( lock->controller->IsCanceled( ) ) {
                        set_message_error( lock->message, ERR_CANCELED,
                                           "Call canceled" );
                    } else {
                        clear_message( lock->message );
                    }
                    send_message( *lock->message );
                }
                mess_cache_.push( lock->message );
            }
        }

        void execute_default( message_type msg )
        {
            using namespace srpc::rpc::errors;
            namespace gpb = google::protobuf;
            typedef srpc::common::protobuf::service::method_type method_type;
            service_sptr svc = get_service( msg );
            if( !svc ) {
                set_message_error( msg, ERR_NOT_FOUND, "Service not found" );
                send_message( *msg );
                mess_cache_.push( msg );
                return;
            }

            const method_type *call =
                    svc->find_method( msg->call( ).method_id( ) );

            if( !call ) {
                set_message_error( msg, ERR_NO_FUNC, "Method not found" );
                send_message( *msg );
                mess_cache_.push( msg );
                return;
            }

            struct proto_closure: public google::protobuf::Closure {
                srpc::weak_ptr<call_keeper> call;
                void Run( )
                {
                    srpc::shared_ptr<call_keeper> lck(call.lock( ));
                    if( lck ) {
                        lck->done( );
                    }
                }
            };

            bool wait = msg->opt( ).wait( );

            call_sptr call_k = call_keeper::create( );

            call_k->message = msg;
            call_k->request.reset( svc->get( )
                                      ->GetRequestPrototype( call ).New( ));

            call_k->response.reset(svc->get( )
                                      ->GetResponsePrototype( call ).New( ));

            call_k->controller.reset( new protobuf::controller );

            call_k->request->ParseFromString( msg->request( ) );

            srpc::unique_ptr<proto_closure> cp(new proto_closure);
            cp->call = call_k;

            call_k->closure.reset( cp.release( ) );

            call_k->done = create_cb( call_k, wait );

            if( wait ) {
                calls_.insert( std::make_pair( msg->id( ), call_k ) );
            } else {

            }

            svc->call( call, call_k->controller.get( ),
                       call_k->request.get( ),
                       call_k->response.get( ),
                       call_k->closure.get( ) );

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

            //// answer
            if( call_type == call_type_ ) {
                this->push_to_slot( mess->id( ), mess );
            } else {
                /// callback
                if( callback ) {
                    this->push_to_slot( mess->target_id( ), mess );
                } else if( call_type != 0 ) { /// call
                    this->execute_call( mess );
                } else {
                    /// invalid message type
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
            const size_t hash_size  = this->hash( )->length( );

            tag_policy::append( tag, *buf );

            mess.AppendToString( buf.get( ) );

            buf->resize( buf->size( ) + hash_size );

            this->hash( )->get( buf->c_str( ) + old_len,
                                buf->size( )  - old_len - hash_size,
                             &(*buf)[buf->size( )    - hash_size]);

            buffer_slice res( &(*buf)[old_len],
                                 buf->size( ) - old_len );

            buffer_slice packed = this->pack_message( buf, res );

            return this->insert_size_prefix( buf, packed );
        }

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

        srpc::uint32_t       call_type_;
        message_cache_type   mess_cache_;
        cache_type           buffer_cache_;
        on_ready_type        on_ready_;
        bool                 ready_;
        call_map             calls_;
        srpc::mutex          calls_lock_;
        srpc::weak_ptr<void> track_;
    };

}}}

#endif // NONAME_H
