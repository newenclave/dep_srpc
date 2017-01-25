#ifndef SRPC_COMMON_PROTOBUF_CLOSURE_HOLDER_H
#define SRPC_COMMON_PROTOBUF_CLOSURE_HOLDER_H

#include "google/protobuf/stubs/common.h"

namespace srpc { namespace common { namespace protobuf {

    class closure_holder {

    public:

        explicit closure_holder( google::protobuf::Closure *cl )
            :cl_(cl)
        { }

#ifdef CXX11_ENABLED

        closure_holder (const closure_holder &) = delete;
        closure_holder & operator = (const closure_holder &) = delete;

        closure_holder (closure_holder &&other)
            :cl_(other.cl_)
        {
            other.cl_ = nullptr;
        }

        closure_holder & operator = (closure_holder &&other)
        {
            cl_ = other.cl_;
            other.cl_ = nullptr;
            return *this;
        }
#else
    private:
        closure_holder (const closure_holder &);
        closure_holder & operator = (const closure_holder &);
    public:
#endif
        ~closure_holder( )
        {
            if( cl_ ) {
                cl_->Run( );
            }
        }

    public:
        google::protobuf::Closure *cl_;
    };

}}}

#endif // CLOSUREHOLDER_H
