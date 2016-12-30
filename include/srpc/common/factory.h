#ifndef SRPC_COMMON_FACTORY_H
#define SRPC_COMMON_FACTORY_H

#include <map>

#include "srpc/common/config/mutex.h"
#include "srpc/common/config/functional.h"

namespace srpc { namespace common {

    template <typename KeyType, typename ObjectType,
              typename MutexType = srpc::dummy_mutex>
    class factory {

        typedef srpc::lock_guard<MutexType> locker_type;

    public:

        typedef KeyType                          key_type;
        typedef ObjectType                       object_type;
        typedef MutexType                        mutex_type;

        typedef srpc::function<object_type ( )> producer_type;

    private:

        typedef std::map<key_type, producer_type> map_type;

    public:

        object_type create( const key_type &name ) const
        {
            locker_type l(map_lock_);
            map_type::const_iterator f = map_.find( name );
            if( f != map_.end( ) ) {
                return f->second( );
            }
            return object_type( );
        }

        void assign( const key_type &name, producer_type producer )
        {
            locker_type l(map_lock_);
            map_[name] = producer;
        }

        void erase( const key_type &name )
        {
            locker_type l(map_lock_);
            map_.erase( name );
        }

    private:
        map_type            map_;
        mutable mutex_type  map_lock_;
    };

}}

#endif // FACTORY_H
