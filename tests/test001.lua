-- test that library loads and provides basic functions needed for test.
io.write 'load nylon.syscore'

require 'site'
require 'nylon.syscore'

assert( NylonSysCore )
assert( NylonSysCore.mainLoop )
assert( NylonSysCore.addCallback )

NylonSysCore.addCallback( function()
                             os.exit(0)
                          end )

NylonSysCore.mainLoop()

