Nylon Copyright (c) 2013 David M. Placek

# Objective

Nylon aims to provide a framework which eases the implementation of coroutine based systems by providing basic communication services such as messaging, events, and timers as well as providing mechanisms for interfacing coroutines with system threads and event loops where it is necessary to utliize blocking or high latency library calls or integrate with an existing event based framework.

# License

Distributed under the MIT license, please see [doc/license.txt] for details.

# Prerequisites

* lua5.4
* cmake
* luabind-deboostified (included as submodule)
* glib-2.0 (linux only)

# Building

Usual CMake build process.  I generally use either ninja or 'Visual Studio 19' as the generator target; anything else is untested.

CMake may not have a "Lua54" package defined, but you can generally find the latest "Lua5x" package in the CMake directory and copy it to Lua54.cmake and update the references from e.g., 5.1 to 5.4 and this works fine.

# Motivation

Managing concurrency is a perennial challenge in designing software systems.  The most common approaches to managing concurrency utilize either preemptive threading or event driven frameworks.  Preemptive threading is notoriously difficult to do properly and breeds subtle race conditions and locking problems.  Event driven frameworks avoid the complexity of preemptive threading but this often requires algorithms and processes to be expressed in a disjointed and fragmented form.

Co-operative multithreading, as with Lua's coroutines, can offer a "sweet spot" between fully preemptive threading and traditional event driven approaches.  Unlike event driven systems, algorithms can be expressed in a straightforward and linear fashion while the absence of preemption substantially reduces the need for locking and the risk of subtle race conditions.

Lua's coroutine functions follow the admirable lua style in providing a powerful, elegant, and minimal primitive for supporting cooperative multithreading.  But it is left as an exercise for the coder to provide traditional concurrency abstractions such as messaging and events- and to successfully integrate cooperative threads with preemptive threads, blocking i/o, and event frameworks. The Nylon core has grown out of one coder's attempt to complete this exercise and perhaps others may find it useful.

# Status

As of 2014-10-24 nylon is still in early beta stage and under active development. Documentation is scarce.
There is a lot of noise in the sources that needs to be cleaned up.  The build process hasn't been fortified
to a wide array of environments and will likely need some tweaking for your setup.

# Usage

    The basic pattern for using Nylon is as follows:
    
    local Nylon = require 'nylon.core'()
    
    local c = Nylon.cord('cordname', 
          function(cord)
            -- do something as a coroutine
          end)
    
    Nylon.run()
    
Nylon.cord creates a new coroutine and executes the given function in the context of the new coroutine. 
The Nylon.run() call at the end executes an event loop which will process any nylon and user events
and schedule runnable cords as needed.

The cord function receives a handle to his own cord which can be used to invoke nylon services.
For example, nylon provides a cord ":sleep" method which causes execution of the coroutine to 
be yielded for a specific amount of time.  

This snippet creates a cord which will wake up every 1.5 seconds and print 'Hello World":

    Nylon.cord('cordname', 
          function(cord)
             while true do
                print 'Hello world!'
                cord:sleep( 1.5 )
<             end
          end)
    
Nylon also provides named events which a cord may wait on.  Events may be signaled by any other coroutine.


    local waiter_cord = Nylon.cord('waiter', 
          function(cord)
             cord.event.AnyEventName.wait()
             print 'Received AnyEventName event!'
          end)
    
    Nylon.cord('signaller', 
          function(cord)
             cord:sleep(5)
             waiter_cord.event.AnyEventName() -- signal AnyEventName
          end)
    

Events may also be used to pass/receive data: 


    local waiter_cord = Nylon.cord('waiter', 
          function(cord)
             cord.event.AnyEventName = function( arg1, arg 2)
                print( 'Received data=', arg1, arg2 )
             end                     
          end)
    
    Nylon.cord('signaller', 
          function(cord)
             cord:sleep(5)
             waiter_cord.event.AnyEventName( 'blue', { foo = 'bar' } ) -- signal AnyEventName with data
          end)
    

Nylon also provides a message queue facility for passing data between cords:


    local receive_cord = Nylon.cord('receive', 
          function(cord)
             while true do
                local msg = cord:getmsg()
                print( 'got msg=', msg )
             end                     
          end)
    
    Nylon.cord('signaller', 
          function(cord)
             receive_cord:msg( 'hello there' )
             receive_cord:msg( { pi = 3.14, foo = 'bar' } )
          end)
    

The getmsg call can accept a timeout as a parameter to resume cord execution if no message
is received within the specified timeout.  Named mailboxes may also be used to support multiple
send/receive queues on the same cord:


    local receive_cord = Nylon.cord('receive', 
          function(cord)
             while true do
                local msg = cord:getmsg 'box1'
                local msg = cord:getmsg 'box2'
             end                     
          end)
    
    Nylon.cord('signaller', 
          function(cord)
             receive_cord:msg( 'box2', 'hello there' )
             receive_cord:msg( 'box1', { pi = 3.14, foo = 'bar' } )
          end)
    

A convenience function is provided to receive messages via iteration:


    local receive_cord = Nylon.cord('receive', 
          function(cord)
             for msg in cord:messages() do
                print( 'got msg=', msg )
             end                     
          end)
    

This form does not allow a timeout.  

Cords may be yielded manually:


    Nylon.cord('yielder', 
          function(cord)
            while true do
               cord:yield()
            end 
          end)
    

In some cases, it may be desireable to unschedule a cord until some specific event occurs, such as a callback from native C code. This can be accomplished with the "sleep_manual" function which provides a manual "wake-up" mechanism.


    Nylon.cord('wait_for_c_library', 
          function(cord)
            local gotCeeCallback = false
            cord:sleep_manual( function(wakeFunction)
                   -- C library does some work, then invokes the given callback
                   SomeCeeLibrary.doSomethingAndThenCall( function()
                      gotCeeCallback = true
                      wakeFunction()                                                       
                   end )
                end)
            assert( gotCeeCallback == true )
          end)
    

Calling "wakeFunction" causes the cord to resume execution.

Nylon also provides basic mechanisms for coordinating native C/C++ threads and nylon cords. 

Here is a simple example which will create a native thread to read in characters from stdin,
causing the nylon cord to wake up when characters are received:


    void do_cthread_getchar( ThreadReporter reporter )
    {
        while( !feof(stdin) )
        {
            int c = getchar();
            if ( c != EOF )
                reporter.report( (char)c );
        }
    }
    
    StdFunctionCaller cthread_getchar()
    {
       return StdFunctionCaller(std::bind( &do_cthread_getchar, std::placeholders::_1 ));
    }
    
    extern "C" int luaopen_nylontest( lua_State* L )
    {
       using namespace luabind;
       module( L, "NylonTest" ) [ def( "cthread_getchar", &cthread_getchar ) ];
    }
    ...
    Nylon.cord('i/o', function(cord)
          print "Type something>"
          cord:cthreaded_multi( NylonTest.cthread_getchar(),
                                function(char)
                                   print( string.format("Got Char=%d", char ) )
                                end )
          print "got EOF"
      end)
    

This allows blocking i/o and high latency operations to be moved into separate system threads, allowing
the primary application thread to run non-blocked cords freely while the cord for handling i/o waits
on the native thread to return results.

A similar (optional) integration is provided for the lua lanes library, which allows existing lua
libraries such as luasocket to be called via lanes if no nylon-specific binding is available.
This is a heavier-weight option as lanes creates a separate lua state for each thread, but it 
similarly allows high latency and blocking i/o operations to be moved to a separate thread while
leaving the main application thread free to run the preemptive cords as neeeded.

Nylon's main event loop is integrated with glib on linux which allows Nylon cords to co-exist well with GUI libraries such as GTK, QT, and IUP, and a similar implementation for Windows works at least with Qt and IUP (not sure about GTK on Windows).


# Installation

See [doc/installation.txt]

