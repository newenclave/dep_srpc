#ifndef CONFIG_MEMORY_H
#define CONFIG_MEMORY_H

#if CXX11_ENABLED

#include <memory>
namespace CONFIG_TOPNAMESPACE {
    using std::shared_ptr;
    using std::weak_ptr;
    using std::make_shared;
    using std::enable_shared_from_this;
    using std::unique_ptr;
}

#else

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/enable_shared_from_this.hpp"

namespace CONFIG_TOPNAMESPACE {

    using boost::shared_ptr;
    using boost::weak_ptr;
    using boost::make_shared;
    using boost::enable_shared_from_this;

/// will be add to config.h soon;
#ifndef STD_HAS_UNIQUE_PTR_IMPL

    /// implementation for internal use only
    template<typename T>
    class unique_ptr // noncopyable
    {
        T *ptr_;

        unique_ptr( unique_ptr const & );
        unique_ptr &operator = ( unique_ptr const & );

        typedef unique_ptr<T> this_type;

        void operator == ( unique_ptr const& ) const;
        void operator != ( unique_ptr const& ) const;

    public:

        typedef T element_type;

        explicit unique_ptr( element_type *p = NULL ) // throws( )
            :ptr_( p )
        { }

        ~unique_ptr( ) // throws( )
        {
            typedef char complete_type[ sizeof(element_type) ? 1 : -1 ];
            (void) sizeof(complete_type);
            delete ptr_;
        }

        void reset( element_type *p = NULL ) // throws( )
        {
            this_type( p ).swap( *this );
        }

        element_type *release( )
        {
            element_type *t = ptr_;
            ptr_ = NULL;
            return t;
        }

        element_type &operator * ( ) const // throws( )
        {
            return *ptr_;
        }

        element_type *operator -> ( ) const
        {
            assert( ptr_ != 0 );
            return ptr_;
        }

        element_type *get( ) const
        {
            return ptr_;
        }

        void swap(unique_ptr & other) // throws( )
        {
            element_type *
            tmp        = other.ptr_;
            other.ptr_ = ptr_;
            ptr_       = tmp;
        }

    };

#endif

}

#endif

namespace CONFIG_TOPNAMESPACE {

    template<typename T>
    class ptr_keeper {

        T *ptr_;

        ptr_keeper( ptr_keeper const & );
        ptr_keeper &operator = ( ptr_keeper const & );

        typedef ptr_keeper<T> this_type;

        void operator == ( ptr_keeper const& ) const;
        void operator != ( ptr_keeper const& ) const;

    public:

        explicit ptr_keeper( T *p = NULL ) // throws( )
            :ptr_( p )
        { }

        ~ptr_keeper( ) // throws( )
        {
            if( ptr_ ) delete ptr_;
        }

        T *get( ) const
        {
            return ptr_;
        }

        T *release(  )
        {
            T *t = ptr_;
            ptr_ = NULL;
            return t;
        }
    };
}

#endif // MEMORY_H
