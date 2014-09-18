-- Test ping-pong communication between 2 cords

io.write ':msg() / :getmsg() ping-pong'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(1); end )

local c1 = Nylon.cord( 'c1', 
       function(cord)
          local m = cord:getmsg()
          if m.ping then
             m.sender:msg{ isokay = true }
          end
       end )

Nylon.cord( 'c2', 
       function(cord)
          c1:msg{ sender = cord, ping = true }
          local pong = cord:getmsg()
          if pong.isokay then
             os.exit(0)
          else
             os.exit(2)
          end
       end )


Nylon.run()
