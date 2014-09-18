-- Test .event: event posted before handler is set should be queued until handler is set 
io.write 'cord.event verify pending events'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(99); end )

local okay = false

local exitvalue = nil

local c2
local c1 = Nylon.cord( 'c1', 
       function(cord)
          c2.event.sync()
          cord.event.sync.wait()
          cord.event.pending = function(value)
             exitvalue = value
             c2.event.sync2()
          end
          while true do cord:sleep(1) end
       end )

c2 = Nylon.cord( 'c2', 
       function(cord)
          c1.event.pending(12345)
          cord.event.sync.wait()
          c1.event.sync()
          cord.event.sync2.wait()
          os.exit( (exitvalue==12345) and 0 or 1)
       end )


Nylon.run()
