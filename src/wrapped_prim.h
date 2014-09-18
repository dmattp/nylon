/*****************************************************************************

     Module Name:      WRAPPED_PRIM.H

     == Description =================================================

     WrappedPrimitive<> template allows one to wrap a primitive C type
     and give it a unique type, e.g., 'Milliseconds', or 'Inches'; so
     that one can say, e.g.,


         MilesPerHour
         compute_velocity( Milliseconds t, Inches d );

     rather than:

         float
         compute_velocity( float t, float d );

     This allows the function to more tightly enforce his contracts, without
     allowing treatment of untyped float/integer/string/etc values as something
     which expresses an implicit type of unit. This eliminates errors from passing
     a millisecond value to a function that expects microseconds, etc.

     Ideally, all operators should apply to the "wrapped primitive" exactly
     as they would on the primitive type, e.g., addition, subtraction,
     multiplication, copy, etc.
     
     Also, this allows for (relatively) "safe" implicit conversion between 
     types such as seconds->milliseconds, inches->miles, etc; that is to say, 
     implicit conversion is correct as long as 1) overflow does not occur, and 
     2) the underlying representation preserves the full precision from one type
     to the other.  E.g., going from Milliseconds to Seconds would lose
     precision, while going from Seconds to Milliseconds (if both are
     represented by e.g., an 'int' value) risks a possible overflow condition.
     It is recommended that conversions which risk overflow or lose precision
     be made explicit conversions rather than allowing them to be made
     implicitly. Or, possibly one could "throw" if a conversion would result
     in an overflow.

     ================================================================

*******************************************************************************/
#ifndef  WRAPPED_PRIM_H__
#define  WRAPPED_PRIM_H__

#include <cstdlib>
#include <iostream>
#include "comparable.h"


/** Default policy for constructing a 'WrappedPrimitive' type without
 * any initialization; sets the value of the primitive to zero.  This
 * algorithm should work for any primitive types I am awary of.
 * char,int,float,double,pointer...
 */
template<class T>
class WrappedPrimDefaultEmptyConstructorPolicy
{
public:
   static void
   EmptyConstuctor( T& v )
   {
//       LOKI_STATIC_CHECK
//       (  0,
//          NO_default_empty_constructor_for_wrapped_primitives__Add_a_custom_policy
//       );
      v = 0;
   }
};


template<
   class T,
   class TEmptyConstructorPolicy  = WrappedPrimDefaultEmptyConstructorPolicy<T>
   >
class WrappedPrimitiveBase
: public LtComparable< WrappedPrimitiveBase<T,TEmptyConstructorPolicy> >
{
protected:
   T v_;

public:
   typedef T prim_t;

   /* Note: using the to_prim() method is essentially like using reinterpret_cast<>
    * and should be used as sparingly. It throws away the type information which the
    * whole purpose of this wrapper class is to preserve.  Truthfully, I don't like
    * the 'value()' method which was added below because it's too "natural" to say
    * "someThing.value()". I prefer keeping with the same philosophy used in naming
    * the C++ style casts, i.e., operations that _are_ ugly *SHOULD* look ugly.
    */
   const T to_prim() const { return v_; }
   const T value()   const { return v_; }

WrappedPrimitiveBase<T,TEmptyConstructorPolicy>( const T initVal )
   : v_( initVal )
   {}

WrappedPrimitiveBase<T,TEmptyConstructorPolicy>()
   {
      TEmptyConstructorPolicy::EmptyConstuctor( v_ );
   }
   
   inline bool
   operator< ( WrappedPrimitiveBase<T,TEmptyConstructorPolicy> other ) const
   {
      return (v_ < other.to_prim());
   }
};



template<
   class T,
   class KnownAs,
   class InterpretedAs,
   class TEmptyConstructorPolicy = WrappedPrimDefaultEmptyConstructorPolicy<T>
>
class WrappedPrimWithArithmeticBase
:  public WrappedPrimitiveBase< T, TEmptyConstructorPolicy >
{
public:
   explicit
   WrappedPrimWithArithmeticBase( T initValue )
   : WrappedPrimitiveBase<T,TEmptyConstructorPolicy>( initValue )
   {}

   explicit
   WrappedPrimWithArithmeticBase()
   {}

   inline const KnownAs
   operator+ ( const KnownAs other ) const
   {
      return KnownAs(this->v_ + other.v_);
   }

   inline const KnownAs
   operator++()
   {
      return KnownAs(++this->v_);
   }

   inline const KnownAs
   operator++ ( int /* dummy */)
   {
      return KnownAs(this->v_++);
   }

   inline const KnownAs
   operator--()
   {
      return KnownAs(--this->v_);
   }

   inline const KnownAs
   operator-- ( int /* dummy */)
   {
      return KnownAs(this->v_--);
   }
   
   inline const KnownAs
   operator- ( const KnownAs other ) const
   {
      return KnownAs(this->v_ - other.v_);
   }

#if 1 
   inline const KnownAs
   operator+= ( const KnownAs other )
   {
      this->v_ += other.v_;
      return KnownAs(this->v_);
   }
#endif 

   inline const KnownAs
   operator-= ( const KnownAs other )
   {
      this->v_ -= other.v_;
      return KnownAs(this->v_);
   }

   inline const KnownAs
   operator- () const
   {
     return KnownAs(-this->v_);
   }

   inline const KnownAs
   operator* ( const T other ) const
   {
      return KnownAs(this->v_ * other);
   }

   /* Dividing type KnownAs by his primitive
    * leaves the type (preserves the units). */
   inline const KnownAs
   operator/ ( const T other ) const
   {
      return KnownAs(this->v_ / other);
   }

   /* Dividing type KnownAs by type Knownas
    * divides out the units, leaving the primitive type */
   inline const T
   operator/ ( const KnownAs other ) const
   {
      return this->v_ / other.to_prim();
   }


   /* Dividing type KnownAs by his primitive
    * leaves the type (preserves the units). */
   inline const KnownAs
   operator% ( const T other ) const
   {
      return KnownAs(this->v_ % other);
   }
};


template<class T, class KnownAs, class InterpretedAs, class TEmptyConstructorPolicy>
inline const WrappedPrimWithArithmeticBase<T,KnownAs,InterpretedAs, TEmptyConstructorPolicy>
abs( const WrappedPrimWithArithmeticBase<T,KnownAs,InterpretedAs, TEmptyConstructorPolicy> &wrapped )
{
   return WrappedPrimWithArithmeticBase<T,KnownAs,InterpretedAs,TEmptyConstructorPolicy>( abs(wrapped.to_prim()));
}


template<
   class KnownAs,
   class TEmptyConstructorPolicy = WrappedPrimDefaultEmptyConstructorPolicy<void*>
>
class WrappedVoidPtr
:  public WrappedPrimitiveBase< void*, TEmptyConstructorPolicy >
{
public:
   typedef WrappedVoidPtr<KnownAs,TEmptyConstructorPolicy> wrapped_t;

   explicit
   WrappedVoidPtr( void* initValue )
   : WrappedPrimitiveBase< void*,TEmptyConstructorPolicy >( initValue )
   {}

   template<typename T>
   T* to_ptr()
   { return static_cast<T*>( this->to_prim() ); }

   template<typename T>
   const T* to_ptr() const
   { return static_cast<const T*>( this->to_prim() ); }
};
   

template<
   class T,
   class KnownAs,
   class InterpretedAs,
   class TEmptyConstructorPolicy = WrappedPrimDefaultEmptyConstructorPolicy<T>
>
class WrappedPrimNOArithmeticBase
:  public WrappedPrimitiveBase< T, TEmptyConstructorPolicy >
{
public:
   explicit
   WrappedPrimNOArithmeticBase( T initValue )
   : WrappedPrimitiveBase<T,TEmptyConstructorPolicy>( initValue )
   {}

   explicit
   WrappedPrimNOArithmeticBase()
   {}
};



template< class T, class Tempty >
std::ostream&
operator<<( std::ostream& o, const WrappedPrimitiveBase<T,Tempty>& x )
{
   o << x.to_prim();
   return o;
}

template< class KnownAs >
const KnownAs
operator*( typename KnownAs::prim_t x, KnownAs y )
{
   return (y * x);
}

/* ****************************************************************** */

template<
   class T,
   class KnownAs,
   template <class w, class x, class y, class z> class TArithmeticPolicy = WrappedPrimWithArithmeticBase,
   class TEmptyConstructorPolicy = WrappedPrimDefaultEmptyConstructorPolicy<T>
   >
class WrappedPrimitive
   : public TArithmeticPolicy<T,KnownAs,unu,TEmptyConstructorPolicy>
{
public:
   typedef WrappedPrimitive<T,KnownAs,TArithmeticPolicy,TEmptyConstructorPolicy>
      wrapped_t;
   
   explicit
   WrappedPrimitive( T initValue )
   : TArithmeticPolicy<T,KnownAs,unu,TEmptyConstructorPolicy>( initValue )
   {}

   explicit
   WrappedPrimitive()
   {}
};



template<
   class T,
   class KnownAs,
   class InterpretedAs,
   template <class w, class x, class y, class z> class TArithmeticPolicy = WrappedPrimWithArithmeticBase,
   class TEmptyConstructorPolicy = WrappedPrimDefaultEmptyConstructorPolicy<T>
   >
class WrappedPrimitiveSemantic
   : public TArithmeticPolicy<T,KnownAs,InterpretedAs,TEmptyConstructorPolicy>
{
public:
   typedef WrappedPrimitiveSemantic<T,KnownAs,InterpretedAs,TArithmeticPolicy,TEmptyConstructorPolicy>
      wrapped_t;
   
   explicit
   WrappedPrimitiveSemantic( T initValue )
   : TArithmeticPolicy<T,KnownAs,InterpretedAs,TEmptyConstructorPolicy>( initValue )
   {}

   explicit
   WrappedPrimitiveSemantic()
   {}
};


/** Define a type based on a primitive which identifies a thing.
 *  Integer values such as decoder number are defining an
 * "identity".  As a general rule, it is meaningless to add or
 * subtract or otherwise perform math on identies. E.g., what would
 * it mean to divide or multiply two social security numbers?
 * These types should be declared in a way such that arithmetic is prohibited.
 */

template<
   class T,
   class KnownAs,
   class TEmptyConstructorPolicy = WrappedPrimDefaultEmptyConstructorPolicy<T>
   >
class IdentityPrimitive
   : public WrappedPrimNOArithmeticBase< T, KnownAs, unu, TEmptyConstructorPolicy >
{
   typedef WrappedPrimNOArithmeticBase<
      T, KnownAs, unu, TEmptyConstructorPolicy > wrapped_prim_t;
   
public:
   explicit
   IdentityPrimitive< T, KnownAs, TEmptyConstructorPolicy >( T initValue )
   : wrapped_prim_t( initValue )
   {}

   IdentityPrimitive<T,KnownAs,TEmptyConstructorPolicy>()
   {}
};


#endif /* CLANG_WRAPPED_PRIM_H__ */
