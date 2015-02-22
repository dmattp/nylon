//////////////////////////////////////////////////////////////////
// Nylon Copyright (c) 2013-2014 David M. Placek see [doc/license.txt]
//////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include <iostream>
#include <functional>
#include "sys/platform-defs.h"


#include "wrapped_prim.h"
#include "time_types.h"

namespace NylonSysCore {
   void AppProcess();
}

#ifdef NYLON_SYSCORE_QT
# include "sys/nylon-sysport-qteventq.h"
#else
# include "sys/nylon-sysport-eventq.h"
#endif

#include "mwsr-cbq.h"


namespace NylonSysCore
{
   class Application
   {
   public:
      static void InsertEvent( const std::function< void(void) >& cb )
      {
         instance().m_eventQueue.InsertEvent( cb );
      }

      static void* PreAllocateEvent( const std::function<void(void)>& cb )
      {
         return instance().m_eventQueue.PreAllocateEvent( cb );
      }

      static void InsertPreAllocatedEvent( void* thing )
      {
         instance().m_eventQueue.InsertPreAllocatedEvent( thing );
      }

      static void Process()
      {
         instance().m_eventQueue.Process();
      }


      static void MainLoop()
      {
          Application& app = instance();
          
          while( !app.m_exit )
          {
              app.m_eventQueue.Wait();
              app.m_eventQueue.Process();
          }
      }

      // This may be needed e.g., in the case that you are using
      // Gtk's main loop rather than Nylon.run(); in that case, the 
      // nylon application must be initialized to hook into Glib's
      // event loop.
      static void InitApplication()
      {
         Application& app = instance();
      }

      static void processAndWait()
      {
          Application& app = instance();

          app.m_eventQueue.Wait();

          app.m_eventQueue.Process();
      }

      static void processAndWaitNylonOrSystem()
      {
          Application& app = instance();

          auto rc = app.m_eventQueue.WaitNylonSysCoreOrSystem();

          switch( rc )
          {
              case ApplicationEventQueue::GOT_SYSTEM_EVENT:
              case ApplicationEventQueue::GOT_NYLON_EVENT:
                  app.m_eventQueue.Process();
                  break;
          }
      }
       

      static void MainLoopWithSyseventCallback( const std::function<void(void)>& cbSysevent );

       static void ForceExit()
       {
           instance().m_exit = true;
       }

   private:
      static Application& instance()
      {
         static Application* instance;
         if( !instance )
            instance = new Application();
         return *instance;
      }

      Application()
      : m_exit(false)
      {
      }
   
      class ApplicationEventQueue : public SysportEventQueueBase
      {
      public:
         ApplicationEventQueue()
         {
         }

         void InsertEvent( const std::function< void(void) >& cb )
         {
            mwsr_cbq_.put( cb );
            this->wakeup();
         }

         void* PreAllocateEvent( const std::function< void(void) >& cb )
         {
            return mwsr_cbq_.newthing( cb );
         }

         void InsertPreAllocatedEvent( void* thing )
         {
            mwsr_cbq_.putVoid( thing );
            this->wakeup();
         }
         
         void Wait()
         {
             this->waitonevent();
         }

         enum EEventAvail {
            GOT_NYLON_EVENT,
            GOT_SYSTEM_EVENT
         };

         EEventAvail WaitNylonSysCoreOrSystem()
         {
             return this->waiteventnylonorsystem() ? GOT_NYLON_EVENT : GOT_SYSTEM_EVENT;
         }
         
         bool Process1()
         {
            if( mwsr_cbq_.isEmpty() )
                return false;

            try {
               std::function<void(void)> cbfn = mwsr_cbq_.get();
               cbfn();
            } catch( ... ) {
               std::cout << "error, empty q?" << std::endl;
            }
            return true;
         }
   
         void Process()
         {
            while( Process1() )
               ;
         }

      private:
         SwisserQ< std::function<void(void)> > mwsr_cbq_;
      };

      ApplicationEventQueue m_eventQueue;
      bool m_exit;
   }; // end class Application

   void
   Application::MainLoopWithSyseventCallback( const std::function<void(void)>& cbSysevent    )
   {
      Application& app = instance();
      while( !app.m_exit )
      {
         auto rc = app.m_eventQueue.WaitNylonSysCoreOrSystem();
         // std::cout << "WaitOrMessage=" << rc << std::endl;
         switch( rc )
         {
            case ApplicationEventQueue::GOT_SYSTEM_EVENT:
               cbSysevent(); // | | ---  fall thru, just to be sure i'm 
                             // V V      processing all of my events
            case ApplicationEventQueue::GOT_NYLON_EVENT:
               instance().m_eventQueue.Process();
               break;
         }
      }
   }

   void AppProcess()
   {
      Application::Process();
   }
}

#include "sys/nylon-sysport-timer.h"

namespace NylonSysCore {
    Timer::Statics Timer::stTimer;
}




extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <luabind/luabind.hpp>


class Lua
{
public:
   Lua()
   : l_( luaL_newstate() )
   {
      luaL_openlibs(l_);
   }

   Lua( lua_State* l )
   : l_( l )
   {
   }
   
   void loadfile( const std::string& filename )
   {
      auto rc = luaL_loadfile(l_,filename.c_str());
      if (rc==0)
      {
         rc = lua_pcall( l_, 0, LUA_MULTRET, 0 );
         if( rc )
         {
            fprintf( stderr, "lua error: '%s'\n", lua_tostring( l_, -1 ) );
         }
         else
            fprintf( stdout, "lua loaded OK file=%s\n", filename.c_str() );
      }
      else /* error occured, should be on the stack */
      {
         const char *errmsg = lua_tostring(l_,-1);

         fprintf( stderr, "Couldn't load script file '%s', error %d/'%s'\n",
            filename.c_str(), rc, errmsg );

         lua_settop(l_,0); // clear the stack.
      }

      fflush( stdout );
      fflush( stderr );
   }

   lua_State* state() { return l_; }
   
private:   
   lua_State* l_;
};


namespace {
   void callLuaObjectNoDelete( luabind::object* o )
   {
      try {
         // std::cout << "callLuaObject, calling" << std::endl;
         (*o)();
      } catch( std::exception& e ) {
         std::cout << "got exception=" << e.what() << std::endl;
      } catch( ... ) {
         std::cout << "got unknown exception" << std::endl;
      }
   }
   
   void callLuaObject( luabind::object* o )
   {
      callLuaObjectNoDelete( o );
      delete o;
   }

    luabind::object rescheduleFunc;

    void doreschedule()
    {
        rescheduleFunc();
    }

    void setReschedule( luabind::object o )
    {
        rescheduleFunc = o;
    }

    static void empty()
    {
    }

    static const std::function<void(void)> kEmpty( &empty );
    
    static void reschedule_empty()
    {
        NylonSysCore::Application::InsertEvent(kEmpty );
    }


    static void exitLoop()
    {
        // std::cout << "insert 'exit' event" << std::endl;
        NylonSysCore::Application::InsertEvent( &NylonSysCore::Application::ForceExit );
    }
    

    void reschedule()
    {
        NylonSysCore::Application::InsertEvent( &doreschedule );
    }
    
   void addCallback( const luabind::object& o )
   {
      luabind::object* o2 = new luabind::object( o );
      NylonSysCore::Application::InsertEvent( std::bind( &callLuaObject, o2 ) );
   }

   void addOneShot( int msExpiration, const luabind::object& o )
   {
      luabind::object* o2 = new luabind::object(o);
      NylonSysCore::Timer::oneShot( std::bind( &callLuaObject, o2 ), Milliseconds(msExpiration) );
   }
}

// This may be useful for external libraries building on Nylon to set
// callback events
DLLEXPORT
void LuaNylonSysCore_AddCallback( const std::function<void(void)>& cbfun )
{
    NylonSysCore::Application::InsertEvent( cbfun );
}


class WakerUpper
{
public:
   WakerUpper( const luabind::object& o )
   : o_( o )
   {}
   ~WakerUpper()
   {}
   void wake()
   {
      NylonSysCore::Application::InsertEvent( std::bind( &WakerUpper::callObject, this ) );
   }
   unsigned long self() {
      return (unsigned long)this;
   }
   static void wakethis1( unsigned long this1 )
   {
      static_cast<WakerUpper*>((void*)this1)->wake();
   }
private:
   luabind::object o_;
   void callObject()
   {
      o_();
   }
};


#include "nylon-runner.h"



// to return value to lua:
// 1. new thread signals 'ready', then blocks waiting for lua to wait for the
// value to be returned.
// 2. Lua sees the threaded activity is ready to report, and the requesting
// cord schedules a "halt" - a blocking operation that will insure the
// original thread is not executing in Lua.  (this probably requires
// that the Nylon scheduler exit completely to the main loop and prevent
// re-scheduling entirely until the report completes - scheduling a
// NylonSysCore event callback that "blocks" in C world will accomplish this.
// 3. When the main thread blocks and waits, he signals a condition to wake
// the sub thread.  At this point, the sub-thread can safely call into lua
// and report his values.
// 4. After the return-value call, the subthread must signal an event to
// wake the main thread back up.



#include "sys/nylon-sysport-threaded.h"


// to be technically correct, there must be a class static lock before
// operating on the lua state to prevent multiple ThreadRunners from
// trying to access the lua state simultaneously.
decltype(ThreadRunner::allThreadsReportlock_) ThreadRunner::allThreadsReportlock_
#ifndef _WINDOWS
  = PTHREAD_MUTEX_INITIALIZER;
#endif
;

DLLEXPORT
void ThreadReporter::preReport()   
{
   runner_.preReport();
}

DLLEXPORT
void ThreadReporter::postReport()
{
   runner_.postReport();
}


namespace {
   void run_c_threaded( StdFunctionCaller caller, const luabind::object& o )
   {
      ThreadRunner* r = new ThreadRunner( caller, o );
      r->run();
   }

   void run_c_threaded_with_exit( StdFunctionCaller caller, const luabind::object& o, const luabind::object& exiter )
   {
      ThreadRunner* r = new ThreadRunner( caller, o, exiter );
      r->run();
   }
   
}

#ifdef _WINDOWS
# include <codecvt>
#else
# include <locale>
#endif


namespace {
   static void sysevtCbLoop( luabind::object sysevtcbfun )
   {
      std::cout << "Enter guiCbLoop" << std::endl;
      sysevtcbfun(); // call once on entry to insure all system events
                     // are cleared before we wait.
      NylonSysCore::Application::MainLoopWithSyseventCallback
      (  std::bind( callLuaObjectNoDelete, &sysevtcbfun )
      );
   }
}

#ifdef _WINDOWS
namespace {
    double uptime()
    {
#if 0
        ULONG ticks = GetTickCount();
        return (double(ticks/1000) + double(ticks%1000)/1000.0);
#else
        static bool gotInitTime;
        static LARGE_INTEGER Frequency;
        static LARGE_INTEGER StartingTime;
        if( !gotInitTime )
        {
            QueryPerformanceFrequency(&Frequency); 
            QueryPerformanceCounter(&StartingTime);
            gotInitTime = true;
        }
        LARGE_INTEGER EndingTime, ElapsedMicroseconds;
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

//
// We now have the elapsed number of ticks, along with the
// number of ticks-per-second. We use these values
// to convert to the number of elapsed microseconds.j
// To guard against loss-of-precision, we convert
// to microseconds *before* dividing by ticks-per-second.
//

        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
        return (double)ElapsedMicroseconds.QuadPart / 1000000.0;
#endif 
    }

    // these should really be moved to a separate "OS support" library, but
    // it was convenient to have them here when I didn't have that yet.
    // Sorry for the mess.
    luabind::object systemTimeToLua( lua_State* L, const SYSTEMTIME& stTimestamp )
    {
        luabind::object t = luabind::newtable( L );

        t["y"] = stTimestamp.wYear;
        t["m"] = stTimestamp.wMonth;
        t["d"] = stTimestamp.wDay;
        t["H"] = stTimestamp.wHour;
        t["M"] = stTimestamp.wMinute;
        t["S"] = stTimestamp.wSecond;
        t["mil"] = stTimestamp.wMilliseconds;
        
        return t;
    }
    luabind::object systemTimeToLua( lua_State* L, const FILETIME& ftTimestamp )
    {
        SYSTEMTIME stTimestamp;
        FileTimeToSystemTime( &ftTimestamp, &stTimestamp );
        return systemTimeToLua( L, stTimestamp );
    }

    luabind::object GetFileTime_( lua_State* L, const std::string& fname )
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wpath = converter.from_bytes( fname );
        luabind::object times = luabind::newtable( L );
        FILETIME ftcreate;
        FILETIME ftaccess;
        FILETIME ftwrite;
        HANDLE hFile = CreateFile( wpath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
        BOOL rc = ::GetFileTime( hFile, &ftcreate, &ftaccess, &ftwrite );
        if( rc )
        {
            times["create"] = systemTimeToLua( L, ftcreate );
            times["access"] = systemTimeToLua( L, ftaccess );
            times["write"]  = systemTimeToLua( L, ftwrite );
        }

        CloseHandle( hFile );

        return times;
    }
} // end, anonymous namespace

#else // not windows
#include <time.h>
namespace {
    double uptime()
    {
       struct timespec ts;
       int rc = clock_gettime( CLOCK_MONOTONIC, &ts );
       return double(ts.tv_sec) + double(ts.tv_nsec)/double(1000*1000*1000);
    }
}
#endif

// these functions are for a quick/easy test of cthreaded type operations.
namespace {
    // for testing threading; read std in until EOF.
    void do_cthread_getchar( ThreadReporter reporter )
    {
        while( !feof(stdin) )
        {
            int c = getchar();
            if ( c != EOF )
                reporter.report( (char)c );
        }
    }
    
    StdFunctionCaller
    cthread_getchar()
    {
       return StdFunctionCaller(std::bind( &do_cthread_getchar, std::placeholders::_1 ));
    }
} // end, anonymous namespace





















extern "C" DLLEXPORT  int luaopen_nylon_syscore( lua_State* L )
{
   Lua lua( L );
   using namespace luabind;

#ifdef _WINDOWS
   ThreadRunner::Init();
#endif 

   open( lua.state() );
   
   module( lua.state(), "NylonSysCore" ) [
      def( "addCallback", &addCallback ),
      def( "addOneShot", &addOneShot ),
      def( "wakeWaker", &WakerUpper::wakethis1 ),
      def( "reschedule", &reschedule ),
      def( "exitLoop", &exitLoop ),
      def( "setReschedule", &setReschedule ),
      def( "initApplication", &NylonSysCore::Application::InitApplication ),
      def( "mainLoop", &NylonSysCore::Application::MainLoop ), 

      def( "processAndWait", &NylonSysCore::Application::processAndWait ),
      def( "processAndWaitNylonOrSystem", &NylonSysCore::Application::processAndWaitNylonOrSystem ),            
      def( "reschedule_empty", &reschedule_empty ),     

      def( "sysevtCbLoop", &sysevtCbLoop )
      , def( "run_c_threaded", &run_c_threaded )
      , def( "run_c_threaded_with_exit", &run_c_threaded_with_exit )
      , def( "uptime", &uptime )
   ];

   
   module( lua.state() ) [
         class_<WakerUpper>("WakerUpper")
         .def( constructor<const object&>() )
         .def( "wake", &WakerUpper::wake )
         .def( "self", &WakerUpper::self )
   ];


   module( lua.state() ) [
         class_<StdFunctionCaller>("StdFunctionCaller")
         .def( "run", &StdFunctionCaller::run )
   ];


   module( lua.state(), "NylonTest" ) [
       def( "cthread_getchar", &cthread_getchar )
   ];
   
   return 0;
}

extern "C" DLLEXPORT void nylon_mainloop()
{
    std::cout << "enter nylon_mainloop" << std::endl;
    NylonSysCore::Application::MainLoop();
    std::cout << "exit nylon_mainloop???" << std::endl;
}


extern "C" DLLEXPORT void nylon_reschedule()
{
    std::cout << "inserting reschedule() call" << std::endl;
    NylonSysCore::Application::InsertEvent( &doreschedule );
}


