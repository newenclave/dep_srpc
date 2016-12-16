#ifndef SRPC_COMMON_OBSERVERS_COMMON_H
#define SRPC_COMMON_OBSERVERS_COMMON_H

#include <list>
#include <set>

#include "srpc/common/config/functional.h"
#include "srpc/common/config/mutex.h"
#include "srpc/common/details/list.h"
#include "srpc/common/observers/traits/simple.h"


namespace srpc { namespace common { namespace observers {

    template <typename T,
              typename MutexType = srpc::mutex,
              typename SlotType  = traits::simple<T> >
    class common {
    public:

        typedef srpc::function<T> sig_type;
        typedef SlotType          slot_type;

    private:

        typedef MutexType                     mutex_type;
        typedef srpc::lock_guard<mutex_type>  guard_type;

        typedef details::list<typename slot_type::value_type> list_type;
        typedef typename list_type::iterator                  list_iterator;

        struct iterator_cmp {
            bool operator ( ) ( const list_iterator &l,
                                const list_iterator &r ) const
            {
                return l.ptr( ) < r.ptr( );
            }
        };

        typedef std::set<list_iterator, iterator_cmp> iterator_set;

        struct param_keeper {

            iterator_set        set_;
            list_type           list_;
            list_type           added_;

            mutable mutex_type  list_lock_;
            mutable mutex_type  tmp_lock_;

            void add2remove( list_iterator itr )
            {
                guard_type lck(tmp_lock_);
                set_.insert( itr );
            }

            bool removed( list_iterator itr ) const
            {
                guard_type lck(tmp_lock_);
                return set_.find( itr ) != set_.end( );
            }

            void clear_removed( )
            {
                iterator_set tmp;
                {
                    guard_type lck(tmp_lock_);
                    tmp.swap( set_ );
                }

                typename iterator_set::iterator b(tmp.begin( ));
                typename iterator_set::iterator e(tmp.end( ));

                for( ; b != e; ++b ) {
                    if( *b != list_.end( ) ) {
                        list_.erase( *b );
                    }
                }
            }

            list_iterator splice_added( )
            {
                guard_type lck(tmp_lock_);
                list_iterator b = added_.begin( );
                list_.splice_back( added_ );
                //list_.splice( list_.end( ), added_ );
                return b;
            }

        };

        typedef srpc::shared_ptr<param_keeper>   param_sptr;
        typedef srpc::weak_ptr<param_keeper>     param_wptr;

    public:

        class connection {

            friend class observers::common<T, MutexType, SlotType>;
            typedef observers::common<T, MutexType, SlotType> parent_type;

            connection( const typename parent_type::param_sptr &parent,
                        typename parent_type::list_iterator me )
                :parent_list_(parent)
                ,me_(me)
            { }

            //parent_type::set_keeper      &parent_list_;
            typename parent_type::param_wptr     parent_list_;
            typename parent_type::list_iterator  me_;

        public:

            connection( ) { }

            void disconnect(  )
            {
                parent_type::param_sptr lck(parent_list_.lock( ));
                if( lck ) {
                    //guard_type l(me_->lock);
                    lck->add2remove( me_ );
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

        connection connect( typename slot_type::value_type call )
        {
            guard_type l(impl_->tmp_lock_);
            impl_->added_.push_back( call );
            return connection( impl_, impl_->list_.rbegin( ) );
        }

        static void disconnect( connection cc )
        {
            cc.disconnect( );
        }

#if CXX11_ENABLED == 0
        void operator ( ) ( )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b );
                }
            }
            impl_->clear_removed( );
        }

        template <typename P>
        void operator ( ) ( const P& p0 )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );

            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            impl_->splice_added( );
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0 );
                }
            }
            impl_->clear_removed( );
        }

        template <typename P0, typename P1>
        void operator ( ) ( const P0& p0, const P1& p1 )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0, p1);
                }
            }
            impl_->clear_removed( );
        }

        template <typename P0, typename P1,
                  typename P2
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2 )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0, p1, p2);
                }
            }
            impl_->clear_removed( );
        }

        template <typename P0, typename P1,
                  typename P2, typename P3
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3 )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0, p1, p2, p3);
                }
            }
            impl_->clear_removed( );
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4 )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0, p1, p2, p3, p4);
                }
            }
            impl_->clear_removed( );
        }

        template <typename P0, typename P1,
                  typename P2, typename P3,
                  typename P4, typename P5
                 >
        void operator ( ) ( const P0& p0, const P1& p1,
                            const P2& p2, const P3& p3,
                            const P4& p4, const P5& p5 )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0, p1, p2, p3, p4, p5);
                }
            }
            impl_->clear_removed( );
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
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, p0, p1, p2, p3, p4, p5, p6);
                }
            }
            impl_->clear_removed( );
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
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec(p0, p1, p2, p3, p4, p5, p6, p7);
                }
            }
            impl_->clear_removed( );
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
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec(p0, p1, p2, p3, p4, p5, p6, p7, p8);
                }
            }
            impl_->clear_removed( );
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
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec(p0, p1, p2, p3, p4, p5, p6, p7, p8);
                }
            }
            impl_->clear_removed( );
        }
#else
        template <typename ...Args>
        void operator ( ) ( const Args& ...args )
        {
            guard_type l(impl_->list_lock_);
            impl_->splice_added( );
            list_iterator b(impl_->list_.begin( ));
            list_iterator e(impl_->list_.end( ));
            for( ;b != e; ++b ) {
                if( !impl_->removed( b ) ) {
                    slot_type::exec( *b, args... );
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

