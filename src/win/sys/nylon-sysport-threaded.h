#include <process.h>

// static unsigned __stdcall // static
//threadEntry( void* arg );

/*
 * HACK ALERT!!
 * Did this at first with windows Mutexes to match behavior of pthread mutex.
 * Windows mutexes are "smart" mutexes, meaning they "belong" to the thread which took them,
 * unlike pthread mutexes.  This is _great_ if that's what you want, but it means that
 * we need extra handshaking to make sure that the mutexes are returned to their "rightful"
 * owners, especially in the case that we are reporting multiple values.
 * This sort of seemed to work for a while but I think it's a lost cause, and I should
 * switch over to semaphores or something dumber.
 */

namespace { // anonymous
    template< class T >
    void hardfail( const std::string& e, const T& p )
    {
        std::cout << e << p << std::endl;
        Sleep(60000);
        exit(-1);
    }

    void hardfail( const std::string& e )
    {
        std::cout << e << std::endl;
        Sleep(60000);
        exit(-1);
    }
}



class ThreadRunner
{
    static const int MAX_SEM_COUNT=5;
   friend class ThreadReporter;
   void initMutexes()
   {
//        if (!waitForValues_)
//            hardfail("ERROR03a did not init mutexes!!");
   }
    void initMainMutex()
    {
        waitForValues_ = CreateSemaphore( 0, 0, MAX_SEM_COUNT, 0 ); // initialize to locked
        waitForReader_ = CreateSemaphore( 0, 0, MAX_SEM_COUNT, 0 ); // initialize to locked
        if (!waitForReader_ || !waitForValues_)
            hardfail("ERROR03b did not init mutexes!!");
    }
public:
   ThreadRunner
   (  const std::function<void(ThreadReporter)>& fun,
      const luabind::object& o
   )
   : fun_( fun ),
     o_( new luabind::object(o) ),
     exiter_( 0 )
   {
       initMainMutex();
   }

   ThreadRunner
   (  const std::function<void(ThreadReporter)>& fun,
      const luabind::object& o,
      const luabind::object& exiter
   )
   : fun_( fun ),
     o_( new luabind::object(o) ),
     exiter_( new luabind::object( exiter ) )
   {
       initMainMutex();
   }
   
   ~ThreadRunner()
   {
      delete o_;
      delete exiter_;
      CloseHandle(waitForReader_);
      CloseHandle(waitForValues_);
   }

   void whenLuaIsReadyToReadValues()
   {
       //std::cout << "main thread now ready to read values" << std::endl;

      gl_MainThreadWaitingForValues = true; 
      ReleaseSemaphore( waitForReader_, 1, 0 );  // allow thread to return values

//      std::cout << "main thread now waiting for values" << std::endl;
      
      auto rc = WaitForSingleObject( waitForValues_, INFINITE ); // wait for thread to finish returning values
      gl_MainThreadWaitingForValues = false;

      if (rc != WAIT_OBJECT_0)
      {
          hardfail( "ERROR02b: waitForValues_ failed, rc=", rc );
      }

      //std::cout << "main thread now released, values should have been returned" << std::endl;
   }

   void whenThreadHasExited()
   {
//      void* rc;
       // ULONG tbeg = GetTickCount();
       WaitForSingleObject( thread_, INFINITE ); // MS equivalent of pthread_join

       //ULONG tend  = GetTickCount();

       // std::cout << "whenThreadHasExited tick elapsed=" << (tend-tbeg) << std::endl;
                                       
       if( exiter_ )
       {
           (*exiter_)();
       }

//      pthread_join( thread_, &rc );

      delete this;
   }

   void runInNewThread()
   {
//      std::cout << "ENTER ThreadRunner::runInNewThread()" << std::endl;

      ThreadReporter reporter( *this, *o_ );

      fun_( reporter );

//      std::cout << "EXIT ThreadRunner::runInNewThread()" << std::endl;

      NylonSysCore::Application::InsertEvent
      (  std::bind( &ThreadRunner::whenThreadHasExited, this )
      );
   }

    static unsigned __stdcall // static
    threadEntry( void* arg )
    {
//        std::cout << "ThreadRunner::threadEntry()" << std::endl;
        ThreadRunner* this1 = static_cast<ThreadRunner*>( arg );
        this1->initMutexes();
        this1->runInNewThread();
        return 0;
    }
      
   void run()
   {
//      std::cout << "ThreadRunner::run()" << std::endl;

// uintptr_t _beginthreadex( // NATIVE CODE
//    void *security,
//    unsigned stack_size,
//    unsigned ( __stdcall *start_address )( void * ),
//    void *arglist,
//    unsigned initflag,
//    unsigned *thrdaddr 
// );

      thread_ = (HANDLE)
          _beginthreadex
          (   0, // void *security,
              0, // unsigned stack_size,
              &threadEntry, // unsigned ( __stdcall *start_address )( void * ),
              this, // void *arglist,
              0, // unsigned initflag, 0 == creates running (rather than suspended)
              &threadaddr_ // unsigned *thrdaddr 
          );
   }

    static void Init()
    {
        allThreadsReportlock_ = CreateMutex(0,0,0);
    }
    
private:
   HANDLE thread_;
    unsigned threadaddr_; // really don't know what this is that's different
                          // from HANDLE; MS docs suggest its the 'thread id'??
   static HANDLE allThreadsReportlock_;
   HANDLE waitForReader_;
   HANDLE waitForValues_;
   HANDLE waitForRendezvous_;    

   std::function<void(ThreadReporter)> fun_;
   luabind::object* o_;
   luabind::object* exiter_;

    static int glThreadCount;
    static bool gl_MainThreadWaitingForValues;

   void preReport()
    {
//      std::cout << "signalling thread is done, ready to return values" << std::endl;
      WaitForSingleObject( allThreadsReportlock_, INFINITE );

      if( glThreadCount != 0 )
      {
          hardfail("ERROR: somebody else reporting=",glThreadCount);
      }

      ++glThreadCount;

      // signal ready
      NylonSysCore::Application::InsertEvent
      (  std::bind( &ThreadRunner::whenLuaIsReadyToReadValues, this )
      );

      // lock
      //std::cout << "waiting for main thread to come and read" << std::endl;
      auto rc = WaitForSingleObject( waitForReader_, INFINITE );
      if (rc != WAIT_OBJECT_0)
      {
          hardfail( "ERROR02a: waitForReader_ failed, rc=", rc );
      }

      if (!gl_MainThreadWaitingForValues)
      {
          hardfail("ERROR01a: main thread not waiting for values!!");
      }
      
      // NylonSysCore blocked; now safe to operate on lua.
   }

   void postReport()
   {
       if( glThreadCount != 1 )
           hardfail("ERROR: glThreadCount not 1=", glThreadCount);

       --glThreadCount;
       
      if (!gl_MainThreadWaitingForValues)
          hardfail("ERROR01b: main thread not waiting for values!!");
      
      // Now unblock NylonSysCore
      ReleaseSemaphore( waitForValues_, 1, 0 );

      // and allow other threads to report
      ReleaseMutex( allThreadsReportlock_ );

      // std::cout << "done returning values, now unblocking main thread" << std::endl;
   }
}; // end, ThreadRunner


int ThreadRunner::glThreadCount = 0;
bool ThreadRunner::gl_MainThreadWaitingForValues=false;
