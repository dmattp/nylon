require 'site'

local rpcsender = (require "jsonrpc").create_sender( '127.0.0.1', 5001 )

print( rpcsender.print 'Hello World from sender!!' )
print( rpcsender.add(25,17) )

print( (require 'JSON'):encode( rpcsender.msgcheck('gooba-wooba') ) )

--local response = rpcit{ id = os.time(), method = "print", params = { "hello world from sender!" } }
--local response = rpcit{ id = os.time(), method = "add",   params = { 3, 5 } }


