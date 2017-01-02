#ifndef SRPC_COMMON_PROTOBUF_SERVICE_H
#define SRPC_COMMON_PROTOBUF_SERVICE_H

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"
#include "srpc/common/config/memory.h"

namespace srpc { namespace common { namespace protobuf {

    class service {

    public:

        typedef google::protobuf::Service           service_type;
        typedef srpc::shared_ptr<service_type>      service_sptr;
        typedef srpc::shared_ptr<service>           shared_type;
        typedef google::protobuf::MethodDescriptor  method_type;

        explicit service( service_sptr svc )
            :service_(svc)
        { }

        explicit service( service_type *svc )
            :service_(svc)
        { }

        virtual ~service( ) { }

        service_type *get( )
        {
            return service_.get( );
        }

        const service_type *get( ) const
        {
            return service_.get( );
        }

        static
        shared_type wrap( service_sptr serv )
        {
            return srpc::make_shared<service>( serv );
        }

        static
        shared_type wrap( service_type *serv )
        {
            return srpc::make_shared<service>( serv );
        }

        virtual const std::string &name( )
        {
            return service_->GetDescriptor()->full_name( );
        }

        virtual void call( const method_type *method,
                           google::protobuf::RpcController* controller,
                           const google::protobuf::Message* request,
                           google::protobuf::Message* response,
                           google::protobuf::Closure* done )
        {
            call_default( method, controller, request, response, done );
        }

    protected:

        const method_type *find_method ( const std::string &name ) const
        {
            return service_->GetDescriptor( )->FindMethodByName( name );
        }

        void call_default( const method_type *method,
                           google::protobuf::RpcController* controller,
                           const google::protobuf::Message* request,
                           google::protobuf::Message* response,
                           google::protobuf::Closure* done )
        {
            service_->CallMethod( method, controller, request, response, done );
        }

    private:
        service_sptr service_;
    };

}}}

#endif // WRAPPER_H
