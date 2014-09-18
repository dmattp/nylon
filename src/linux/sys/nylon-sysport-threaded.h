class ThreadRunner
{
   friend class ThreadReporter;
   void init()
   {
      pthread_mutex_init( &waitForReader_, 0 );
      pthread_mutex_init( &waitForValues_, 0 );
      pthread_mutex_lock( &waitForReader_ );  // initialize to locked
      pthread_mutex_lock( &waitForValues_ );  // initialize to locked
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
      init();
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
      init();
   }
   
   ~ThreadRunner()
   {
      delete o_;
      delete exiter_;
   }

   void whenLuaIsReadyToReadValues()
   {
      std::cout << "main thread now ready to read values" << std::endl;

      pthread_mutex_unlock( &waitForReader_ );  // allow thread to return values

      std::cout << "main thread now waiting for values" << std::endl;
      
      pthread_mutex_lock( &waitForValues_ ); // wait for thread to finish
                                             // returning values

      std::cout << "main thread now released, values should have been returned" << std::endl;
   }

   void whenThreadHasExited()
   {
      void* rc;
      
      pthread_join( thread_, &rc );

      delete this;
   }

   void runInNewThread()
   {
      std::cout << "ENTER ThreadRunner::runInNewThread()" << std::endl;

      ThreadReporter reporter( *this, *o_ );

      fun_( reporter );

      if( exiter_ )
      {
         preReport();
         (*exiter_)();
         postReport();
      }
      
      std::cout << "EXIT ThreadRunner::runInNewThread()" << std::endl;

      NylonSysCore::Application::InsertEvent
      (  std::bind( &ThreadRunner::whenThreadHasExited, this )
      );
   }
   
   static void* // static
   threadEntry( void* arg )
   {
      std::cout << "ThreadRunner::threadEntry()" << std::endl;
      ThreadRunner* this1 = static_cast<ThreadRunner*>( arg );
      this1->runInNewThread();
   }
      
   void run()
   {
      std::cout << "ThreadRunner::run()" << std::endl;
      pthread_attr_t attr;
      pthread_attr_init( &attr );
      pthread_create
      (  &thread_, &attr,
         &ThreadRunner::threadEntry,
         this
      );
   }
private:
   pthread_t thread_;
   static pthread_mutex_t allThreadsReportlock_;
   pthread_mutex_t waitForReader_;
   pthread_mutex_t waitForValues_;
   std::function<void(ThreadReporter)> fun_;
   luabind::object* o_;
   luabind::object* exiter_;

   void preReport()
   {
      std::cout << "signalling thread is done, ready to return values" << std::endl;

      // signal ready
      NylonSysCore::Application::InsertEvent
      (  std::bind( &ThreadRunner::whenLuaIsReadyToReadValues, this )
      );

      // lock
      std::cout << "waiting for main thread to come and read" << std::endl;
      pthread_mutex_lock( &waitForReader_ );

      pthread_mutex_lock( &allThreadsReportlock_ );

      // NylonSysCore blocked; now safe to operate on lua.
   }

   void postReport()
   {
      // Now unblock NylonSysCore
      pthread_mutex_unlock( &waitForValues_ );

      // and allow other threads to report
      pthread_mutex_unlock( &allThreadsReportlock_ );

      std::cout << "done returning values, now unblocking main thread" << std::endl;
   }
};
