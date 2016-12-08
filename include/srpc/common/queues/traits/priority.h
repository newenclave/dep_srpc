#ifndef SRPC_COMMON_QUEUE_TRAIT_PRIORITY_H
#define SRPC_COMMON_QUEUE_TRAIT_PRIORITY_H

#include <algorithm>
#include <vector>

namespace srpc { namespace common { namespace queues {

namespace traits {

    template<typename ValueType,
             typename Container = std::vector<ValueType>,
             typename Compare = std::less<ValueType> >
    struct priority {

        typedef ValueType  value_type;
        typedef Container  queue_type;

        static
        void push( queue_type &self, const value_type &val )
        {
            self.push_back( val );
            std::push_heap( self.begin( ), self.end( ), Compare( ) );
        }

        static
        void pop( queue_type &self )
        {
            std::pop_heap( self.begin( ), self.end( ), Compare( ) );
            self.pop_back( );
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

#endif // PRIORITY_H
