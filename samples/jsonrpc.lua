-- Required packages:
--   * lanes
--   * JSON
--   * LuaSocket

-- Demonstrates the use of lua lanes with Nylon to create a TCP server where
-- the JSON-RPC requests are handled by a Nylon cord. The socket I/O for the
-- server is handled by a lane thread so that the main Nylon application isn't
-- blocked waiting on socket I/O.
--
-- See jsonrpc-listen.lua and jsonrpc-send.lua

local function create_listener( ip, remoteTablePort, methods )
   local remoteTablePort = 5001

   local Nylon = require 'nylon.core'()
   local wv = require 'nylon.debug'{ name = 'jrpc' }

   local linda_response

   local function laned_server( cbfun )
      local socket = require("socket")
      local JSON = require 'JSON'
      local wv = require 'nylon.debug' { name = 'lanedserver' }
      -- create a TCP socket and bind it to the local host, at any port
      local server = assert(socket.bind("*", remoteTablePort))
      -- find out which port the OS chose for us
      local ip, port = server:getsockname()
      -- print a message informing what's up
      wv.log('debug',"remote table server on ip=%s port=%d",ip, port)
      --   print("After connecting, you have 10s to enter a line to be echoed")
      -- loop forever waiting for clients
      while 1 do
         -- wait for a connection from any client
         local client = server:accept()
         wv.log('debug,server','client connect')
         -- make sure we don't block waiting for this client's line
         client:settimeout(30)
         -- receive the line
         while true do
            local line, err = client:receive()
            wv.log('debug,server','got line=%s e=%s, passing to Cort handler', line, tostring(err) )
            if err or line == '^' then 
               wv.log('debug','client disconnect')
               break
            else
               cbfun( line ) 
               wv.log('debug,server','passed request back to Cort handler' )
               local k, v = linda_response:receive( 'response' )
               --local v = JSON:encode{ params={} }
               wv.log('debug,server','response=%s; sending to client', v )
               client:send( v )
               client:send( '\n' )
               wv.log('debug,server','sent to client', v )
            end
         end
         -- done with client, close the object
         client:close()
      end   
   end

   local JSON=require 'JSON'

   local function cortTableService( cort, methods )
      local lanes = require 'lanes'
      if lanes.configure then
         lanes.configure()
      end
      linda_response = lanes.linda()
      
      while true do
         -- implement simple JSONRPC-like interface
         local server = 
            cort:lanediter( function( line )
                               wv.log('prime','Cort handler got line="%s"', line )
                               -- print('Cort handler got line=', line )
                               local cmd
                               local rc, e = pcall( function()
                                                       cmd = JSON:decode( line )
                                                    end )
                               if not rc then
                                  wv.log('error','JSON decode %s: %s', line, e )
                                  linda_response:send( 'response', '{ "result": 0 }' )
                               elseif cmd.method and methods[ cmd.method ] then
                                  wv.log('debug,server','Cort handler, method=%s', cmd.method )
                                  local method_rc
                                  local okay, err = pcall( 
                                         function()
                                            method_rc = methods[ cmd.method ]( table.unpack( cmd.params or {}) )
                                         end )
                                  local rcjson
                                  if not okay then
                                     rcjson = JSON:encode{ id = cmd.id, error = err }
                                  else
                                     rcjson = JSON:encode{ id = cmd.id, result = method_rc }
                                  end
                                  wv.log('debug,server','handler response=%s', rcjson )
                                  linda_response:send( 'response', rcjson )
                               else
                                  wv.log('error','Invalid request or unknown method [%s]', 
                                         tostring( cmd.method ) )
                               end
                            end, laned_server )
      end
   end

   return Nylon.cord('listener', cortTableService, methods )
end




local function create_sender( server, port )

   local socket = require 'socket'
   local JSON = require 'JSON'

   local function rpcit( json )
      if type(json) == 'table' then
         json = JSON:encode(json)
      end
      -- local ip = assert( socket.dns.toip( server ) )
      local s = socket.tcp()
      local conn = s:connect( server, port )
      s:send( json .. "\n" )
      local response = s:receive()
      s:close()
      return response
   end

   return setmetatable({}, {
             __index = function(t,method)
                          return function(...)
                                    local id = os.time()
                                    local response = rpcit{ method = method, id=id, params={...} }
                                    local rc = JSON:decode( response )
                                    if rc.error then
                                       error( rc.error )
                                    elseif rc.id == id then
                                       return rc.result
                                    else
                                       error( "Invalid JSON-RPC for method=" .. method .. 
                                              " id=" .. id .. " response=" .. response )
                                    end
                                 end
                       end })
end


return {
   create_sender = create_sender,
   create_listener = create_listener
}
