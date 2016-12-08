#ifndef SRPC_COMMON_QUEUE_TRAIT_SIMPLE_H
#define SRPC_COMMON_QUEUE_TRAIT_SIMPLE_H

#include <queue>

namespace srpc { namespace common { namespace queues {

namespace traits {
    template<typename ValueType,
             typename Container = std::queue<ValueType> >
    struct simple {

        typedef ValueType  value_type;
        typedef Container  queue_type;

        static
        void push( queue_type &self, const value_type &val )
        {
            self.push( val );
        }

        static
        void pop( queue_type &self )
        {
            self.pop( );
        }

        static
        value_type &front( queue_type &self )
        {
            return self.front( );
        }

        static
        const value_type &front( const queue_type &self )
        {
            return self.front( );
        }

        static
        bool empty( const queue_type &self )
        {
            return self.empty( );
        }
    };

}

}}}

#endif // SIMPLE_H
