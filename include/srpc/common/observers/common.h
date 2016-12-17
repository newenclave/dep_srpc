#ifndef SRPC_COMMON_OBSERVERS_COMMON_H
#define SRPC_COMMON_OBSERVERS_COMMON_H

#include <list>
#include <set>

#include "srpc/common/config/mutex.h"
#include "srpc/common/details/list.h"
#include "srpc/common/observers/traits/simple.h"

namespace srpc { namespace common { namespace observers {

    template <typename SlotType, typename MutexType>
    class common {
    public:

        typedef SlotType slot_traits;
        typedef typename SlotType::value_type  slot_type;

    private:

        struct slot_info {
            slot_info( const slot_type &slot, size_t id )
                :slot_(slot)
                ,id_(id)
            { }
            slot_type slot_;
            size_t    id_;
        };

        typedef MutexType                     mutex_type;
        typedef srpc::lock_guard<mutex_type>  guard_type;

        typedef details::list<slot_info>      list_type;
        typedef typename list_type::iterator  list_iterator;

        struct iterator_cmp {
            bool operator ( ) ( const list_iterator &l,
                                const list_iterator &r ) const
            {
                /// ptr -> details::list::node
                return l.ptr( ) < r.ptr( );
            }
        };

        typedef std::set<size_t> iterator_set;

        struct param_keeper {

            iterator_set        removed_;
            list_type           list_;
            list_type           added_;

            mutable mutex_type  list_lock_;
            mutable mutex_type  tmp_lock_;
            size_t              id_;

            param_keeper( )
                :id_(1)
            { }

            void add_remove( size_t itr )
            {
                guard_type lck(tmp_lock_);
                removed_.insert( itr );
                remove_by_index(added_, itr);
            }

            bool is_removed( size_t itr )
            {
                guard_type lck(tmp_lock_);
                return (removed_.erase( itr ) != 0);
            }

            void remove_by_index( list_type &lst, size_t id )
            {
                const list_iterator e(lst.end( ));    /// null iterator

                if( lst.size( ) > 0 ) {

                    size_t min_id = lst.begin( )->id_;
                    size_t max_id = lst.rbegin( )->id_;

                    if( (id < min_id) || (id > max_id) ) {

                        return;

                    } else if( (id - min_id) < (max_id - id) ) {
                        /// CLOSE to begin!
                        list_iterator b(lst.begin( ));

                        for( ; (b!=e) && (b->id_<id); ++b );
                        if( (b!=e) && (b->id_ == id) ) {
                            b = lst.erase( b );
                        }

                    } else {
                        /// CLOSE to end!
                        list_iterator b(lst.rbegin( ));

                        for( ; (b!=e) && (id < b->id_); --b );
                        if( (b!=e) && (b->id_ == id) ) {
                            b = lst.rerase( b );
                        }
                    }
                }
            }

            void clear_removed( )
            {
                iterator_set tmp;
                {
                    guard_type lck(tmp_lock_);
                    tmp.swap( removed_ );
                }

                typename iterator_set::iterator b(tmp.begin( ));
                typename iterator_set::iterator e(tmp.end( ));

                list_iterator bl(list_.begin( ));
                list_iterator el(list_.end( ));

                for( ; (b!=e) && (bl!=el); ++b ) {
                    for( ; (bl!=el) && (bl->id_ < *b); ++bl );
                    if( (bl!=el) && (bl->id_ == *b) ) {
                        bl = list_.erase( bl );
                    }
                }
            }

            void clear( )
            {
                guard_type l0(list_lock_);
                guard_type l1(tmp_lock_);
                list_.clear( );
                added_.clear( );
                removed_.clear( );
            }

            void splice_added( )
            {
                guard_type lck(tmp_lock_);
                list_.splice_back( added_ );
            }

            size_t connect( slot_type call )
            {
                guard_type l(tmp_lock_);
                size_t next = id_++;
                added_.push_back( slot_info(call, next) );
                return next;
            }

        };

        typedef srpc::shared_ptr<param_keeper>   param_sptr;
        typedef srpc::weak_ptr<param_keeper>     param_wptr;

    public:

        class connection {

            friend class observers::common<SlotType, MutexType>;
            typedef observers::common<SlotType, MutexType> parent_type;

            connection( const typename parent_type::param_sptr &parent,
                        size_t me )
                :parent_list_(parent)
                ,me_(me)
            { }

            typename parent_type::param_wptr parent_list_;
            size_t                           me_;

        public:

            connection( )
                :me_(0)
            { }

            void disconnect(  )
            {
                parent_type::param_sptr lck(parent_list_.lock( ));
                if( me_ && lck ) {
                    lck->add_remove( me_ );
                    me_ = 0;
                }
            }

            void swap( connection &other )
            {
                parent_list_.swap( other.parent_list_ );
            }
        };

        common( )
            :impl_(srpc::make_shared<param_keeper>( ))
        { }

        //virtual ~common( ) { }

        connection connect( slot_type call )
        {
            size_t next = impl_->connect( call );
            return connection( impl_, next );
        }

        static void disconnect( connection cc )
        {
            cc.disconnect( );
        }

        void disconnect_all(  )
        {
            impl_->clear( );
        }

#if CXX11_ENABLED == 0

#define SRPC_OBSERVER_OPERATOR_PROLOGUE \
            guard_type l(impl_->list_lock_); \
            impl_->splice_added( ); \
            list_iterator b(impl_->list_.begin( )); \
            list_iterator e(impl_->list_.end( )); \
            while( b != e ) { \
                if( !impl_->is_removed( b ) ) { \
                    slot_traits::exec( b->slot_

#define SRPC_OBSERVER_OPERATOR_EPILOGUE \
                    ); \
                    ++b; \
                 } else { \
                    b = impl_->list_.erase( b ); \
                 }\
            } \
            impl_->clear_removed( )

        void operator ( ) ( )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P>
        void operator ( ) ( const P& p0 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1>
        void operator ( ) ( const P0& p0, const P1& p1 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6, typename P7
                  >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6, const P7& p7 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6, p7
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6, typename P7,
                  typename P8
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6, const P7& p7,
                            const P8& p8 )
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6, p7, p8
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5,
                  typename P6, typename P7,
                  typename P8, typename P9
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5,
                            const P6& p6, const P7& p7,
                            const P8& p8, const P9& p9)
        {
            SRPC_OBSERVER_OPERATOR_PROLOGUE,
                    p0, p1, p2, p3, p4, p5, p6, p7, p8, p9
            SRPC_OBSERVER_OPERATOR_EPILOGUE;
        }

#undef SRPC_OBSERVER_OPERATOR_PROLOGUE
#undef SRPC_OBSERVER_OPERATOR_EPILOGUE

#else
        template <typename ...Args>
        void operator ( ) ( const Args& ...args )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            while( b != e ) {
                if( impl_->is_removed( b->id_ ) ) {
                    b = impl_->list_.erase( b );
                } else {
                    slot_traits::exec( b->slot_, args... );
                    ++b;
                }
            }
            impl_->clear_removed( );
        }

#endif
    private:
        param_sptr impl_;
    };

}}}

#endif // SIMPLE_H

