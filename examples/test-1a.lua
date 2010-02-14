require "stdlib"
require "src"
require "muxdmx"
require "misc"
-- Example test-1.lua: Derived from yats example 'tst'.
--
-- cbrsrc_1 --> |\      /|--> line_1 --> sink_1
-- cbrsrc_2 --> | |    | |--> line_2 --> sink_2
-- cbrsrc_3 --> | | -->| |--> line_3 --> sink_3
-- cbrsrc_4 --> | |    | |--> line_4 --> sink_4
-- cbrsrc_n --> |/      \|--> line_n --> sink_n
--              mux    demux

-- Setting test non nil provides additional output.
local test = nil

-- Show a list of loaded packages.

-- Init the yats random generator.
yats.sim:SetRand(10)

-- Reset simulation time.
yats.sim:ResetTime()

-- Use any number you like here
local nsrc = 6

-- Create Sources
local src = {}
for i = 1, nsrc do
  src[i] = yats.cbrsrc{"src"..i, delta = 2, vci = i, out = {"mux", "in"..i}}
end

-- Create Multiplexer
local mx = yats.mux{"mux", ninp=nsrc, buff=10, out={"demux", "demux"}}

-- Create Demultiplexer
-- We first create the list of otputs.
local dout = {}
for i = 1, nsrc do
  dout[i] = {"line"..i, "line"}
end
local dmx = yats.demux{"demux", maxvci = nsrc, nout = nsrc, out = dout}

-- Create Lines and Sinks
local lin, snk = {}, {}
for i = 1, nsrc do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"sink"..i, "sink"}}
  snk[i] = yats.sink{"sink"..i}
  -- Provide a routing table entry for this source
  dmx:signal{i,i,i}
end

-- Connection management
yats.sim:connect()

-- Run simulation: 1. path
yats.sim:run(10000, 1000)
