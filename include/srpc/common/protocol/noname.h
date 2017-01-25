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
#include "srpc/common/protobuf/closure-holder.h"

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

        typedef srpc::shared_ptr<void>  void_sptr;
        typedef srpc::weak_ptr<void>    void_wptr;

        typedef noname<SizePackPolicy> this_type;

        typedef srpc::function<void (void)> empty_call;

        static
        srpc::uint32_t get_call_type( bool server )
        {
            return server ? srpc::rpc::call_info::TYPE_SERVER_CALL
                          : srpc::rpc::call_info::TYPE_CLIENT_CALL;
        }

        class channel: public protobuf::channel {

        public:

            channel( )
                :parent_(NULL)
                ,target_call_(0)
            { }

            channel( srpc::uint64_t target_call )
                :parent_(NULL)
                ,target_call_(target_call)
            {
                if( target_call_ != 0 ) {
                    set_flag( protobuf::channel::USE_CONTEXT_CALL );
                }
            }

            bool alive( ) const
            {
#ifdef CXX11_ENABLED
                return track_.lock( ).get( ) != nullptr;
#else
                return track_.lock( ).get( ) != NULL;
#endif
            }



            void CallMethod( const google::protobuf::MethodDescriptor* method,
                             google::protobuf::RpcController* controller,
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response,
                             google::protobuf::Closure* done)
            {

                typedef typename parent_type::slot_ptr slot_ptr;
                typedef typename parent_type::queue_type::result_enum eresult;

                protobuf::closure_holder hold(done);
                void_sptr lock(track_.lock( ));
                if( lock ) {
                    message_type ll = parent_->mess_cache_.get( );

                    bool wait = !check( protobuf::channel::DISABLE_WAIT );

                    ll->mutable_opt( )->set_wait( wait );
                    parent_->setup_message( *ll, target_call_ );

                    ll->mutable_call( )->set_service_id(
                                method->service( )->full_name( ) );

                    ll->mutable_call( )->set_method_id(
                                method->name( ) );
                    if( request ) {
                        ll->set_request( request->SerializeAsString( ) );
                    }
                    ll->mutable_opt( )->set_accept_response( response != NULL );

                    slot_ptr sl;
                    if( wait ) {
                        sl = parent_->add_slot( ll->id( ) );
                    }

                    bool sent = parent_->send_message( *ll );

                    if( sent ) {
                        if( wait ) {

                            message_type answer;
                            eresult res = sl->read_for( answer,
                                    srpc::chrono::microseconds( timeout( ) ) );

                            std::string fail;

                            switch( res ) {
                            case parent_type::queue_type::OK:
                                break;
                            case parent_type::queue_type::CANCELED:
                                fail = "Call canceled";
                                break;
                            case parent_type::queue_type::TIMEOUT:
                                fail = "Timeout";
                                break;
                            case parent_type::queue_type::NOTFOUND:
                                break;
                            };

                            if( res || answer->error( ).code( ) != 0 ) {
                                if( controller ) {
                                    if( fail.empty( ) ) {
                                        controller->SetFailed(
                                              answer->error( ).additional( ) );
                                    } else {
                                        controller->SetFailed( fail );
                                    }
                                }
                            } else if( response != NULL ) {
                                response->ParseFromString( ll->response( ) );
                            }

                            parent_->mess_cache_.push( answer );
                        }
                    } else {
                        if(controller) {
                            controller->SetFailed( "Channel is not ready" );
                        }
                    }

                    if( sl ) {
                        parent_->erase_slot( sl );
                    }
                    parent_->mess_cache_.push( ll );

                } else {
                    if( controller ) {
                        controller->SetFailed( "Connection is lost." );
                    }
                }
            }

            this_type          *parent_;
            mutable void_wptr   track_;
            srpc::uint64_t      target_call_;
        };

        class call_controller: public google::protobuf::RpcController {
        public:

            call_controller( )
                :failed_(false)
                ,canceled_(false)
                ,cancel_cl_(NULL)
                ,parent_(NULL)
            { }

            void Reset( )
            {
                failed_      = false;
                canceled_    = false;
                cancel_cl_   = NULL;
                error_string_.clear( );
            }

            bool Failed( ) const
            {
                return failed_;
            }

            std::string ErrorText( ) const
            {
                return error_string_;
            }

            void StartCancel( )
            {
                canceled_ = true;
                if( cancel_cl_ ) {
                    cancel_cl_->Run( );
                    cancel_cl_ = NULL;
                }
            }

            void SetFailed( const std::string& reason )
            {
                failed_ = true;
                error_string_.assign( reason );
            }

            bool IsCanceled( ) const
            {
                return canceled_;
            }

            void NotifyOnCancel( google::protobuf::Closure* callback )
            {
                if( canceled_ ) {
                    callback->Run( );
                } else {
                    cancel_cl_ = callback;
                }
            }

            bool                        failed_;
            bool                        canceled_;
            google::protobuf::Closure  *cancel_cl_;
            this_type                  *parent_;
            void_wptr                   track_;
            std::string                 error_string_;
        };

        friend class call_controller;

        struct proto_closure;

        struct call_keeper {

            srpc::unique_ptr<call_controller>            controller;
            srpc::unique_ptr<google::protobuf::Message>  request;
            srpc::unique_ptr<google::protobuf::Message>  response;
            srpc::unique_ptr<proto_closure>              closure;
            srpc::shared_ptr<srpc::rpc::lowlevel>        message;

            static
            srpc::shared_ptr<call_keeper> create( )
            {
                return srpc::make_shared<call_keeper>( );
            }
        };

        struct proto_closure: public google::protobuf::Closure {

            srpc::shared_ptr<call_keeper> call;
            void_wptr                     lck;
            this_type                    *parent;
            bool                          wait;

            void Run( )
            {
                void_sptr lock(lck.lock( ));
                if( lock ) {
                    parent->call_closure( call, wait );
                    lck.reset( );
                    call.reset( );
                }
            }
        };
        friend struct proto_closure;

        typedef srpc::shared_ptr<call_keeper>       call_sptr;
        typedef srpc::weak_ptr<call_keeper>         call_wptr;
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

        typedef srpc::function<void (tag_type,
                                     buffer_type,
                                     const_buffer_slice)> call_function;

        noname( bool server, size_t max_length )
            :parent_type(server ? 100 : 101, max_length)
            ,call_type_(get_call_type(server))
            ,on_ready_(&noname::default_on_ready)
            ,ready_(false)
        { }

        virtual void init( )
        {
            //set_ready( true );
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

        void push_cache_back( const error_code &err,
                              buffer_type buff, empty_call cb,
                              void_wptr track )
        {
            if( !err ) {
                void_sptr lck(track.lock( ));
                if( lck ) {
                    buffer_cache_.push( buff );
                    cb( );
                }
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
                                buff, cb, track_ );

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

        void track( void_sptr t )
        {
            track_ = t;
        }

        protobuf::channel * create_channel( )
        {
            srpc::unique_ptr<channel> inst(new channel);
            inst->track_  = track_;
            inst->parent_ = this;
            return inst.release( );
        }

    protected:

        void set_ready( bool value )
        {
            if( ready_ != value ) {
                ready_ = value;
                on_ready_(ready_);
            }
        }

        void push_message_to_cache( message_type msg )
        {
            mess_cache_.push( msg );
        }

        void clear_message( message_type &msg )
        {
            msg->clear_call( );
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

        void remove_call( call_sptr call )
        {
            srpc::lock_guard<srpc::mutex> lg(calls_lock_);
            calls_.erase( call->message->id( ) );
        }

        void remove_call( srpc::uint64_t id )
        {
            srpc::lock_guard<srpc::mutex> lg(calls_lock_);
            calls_.erase( id );
        }

        call_sptr get_call( srpc::uint64_t id )
        {
            srpc::lock_guard<srpc::mutex> lg(calls_lock_);
            typename call_map::iterator f = calls_.find( id );
            if( calls_.end( ) != f ) {
                return f->second;
            }
        }

        void add_call( call_sptr call )
        {
            srpc::lock_guard<srpc::mutex> lg(calls_lock_);
            calls_.insert( std::make_pair( call->message->id( ), call ) );
        }

        void call_closure( call_sptr call, bool wait )
        {
            using namespace srpc::rpc::errors;

            remove_call( call );

            if( wait ) {
                if( call->controller->IsCanceled( ) ) {
                    set_message_error( call->message, ERR_CANCELED,
                                       "Call canceled" );
                } else {
                    clear_message( call->message );
                    if( call->message->opt( ).accept_response( ) ) {
                        call->message->set_response(
                                 call->response->SerializeAsString( ) );
                    }
                }
                send_message( *call->message );
            }
            mess_cache_.push( call->message );
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

            bool wait = msg->opt( ).wait( );

            call_sptr call_k = call_keeper::create( );

            call_k->message = msg;

            call_k->request.reset( svc->get_request_proto( call ).New( ) );
            call_k->request->ParseFromString( msg->request( ) );

            call_k->response.reset( svc->get_response_proto( call ).New( ) );

            call_k->controller.reset( new call_controller );
            call_k->controller->track_ = track_;
            call_k->controller->parent_ = this;

            srpc::unique_ptr<proto_closure> cp(new proto_closure);
            cp->call   = call_k;
            cp->lck    = track_;
            cp->parent = this;
            cp->wait   = wait;

            call_k->closure.reset( cp.release( ) );

            if( wait ) {
                add_call( call_k );
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
                set_message_error( msg, ERR_CALL_FAILED, ex.what( ) );
            } catch( ... ) { /// all error
                set_message_error( msg, ERR_CALL_FAILED, "..." );
            }

            if( msg->error( ).code( ) != 0 ) {
                remove_call( msg->id( ) );
                send_message( *msg );
                mess_cache_.push( msg );
            }
        }

        void process_internal( message_type )
        {

        }

        void process_service( message_type )
        {

        }

        void process_invalid( message_type )
        {

        }

        void process_answer( message_type mess )
        {
            this->push_to_slot( mess->id( ), mess );
        }

        void process_callback( message_type mess )
        {
            this->push_to_slot( mess->target_id( ), mess );
        }

        void process_call( message_type mess )
        {
            this->execute_call( mess );
        }

        void rpc_ready( tag_type, buffer_type,
                        const_buffer_slice slice )
        {
            message_type mess = mess_cache_.get( );
            mess->ParseFromArray( slice.data( ), slice.size( ) );

            static const srpc::uint32_t cb_mask =
                            srpc::rpc::call_info::TYPE_CALLBACK_MASK;

            const bool callback = !!( mess->info( ).call_type( ) & cb_mask );
            const srpc::uint32_t call_type = ( mess->info( ).call_type( )
                                             & ~cb_mask );

            switch( mess->info( ).call_type( ) ) {
            case srpc::rpc::call_info::TYPE_INTERNAL:
                process_internal( mess );
                break;
            case srpc::rpc::call_info::TYPE_SERVICE:
                process_service( mess );
                break;
            default:
                if( call_type == call_type_ ) {  //// answer
                    process_answer( mess );
                } else {
                    if( callback ) {
                        process_callback( mess );
                    } else if( (call_type & 3 ) != 0 ) { /// direct call
                        process_call( mess );
                    } else {
                        process_invalid( mess );
                    }
                }
            }
        }

        void set_default_call( )
        {
            namespace ph = srpc::placeholders;
            call_ = srpc::bind( &this_type::rpc_ready, this,
                                ph::_1, ph::_2, ph::_3 );
        }

        void set_call( call_function call )
        {
            call_ = call;
        }

        void on_message_ready( tag_type t, buffer_type b,
                               const_buffer_slice slice )
        {
            call_(t, b, slice);
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

            const size_t old_len   = buf->size( );
            const size_t hash_size = this->hash( )->length( );

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

        virtual
        service_sptr get_service( const message_type & )
        {
            return service_sptr( );
        }

        void on_close( )
        {
            set_ready( false );
            this->cancel_all(  );
        }

    private:

        srpc::uint32_t       call_type_;
        message_cache_type   mess_cache_;
        cache_type           buffer_cache_;
        on_ready_type        on_ready_;
        bool                 ready_;
        call_map             calls_;
        srpc::mutex          calls_lock_;
        void_wptr            track_;
        call_function        call_;
    };

}}}

#endif // NONAME_H
