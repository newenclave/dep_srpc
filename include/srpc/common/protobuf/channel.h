#ifndef SRPC_COMMON_PROTOBUF_CHANNEL_H
#define SRPC_COMMON_PROTOBUF_CHANNEL_H

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common { namespace protobuf {

    class channel: public google::protobuf::RpcChannel {
//        virtual void CallMethod(const MethodDescriptor* method,
//                                RpcController* controller,
//                                const Message* request,
//                                Message* response,
//                                Closure* done) = 0;
    private:

    };

}}}

#endif // CHANNEL_H
