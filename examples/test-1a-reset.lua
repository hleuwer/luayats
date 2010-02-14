require "stdlib"
require "src"
require "muxdmx"
require "misc"

for i = 1, 100 do
  print("restart")
  dofile("examples/test-1a.lua")
  print("done a"..i..": ", gcinfo())
  yats.sim:reset()
  collectgarbage(0)
  print("done b"..i..": ", gcinfo())
end
  
