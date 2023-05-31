
package.path  = package.path .. ';../?.lua'


if package.cpath:sub(1,1) == '/' then
   package.cpath = package.cpath .. ';../?.so'
else          
   package.cpath = package.cpath .. ';../?.dll'
end
