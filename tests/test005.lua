-- Test that a cord can receive a message.
io.write ':msg() / :getmsg() send/receive'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(1); end )

local c1 = Nylon.cord( 'c1', 
       function(cord)
          local m = cord:getmsg()
          if m.value == 1234 then
             os.exit(0)
          end
       end )

c1:msg{ value = 1234 }

Nylon.run()
