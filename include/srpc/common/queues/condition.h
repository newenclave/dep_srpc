#ifndef SRPC_COMMON_QUEUE_CONDITION_H
#define SRPC_COMMON_QUEUE_CONDITION_H

#include <map>
#include "srpc/common/config/mutex.h"
#include "srpc/common/config/memory.h"
#include "srpc/common/config/condition-variable.h"
#include "srpc/common/config/stdint.h"

#include "srpc/common/queues/traits/simple.h"
#include "srpc/common/queues/traits/priority.h"

namespace srpc { namespace common { namespace queues {

    template<typename KeyType, typename ValueType,
             typename QueueTrait = traits::simple<ValueType> >
    class condition {

        typedef srpc::lock_guard<srpc::mutex>  locker;
        typedef srpc::unique_lock<srpc::mutex> unique_lock;

    public:

        typedef QueueTrait queue_trait;
        typedef typename   queue_trait::queue_type queue_type;

        enum result_enum {
            CANCELED = 0,
            OK       = 1,
            TIMEOUT  = 2,
            NOTFOUND = 3,
        };

        struct slot_type {

            queue_type               data_;
            mutable srpc::mutex      lock_;
            srpc::condition_variable cond_;
            srpc::uint32_t           flag_;

            typedef condition<KeyType, ValueType, QueueTrait> parent_type;
            typedef typename parent_type::result_enum result_enum;
        public:

            typedef typename parent_type::value_type value_type;

            enum flags {
                NONE      = 0x00,
                CANCELED  = 0x01,
            };

            slot_type( )
                :flag_(NONE)
            { }

            void cancel( )
            {
                locker l(lock_);
                flag_ |= CANCELED;
                cond_.notify_all( );
            }

            bool empty( ) const
            {
                return queue_trait::empty( data_ );
            }

            void push( const value_type &value )
            {
                locker l(lock_);
                queue_trait::push( data_, value );
                cond_.notify_one( );
            }

            template <typename TimeDuration>
            result_enum read_for( value_type &result, const TimeDuration &td )
            {
                unique_lock ul(lock_);
                bool res = cond_.wait_for( ul, td, not_empty( this ) );
                if( res ) {
                    if( !canceled( ) ) {
                        result = queue_trait::front( data_ );
                        queue_trait::pop( data_ );
                        return parent_type::OK;
                    } else {
                        return parent_type::CANCELED;
                    }
                } else {
                    return parent_type::TIMEOUT;
                }
            }

            bool canceled( ) const
            {
                return ( flag_ & CANCELED );
            }

            struct not_empty {
                const slot_type *parent;

                not_empty( const slot_type *p )
                    :parent(p)
                { }

                not_empty( const not_empty &o )
                    :parent(o.parent)
                { }

                not_empty & operator = ( const not_empty &o )
                {
                    parent = o.parent;
                    return *this;
                }

                // locked
                bool operator ( ) ( ) const
                {
                    return !parent->empty( ) || parent->canceled( );
                }
            };
        };

        typedef srpc::shared_ptr<slot_type> slot_ptr;

    private:

        typedef std::map<size_t, slot_ptr> map_type;

        map_type     map_;
        srpc::mutex  map_lock_;

    public:

        typedef KeyType   key_type;
        typedef ValueType value_type;

        condition( )
        { }

        slot_ptr get_slot( const KeyType &index )
        {
            slot_ptr res;
            {
                locker l(map_lock_);
                typename map_type::iterator f = map_.find( index );
                if( f != map_.end( ) ) {
                    res = f->second;
                }
            }
            return res;
        }

        slot_ptr add_slot( const KeyType &index )
        {
            slot_ptr slot = srpc::make_shared<slot_type>( );
            {
                locker l(map_lock_);
                map_.insert( std::make_pair( index, slot ) );
            }
            return slot;
        }

        result_enum erase_slot( const KeyType &index )
        {
            locker l(map_lock_);
            typename map_type::iterator f = map_.find( index );
            if( f != map_.end( ) ) {
                if( !f->second->canceled( ) ) {
                    f->second->cancel( );
                }
                map_.erase( f );
                return OK;
            }
            return NOTFOUND;
        }

        result_enum cancel_slot( const KeyType &index )
        {
            locker l(map_lock_);
            typename map_type::iterator f = map_.find( index );
            if( f != map_.end( ) ) {
                f->second->cancel( );
                return OK;
            }
            return NOTFOUND;
        }

        result_enum push_to_slot( const key_type &index,
                                  const value_type &value )
        {
            slot_ptr slot = get_slot( index );
            if( slot ) {
                slot->push( value );
                return OK;
            }
            return NOTFOUND;
        }

        template <typename TimeDuration>
        result_enum read_slot( const key_type &id, value_type &result,
                               const TimeDuration &td )
        {
            slot_ptr slot = get_slot( id );
            if( slot ) {
                return slot->read_for( result, td );
            } else {
                return NOTFOUND;
            }
        }

    };


}}}

#endif
