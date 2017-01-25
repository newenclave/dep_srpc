#ifndef SRPC_COMMON_PROTOBUF_CHANNEL_H
#define SRPC_COMMON_PROTOBUF_CHANNEL_H

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "srpc/common/config/stdint.h"
#include "srpc/common/config/chrono.h"

namespace srpc { namespace common { namespace protobuf {

    class channel: public google::protobuf::RpcChannel {

    public:

        enum flags {
             DEFAULT             = 0
            ,DISABLE_WAIT        = 1
            ,USE_CONTEXT_CALL    = 1 << 1
//            ,USE_STATIC_CONTEXT  = 1 << 2 /// only with USE_CONTEXT_CALL
//            ,CONTEXT_NOT_REQUIRE = 1 << 3 /// only with USE_CONTEXT_CALL
        };

        channel( )
            :options_(0)
            ,timeout_(0)
        { }

        virtual void set_flags( srpc::uint32_t flags )
        {
            options_ = flags;
        }

        bool check( srpc::uint32_t flag ) const
        {
            return ( options_ & flag ) == flag;
        }

        virtual srpc::uint32_t get_flags( ) const
        {
            return options_;
        }

        void set_flag( srpc::uint32_t flag )
        {
            options_ |= flag;
        }

        void reset_flag( srpc::uint32_t flag )
        {
            options_ &= (~flag);
        }

        srpc::uint64_t timeout( ) const
        {
            return timeout_;
        }

        void set_timeout( srpc::uint64_t new_value )
        {
            timeout_ = new_value;
        }

        virtual bool alive( ) const = 0;

//        virtual void CallMethod(const MethodDescriptor* method,
//                                RpcController* controller,
//                                const Message* request,
//                                Message* response,
//                                Closure* done) = 0;
    private:
        srpc::uint32_t options_;
        srpc::uint64_t timeout_;
    };

}}}

#endif // CHANNEL_H
