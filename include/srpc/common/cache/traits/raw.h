#ifndef SRPC_COMMON_CHACHE_TRAITS_RAW_H
#define SRPC_COMMON_CHACHE_TRAITS_RAW_H

namespace srpc { namespace common { namespace cache {

namespace traits {

    template <typename T>
    struct raw {
        typedef T * value_type;

        static
        value_type create( )
        {
            return new T;
        }

        static
        void destroy( value_type val )
        {
            delete val;
        }
    };
}

}}}


#endif // RAW_H
