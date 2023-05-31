#ifndef WIN_NYLON_SYSPORT_EVENTQ_H__
#define WIN_NYLON_SYSPORT_EVENTQ_H__

#include <iostream>
#include <functional>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <poll.h>

#include <glib.h>


namespace NylonSysCore {

    class SysportEventQueueBase
    {
       static SysportEventQueueBase* i;
       void clearPipesAndDispatch()
       {
          char clearit[16];
          int rc2;
          do
             rc2 = read( selfpipes[0], clearit, 16 );
          while( rc2 == 16 );
          AppProcess();
       }
    public:
       void setEventTimeoutMs( unsigned msTimeout )
       {
           // this seems to have been introduced in windows build to
           // diagnose flaky wakeups.  do nothing.
       }
        
       static gint gtk_sys_poller( GPollFD* ufds, guint nfds, gint timeout_ )
       {
//            if( 0 == ++calls%100000) {
//              std::cout << calls << " gtk poll, i=" << (void*)i
//                        << " nfds=" << nfds << " tout=" << timeout_ << std::endl;
//            }
          pollfd fds[nfds+1];
          int i;
          for( i = 0; i < nfds; ++i )
             fds[i] = reinterpret_cast<pollfd&>(ufds[i]);
          fds[i].fd = SysportEventQueueBase::i->selfpipes[0];
          fds[i].events = POLLIN;

          struct timespec ts = { 1,
                                 100*(1000*1000) //nanoseconds 
          }; // 100ms, i hope
          
          int rc1 = ppoll( fds, nfds+1, &ts, nullptr );
//           return rc1;
//           if( fds[i].revents )
//           {
//              std::cout << "Nylon ppol rc=" << rc1 << " i=" << i << " POLLIN=" << POLLIN
//                        << " fds[i].revents=" << fds[i].revents << std::endl;
//           }
          
          if( fds[i].revents & POLLIN )
             SysportEventQueueBase::i->clearPipesAndDispatch();
          
          return rc1;
       }

    protected:
       struct MySource : public GSource {
          SysportEventQueueBase* self;
          GPollFD mypoll;
       };

       static gboolean
       gs_prepare( GSource    *source, gint       *timeout_ )
       {
          MySource* this1 = reinterpret_cast<MySource*>( source );
          this1->mypoll.revents = 0;
          return false;
       }
       
       static gboolean gs_check    (GSource    *source) {
          MySource* this1 = reinterpret_cast<MySource*>( source );
          
          pollfd fds[1];
          fds[0].fd = this1->self->selfpipes[0];
          fds[0].events = POLLIN;
          fds[0].revents = 0;
          int rc = poll( fds, 1, 0 );
          return (rc > 0);
       }
       static gboolean gs_dispatch (GSource    *source,
          GSourceFunc callback,
          gpointer    user_data)
       {
          MySource* this1 = reinterpret_cast<MySource*>( source );
          this1->self->clearPipesAndDispatch();
          this1->mypoll.revents = 0;
          return true;
       }
       static void gs_finalize (GSource    *source ){ /* Can be NULL */
//          std::cout << "gs_finalize !!!" << std::endl;
       }

        SysportEventQueueBase()
        {
            int rc = pipe2( selfpipes, O_NONBLOCK );
            (void)rc;
            i = this;

            static GSourceFuncs funcs = { 0 };
            funcs.prepare = gs_prepare;
            funcs.check = gs_check;
            funcs.dispatch = gs_dispatch;
            funcs.finalize = gs_finalize;

            MySource* source = (MySource*)g_source_new( &funcs, sizeof(MySource) );
            source->self = this;

            source->mypoll.fd = selfpipes[0];
            source->mypoll.events = G_IO_IN|G_IO_ERR|G_IO_HUP;
            
// struct GSourceFuncs {
//   gboolean (*prepare)  (GSource    *source,
//                         gint       *timeout_);
//   gboolean (*check)    (GSource    *source);
//   gboolean (*dispatch) (GSource    *source,
//                         GSourceFunc callback,
//                         gpointer    user_data);
//   void     (*finalize) (GSource    *source); /* Can be NULL */

//   /* For use by g_source_set_closure */
//   GSourceFunc     closure_callback;        
//   GSourceDummyMarshal closure_marshal; /* Really is of type GClosureMarshal */
// };


            g_source_add_poll( source, &source->mypoll );
            g_source_attach( source, g_main_context_default() );
            
//             g_main_context_add_poll(
//                g_main_context_default(), &mypoll, G_PRIORITY_DEFAULT
//             );
//             g_main_context_set_poll_func(
//                g_main_context_default(), &gtk_sys_poller );
        }
       
        void wakeup()
        {
//           std::cout << "Got Cort wakeup" << std::endl;
            int rc = write( selfpipes[1], &selfpipes, 1 ); // wake up "mainloop"
            (void)rc;
        }
       
        void waitonevent()
        {
            // current linux implementation:
            // wait a maximum of 100ms, but wake up before that if an
            // event is put in the queue.  So, we may return early even
            // if there are no events in the queue, but we should return
            // with minimal latency when an event is actually added to
            // the queue.
            struct pollfd p;
            struct timespec ts = { 0,
                                   100*(1000*1000) //nanoseconds 
            }; // 100ms, i hope
            char clearit[16];

            p.fd = selfpipes[0];
            p.events = POLLIN;

            (void)ts;
            (void)p;

            int rc = ppoll( &p, 1, &ts, NULL );

            do
            {
               rc = read( selfpipes[0], clearit, 16 );
            }
            while( rc == 16 );
        }

        // return true if nylon event rcvd, false if system event
        // no clear impl. for linux (compared to eg windows), so
        // just wait on nylon events only.  For e.g., Qt implementation
        // one can patch into Qt and have it call the wakeup mechanism.
        bool waiteventnylonorsystem()
        {
           this->waitonevent();
           return true;
        }
    private:
       int selfpipes[2];
    }; // class SysportEventQueueBase

    SysportEventQueueBase* SysportEventQueueBase::i;    
}
       
#endif
