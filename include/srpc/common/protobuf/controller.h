#ifndef SRPC_COMMON_PROTOBUF_CONTROLLER_H
#define SRPC_COMMON_PROTOBUF_CONTROLLER_H

#include "google/protobuf/service.h"
#include "google/protobuf/descriptor.h"

namespace srpc { namespace common { namespace protobuf {

    class controller: public google::protobuf::RpcController {

    public:
        controller( )
            :failed_(false)
            ,canceled_(false)
            ,cancel_cl_(NULL)
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

    private:

        bool                        failed_;
        std::string                 error_string_;
        bool                        canceled_;
        google::protobuf::Closure  *cancel_cl_;
    };

}}}

#endif // CONTROLLER_H
