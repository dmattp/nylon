// Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]
#ifndef  PURE_COMPARABLE_H__
#define  PURE_COMPARABLE_H__

#include "all.h"


/** template class LtComparable
 *
 * \brief For any class which implements operator<,
 *        provide operators >, <=, >=, == and !=
 *        which are built upon the operator< implementation.
 *
 * Operator< was chosen as the 'core' operator because many
 * std. library algoritms (sort,bsearch) use Operator< for their
 * default implementation.  Generally speaking, it is redundant
 * and error-prone to independently define logical operators unless there
 * is compelling reason to do so;
 * E.g., (a != b) should always be equivalent to !(a == b).
 * 
 */
template< class ALtComparable >
class LtComparable
{
public:
   bool inline
   operator>  ( const ALtComparable& rhs ) const
   {
      return
      (rhs    < *(static_cast< const ALtComparable* >( this )));
   }
   
   bool inline
   operator<= ( const ALtComparable& rhs ) const
   {
      return
      !(rhs < *(static_cast< const ALtComparable* >( this )));
   }
   
   bool inline
   operator>= ( const ALtComparable& rhs ) const
   {
      return
      !(*(static_cast< const ALtComparable* >( this )) < rhs);
   }

   bool inline
   operator!= ( const ALtComparable& rhs ) const
   {
      return
      (  (rhs < *static_cast< const ALtComparable* >( this ))
      || (*static_cast< const ALtComparable* >( this ) < rhs)
      );
   }

   bool inline
   operator== ( const ALtComparable& rhs ) const
   {
      return
//      !(*static_cast< const ALtComparable* >( this ) != rhs);
      !(operator!=( rhs ));      
   }
};




template< class ALtComparable >
class LteComparable
{
public:
   bool inline
   operator>  ( const ALtComparable& rhs ) const
   {
      return
      (rhs    < static_cast< const ALtComparable& >( *this ));
   }
   
   bool inline
   operator<= ( const ALtComparable& rhs ) const
   {
      return
      !(rhs < static_cast< const ALtComparable& >( *this ));
   }
   
   bool inline
   operator>= ( const ALtComparable& rhs ) const
   {
      return
      !( static_cast< const ALtComparable& >( *this ) < rhs);
   }

   bool inline
   operator!= ( const ALtComparable& rhs ) const
   {
      return
      !(operator==( rhs ));      
   }
};







#endif /* PURE_COMPARABLE_H__ */
