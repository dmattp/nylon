-- Confirm that Nylon core can start a cord.
io.write 'Nylon.cord creation'

require 'site'

require 'nylon.syscore'
NylonSysCore.addOneShot( 1000, function() -- failsafe timer
                                  print '\ngot failsafe'
                                  os.exit(1)
                               end )

local Nylon = require 'nylon.core'()

assert( Nylon.cord )
assert( Nylon.run )

Nylon.cord( 'test', function(cord)
                       os.exit(0)
                    end )

Nylon.run()
