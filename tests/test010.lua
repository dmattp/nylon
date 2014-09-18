-- Test :sleep()
io.write ':sleep() scheduling'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(99); end )

local wokefromsleep = false

local c1 = Nylon.cord( 'c1', 
       function(cord)
          cord:sleep(0.01)
          wokefromsleep = true 
       end )

Nylon.cord( 'c2', 
       function(cord)
          cord:sleep(0.05)
          os.exit( wokefromsleep and 0 or 1)
       end )


Nylon.run()
