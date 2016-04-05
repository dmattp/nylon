#ifndef WIN_NYLON_SYSPORT_EVENTQ_H__
#define WIN_NYLON_SYSPORT_EVENTQ_H__

#include "stdafx.h"
#include <Windows.h>


namespace NylonSysCore {

    // windows vs2010 doesn't have std::recursive_mutex?
    class recursive_mutex
    {
    public:
        recursive_mutex()
        {
            InitializeCriticalSection(&m_cs);
        }
        ~recursive_mutex()
        {
            DeleteCriticalSection(&m_cs);
        }
        void lock()
        {
            EnterCriticalSection(&m_cs);
        }
        void unlock()
        {
            LeaveCriticalSection(&m_cs);
        }
        bool try_lock()
        {
            return !!TryEnterCriticalSection(&m_cs);
        }
    private:
        CRITICAL_SECTION m_cs;
        recursive_mutex( const recursive_mutex& ); // not implemented; private to prevent copying.
    };

    class SysportEventQueueBase
    {
    protected:
        SysportEventQueueBase()
        {
            hEventsAvailable =
                CreateEvent
                (  nullptr, FALSE, FALSE,
                    // TEXT("nylon::ApplicationEventQueueApvailable")
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

        // return true if nylon event rcvd, false otherwise
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
                   return true;  // nylon event

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
