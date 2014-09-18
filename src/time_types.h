// Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]
/*****************************************************************************
      Provides the following "strongly typed" time classes:

      * Milliseconds   - a signed integer based value representing Milliseconds
      * MillisecondsLL - a 64-bit signed integer based value representing Milliseconds
      * FloatSeconds   - a 'double' based value representing seconds.
      * FloatHerz      - a 'double' based value representing herz.

      Implicit construction is allowed from other types where precision would be preserved..
      Where precision could be lost, explicit conversion/construction is required.

      Dividing or multiplying a type by an object of its primitive type (int,double,...)
      retains the Unit signature; e.g., Milliseconds(5) * 100 == Milliseconds(500).
      However, dividing by two typed units removes the unit indication; e.g.,
      Milliseconds(500)/Milliseconds(100) == 5.

      Also, division of the primitive type by FloatSeconds or FloatHerz inverts
      the type, so that the result of [(int or double)/FloatSeconds] -> [FloatHerz],
      while [(int or double)/FloatHerz] -> [FloatSeconds]..

*******************************************************************************/
#ifndef  NYLON_TIME_TYPES_H__
#define  NYLON_TIME_TYPES_H__

#include "all.h"
#include "wrapped_prim.h"

class Milliseconds;
class FloatSeconds;

class Milliseconds : public WrappedPrimitive<int,Milliseconds>
{
public:
   explicit
   Milliseconds( int v )
   : WrappedPrimitive<int,Milliseconds>(v)
   {}

   explicit
   Milliseconds( const FloatSeconds& s );
};

/* 'FloatSeconds' allows implicit construction _from_ Milliseconds.
 *
 *  For an integer based "seconds" type, such a conversion would
 *  lose precision, so the construction should be explicit.
 *
 *  Since conversion to Milliseconds from FloatSeconds could
 *  also lose precision, this should be done explicitly using
 *  the Milliseconds(FloatSeconds) constructor.
 */
class FloatSeconds : public WrappedPrimitive<double,FloatSeconds>
{
public:
   explicit
   FloatSeconds( double v )
   : WrappedPrimitive<double,FloatSeconds>(v)
   {}

   FloatSeconds( Milliseconds v )
   : WrappedPrimitive<double,FloatSeconds>( ((double)v.to_prim()) / 1000.0 )
   {}

   FloatSeconds() {}
};


#endif /* NYLON_TIME_TYPES_H__ */
