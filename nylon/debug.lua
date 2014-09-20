-- Nylon Copyright (c) 2013 David M. Placek see [doc/license.txt]

local Debug = {}

local config = nil

local function openOutFile( name )
   config.outfile = io.open( name, 'w' )
   if not config.outfile then
      config.outfile = io.stdout
   end
end

local pluggable_log_io = function( groups, text )
   if not config.outfile then
      if not config.filename then
         config.filename = string.format('%s/%s.log', config.dir, config.name )
      end
      openOutFile( config.filename )
   end
   config.outfile:write( string.format("%4.3f ", NylonSysCore.uptime()) )
   config.outfile:write( string.format("{%s} ", groups) )
   config.outfile:write( text )
   config.outfile:write( '\n' )
   config.outfile:flush()
end

local log_disable = {
   mbox = true, evt = true,
   ['trace,core,post'] = true
}

function Debug.configure( p )
   -- print( 'Got call to debug.configur, p.name=%s', tostring(p.name) )
   local o = {}
   local lconfig = {
      dir = '/tmp',
      name = 'nylon'
   }
   config = lconfig
   if p and p.name then
      lconfig.name = p.name
   end
   lconfig.filename = string.format('%s/%s.log', lconfig.dir, lconfig.name )
   if p and p.filename then
      lconfig.filename = p.filename
   end
   openOutFile( lconfig.filename )
   return setmetatable( o, {
                           __index = function(t,key)
                              config = lconfig
                              return Debug[key]
                           end
   } )
end

function Debug.log_active( grps )
   if log_disable[grps] then
      return false
   else
      return true
   end
end


function Debug.log( grps, fmt, ... )
   if not fmt then
      fmt = grps
      grps = 'debug'
   end
   if not Debug.log_active( grps ) then
      return
   end
   local formatted_msg
   local ok, err = pcall( function(...)
                             formatted_msg = string.format( fmt, ... )
                          end, ... )
   if ok then
      local info = debug.getinfo( 2, "Sln" )
      pluggable_log_io( grps, string.format("%s:%d %s", info.short_src, info.currentline, formatted_msg ) )
   else
      pluggable_log_io( 'error', 
                        string.format( '%s\n\t%s\n%s', err, (fmt or '[[NULL-FMT-STRING]]'), Debug.backtrace() ) )
   end
end


Debug.setpluggable = {
   log_io = function( fn ) pluggable_log_io = fn end,
}


local function backtrace_msg_x_( ignoreLevels, returnOpt )
   
--   if dbgoff then
--      dbgoff()
--   end

    local traces = {}

    local fudge    = ((returnOpt and 1) or 0)
    local tablePos = 1-fudge
    local level    = 2 + (ignoreLevels or 0)

    local tailCallDepth = 0
    while true do
       -- usw.log( "check level:" .. (level) )

       local info = debug.getinfo( level, "Sln" )
       if not info then break end

       -- usw.log( "level:" .. (level) .. " what:" .. (info.what) )
       if info.what == 'tail' then
          tailCallDepth = tailCallDepth + 1

          if (not gDebug) and (tailCallDepth > 100) then
             traces[tablePos] = '[TAIL CALL] x {{max depth exceeded}} (' .. (tailCallDepth) .. ')'
             break
          end

       else
          if tailCallDepth > 0 then
             traces[tablePos] = '[TAIL CALL] x ' .. (tailCallDepth)
             tailCallDepth = 0
             tablePos = tablePos + 1
          end

          if info.what == 'C' then
             traces[tablePos] = "C Function"
          else
             traces[tablePos] = string.format("[%s]:%d {%s}", info.short_src, (info.currentline or -1), --info.what, 
                                              info.name or '???' )-- , info.func )
          end
          tablePos = tablePos + 1
       end
       level = level+1
    end

    if returnOpt then
       return traces[0], table.concat( traces, "\n\t" ) -- {'foo','bar'} ) -- traces, "\n\t" )
    else
       return table.concat( traces, "\n\t" ) -- {'foo','bar'} ) -- traces, "\n\t" )
    end
 end


function Debug.backtrace( ignoreLevels, returnOpt )
   return backtrace_msg_x_( ignoreLevels, returnOpt )
end

function Debug.deprecated( msg )
   Debug.log('abnorm','Deprecated call%s\n\t%s', ((msg and ': ' .. msg) or ''), Debug.backtrace(2) )
end

setmetatable( Debug, {
                 __call = function( t, p )
                    return Debug.configure( p )
                 end
})


return Debug