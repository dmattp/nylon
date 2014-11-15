-- Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]

-- Cord object public methods:
--
--  :getmsg()
--  :msg( cord_other, msg_content )
--  :sleep( seconds_float )
--  :sleep_manual( cbfun, args )
--  :waiton( cord_other )
--  :yield()
-- .event


-------- Need tests
-- :setenv(k,v)
-- :laned()
-- :lanediter()
-- :inmain(fn)
-- :hasmsg
-- :getmsg()/:msg() with mailbox
-- :cthreaded, :cthreaded_multi
-- Nylon.mklock

-------- Not tested or private
-- :nylonstatus()
-- :test_pause(), :test_unpause()
-- :isDead()
-- :yield_to_sleep()
-- :resume()
-- :reschedule_()
-- :_wake_up()
-- :process_pending_()
-- :add_pending
-- :status
-- :

local localdbg = false

-- local SysCore

local function metacow( orig_table, new_tbl )
   new_tbl = new_tbl or {}
   setmetatable( new_tbl, {
             __index = orig_table
          } )
   return new_tbl
end

local wv = require 'nylon.debug'{ name = 'nylon-core' }


-- extern reschedule is the program environment rescheduling function,
-- which will schedule a callback into the nylon scheduling system
-- after exiting back out to the main program loop, as a sort of
-- 'idle' callback.
local extern_reschedule

-- maintain a list of functions to run from the "main loop"
-- this is currently needed with luabind because luabind
-- gets angry, eg., if you call a luabind::object function
-- from a coroutine context other than the one in which it
-- was created.  So the typical pattern is to register the
-- function that will be called by NylonSysCore.addCallback from
-- the main loop.
local nylon_main_state_cbs

local function nylon_really_in_main_state( fn )
   if nylon_main_state_cbs then
      table.insert( nylon_main_state_cbs, fn )
   else
      nylon_main_state_cbs = { fn }
   end
   extern_reschedule()
end

local function nylon_in_main_state( fn )
   -- none of this 'nylon_in_main_state' crap seems necessary after fixing
   -- SysCore to reschedule with a luabind::object function that was 
   -- created in the main loop.  More "efficient", anyway.
   if true then
      fn()
   else
      nylon_really_in_main_state( fn )
   end
end

local function process_main_state_cbs()
   if nylon_main_state_cbs then
      local cbt = nylon_main_state_cbs
      nylon_main_state_cbs = nil
      for _, cb in ipairs( cbt ) do 
         cb() 
      end
   end
end


local comapper = {}
local gThreadsByName = setmetatable({}, { __mode = 'v' })


-- right now, the only list of threads are those in the 'waiting' list, 
-- which means "ready-to-run".
local threads = {
   waiting   = nil,
--   running   = nil,
--   dead      = nil
}


local function bool2yesno( theBool )
   return ( (theBool and 'yes') or 'no' )
end


local cord = {}

function cord:nylonstatus()
   return string.format( "%s [%15.15s] msgs=%d [%s%s] ",
          (self.co == coroutine.running() and "*" or " "),
          self.name, 
          #(self.mail.q),
          (self.req_pause and 'paused/' or ''),
          coroutine.status(self.co) )
end

local function dump_costatus()
   local info = {}
   for _, nylonthread in pairs( comapper ) do
      table.insert( info, nylonthread:nylonstatus() )
   end
   return table.concat( info, "\n" )
end


local function nylon_find_cord_by_name( n )
   return gThreadsByName[n]
end

function cord:test_pause()
   self.req_pause = true
end

function cord:test_unpause()
   self.req_pause = nil
end

local waitlist
local function waitlist_insert( s )
   if waitlist then
      table.insert( waitlist, s )
   else
      waitlist = { s }
   end
end

local function nylon_move_from_paused_to_waiting( t )
   local p = threads.paused
   if not p then 
      return
   end
   local s = p
   repeat
      if p == t then
         clist_remove( threads, 'paused', t )
         waitlist_insert( t )
         return
      end
      p = p.next
   until p == s
end


local function nylon_console( a1, a2, ... )
   print( 'nylon:', a1 )
   if not a1 then 
      a1 = 'stat'
   end
   if a1 == 'stat' then
      return dump_costatus()
   elseif a1:sub(1,1) == '.' then
      local name = a1:sub(2)
      local t = nylon_find_cord_by_name( name )

      if not t then
         return( "Error, cannot find thread named=" .. name )
      end
      
      if a2 == 'pause' then
         t:test_pause()
      elseif a2 == 'unpause' then
         t:test_unpause()
         nylon_move_from_paused_to_waiting( t )
      elseif a2 == nil then
         return t:nylonstatus() .. debug.traceback( t.co )
      else
         return( "Error, unknown command=" .. a2 )
      end
   else
      return( "Error, unknown command=" .. a1 .. " or need '.threadname' argument first." )
   end
end



function clist_insert( t, headkey, o )
   if not o then
      return
   end
   local head = t[headkey]
   if not head then
      t[headkey] = o
      o.next = o
      o.prev = o
   else
      local tail = head.prev
      o.next = head
      o.prev = tail
      tail.next = o
      head.prev = o
   end
end

function clist_remove( t, head, o )
   if not o then
      return nil
   end
   if t[head] == o then
      if o.next == o then
         t[head] = nil
         return o
      else
         t[head] = o.next
      end
   end
   o.prev.next = o.next
   o.next.prev = o.prev
end



function cord:isDead()
   -- wv.log('debug','[%s] dead=%s',self.name,tostring(self.dead))
   return self.dead or (coroutine.status( self.co ) == 'dead')
end


-- Give other processes a time slice.
-- I am still running, so I keep myself on the waitlist
function cord:yield()
   if self.dead then
      return false
   end
   waitlist_insert( self )
   return coroutine.yield()
end

-- yields without adding myself to the waiting list.
-- This is typically used when I am waiting on an out of band event-
-- e.g., timer callback, sigchld callback- which will make the thread
-- active again (the callback is responsible to add the task to the waiting list).
-- note that everytime we wake up from this sleep, we check for
-- pending events.
function cord:yield_to_sleep( )
   if localdbg then wv.log('debug','co=%s yielding callstack:\n%s', self.name, wv.backtrace() ) end
   coroutine.yield()
   self:process_pending_()
end

-- add a callback function to a list of functions to run.
-- optimized to only create a new table when more than one
-- callback is available.
local function cblist_add( cbfun, tbl, field )
   if not tbl[field] then -- first one in the list
      tbl[field] = cbfun
   else
      if 'table' == type(tbl[field]) then
         table.insert( tbl[field], cbfun ) -- already a list of 2 or more
      else
         tbl[field] = { tbl[field], cbfun } -- only one, change to a list
      end
   end
 end

-- 
local function cblist_run( tbl, field )
   if not tbl[field] then
      return
   end
   local the_waiting = tbl[field]
   tbl[field] = nil
   if the_waiting and field == 'exitwaiter' then
      wv.log('prime','cord has exit waiters')
   end
   if 'table' == type(the_waiting) then
      for _, waiter in pairs(the_waiting) do
         waiter()
      end
   else
      the_waiting()
   end
end



local function cord_cleanup_on_dead_or_exit( self )
   wv.log('core','cleanup dead/exited name=%s', self.name)
   comapper[self.co] = nil
   gThreadsByName[self.name] = nil
   cblist_run( self, 'exitwaiter' )
end


function cord:resume()
   if self.dead then
      if comapper[self.co] then
         cord_cleanup_on_dead_or_exit()
      end
      return -1,"thread is dead"
   end

   if localdbg then
      wv.log('core,prime',"resuming coroutine %s p=%d from:\n%s\n%s",
             self.name, (self.req_pause and 1 or 0), wv.backtrace(), dump_costatus() )
   end

--    wv.log( 'debug', 'resuming coroutine[%s/%s] from [%s]', tostring(self.co), coroutine.status(self.co), tostring(coroutine.running()) )

   if localdbg and self.co == coroutine.running() then
      wv.log('error','huh?? already running co=%s', self.name )
      print( wv.backtrace() )
      return true
   end

   local rc, err = coroutine.resume( self.co )

   if not rc then
      wv.log('core,error','coroutine "%s" resume ERROR, returned [%s:%s]...\n\t%s', 
             self.name, tostring(rc), tostring(err), debug.traceback(self.co) )
      if true then
         print(string.format('coroutine "%s" resume ERROR, returned [%s:%s]...\n\t%s', 
                             self.name, tostring(rc), tostring(err), debug.traceback(self.co)) )
         os.exit(1)
      end
      self.dead = true
      cord_cleanup_on_dead_or_exit( self )
   else
      if localdbg then
         wv.log('core,prime','coroutine resume returned [%s:%s]...\n%s', 
                self.name, coroutine.status(self.co),
                dump_costatus() )
      end
   end

   return rc, err
end



local function nylon_schedule()

   local function run_current_tasklist( currlist )
      for _, curr in ipairs( currlist ) do
         -- wv.log('extra','reschedule_tasks() task=%s co=%s', curr.name, tostring(curr.co) )
         if not curr.dead then
            if curr.req_pause then
               clist_insert( threads, 'paused', head )
            else
               --            wv.log('extra','RESUMING task=%s co=%s', curr.name, tostring(curr.co) )
               local rc, err = curr:resume()
               if not rc then
                  wv.log( 'error', 'Error in thread "%s"; err=%s',
                         curr.name, (err or 'none') )
                  curr.status = err
               else
                  -- wv.log( 'extra', 'thread "%s" returned OK', curr.name )
               end
            end
         end
      end
   end

   -- wv.log('extra','nylon_schedule() waitlist=%s co=%s', type(waitlist), tostring(coroutine.running()) )
   if waitlist then
      local currlist = waitlist
      waitlist = nil

      run_current_tasklist( currlist )

      -- if any new tasks were added to the waitlist, be sure to schedule a wake-up
      if waitlist then 
         extern_reschedule()
      end
   end

   process_main_state_cbs() -- dmp/13.1205 not needed
end

-- @todo 13.1121 currently broken (no waitlist_remove_head() implementation)
local function nylon_destroy()
   for i,thread in pairs(comapper) do
       thread.dead = true
       comapper[i] = nil
       gThreadsByName[thread.name] = nil
   end

   head = waitlist_remove_head() -- clist_remove_head( threads, 'waiting' )
   while head do 
      head.dead = true
      head = waitlist_remove_head() -- clist_remove_head( threads, 'waiting' )
   end
end



-- 'thread local' storage - provides a table/environment 
-- for registering per-task values 
local nylon_tls = setmetatable({ [true] = {} }, { __mode = 'k' })

function cord:setenv( k, v )
   nylon_tls[self.co][k] = v
end

local function nylon_getenv(k)
   if k then
      return nylon_tls[coroutine.running() or true][ k ]
   else
      return nylon_tls[coroutine.running() or true]
   end
end




------------------------------------------------------------------
--  * MAILBOXES *
-- A mailbox is simply a table with a 'q' field
-- optionally, it has a name field.  The name is included in the
-- mailbox table so that if we wait on a message from multiple
-- mailboxes, we can return the name of the mailbox on which
-- the message was received.
------------------------------------------------------------------


-- deliver a message to a mailbox
-- wakes the coroutine if it is waiting on a message
local function mbox_msg( self, mbox, content )
   table.insert( mbox.q, (content or false) )
   wv.log('mbox', 'mbox msg for %s, q=%s len=%d waiting=%s', 
          self.name, tostring(mbox.q), #mbox.q, type(mbox.waiting) )
   if mbox.waiting then
      waitlist_insert( self )
      extern_reschedule()
   end
end

local function mbox_hasmsg( mbox )
   return #mbox.q > 0
end

-- obtain a mailbox by name; creates the
-- mailbox if it doesn't exist
local function mbox_find( mailboxes, boxname )
   if mailboxes[boxname] then
      return mailboxes[boxname]
   else
      -- wv.log('debug','creat mailbox=%s',boxname)
      local mailbox = { q = {}, name = boxname }
      mailboxes[boxname] = mailbox
      return mailbox
   end
end

-- Wait on a message from one specific mailbox
local function mbox_getmsg( self, box, timeout )
   while #box.q < 1 do
      wv.log( 'mbox', '%s: waiting on msg q=%s len=%d', 
              self.name, tostring(box.q), #box.q )
      box.waiting = true
      if not timeout then
         self:yield_to_sleep()
      else
         if timeout <= 0 then
            return nil
         else -- :msg
            local htnow = CordServices.Time.uptime()
            local maxTime = htnow + timeout
            self:sleep_until( maxTime, true )
            break
         end
      end
   end
   wv.log( 'mbox', '%s: exit getmsg loop; msgq len=%d', self.name, #box.q )
   box.waiting = false
   if #box.q > 0 then -- if we 'timed out', then inbox table may be empty
      local msg = table.remove( box.q, 1 )
      return msg
   end
end


-- wait on a message from a list of mailboxes
-- returns: message, mailbox name
local function mbox_getmsg_any( self, boxnames )
   local boxes = {}
   for _, name in ipairs( boxnames ) do
--      if name == true then
--         table.insert( boxes, self.mail )
--      else
         -- wv.log('debug','ANY add box=%s', name )
         table.insert( boxes, mbox_find( self.mailboxes, name ) )
--      end
   end
   
   -- first check to see if any messages are pending. 
   -- if so, then we can return immediately without yielding
   for _, box in ipairs( boxes ) do 
      if #box.q >= 1 then
         local msg = table.remove( box.q, 1 )
         return msg, box.name
      end
   end

   -- no messages available; set a flag for all mailboxes
   -- we're waiting on so that we know to wake the cord 
   -- up when a message is delivered, then yield and
   -- wait for a message.
   while true do
      for _, box in ipairs( boxes ) do box.waiting = true end

      self:yield_to_sleep()

      wv.log('mbox','wokeup waiting for message')

      -- after we awake, check to see which (if any) mailboxes
      -- have a message
      for _, box in ipairs( boxes ) do 
         wv.log('mbox','box=%s len=%d', box.name, #box.q)
         if #box.q >= 1 then
            -- clear all the waiting flags
            for _, box in ipairs( boxes ) do box.waiting = false end
            local msg = table.remove( box.q, 1 )
            return msg, box.name
         end
      end
   end
end




-- Event semantics:
-- only one handler / waiter for event
-- if event is sent before handler is set (or if handler has been reset to nil), the event is queued in a mailbox
-- if a handler exists, mailbox is not used but handler is scheduled to run in owning cord.
-- when a handler is added, it will be invoked immediately to process pending events if they exist
-- if somebody is running 'event.wait()', any handler is ignored while waiting is in progress
-- it is an error (probably) to wait on an event from a cord which does not own the event.

local function new_evt( cord, evtname, evtfun )
   wv.log('evt', 'new evt=%s for co=%s', evtname, cord.name )
   local mbox = { q = {} }
   local handler = nil
   local function setOnHandler( onEvent )
      wv.log('evt','%s add on event==%s', cord.name, evtname )
      -- process any pending events
      if mbox_hasmsg( mbox ) then
         -- must use add_pending for lua 5.1, because we're doing this from a
         -- a metamethod, so the onEvent handler is barred from yielding
         cord:add_pending( function()
                              while mbox_hasmsg( mbox ) do
                                 onEvent( mbox_getmsg( cord, mbox ) )
                              end
                           end )
      end
      handler = onEvent
   end

   local e = {
      wait = function( flushPrior )
         wv.log('evt','%s waiting on event==%s', cord.name, evtname )
         local oldhandler = handler
         handler = nil
         local data = mbox_getmsg( cord, mbox )
         handler = oldhandler
         wv.log('evt','%s GOT event==%s', cord.name, evtname )
         return data
      end,
      clear = function() -- clears any pending events
         -- there is probably a more 'direct' way to do this by resetting
         -- mailbox internals, but this works for now.
         while mbox_hasmsg( mbox ) do
            mbox_getmsg( cord, mbox )
         end
      end
   }
   if evtfun then
      setOnHandler( evtfun )
   end
   return setmetatable( e, {
                    __call = function( t, data )
                       wv.log('debug','called event=%s for %s handler=%s', evtname,  cord.name, type(handler) )
                       if handler then
                          cord:add_pending( function() 
                                               -- wv.log('evt','calling event handler cord=%s running=%s', cord.name, comapper[coroutine.running()].name)
                                               handler(data) 
                          end )
                       else
                          mbox_msg( cord, mbox, data )
                       end
                    end,
                    __index = { on = setOnHandler },
                    __newindex = function( t, k, v )
                       if k == 'on' then
                          setOnHandler( v )
                       end
                    end
   })
end

-- this is actually a nice generic pattern to set up hashed invokers/handlers that can be used in the form
-- object.something = function(...) end
-- object.something( args )
-- it's really quite a useful pattern.  The only ugly thing really is the use of [t.__eventstore] which
-- can conflict with the hash key namespace
local evtmetatable = {
   __index = function( t, ndx )
      -- wv.log('evt', 'EVENT index=%s', ndx )
      -- print('got event.__ndx=',ndx)
      if t.__eventstore[ndx] then
         return t.__eventstore[ndx]
      else
--         print( '__index create evt=', ndx )
         local evt = new_evt( t.__cord, ndx )
         t.__eventstore[ndx] = evt
         return evt
      end
   end,
   __newindex = function( t, ndx, v )
--      print( 'newindex set evt=', ndx )
      wv.log('evt','make new key=%s',ndx)
      if 'function' == type(v) then
         if not t.__eventstore[ndx] then
            local evt = new_evt( t.__cord, ndx, v )
            t.__eventstore[ndx] = evt
         else
--            print( 'set new on handler via direct assign for', ndx )
            t.__eventstore[ndx].on( v )
         end
      end
   end,
   __call = function( t )
      return new_evt( t.__cord, '(anonymous)' )
   end
}


function cord:has_event(name)
   return self.event.__eventstore[name]
end



local function nylon_create_cord( name, mainfun, ... )
   local self = metacow( cord )
   local argt = { ... }

   self.name = name

   self.dead = false
   self.mail = { q = {} } -- for thread specific messages
   self.mailboxes = {} -- created as needed in mbox_find
   self.pending = {}
   self.event = setmetatable( { __cord = self,
                                __eventstore = {},
                              }, evtmetatable )
   -- print( self.event.red )
   
   gThreadsByName[self.name] = self

   self.co = coroutine.create( 
          function()
             wv.log( 'extra', "Starting luathread '%s'", self.name )
             self.exitrc = mainfun( self, table.unpack(argt) )
             self.exited = true
             self.dead = true
             wv.log( 'core,prime', "luathread '%s' mainfun returned", self.name )
             cord_cleanup_on_dead_or_exit(self)
          end )

   nylon_tls[self.co] = { thread = self }
   comapper[self.co] = self

   wv.log( 'core,prime', "Created luathread '%s'", self.name )

   waitlist_insert( self )
   extern_reschedule()

   return self
end

function cord:reschedule_()
   waitlist_insert( self )
   extern_reschedule()
end



-- wait for another cord to exit his main function
function cord:waiton( other )
   if not other then
      return
   elseif other.dead then
      return other.exitrc
   else
      -- wait for the thread to exit; add myself to waiter list if necessary
      local function onexit()
         wv.log('debug','cord %s waiting on %s WOKE UP', self.name, other.name)
         waitlist_insert( self )
         extern_reschedule()
      end
      wv.log('debug','cord %s waiting on %s', self.name, other.name)
      cblist_add( onexit, other, 'exitwaiter' )
      while not other.dead do
         self:yield_to_sleep()
      end
      return other.exitrc
   end
end


-- try to load lanes, but don't crash if it's unavailable
local lanes
local function initlanes()
   if not lanes then
      local ok, e = pcall( function()
                              lanes = require 'lanes'
                              if lanes and lanes.configure then
                                 lanes.configure()                           
                              end
                           end )
      if not ok then
         wv.log('error','could not load lanes, failure=%s', e )
      else
   --   wv.log('debug','lanes=%s',type(lanes))
   --   for k,v in pairs( lanes ) do
   --      wv.log('debug','lanes [%s]=%s',k,type(v))
   --   end
      end
   end
end


function cord:laned( fn, ...  )
   local complete = false
   local args1 = { ... }
   local rc

   local function wakeUp()
--      wv.log('debug','nylon:laned()::wakeUp() called')
      complete = true
      waitlist_insert( self )
      extern_reschedule()
   end

   local waker
   local promise
   local function runTheLane()
      waker = WakerUpper( wakeUp )
      local wakeId = waker:self()
      -- wv.log('debug','runTheLane()')
       -- local gennedLane = 
      initlanes()
      local gennedLane = lanes.gen(  '*', 
                  function()
                     -- print 'IN THE LANE MOFO'
                     io.stdout:flush()
                     -- wv.log('debug','in the lane()')
                     local rcinner = fn( table.unpack( args1 ) )
                     -- wv.log('debug','exiting the lane()')
                     --print('RCINNER=' .. rcinner .. ' waker=' .. tostring(waker) )
                     -- print('RCINNER=' .. rcinner .. ' wakerid=' .. tostring(wakeId) )
                     -- io.stdout:flush()
                     require 'nylon.syscore'
                     NylonSysCore.wakeWaker( wakeId );
                     return rcinner
                  end )
      promise = gennedLane()
      -- wv.log('debug','runTheLane() -- made the promise')
      -- rc = promise[1]
   end

   nylon_in_main_state( runTheLane )

   -- @todo: not sure what should be the return args here
   -- ie the values returned by the call to coroutine.yield
   repeat 
--      wv.log( 'trace,core,pre', "[%s] nylon:laned() -- yielding.", self.name )
      self:yield_to_sleep()
      if wv.log_active('trace,core,post') then
         wv.log('trace,core,post', "[%s] Nylony::sleep_until resumed; expired=%s \n%s", 
                self.name, bool2yesno(expired), dump_costatus() )
      end
   until complete

   -- The lane process should be complete (or nearly so) at this point, 
   -- because we only rescheduled this task (with NylonSysCore.wakeWaker()) just
   -- prior to returning from the lane function; so, return the value
   -- returned by the lane function
   return promise[1] 
end



function cord:lanediter( cbfun, fn, ...  )
   local complete = false
   local args1 = { ... }
   local rc

--   wv.log('debug','nylon:lanedite() 001a')

   
   local function wakeUp()
      waitlist_insert( self )
      extern_reschedule()
   end

   local function wakeUpAndExit()
      complete = true
      wakeUp()
   end

   initlanes()
   local linda = lanes.linda()

--   wv.log('debug','nylon:lanedite() 001b')

   local waker
   local wakerExit
   local promise
   local function runTheLane()
      waker = WakerUpper( wakeUp );
      wakerExit = WakerUpper( wakeUpAndExit );
      local wakeId = waker:self()
      local wakeIdExit = wakerExit:self() 
--      wv.log('debug','nylon:lanedite() 001c')
      local gennedLane = lanes.gen(  '*', 
                  function()
                     require 'nylon.syscore'
                     local rcinner = fn( 
                        function( databack )
                           -- print('prime','nylon databack got t=%s', type(databack) )
                           linda:send( 'rcvd_data', databack )
                           NylonSysCore.wakeWaker( wakeId );
                        end,
                        table.unpack( args1 ) )
                     -- wv.log('debug','exiting the lane()')
                     NylonSysCore.wakeWaker( wakeIdExit );
                     return rcinner
                  end )
      promise = gennedLane()
      -- wv.log('debug','runTheLane() -- made the promise')
   end

   nylon_in_main_state( runTheLane )

   -- nylon_in_main_state( function()
   while not complete do
  --    wv.log('prime','self=%s nylon landeiter 001d woke from yield', tostring(self))
      self:yield_to_sleep()
      if complete then
         break
      else
--      wv.log('prime','self=%s nylon landeiter 001d woke from yield', tostring(self))
         local k, v = linda:receive( 'rcvd_data' )
--      wv.log('prime','self=%s returned from linda rceive', tostring(self))
         cbfun( v )
      end
   end
   -- end )

   -- @todo: not sure what should be the return args here
   -- ie the values returned by the call to coroutine.yield
---    repeat 
--- --      wv.log( 'trace,core,pre', "[%s] nylon:laned() -- yielding.", self.name )
---       -- self:_yield()
---       self:yield_to_sleep()
---       if wv.log_active('trace,core,post') then
---          wv.log('trace,core,post', "[%s] Nylony::sleep_until resumed; expired=%s \n%s", 
---                 self.name, bool2yesno(expired), dump_costatus() )
---       end
---    until complete

   return promise[1] -- should be complete at this point
end



function cord:sleep_manual( setWakeUp, ... ) 
   local expired = false
   local rc
   local function wkfun(val)
      expired = true
      rc = val
      wv.log( 'trace,core,pre', "wkfun [%s] co=%s", self.name, tostring(coroutine.running()) )
      NylonSysCore.addCallback( function()
                                   waitlist_insert( self )
                                   extern_reschedule()
      end )
   end
   setWakeUp( wkfun, ... )

   -- @todo: not sure what should be the return args here
   -- ie the values returned by the call to coroutine.yield
   repeat 
      wv.log( 'trace,core,pre', "[%s] Nylony::sleep_until manual wakeup; yielding.", self.name )
      -- self:_yield()
      self:yield_to_sleep()
      if wv.log_active('trace,core,post') then
         wv.log('trace,core,post', "[%s] Nylony::sleep_until resumed; expired=%s \n%s", 
                self.name, bool2yesno(expired), dump_costatus() )
      end
   until expired

   return rc
end


function cord:inmain( fn )
   local complete

   nylon_really_in_main_state( 
      function()
         fn()
         complete = true
         waitlist_insert( self )
         extern_reschedule()
   end )

   -- note that we must always have an 'expired' type flag, because
   -- other events could cause the cord to be scheduled, eg., a
   while not complete do 
      self:yield_to_sleep()
   end
end



-- put the cord to sleep for so many seconds.
function cord:sleep( secs )
--   wv.log('core,prime', "In lcore wait, %f secs", secs )
   if nil == secs then -- nothing to do
      return self:yield_to_sleep()
   end
   local expired = false

   nylon_in_main_state( function()
      -- wv.log('extra','in main state; adding one-shot timer for %fs', secs)                         
      NylonSysCore.addOneShot( secs*1000, 
         function() 
            -- wv.log( 'debug', 'got one-shot timer callback' )
            expired = true
            waitlist_insert( self )
            extern_reschedule()
         end )
   end )

   -- note that we must always have an 'expired' type flag, because
   -- other events could cause the cord to be scheduled, eg., a
   -- call to 'add_pending'.
   while not expired do 
      self:yield_to_sleep()
   end
end

-- add the cord to the 'waiting to run' list
-- and signal the external scheduler to schedule
-- waiting task processing (invokes nylon_schedule())
function cord:_wake_up()
   --if self.co == coroutine.running() then
--       wv.log('error','already running?')
--    else
      waitlist_insert( self )
      extern_reschedule()
   --end
end

-- process any pending events
function cord:process_pending_()
   if self.pending then
      cblist_run( self, 'pending' )
   end
end

-- add an event to be processed as soon as the cord wakes up. 
function cord:add_pending( fun )
   cblist_add( fun, self, 'pending' )
   self:_wake_up()
end

-- send a message to the cord
-- mailbox name is optional
function cord:msg( box_or_content, content )
   if content then
      -- wv.log('debug','mail to box=%s  self.mailboxes=%s', box_or_content, type(self.mailboxes) )
--%      local JSON = require 'JSON'
--%      wv.log( 'debug', 'mailboxes=%s', JSON:encode( self.mailboxes ) )
      
      return mbox_msg( self, mbox_find( self.mailboxes, box_or_content ), content )
   else
      return mbox_msg( self, self.mail, box_or_content )
   end
end


function cord:hasmsg( box )
   return mbox_hasmsg( box and mbox_find(self.mailboxes,box) or self.mail )
end






------------------------------------------------------------------
-- Retrieve next message
-- Returns nil if timeout expires without any messages
function cord:getmsg( box_or_timeout, timeout )
   if box_or_timeout then
      if 'number' == type(box_or_timeout) then
         return mbox_getmsg( self, self.mail, box_or_timeout )
      elseif 'table' == type(box_or_timeout) then -- check multiple boxes
         return mbox_getmsg_any( self, box_or_timeout, timeout )
      else
         return mbox_getmsg( self, mbox_find( self.mailboxes, box_or_timeout ), timeout )
      end
   else -- use default mailbox
      return mbox_getmsg( self, self.mail )
   end
end

function cord:messages( box )
   return function()
      return self:getmsg( box )
   end
end


function cord:getmsg_any( boxlist )
   return mbox_getmsg_any( self, boxlist )
end

function cord:status()
   return self.status
end





function cord:cthreaded( cfun )
   local done
   local vals

   local function onDone()
      --print('ondone, done=',done)
      if not done then
         done = true
         waitlist_insert( self )
         --print('ondone, waitlist inserted')
         extern_reschedule()
         -- print('ondone, rescheduled')
      end
   end

   local function reportValues( ... )
      vals = { ... }
      -- onDone()
   end

   if localdbg then wv.log('debug','calling run_c_threaded_with_exit') end
   NylonSysCore.run_c_threaded_with_exit( cfun, reportValues, onDone )
   -- NylonSysCore.run_c_threaded( cfun, reportValues )

   while not done do
      if localdbg then wv.log('debug','sleeping, wait 4 run_c_threaded_with_exit') end
      self:yield_to_sleep()
   end
   if type(vals) == 'table' then
      return table.unpack( vals )
   else
      wv.log('abnorm','no values, type(vals)=%s', type(vals))
   end
end


function cord:cthreaded_multi( cfun, cbvalues )
   local done
   local vals = {}
   local function reportValues( ... )
      table.insert( vals, { ... } )
      waitlist_insert( self )
      extern_reschedule()
   end
   local function onExit()
      done = true
      waitlist_insert( self )
      extern_reschedule()
   end
   NylonSysCore.run_c_threaded_with_exit( cfun, reportValues, onExit )
   while not done do
      self:yield_to_sleep()
      if #vals > 0 then
         local toProcess = vals
         vals = {}
         for _, v in ipairs(toProcess) do
            cbvalues( table.unpack( v ) )
         end
      end
   end
   return true
end


local function nylon_selfthread()
   return comapper[coroutine.running()]
end



local function nylock_acquire( lock, cord )
   cord = cord or nylon_selfthread()
   local token = {}
   if not lock.held then
      lock.held = token
   else
      local function gotit()
         lock.held = token
         waitlist_insert( cord )
         extern_reschedule()
      end
      if lock.waiting then
         table.insert( lock.waiting, gotit )
      else
         lock.waiting = { gotit }
      end
      while lock.held ~= token do
         if not cord then
            wv.log( 'error', 'attempt to acquire cord lock, not from cord; bt:\n\t%s',
                   wv.backtrace() )
         end
         cord:yield_to_sleep()
      end
   end
   local function releaser()
      if lock.held ~= token then
         wv.log('error','attempt to release lock that I do not hold')
         return
      end
      lock.held = false
      if lock.waiting then
         local firstWaiter = lock.waiting[1]
         if #lock.waiting > 1 then
            table.remove( lock.waiting, 1 )
         else
            lock.waiting = nil
         end
         firstWaiter()
      end
   end
   return releaser
end
local function nylock_with( lock, fun, cord )
   local release = nylock_acquire( lock, cord )
   fun()
   release()
end
local nylock_meta = {
   __index = {
      acquire = nylock_acquire,
      with = nylock_with
   }
}
local function nylon_mklock()
   return setmetatable( {}, nylock_meta )
end

local function nylon_inmain(cbfun)
   local thr = nylon_selfthread()
   if thr then
      thr:inmain( cbfun )
   else
      cbfun()
   end
end

-- local function nylon_uptime()

local keepRunning = true

local exports = {
             cord    = nylon_create_cord,   --
             find    = nylon_find_cord_by_name,     -- ( byName )
             destroy = nylon_destroy,
             plugin  = nylon_add_plugin,
             console = nylon_console,
             getenv  = nylon_getenv,
             mklock  = nylon_mklock, 
             inmain  = nylon_inmain,
             uptime  = function()
                require 'nylon.syscore'
                return NylonSysCore.uptime()
             end,
             run  = function()
                extern_reschedule = function() NylonSysCore.reschedule_empty() end
                while keepRunning do
                   NylonSysCore.processAndWait()
                   nylon_schedule()
                end
             end,
             halt = function()
                keepRunning = false
                NylonSysCore.reschedule_empty()
             end,
             runWithCb  = function(cb)
                extern_reschedule = function() NylonSysCore.reschedule_empty() end
                while keepRunning do
                   NylonSysCore.processAndWaitNylonOrSystem()
                   nylon_schedule()
                   cb()
                end
             end,
             
             -- dmp/140916 temp for test
             -- schedule = nylon_schedule
          }

setmetatable( exports, {
          __index = function( t, ndx )
                       if ndx == 'self' then
                          local self = nylon_selfthread()
                          -- wv.log( 'debug', 'got call to nylon.self=%s', type(self) )
                          return self
                       end
                    end })

return function( libargs )

   if (not libargs or (not libargs.reschedule)) and not extern_reschedule then
      -- load default scheduler if 'nylon' is loaded without providing a scheduler
      libargs = require 'nylon.scheduler'
   end
      
   if libargs then
      if libargs.reschedule then
         extern_reschedule = libargs.reschedule( nylon_schedule )
         -- exports.reschedule = extern_reschedule
      end
      --             if libargs.SysCore then
      --                SysCore = libargs.SysCore
      --             end
      --             if not SysCore then
      --                pcall( function()
      --                          require 'NylonSysCore'
      --                end )
      --                SysCore = NylonSysCore
      --             end
   end

   return exports
end
