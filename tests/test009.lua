-- Test sleep_manual
io.write ':sleep_manual()'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(1); end )

Nylon.cord( 'c1', 
       function(cord)
          local gotTimer = false
          cord:sleep_manual( 
             function(wakeup)
                NylonSysCore.addOneShot( 10, function() gotTimer = true; wakeup() end )
             end )
          os.exit( gotTimer and 0 or 1 )
       end )

Nylon.run()
