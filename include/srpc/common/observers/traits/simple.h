#ifndef SRPC_COMMON_OBSERVERS_SLOTS_SIMPLE_H
#define SRPC_COMMON_OBSERVERS_SLOTS_SIMPLE_H

#include "srpc/common/config/functional.h"

namespace srpc { namespace common { namespace observers {

namespace traits {

    template <typename SigType>
    struct simple {
        typedef srpc::function<SigType> value_type;
#if CXX11_ENABLED == 0
        static
        void exec( value_type &self )
        {
            self( );
        }

        template <typename P0>
        static
        void exec( value_type &self, const P0 &p0 )
        {
            self(p0);
        }

        template < typename P0, typename P1>
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1 )
        {
            self(p0, p1);
        }

        template < typename P0,  typename P1,
                   typename P2
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2 )
        {
            self(p0, p1, p2);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3 )
        {
            self(p0, p1, p2, p3);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3,
                   typename P4
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3,
                   const P4 &p4 )
        {
            call_(p0, p1, p2, p3, p4);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3,
                   typename P4,  typename P5
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3,
                   const P4 &p4, const P5 &p5 )
        {
            self(p0, p1, p2, p3, p4, p5);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3,
                   typename P4,  typename P5,
                   typename P6
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3,
                   const P4 &p4, const P5 &p5,
                   const P6 &p6 )
        {
            self(p0, p1, p2, p3, p4, p5, p6);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3,
                   typename P4,  typename P5,
                   typename P6,  typename P7
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3,
                   const P4 &p4, const P5 &p5,
                   const P6 &p6, const P7 &p7 )
        {
            self(p0, p1, p2, p3, p4, p5, p6, p7);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3,
                   typename P4,  typename P5,
                   typename P6,  typename P7,
                   typename P8
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3,
                   const P4 &p4, const P5 &p5,
                   const P6 &p6, const P7 &p7,
                   const P8 &p8 )
        {
            self(p0, p1, p2, p3, p4, p5, p6, p7, p8);
        }

        template < typename P0,  typename P1,
                   typename P2,  typename P3,
                   typename P4,  typename P5,
                   typename P6,  typename P7,
                   typename P8,  typename P9
                 >
        static
        void exec( value_type &self,
                   const P0 &p0, const P1 &p1,
                   const P2 &p2, const P3 &p3,
                   const P4 &p4, const P5 &p5,
                   const P6 &p6, const P7 &p7,
                   const P8 &p8, const P9 &p9 )
        {
            self(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
        }

#else
        template <typename ...Args>
        static
        void exec( value_type &self, const Args& ...args )
        {
            self( args... );
        }

        static
        bool expired( value_type & )
        {
            return false;
        }

        static
        void erase( value_type & )
        {
            // std::cout << "Erase!!!!\n";
        }
#endif
    };

}


}}}


#endif // SIMPLE_H
