#include <iostream>
#include <list>
#include <mutex>
#include <set>
#include <atomic>

#include "srpc/common/config/memory.h"
#include "srpc/common/config/functional.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"

#include "boost/signals2.hpp"

using namespace srpc;

using size_policy = common::sizepack::varint<size_t>;

struct keeper: public srpc::enable_shared_from_this<keeper> {

};

template <typename T,
          typename MutexType = std::mutex>
class signal {
public:
    typedef std::function<T> call_type;

private:

    typedef MutexType                     mutex_type;
    typedef std::lock_guard<mutex_type>   guard_type;

    struct call_holder {

        explicit call_holder( const call_type &c )
            :call(c)
        { }

        call_holder (const call_holder &other)
            :call(other.call)
        { }

        call_holder &operator = (const call_holder &other)
        {
            call = other.call;
            return *this;
        }

        call_type     call;
    };

    typedef std::list<call_holder>        list_type;

    typedef typename list_type::iterator  list_iterator;
    struct iterator_cmp {
        bool operator ( ) ( const list_iterator &l,
                            const list_iterator &r ) const
        {
            return &(*l) < &(*r);
        }
    };

    typedef std::set<list_iterator, iterator_cmp> iterator_set;

    struct param_keeper {

        iterator_set        set_;
        list_type           list_;

        mutable mutex_type  set_lock_;
        mutable mutex_type  list_lock_;

        void add( list_iterator itr )
        {
            guard_type lck(set_lock_);
            set_.insert( itr );
        }

        bool removed( list_iterator itr ) const
        {
            guard_type lck(set_lock_);
            return set_.find( itr ) != set_.end( );
        }

        void clear_removed( )
        {
            iterator_set tmp;
            {
                guard_type lck(set_lock_);
                tmp.swap( set_ );
            }

            typename iterator_set::iterator b(tmp.begin( ));
            typename iterator_set::iterator e(tmp.end( ));

            for( ;b != e ; ++b ) {
                list_.erase( *b );
            }
        }
    };

    typedef std::shared_ptr<param_keeper>   param_sptr;
    typedef std::weak_ptr<param_keeper>     param_wptr;

public:

    class connection {

        friend class signal<T, MutexType>;
        typedef signal<T, MutexType> parent_type;

        connection( const parent_type::param_sptr &parent,
                    parent_type::list_iterator me )
            :parent_list_(parent)
            ,me_(me)
        { }

        //parent_type::set_keeper      &parent_list_;
        parent_type::param_wptr     parent_list_;
        parent_type::list_iterator  me_;

    public:

        void disconnect(  )
        {
            parent_type::param_sptr lck(parent_list_.lock( ));
            if( lck ) {
                lck->add( me_ );
            }
        }
    };

    signal( )
        :impl_(std::make_shared<param_keeper>( ))
    { }

    connection connect(  call_type call )
    {
        guard_type l(impl_->list_lock_);
        impl_->list_.push_back( call_holder(call) );
        return connection( impl_, impl_->list_.rbegin( ).base( ) );
    }

    static
    void disconnect( connection cc )
    {
        cc.disconnect( );
    }

    void operator ( ) ( )
    {
        guard_type l(impl_->list_lock_);
        list_iterator b(impl_->list_.begin( ));
        list_iterator e(impl_->list_.end( ));
        for( ;b != e; ++b ) {
            if( !impl_->removed( b ) ) {
                b->call( );
            }
        }
        impl_->clear_removed( );
    }

    template <typename P>
    void operator ( ) ( const P& p0 )
    {
        guard_type l(impl_->list_lock_);
        list_iterator b(impl_->list_.begin( ));
        list_iterator e(impl_->list_.end( ));
        for( ;b != e; ++b ) {
            if( !impl_->removed( b ) ) {
                b->call(p0);
            }
        }
        impl_->clear_removed( );
    }

    template <typename P0, typename P1>
    void operator ( ) ( const P0& p0, const P1& p1 )
    {
        guard_type l(impl_->list_lock_);
        list_iterator b(impl_->list_.begin( ));
        list_iterator e(impl_->list_.end( ));
        for( ;b != e; ++b ) {
            if( !impl_->removed( b ) ) {
                b->call(p0, p1);
            }
        }
        impl_->clear_removed( );
    }

private:
    param_sptr impl_;
};

struct fake_mutex {
    void lock( ) { }
    void unlock( ) { }
};

namespace rrr {
    template <typename T>
    struct result;

    template <typename T>
    struct result<T()> {
        typedef T type;
    };

    template <typename T, typename P0>
    struct result<T(P0)> {
        typedef T type;
    };

    template <typename T, typename P0, typename P1>
    struct result<T(P0, P1)> {
        typedef T type;
    };

    template <typename T, typename P0,
                          typename P1,
                          typename P2>
    struct result<T(P0, P1, P2)> {
        typedef T type;
    };
}

std::atomic<std::uint32_t> gcounter {0};

int main( int argc, char *argv[] )
{

    typename rrr::result<double(int, const std::string&, int)>::type K = 100.100;

    std::cout << K << "\n";
    return 0;
    //using sig  = signal<void (int), fake_mutex>;
    using sig  = signal<void (int)>;

//    using bsig = boost::signals2::signal_type<void (int),
//                         boost::signals2::keywords::mutex_type<boost::signals2::dummy_mutex> >::type;
    using bsig = boost::signals2::signal_type<void (int)>::type;


    std::cout << sizeof(sig) << " "
              << sizeof(bsig) << " "
              << sizeof(std::mutex)
              << std::endl;

    sig s;

    auto lambda = []( int i ){
        gcounter += i;
    };

    for( int i = 0; i<300000; i++ ) {
        s.connect( lambda );
    }

    for( int i = 0; i<1000; i++ ) {
        s( 1 );
    }

//    for( int i = 0; i<30000000; i++ ) {
//        lambda( 1 );
//    }

    std::cout << gcounter << "\n";

    return 0;
}
