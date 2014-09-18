-- Test cord yield
io.write ':yield()'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(1); end )

local c1_exited = false

local c1 = Nylon.cord( 'c1', 
       function(cord)
          while true do
             cord:yield()
          end
       end )

Nylon.cord( 'c2', 
       function(cord)
          cord:sleep(0.01)
          os.exit(0)
       end )


Nylon.run()
