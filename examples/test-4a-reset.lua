require "stdlib"
require "src"
require "muxdmx"
require "misc"
require "graphics"
for i = 1, 10 do
  dofile("examples/test-4a.lua")
  yats.sim:reset()
  collectgarbage(0)
  print("done "..i..": ", gcinfo())
end
