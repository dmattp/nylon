#include <process.h>
#include <stack> // for ThreadPool, may go away
#include <memory> // for ThreadPool, may go away

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

class ThreadPool
{
public:
    class Thread
    {
        static const int MAX_SEM_COUNT=5; // should never be but 1 or 0
        ThreadPool& pool_;
        HANDLE thread_;
        HANDLE waitForDemand_; // semaphore to block on when idle
        unsigned threadaddr_; // really don't know what this is that's different
                              // from HANDLE; MS docs suggest its the 'thread id'??
        ThreadRunner* pActiveRunner_;

        void inMainThreadAddMeToPool()
        {
//            std::cout << "Thread=" << (void*)this << " inMainThreadAddMeToPool()\n";
            pool_.insert( this );
        }

        ~Thread()
        {
            CloseHandle(waitForDemand_);

            // this is how to wait for thread exit, but we're not really doing this
            // right now.
            // WaitForSingleObject( thread_, INFINITE ); // MS equivalent of pthread_join
        }

        void loopForThreadInThreadPool()
        {
            while (true)
            {
//                std::cout << "Thread=" << (void*)this << " waiting for demand\n";
                auto rc = WaitForSingleObject( waitForDemand_, INFINITE ); // wait for thread to finish returning values
//                std::cout << "Thread=" << (void*)this << " now demanded; running\n";
                this->whenActivated();
            }
        }

        static unsigned __stdcall // static
        threadEntry( void* arg )
        {
            Thread* this1 = static_cast<Thread*>( arg );
            this1->loopForThreadInThreadPool();
            return 0;
        }

        void whenActivated(); // first called in thread
        void whenThreadHasCompletedDoThisInMainThread(); // main thread callback after completing task
        

    public:
        Thread(ThreadPool& pool )
        : pool_( pool ), // so he can know to return himself to the pool
          threadaddr_(0),
          pActiveRunner_(0)
        {
            waitForDemand_ = CreateSemaphore( 0, 0, MAX_SEM_COUNT, 0 ); // initialize to locked

            this->inMainThreadAddMeToPool();
            
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

        void kickoff( ThreadRunner& runner )
        {
            this->pActiveRunner_ = &runner; // this is safe since thread is idle
            ReleaseSemaphore( waitForDemand_, 1, 0 );  // allow thread to return values
        }
    }; // end, class Thread
    
private:
    HANDLE waitForThread_; // semaphore to block on when idle
    std::stack< Thread* > pool_;
    static const int MAX_SEM_COUNT=5;
    
    void insert( Thread* thread )
    {
        pool_.push( thread );
        ReleaseSemaphore( waitForThread_, 1, 0 );  // allow thread to return values
    }

    ThreadPool()
    {
        waitForThread_ = CreateSemaphore( 0, 0, MAX_SEM_COUNT, 0 ); // initialize to locked
    }

public:
    Thread* get()
    {
        // should do something like, if pool empty then create a thread
        if (pool_.empty())
        {
//            std::cout << "thread pool empty, make a new one" << std::endl;
            new Thread(*this);
            auto rc = WaitForSingleObject( waitForThread_, INFINITE ); // wait for thread to finish returning values
//            std::cout << "got new thread, empty? = " << pool_.empty() << std::endl;
        }
        
        Thread* thread = pool_.top();
        
        pool_.pop();
        
        return thread;
    }
    
    static ThreadPool& instance()
    {
        static ThreadPool singleton_;
        return singleton_; // thank you C++11
    }
};


//volatile ThreadPoool& ii9944x = ThreadPool::instance();


class ThreadRunner
{
    static const int MAX_SEM_COUNT=5;
   friend class ThreadReporter;
//     void initMainMutex()
//     {
// //         waitForValues_ = CreateSemaphore( 0, 0, MAX_SEM_COUNT, 0 ); // initialize to locked
// //         waitForReader_ = CreateSemaphore( 0, 0, MAX_SEM_COUNT, 0 ); // initialize to locked
// //         if (!waitForReader_ || !waitForValues_)
// //             hardfail("ERROR03b did not init mutexes!!");
//     }
public:
   ThreadRunner
   (  const std::function<void(ThreadReporter)>& fun,
      const luabind::object& o
   )
   : fun_( fun ),
     o_( new luabind::object(o) ),
     exiter_( 0 ),
     hadException( false )
   {
////       initMainMutex();
   }

   ThreadRunner
   (  const std::function<void(ThreadReporter)>& fun,
      const luabind::object& o,
      const luabind::object& exiter
   )
   : fun_( fun ),
     o_( new luabind::object(o) ),
     exiter_( new luabind::object( exiter ) ),
     hadException( false )
   {
//       initMainMutex();
   }
   
   ~ThreadRunner()
   {
      delete o_;
      delete exiter_;
//       CloseHandle(waitForReader_);
//       CloseHandle(waitForValues_);
   }

//    void whenLuaIsReadyToReadValues()
//    {
//        //std::cout << "main thread now ready to read values" << std::endl;

//       gl_MainThreadWaitingForValues = true; 
//       ReleaseSemaphore( waitForReader_, 1, 0 );  // allow thread to return values

// //      std::cout << "main thread now waiting for values" << std::endl;
      
//       auto rc = WaitForSingleObject( waitForValues_, INFINITE ); // wait for thread to finish returning values
//       gl_MainThreadWaitingForValues = false;

//       if (rc != WAIT_OBJECT_0)
//       {
//           hardfail( "ERROR02b: waitForValues_ failed, rc=", rc );
//       }

//       //std::cout << "main thread now released, values should have been returned" << std::endl;
//    }

   void whenThreadHasExited()
   {
       if( exiter_ )
       {
           if (!hadException)
           {
               (*exiter_)();
           }
           else
           {
               (*exiter_)( eWhat_ );
           }
       }

       if (!hadException)
       {
           delete this;
       }
       else
       {
           lua_State* L = o_->interpreter();
           const std::string eWhat = eWhat_;

           std::cerr << "runInNewThread/whenThreadHasExited FATAL: " << eWhat << std::endl; 
           delete this;  // Careful! don't use members after this

#if 0 // huh, I wonder why I abandoned this?  I guess errors are reported via the 'exiter' function now.
           lua_pushlstring( L, eWhat.c_str(), eWhat.length() );
           lua_error(L);
#endif 
       }

   }

   // this should be the first think called from the new thread
   void runInNewThread()
   {
      ThreadReporter reporter( *this, *o_ );

      try
      {
          fun_( reporter );
      }
      catch( const std::exception& e )
      {
          hadException = true;
          std::cerr << "runInNewThread FATAL: " << e.what() << std::endl;
          eWhat_ = e.what();
      }
   }


   // this will be called by the main thread to 'kick off' execution of the
   // new thread
   void run()
   {
       // need to get a thread from the thread pool, and send him an init message
       // with the 'this' pointer
       ThreadPool::Thread* t = ThreadPool::instance().get();
       t->kickoff( *this );
   }

    static void Init()
    {
        allThreadsReportlock_ = CreateMutex(0,0,0);
    }
    
private:
   static HANDLE allThreadsReportlock_;
//    HANDLE waitForReader_;
//    HANDLE waitForValues_;
//    HANDLE waitForRendezvous_;    

   std::function<void(ThreadReporter)> fun_;
   luabind::object* o_;
   luabind::object* exiter_;
    bool hadException;
    std::string eWhat_;
    

    static int glThreadCount;
    static bool gl_MainThreadWaitingForValues;

   void preReport()
    {
//      std::cout << "signalling thread is done, ready to return values" << std::endl;
//      WaitForSingleObject( allThreadsReportlock_, INFINITE );
      NylonSysCore::Application::Lock(true);

      if( glThreadCount != 0 )
      {
          hardfail("ERROR: somebody else reporting=",glThreadCount);
      }

      ++glThreadCount;
      
      // NylonSysCore blocked; now safe to operate on lua.
   }

   void postReport()
   {
        if( glThreadCount != 1 )
            hardfail("ERROR: glThreadCount not 1=", glThreadCount);

        --glThreadCount;
       
       NylonSysCore::Application::Lock(false);

      // std::cout << "done returning values, now unblocking main thread" << std::endl;
   }
}; // end, ThreadRunner


void ThreadPool::Thread::whenActivated()
{
    pActiveRunner_->runInNewThread();
            
    NylonSysCore::Application::InsertEvent
    (  std::bind( &Thread::whenThreadHasCompletedDoThisInMainThread, this )
    );
}

void ThreadPool::Thread::whenThreadHasCompletedDoThisInMainThread()
{
    pActiveRunner_->whenThreadHasExited();
    pActiveRunner_ = 0;
    this->inMainThreadAddMeToPool();
}


int ThreadRunner::glThreadCount = 0;
bool ThreadRunner::gl_MainThreadWaitingForValues=false;
