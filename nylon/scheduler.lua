-- Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]
--
-- Loads the 'silk' core module with an idle scheduler based
-- on the "NylonSysCore" scheduling backend
--

local debug_mode = false

require 'nylon.syscore'

--print 'LOAD SCHEDULER'

local function createIdleLoopScheduler( silkScheduleFun )

   local wv
   if debug_mode then
      wv = require 'nylon.debug'
   end
   
   local isRescheduled = false

   local function reScheduleLoop()
      isRescheduled = false
      -- print 'in reScheduleLoop'
      -- wv.log('debug','got call to reScheduleLoop()')
      if debug_mode then
        -- using pcall here can be useful for debugging
        local rc, err = pcall( silkScheduleFun )
        if not rc then
           wv.log('error','got error=%s',err )
        end
      else
         silkScheduleFun()
      end

   end

--   print 'SET RESCHEDULE LOOP'   
   NylonSysCore.setReschedule( reScheduleLoop )

   -- this function will be called from silk when it needs to perform rescheduling.
   return function()
             -- wv.log('prime', 'adding reschedule after main loop')
             if not isRescheduled then
                isRescheduled = true
                NylonSysCore.reschedule() -- ( reScheduleLoop )
             end
          end
end

-- local silkmodule = require 'silk'

return { 
   reschedule = createIdleLoopScheduler
}
