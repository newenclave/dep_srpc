#ifndef SRPC_COMMON_DETAILS_LIST_H
#define SRPC_COMMON_DETAILS_LIST_H

#include "srpc/common/config/stdint.h"

namespace srpc { namespace common { namespace details {

    //// NOT STL LIST!!!
    template <typename T>
    class list {
        struct node {
            node *prev_;
            node *next_;
            T data_;
            node( const T &data )
                :prev_(NULL)
                ,next_(NULL)
                ,data_(data)
            { }

            T &get( )
            {
                return data_;
            }

            const T &get( ) const
            {
                return data_;
            }

            T * operator -> ( )
            {
                return &data_;
            }

            const T * operator -> ( ) const
            {
                return &data_;
            }
        };

    public:

        typedef T value_type;

        class iterator {

            friend class list<T>;
            node *node_;
            iterator(node *n)
                :node_(n)
            { }

            void invalidate( )
            {
                node_ = NULL;
            }

            bool invalid( ) const
            {
                return node_ == NULL;
            }

        public:

            srpc::uintptr_t ptr( ) const
            {
                return static_cast<srpc::uintptr_t>(node_);
            }

            T & operator * ( )
            {
                return node_->get( );
            }

            const T & operator * ( ) const
            {
                return node_->get( );
            }

            T * operator -> ( )
            {
                return &node_->data_;
            }

            const T * operator -> ( ) const
            {
                return &node_->data_;
            }

            iterator operator ++ ( )
            {
                node_ = node_->next_;
                return iterator(node_);
            }

            iterator operator -- ( )
            {
                node_ = node_->prev_;
                return iterator(node_);
            }

            iterator operator ++ ( int )
            {
                iterator tmp(node_);
                node_ = node_->next_;
                return tmp;
            }

            iterator operator -- ( int )
            {
                iterator tmp(node_);
                node_ = node_->prev_;
                return tmp;
            }

            bool operator == ( const iterator &o ) const
            {
                return node_ == o.node_;
            }

            bool operator != ( const iterator &o ) const
            {
                return node_ != o.node_;
            }

        };

        list( )
            :front_(NULL)
            ,back_(NULL)
        { }

        ~list( )
        {
            node *p = front_;
            while( p ) {
                node *tmp = p->next_;
                delete p;
                p = tmp;
            }
        }

        iterator begin( )
        {
            return iterator(front_);
        }

        iterator rbegin( )
        {
            return iterator(back_);
        }

        iterator end( )
        {
            return iterator(NULL);
        }

        iterator rend( )
        {
            return end( );
        }

        iterator erase( iterator itr )
        {
            if( !itr.invalid( ) ) {
                node *tmp = itr.node_->next_;
                if( itr.node_->prev_ ) {
                    itr.node_->prev_->next_ = itr.node_->next_;
                }

                if( itr.node_->next_ ) {
                    itr.node_->next_->prev_ = itr.node_->prev_;
                }

                if( itr.node_ == back_ ) {
                    back_ = itr.node_->prev_;
                }

                if( itr.node_ == front_ ) {
                    front_ = itr.node_->next_;
                }


                delete itr.node_;
                itr.invalidate( );

                return iterator(tmp);
            } else {
                return itr;
            }
        }

        void concat( list<T> &other )
        {
            if( &other != this ) {
                if( front_ == NULL ) {
                    front_ = other.front_;
                    back_  = other.back_;
                } else if( other.front_ ) {
                    back_->next_        = other.front_;
                    other.front_->prev_ = back_;
                    back_               = other.back_;
                }
                other.front_ = other.back_= NULL;
            }
        }

        void push_back( const T &data )
        {
            node *new_node = new node(data);
            if( front_ ) {
                new_node->prev_ = back_;
                back_->next_    = new_node;
                back_           = new_node;
            } else {
                back_ = front_  = new_node;
            }
        }

        void push_front( const T &data )
        {
            node *new_node = new node(data);
            if( front_ ) {
                new_node->next_ = front_;
                front_->prev_   = new_node;
                front_          = new_node;
            } else {
                back_ = front_  = new_node;
            }
        }

        void swap( list<T> &other )
        {
            node *tf = front_;
            front_ = other.front_;
            other.front_ = tf;

            node *tb = back_;
            back_ = other.back_;
            other.back_ = tb;
        }

    private:
        node *front_;
        node *back_;
    };

}}}

#endif // LIST_H
