-- test 'addOneShot' function. 
io.write 'syscore.addOneShot timer'

require 'site'
require 'nylon.syscore'

assert( NylonSysCore.addOneShot )

NylonSysCore.addOneShot( 10, function()
                                  os.exit(0)
                               end )

NylonSysCore.mainLoop()

