#ifndef LINUX_NYLON_SYSPORT_TIMER_H__
#define LINUX_NYLON_SYSPORT_TIMER_H__
// coypright 2013 david m. placek all rights reserved

#include <time.h>
#include <vector>
#include <algorithm>
#include <signal.h>

namespace NylonSysCore {

   // this does make the assumption that timespec is normalized!
   static bool operator<( const timespec& lhs, const timespec& rhs )
   {
      if (lhs.tv_sec == rhs.tv_sec)
         return (lhs.tv_nsec < rhs.tv_nsec);
      else
         return (lhs.tv_sec < rhs.tv_sec);
   }
   
   class Timer
   {
   public:
      static void
      oneShot
      (  const std::function< void(void)>& callback,
          Milliseconds expirationTime
      )
      {
         new TQTimer( callback, expirationTime );
      }

   private:
      class TQTimer;

      struct QueuedTimer {
         TQTimer* timer;
         QueuedTimer( TQTimer* t ) : timer( t )
         {}
         inline const struct timespec& expiry() { return timer->expiry; }

         inline bool operator< ( const QueuedTimer& rhs )
         {
            return (rhs.timer->expiry < timer->expiry);
         }
         inline void operator()()
         {
            timer->m_callback();
         }
         void deleteTimer()
         {
            delete timer;
         }
      };
      struct Statics {
         std::vector<QueuedTimer> active;

         void* event;
         bool set;
         
         void resetExpiry( const struct timespec& now, const struct timespec& front_expiry );
         

         // reschedule the timer processing alarm to wake up at the first
         // time found in the heap.
         void resetExpiry()
         {
            struct timespec now;
            clock_gettime( CLOCK_MONOTONIC, &now );
            resetExpiry( now, active.front().expiry() );
         }

         void mProcessTimers()
         {
//            std::cout << "processing timers" << std::endl;
            struct timespec now;
            clock_gettime( CLOCK_MONOTONIC, &now );
            while( 1 )
            {
               QueuedTimer front = active.front();
               auto expiry = front.expiry();

//                std::cout << "head=" 
//                          << expiry.tv_sec << " ns=" << expiry.tv_nsec << " NOW="
//                          << now.tv_sec << " ns=" << now.tv_nsec << std::endl;

               if( now < expiry ) // head is not yet expired
               {
                  resetExpiry( now, expiry );
                  return;
               }

               // remove front from heap
               std::pop_heap( active.begin(), active.end() );
               active.pop_back();

               front(); // execute the timer

               front.deleteTimer();

               if( active.empty() ) // no more timers
                  return;
            }
         }
         

         static void processTimers()
         {
            stTimer.mProcessTimers();
         }


         static void timersignal( int signo )
         {
//            std::cout << "got timersignal" << std::endl;
            stTimer.set = 0;
            Application::InsertPreAllocatedEvent( stTimer.event );
            stTimer.event = 0;
            struct itimerval itimer = { { 0 } };
            setitimer( ITIMER_REAL, &itimer, nullptr );
         }
         
         Statics()
         {
            struct sigaction sigd;
            sigemptyset( &sigd.sa_mask );
            sigd.sa_flags = 0;
            sigd.sa_handler = timersignal;
            sigaction( SIGALRM, &sigd, nullptr );
            std::make_heap( active.begin(), active.end() );
         }
      };


      static Statics stTimer;
       
      class TQTimer
      {
         friend class QueuedTimer;
         std::function<void(void)> m_callback;
         struct timespec expiry;
      public:
         TQTimer( const std::function< void(void) >& cb, Milliseconds expirationTime )
         : m_callback( cb )
         {
            struct timespec now;
            clock_gettime( CLOCK_MONOTONIC, &now );
            
            expiry = now;

            // add the expirationTime to 'now'
            if( expirationTime >= Milliseconds(1000) )
            {
               int seconds = expirationTime.to_prim() / 1000;
               expiry.tv_sec += seconds;
            }
            
            int remain = expirationTime.to_prim() % 1000;
            expiry.tv_nsec += remain * 1000000; // ms->nanoseconds
            while( expiry.tv_nsec > 1000*1000000 )
            {
               expiry.tv_nsec -= 1000*1000000;
               expiry.tv_sec += 1;
            }


            // @todo: move this into system latent callback (to avoid locking)
            stTimer.active.push_back( this );
            std::push_heap( stTimer.active.begin(), stTimer.active.end() );

//             std::cout << "expire=" << expirationTime << "ms NOW s="
//                       << now.tv_sec << " ns=" << now.tv_nsec << " THEN s="
//                       << expiry.tv_sec << " ns=" << expiry.tv_nsec << std::endl;
            
            if( stTimer.active.front().timer == this ) // looks like i'm next up!
               stTimer.resetExpiry( now, expiry );
         }
      
      private:
         ~TQTimer()
         {
         }
      };
    
      static void InitTimerQueue()
      {
      }
   };

   void Timer::Statics::resetExpiry( const struct timespec& now, const struct timespec& front_expiry )
   {
      struct itimerval itimer = { { 0 } };
      itimer.it_interval.tv_usec = 100*1000; // 100ms re-send if we miss the signal
      itimer.it_value.tv_sec  = front_expiry.tv_sec - now.tv_sec;
      itimer.it_value.tv_usec = (front_expiry.tv_nsec - now.tv_nsec)/1000;
      while( itimer.it_value.tv_usec < 0 )
      {
         --itimer.it_value.tv_sec;
         itimer.it_value.tv_usec += 1000000;
      }
//             std::cout << "TIMER reset expiry s=" << itimer.it_value.tv_sec
//                       << " us=" << itimer.it_value.tv_usec << std::endl;
      event = Application::PreAllocateEvent( &Timer::Statics::processTimers );
               
      stTimer.set = 1;
      if( itimer.it_value.tv_sec < 0  ||
         ( itimer.it_value.tv_sec == 0 && itimer.it_value.tv_usec <= 0 ) )
      {
         itimer.it_value.tv_sec = 0;
         itimer.it_value.tv_usec = 1;
      }
      setitimer( ITIMER_REAL, &itimer, nullptr );
   }
            
}
       
#endif
