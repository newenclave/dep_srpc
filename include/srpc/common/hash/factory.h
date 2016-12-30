#ifndef SRPC_COMMON_HASH_FACTORY_H
#define SRPC_COMMON_HASH_FACTORY_H

#include <map>

#include "srpc/common/hash/interface.h"
#include "srpc/common/config/mutex.h"
#include "srpc/common/config/functional.h"

namespace srpc { namespace common { namespace hash {

    template <typename MutexType = srpc::dummy_mutex>
    class factory {

        typedef srpc::lock_guard<MutexType> locker_type;

    public:

        typedef MutexType                            mutex_type;
        typedef srpc::function<hash::interface *( )> producer_type;

    private:

        typedef std::map<std::string, producer_type> map_type;

    public:

        hash::interface *create( const std::string &name ) const
        {
            locker_type l(map_lock_);
            map_type::const_iterator f = map_.find( name );
            if( f != map_.end( ) ) {
                return f->second( );
            }
            return NULL;
        }

        void assign_producer( const std::string &name, producer_type producer )
        {
            locker_type l(map_lock_);
            map_[name] = producer;
        }

        void erase( const std::string &name )
        {
            locker_type l(map_lock_);
            map_.erase( name );
        }

    private:
        map_type            map_;
        mutable mutex_type  map_lock_;
    };
}}}

#endif // SRPC_COMMON_HASH_FACTORY_H
