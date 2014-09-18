#ifndef WIN_WEAVE_SYSPORT_EVENTQ_H__
#define WIN_WEAVE_SYSPORT_EVENTQ_H__

#include "stdafx.h"
#include <Windows.h>


namespace NylonSysCore {

    class SysportEventQueueBase
    {
    protected:
        SysportEventQueueBase()
        {
            hEventsAvailable =
                CreateEvent
                (  nullptr, FALSE, FALSE,
                    // TEXT("Weave::ApplicationEventQueueApvailable")
                    nullptr
                );
        }
        void wakeup()
        {
            SetEvent( hEventsAvailable );
        }
        void waitonevent()
        {
            WaitForSingleObject( hEventsAvailable, INFINITE );
        }

        // return true if weave event rcvd, false otherwise
        bool waiteventnylonorsystem()
        {
             // see e.g.,
             // http://blogs.msdn.com/b/oldnewthing/archive/2006/01/26/517849.aspx;
             // be sure all messages are processed before waiting again, see http://blogs.msdn.com/b/oldnewthing/archive/2005/02/17/375307.aspx
            auto rc =
               MsgWaitForMultipleObjects
               ( 1, &hEventsAvailable,
                  FALSE, INFINITE, QS_ALLINPUT
               );

            // std::cout << "MsgWaitForMultipleObjects rc=" << rc << std::endl;

            switch( rc )
            {
               case WAIT_OBJECT_0:
                   return true;  // weave event

               case (WAIT_OBJECT_0 + 1):
                   return false;

               default:
                  std::cout << "UnK return from MsgWaitForMultipleObjects=" << rc << std::endl;
                  return false;
            }
        }
    private:
        HANDLE hEventsAvailable;
    }; // class SysportEventQueueBase
}
       
#endif
