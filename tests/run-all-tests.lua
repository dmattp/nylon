local maxtest = 12
local exelua = arg[-1]

print '\nRunning all tests...\n======================'

for i = 1,maxtest do
   io.write( string.format('Running test %d...  ',i) )
   io.stdout:flush()
   local rc = os.execute( string.format('%s test%03d.lua',exelua,i) ) and 0 or 99

   print( (rc==0) and ' ...OK'  or ' ... FAIL !!!' )
   
   if rc ~= 0 then
      print( string.format('\n\n*** Failure executing test%03d.lua rc=%d,%d ***\n',i,math.floor(rc/256),(rc%256)) )
      os.exit(1)
   end
end

print '======================\nAll tests completed successfully!\n'

os.exit(0)
