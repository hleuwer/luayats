require "yats.stdlib"
require "yats.src"
require "yats.muxdmx"
require "yats.misc"
require "yats.switch"

-- Example test-1.lua: Derived from yats example 'tst'.
--
-- cbrsrc_1 --> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 --> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 --> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 --> | |    | |--> line_4 --> sink_4
-- cbrsrc_n --> |/      \|--> line_n --> sink_n
--              mux    demux

-- Setting test non nil provides additional output.
test = nil
_DEBUG = {level = 5}

local log = yats.log

-- Show a list of loaded packages.
print("Loaded packages: \n"..pretty(_LOADED))

-- Init the yats random generator.
print("Init random generator.")
yats.sim:SetRand(10)

-- Reset simulation time.
print("Reset simulation time.")
yats.sim:ResetTime()

-- Use any number you like here
nsrc = 6

-- Create Sources
src = {}
for i = 1, nsrc do
  src[i] = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"switch", "in"..i}}
  yats.log:debug("object '"..src[i].name.."' of class '"..(src[i].clname or "unknown").."' created.\n")
end

-- Create Switch
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
swi = yats.switch{"switch", ninp = nsrc, maxvci = nsrc, buff = 10, out = dout}
log:debug("object '"..swi.name.."' of class '"..(swi.clname or "unknown").."' created.\n")

-- Create Lines and Sinks
lin, snk = {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"sink"..i, "sink"}}
  log:debug("object '"..lin[i].name.."' of class '"..(lin[i].clname or "unknown").."' created.")
  snk[i] = yats.sink{"sink"..i}
  print("object '"..snk[i].name.."' of class '"..(snk[i].clname or "unknown").."' created.", t)
  -- Provide a routing table entry for this source
  swi:signal{i,i,i}
end

-- Connection management
--yats.sim:run(0,0)
yats.sim:connect()

result = {}

-- Display demux switch table
log:debug(string.format("demux switch table = %s\n",pretty(swi:getRouting())))
local st, entries 
st, entries = swi:getRouting()
print("Table has "..entries.." entries.")
for i = 1, table.getn(st) do
  log:debug("(from, to, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
  table.insert(result, st[i].from)
  table.insert(result, st[i].to)
  table.insert(result, st[i].outp)
end

-- Run simulation: 1. path
yats.sim:run(1000000, 100000)
print("Mux Queue Len: "..swi:getQLen())
table.insert(result, swi:getQLen())
-- Run simulation: 2. path (optional)
yats.sim:run(1000000, 100000)

-- Display some values from the Multiplexer.
print("Mux Queue Len: "..swi:getQLen())
print("Mux Queue MaxLen: "..swi:getQMaxLen())
table.insert(result, swi:getQLen())
table.insert(result, swi:getQMaxLen())
for i = 1, nsrc do
  print("Mux "..swi.name.." Loss "..i..": "..swi:getLoss(i).."  "..swi:getLossVCI(i))
  -- Note: tolua maps lua table index 1 to C index 0. 
  -- Hence, we to add 1 to the vci index in mx.yatsobj.lostVCI[i]
  print("Mux "..swi.name.." Loss "..i..": "..swi.mux.lost[i].."  "..swi.mux.lostVCI[i+1])
  table.insert(result, swi:getLossVCI(i))
end
-- Display source counters.
for i = 1, nsrc do
  print("Source "..src[i].name.." couont: "..src[i]:getCounter())
  table.insert(result, src[i]:getCounter())
end
-- Display sink counter.
for i = 1, nsrc do
  print("Sink "..snk[i].name.." count: "..snk[i]:getCounter())
  table.insert(result, snk[i]:getCounter())
end

return result