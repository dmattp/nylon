// Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]
#include <luabind/luabind.hpp>
#include "sys/platform-defs.h"

class ThreadRunner;
class ThreadReporter
{
public:
   ThreadReporter( ThreadRunner& runner, luabind::object& o )
   : runner_( runner ),
     o_( o )
   {}

   lua_State* interpreter()
   {
      return o_.interpreter();
   }

   void report()
   {
      preReport();
      o_();
      postReport();
   }

   template< class T >
   void report( T&& p1 )
   {
      preReport();
      o_( std::forward<T>(p1) );
      postReport();
   }
   
   template< class T1, class T2 >
   void report( T1&& p1, T2&& p2 )
   {
      preReport();
      o_( std::forward<T1>(p1), std::forward<T2>(p2) );
      postReport();
   }
   
   template< class T1, class T2, class T3 >
   void report( T1&& p1, T2&& p2, T3&& p3 )
   {
      preReport();
      try
      {
          o_
          (  std::forward<T1>(p1),
              std::forward<T2>(p2),
              std::forward<T3>(p3)
          );
      }
      catch ( const luabind::error& e )
      {
          // lua_State* l = e.state();
          std::cout << "got exception during report=" << e.what() << std::endl;
      }
      postReport();
   }

   luabind::object& access() { return o_; }

   DLLEXPORT void preReport();
   DLLEXPORT void postReport();
   
private:
   ThreadRunner& runner_;
   luabind::object& o_;
};

class StdFunctionCaller
{
public:
   StdFunctionCaller( const std::function<void(ThreadReporter)>& fun )
   : fun_( fun )
   {}
   void run( ThreadReporter r ) { fun_( r ); }
   operator std::function<void(ThreadReporter)>() { return fun_; }
private:
   std::function<void(ThreadReporter)> fun_;
};

