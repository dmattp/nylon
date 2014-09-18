//////////////////////////////////////////////////////////////////
// Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]
// multiple writer, single reader lock free queue 
//////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
#include <memory>
#include <functional>

#include "atomic.h"

#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


# define pty (void)0

#define fdprintf( f, ... ) (void)0
#define dprintf( ... ) fdprintf( stdout, ##__VA_ARGS__ )

template< class Thing, class FreePolicy >
class SwisserQBackend : public FreePolicy
{
   Thing  dummy_;

   Thing* volatile head_;
   Thing* tail_;
   Thing* dispose_;

public:
   SwisserQBackend()
   : head_( &dummy_ ),
     tail_( &dummy_ ),
     dispose_( &dummy_ )
   {}

   ~SwisserQBackend()
   {
      if( tail_ != &dummy_ )
      {
         tail_->sqprev = 0;
         this->dispose( tail_ );
      }
   }

   void
   push( Thing* t )
   {
      // Maybe still a race condition between getting head and ref'ing head;
      // not sure how to fix?      
      
      Thing* nodispose = head_->ref();

      pty;
      
      Thing* wasfree;

      while( 1 )
      {
         wasfree = Atomic::cmpxchg( head_->sqprev, (Thing*)0, t );

         pty;

         for( Thing* h = head_; h->sqprev; h = h->sqprev )
         {
            Thing* prev = h->sqprev;

            pty;
         
            Thing* lasthead = Atomic::cmpxchg( head_, (Thing*)h, prev );

            pty;
         }

         if( 0 == wasfree )
            break;

         pty;
      }

//      pthread_mutex_unlock( &mute );
      nodispose->unref();
   }

    bool isEmpty()
    {
        Thing* h = head_;

        if( !h || tail_ == h )
            return true;  // empty
        else
            return false;
    }

   Thing*
   pop()
   {
       if( this->isEmpty() )
           return 0;
      pty;

      Thing* prev = static_cast<Thing*>( tail_->sqprev );

      pty;

      while( dispose_ != tail_ )
      {
         dprintf( "dispose; d=%p tail_=%p dref=%d\n",
            dispose_, tail_, dispose_->refcount );
         
         pty;
         
         if( dispose_->refcount ) // can't dispose past here, in use
         {
            break;
         }
         
         pty;

         Thing *next = dispose_->sqprev;
         
         pty;
         
         if( dispose_ != &dummy_ )
         {
            this->dispose( dispose_ );  //~~~ messy, but works; integrate with
                                        //SwisserQ::get/::put, or make shared ptr.
         }

         pty;
         dispose_ = next;
      }
      
      tail_ = prev;

      pty;

//      pthread_mutex_unlock( &mute );

      return prev;
   }
};





template< class T >
class
DeleteThing
{
public:
   void
   dispose( T* thing )
   {
      delete thing;
   }

   T*
   getfree()
   {
      T*q = (T*)malloc( sizeof(T) );
      return q;
   }
};



/* Simple buffer Q with sleeps when Q is empty/full */
template< class T >
class SwisserQ
{
    struct Thing //  : public LfStackItemBase<Thing>
    {
        //static int total_things;
        Thing* sqprev;
        int refcount;

        T t_;

        Thing*
        ref()
        {
            Atomic::add( refcount, 1 );
            return this;
        }

        void
        unref()
        {
            int prev = Atomic::add( refcount, -1 );
        }

        ~Thing()
        {
            // Atomic::add( total_things, -1 ); // just for debug
        }

        Thing()
        : sqprev( 0 ),
          refcount( 0 )
        {}
      
        Thing( const T& t )
        : sqprev( 0 ),
          refcount( 0 ),
          t_( t )
        {
            // Atomic::add( total_things, 1 );  // just for debug
        }
    };

   SwisserQBackend< Thing, DeleteThing<Thing> >  backend_;   
   
public:
   Thing* newthing( const T& item )
   {
        Thing* thing = backend_.getfree();
        if( 0 == thing )
        {
            thing = new Thing( item );
        }
        else
        {
            new (thing) Thing( item );
        }
        return thing;
   }

   void put( Thing* thing )
   {
       backend_.push( thing );
   }
   void putVoid( void* thing )
   {
//      std::cout << "putVoid, backend_=" << (void*)&backend_ << std::endl;
      backend_.push( static_cast<Thing*>( thing ) );
   }
   
    void put( const T& item )
    {
       put( newthing( item ) );
    }
   
    bool isEmpty() {
//       std::cout << "isEmpty, backend_=" << (void*)&backend_ << std::endl;
       return backend_.isEmpty();
    }

    T get()
    {
//       std::cout << "get, backend_=" << &backend_ << std::endl;
        Thing* thing = backend_.pop();
      
        if( thing )
            return thing->t_;
        else
            throw std::runtime_error("Get called on empty Q; check isEmpty() first!");
    }
};









