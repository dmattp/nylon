// Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]
#ifndef NYLON_ATOMIC_H__
#define NYLON_ATOMIC_H__

#include "sys/nylon-sysport-atomic.h"

namespace Atomic
{
   template< int kSize >
   class BaseMemcast
   {
   public:
      typedef long mem_t;
   };

   template<>
   class BaseMemcast<sizeof(long long)>
   {
   public:
      typedef long long mem_t;
   };

   template< int kSize >
   class Memcast : public BaseMemcast<kSize>
   {
   public:
      template<class TAnything>
      static inline
      typename BaseMemcast<kSize>::mem_t*
      memcast( TAnything* pMemory )
      {
         return reinterpret_cast<typename BaseMemcast<kSize>::mem_t*>( pMemory );
      }
   };

   template< class T >
   inline T
   cmpxchg( volatile T& var, T oldval, T newval )
   {
      typename Memcast<sizeof(T)>::mem_t rc =
         nylon_sysport_cmpxchg
         (
            reinterpret_cast<volatile typename BaseMemcast<sizeof(T)>::mem_t*>( &var ),
            *(Memcast<sizeof(T)>::memcast(&oldval)),
            *(Memcast<sizeof(T)>::memcast(&newval))
         );

      return *( (T*)(void*)&rc );
   }
    
   template< typename TVAR, typename TADDEND >
   inline TVAR
   add( volatile TVAR& var, TADDEND addend )
   {
      
      while( true )
      {
         TVAR oldVal( var );
         TVAR newVal( oldVal + addend );
         if( oldVal == Atomic::cmpxchg( var, oldVal, newVal ) )
            return oldVal;
      }
   }
}

#endif 
