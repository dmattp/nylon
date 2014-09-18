-- Test wait on cord exit
io.write ':waiton() cord exit'

require 'site'

local Nylon = require 'nylon.core'()

Nylon.cord( 'failsafe', function(cord) cord:sleep(1.5); os.exit(1); end )

local c1_exited = false

local c1 = Nylon.cord( 'c1', 
       function(cord)
          cord:sleep(0.05)
          c1_exited = true
       end )

Nylon.cord( 'c2', 
       function(cord)
          cord:waiton(c1)
          if c1_exited then
             os.exit(0)
          else
             os.exit(1)
          end
       end )


Nylon.run()
