-- Test .event
io.write 'cord.event'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(99); end )

local okay = false

local c1 = Nylon.cord( 'c1', 
       function(cord)
          cord.event.red() -- events can be posted though the handler hasn't been set yet, and will be processed when the handler is set
          local count = 0
          cord.event.red = function()
             count = count + 1
          end
          cord.event.blue = function()
             count = count + 1
          end
          -- this is a little weird, because I'm not sure what guarantees I can
          -- actually make about the order in which events will be processed.
          -- explicit message box priorities might allow stricter guarantees?
          -- it seems to work as is FWIW but I don't know that this is guaranteed.
          cord.event.green = function( wake )
             if count == 2 then
                okay = true
             end
             wake()
          end
          while true do cord:sleep() end
       end )

Nylon.cord( 'c2', 
       function(cord)
          cord:sleep_manual( function( wake ) 
                                c1.event.blue()
                                c1.event.green( wake )
          end )
--          print( 'exiting', okay )
          os.exit( okay and 0 or 1)
       end )


Nylon.run()
