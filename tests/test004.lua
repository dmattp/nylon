-- Confirm that a Nylon cord can sleep and wakeup.
io.write ':sleep() confirm wakeup'

require 'site'

require 'nylon.syscore'
NylonSysCore.addOneShot( 1000, function()
                                  print '\tFAILSAFE TIMER EXPIRED'
                                  os.exit(1)
                               end )

local Nylon = require 'nylon.core'()

Nylon.cord( 'test', function(cord)
                       cord:sleep(0.02)
                       os.exit(0)
                    end )

Nylon.run()
