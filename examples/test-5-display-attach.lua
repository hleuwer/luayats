require "stdlib"
require "src"
require "muxdmx"
require "misc"
require "graphics"

-- example test-1.lua: derived from yats example 'tst'.
--
-- cbrsrc_1 --> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 --> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 --> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 --> | |    | |--> line_4 --> sink_4
-- cbrsrc_n --> |/      \|--> line_n --> sink_n
--              mux    demux

-- setting test non nil provides additional output.
test = nil

-- show a list of loaded packages.
print("loaded packages: \n"..ptostring(_loaded))

-- init the yats random generator.
print("init random generator.")
yats.sim:setrand(10)

-- reset simulation time.
print("reset simulation time.")
yats.sim:resettime()

-- use any number you like here
nsrc = 6

-- create sources
src = {}
for i = 1, nsrc do
  src[i], name = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}
  print("object '"..name.."' of class '"..(src[i].clname or "unknown").."' created.")
end

-- create multiplexer
mx, name = yats.mux{"mux", ninp=nsrc, buff=10, out={"demux", "demux"}}
print("object '"..name.."' of class '"..(mx.clname or "unknown").."' created.")

-- create demultiplexer
-- we first create the list of outputs.
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
dmx, name = yats.demux{"demux", maxvci = nsrc, nout = nsrc, out = dout}
print("object '"..name.."' of class '"..(dmx.clname or "unknown").."' created.")

-- create lines and sinks
lin, snk, meas = {}, {}, {}
for i = 1, nsrc do
  lin[i], name = yats.line{"line"..i, delay = 2, out = {"meas"..i, "meas"}}
  print("object '"..name.."' of class '"..(lin[i].clname or "unknown").."' created.")
  snk[i], name = yats.sink{"sink"..i}
  print("object '"..name.."' of class '"..(snk[i].clname or "unknown").."' created.")
  meas[i], name = yats.meas{"meas"..i, vci = i, maxtim = 1000, out = {"sink"..i, "sink"}}
  print("object '"..name.."' of class '"..(meas[i].clname or "unknown").."' created.")
  -- provide a routing table entry for this source
  dmx:signal{i,i,i}
end

--histo = yats.histo2{"hist", val={mx,"qlen"}, title = "qlen", win={100,100,400,200}, nvals = 100, update=10000}
meter = yats.meter{"meter", val={mx,"qlen"}, title = "qlen-meter", win={500, 100, 400, 200}, nvals = 100, delta = 1000, mode=yats.absmode, maxval = 20, linemode=1, display=false}

-- some test output
if test then
  print(yats.sim:list("\nlist of objects","\t"))
  print("list of objects:"..tostring(yats.sim:list()))
  print("\na more detailed look on objects:")
  print(ptostring(yats.sim.objectlist))
end

-- connection management
--yats.sim:run(0,0)
yats.sim:connect()

-- check connection
print("connection check:")
for i = 1, nsrc do
  print("\tsuc of "..src[i].name..": "..src[i]:get_suc().luaobj.name, src[i]:get_shand())
  print("\tsuc "..i.." of "..dmx.name..": "..dmx:get_suc(i).luaobj.name, dmx:get_shand(i))
  print("\tsuc of "..lin[i].name..": "..lin[i]:get_suc().luaobj.name, lin[i]:get_shand())
end

-- display demux switch table
print("demux switch table: "..tostring(dmx:getrouting()))
local st, entries 
st, entries = dmx:getrouting()
print("table has "..entries.." entries.")
for i = 1able.getn(st) do
  print("(fromo, outp) = ("..st[i].from..", "..st[i].to..", "..st[i].outp..")")
end

disp = yats.display{"display", width=600, height=150}
iup.message("question", "ready to attach?") 
disp:attach(meter)
-- run simulation: 1. path
yats.sim:run(1000000, 100000)
print("mux queue len: "..mx:getqlen())

-- run simulation: 2. path (optional)
--yats.sim:run(1000000, 100000)

-- display some values from the multiplexer.
print("mux queue len: "..mx:getqlen())
print("mux queue maxlen: "..mx:getqmaxlen())
print("mux losses per vc:")
for i = 1, nsrc do
  print("mux "..mx.name.." loss "..i..": "..mx:getloss(i).."  "..mx:getlossvci(i))
end
print("\nsource cell counters:")
-- display source counters.
for i = 1, nsrc do
  print("source "..src[i].name.." couont: "..src[i]:getcounter())
end
-- display meas counter.
print("\nmeasurement cell counters:")
for i = 1, nsrc do
  print("meas "..meas[i].name.." count: "..meas[i]:getcounter())
end
-- display sink counter.
print("\nsink cell counters:")
for i = 1, nsrc do
  print("sink "..snk[i].name.." count: "..snk[i]:getcounter())
end
-- display meas distances.
print("\ncell delays:")
for i = 1, nsrc do
  io.stdout:write(string.format("%2d:",i))
  for j = 1, 15 do
    io.stdout:write(string.format("%7d",meas[i].yatsobj:getdist(j)))
  end
  print()
end
