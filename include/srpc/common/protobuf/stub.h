#ifndef STUB_H
#define STUB_H

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

#include "srpc/common/config/memory.h"
#include "srpc/common/protobuf/channel.h"

namespace srpc { namespace common { namespace protobuf {

    template < typename StubType,
               typename ChannelType = srpc::common::protobuf::channel >
    class stub {

        /// compiletime derived check;
        /// StubType must be derived from google::protobuf::RpcChannel
    //        typedef google::protobuf::RpcChannel gpb_channel;
    //        static const char tc__( gpb_channel * );
    //        enum {
    //            is_gpb_channel__
    //                    = sizeof( tc__( static_cast<ChannelType *>(0) ) )
    //        };

    public:

        typedef StubType    stub_type;
        typedef ChannelType channel_type;

    private:

        typedef stub<stub_type, channel_type> this_type;

    public:

        channel_type *get_channel( )
        {
            return channel_;
        }

        const channel_type *get_channel( ) const
        {
            return channel_;
        }

        stub( channel_type *chan, bool own_channel = false )
            :channel_holder_(own_channel ? chan : NULL)
            ,channel_(chan)
            ,stub_(channel_)
        { }

        stub( srpc::shared_ptr<channel_type> chan )
            :channel_holder_(chan)
            ,channel_(channel_holder_.get( ))
            ,stub_(channel_)
        { }

        /// call(controller, request, response, closure)
        template <typename StubFuncType, typename ReqType, typename ResType>
        void call( StubFuncType func,
                   google::protobuf::RpcController *controller,
                   const ReqType *request, ResType *response,
                   google::protobuf::Closure *closure )
        {
            (stub_.*func)(controller, request, response, closure);
        }

        /// call( request, response )
        template <typename StubFuncType, typename ReqType, typename ResType>
        void call( StubFuncType func, const ReqType *request, ResType *response)
        {
            (stub_.*func)(NULL, request, response, NULL);
        }

        /// call(  )
        template <typename StubFuncType>
        void call( StubFuncType func )
        {
            (stub_.*func)(NULL, NULL, NULL, NULL);
        }

        /// call( request ) send request. Don't need response
        template <typename StubFuncType, typename RequestType>
        void call_request( StubFuncType func, const RequestType *request)
        {
            (stub_.*func)(NULL, request, NULL, NULL);
        }

        /// call( response ) send empty request and get response
        template <typename StubFuncType, typename ResponseType>
        void call_response( StubFuncType func, ResponseType *response )
        {
            (stub_.*func)(NULL, NULL, response, NULL);
        }
    private:
        srpc::shared_ptr<channel_type>   channel_holder_;
        channel_type                    *channel_;
        stub_type                        stub_;
    };

}}}

#endif // STUB_H
