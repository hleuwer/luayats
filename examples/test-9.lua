require "yats.stdlib"
require "yats.src"
require "yats.muxdmx"
require "yats.misc"
require "yats.switch"
require "yats.block"

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
nsw = 2

-- Define the superswitch block out of nsw switch blocks
yats.superswitch = class(yats.block)

function yats.superswitch:init(param)
  local self = yats.block()
  -- Generic part
  self.name = autoname(param)
  self.clname = "superswitch"
  self.parameters = {
    ninp = true,
    buff = true,
    out = true,
    maxvci = true,
    nsw = true
  }
  self.swi = {}
  self:adjust(param)

  local k,i
  local dout = {}
  local n = 1
  for k = 1, param.nsw do
    dout[k] = {}
    for i = 1, param.ninp do
      dout[k][i] = {self:localname("out"..n)}
      n = n + 1
    end
  end
  
  for i = 1, param.nsw do
    self.swi[i] = self:addobj(yats.switch{
				self:localname("swi"..i),
				ninp = param.ninp,
				maxvci = param.ninp*param.nsw,
				buff = 10,
				out = dout[i]
			      })
  end
  for k = 1, param.nsw do
    local n = 1
    for i = 1, param.ninp do
      self:definp{self:localname("swi"..k), "in"..n}
      n = n + 1
    end
  end
  self:defout(param.out)
  return self:finish()
end


function yats.superswitch:getsw(sw) return self.swi[sw] end
-- Create Sources
src = {}
local n = 1
local vci = 1
for k = 1, nsw do
  for i = 1, nsrc do
    src[n] = yats.cbrsrc{"src"..n, delta = 2, vci = vci, out = {"superswitch", "in"..n}}
    log:debug("object '"..src[n].name.."' of class '"..(src[n].clname or "unknown").."' created.\n")
    n = n + 1
    vci = vci + 1
  end
end

-- Create SuperSwitch
local dout = {}
for i = 1, nsrc*nsw do
  dout[i] = {"line"..i, "line"}
end
suswi = yats.superswitch{
  "superswitch", nsw = nsw, ninp = nsrc, maxvci = nsrc*nsw, buff = 10, out = dout
}
log:debug("object '"..suswi.name.."' of class '"..(suswi.clname or "unknown").."' created.\n")
-- Create Lines and Sinks
lin, snk = {}, {}
for i = 1, nsrc*nsw do
  lin[i] = yats.line{"line"..i, delay = 2, out = {"sink"..i, "sink"}}
  log:debug("object '"..lin[i].name.."' of class '"..(lin[i].clname or "unknown").."' created.")
  snk[i] = yats.sink{"sink"..i}
  log:debug("object '"..snk[i].name.."' of class '"..(snk[i].clname or "unknown").."' created.")
end

local from, to = 1, 1
for k = 1, nsw do
  local out = 1
  for i = 1, nsrc do
    -- Provide a routing table entry for this source
    suswi:getsw(k):signal{from,to,out}
    from = from + 1
    to = to + 1
    out = out + 1
  end
end

-- Connection management
--yats.sim:run(0,0)
yats.sim:connect()

result = {}

-- Display demux switch table
for k = 1, nsw do
  log:debug(string.format("switch[%d] table = %s\n",k,pretty(suswi:getsw(k):getRouting())))
--  table.insert(result, suswi:getsw(k):getRouting())
end

-- Run simulation: 1. path
yats.sim:run(200000, 100000)

-- Display some values from the Multiplexer.
print()
for k = 1, nsw do
  print("Mux Queue Len: "..suswi:getsw(k):getQLen())
  print("Mux Queue MaxLen: "..suswi:getsw(k):getQMaxLen())
  table.insert(result, suswi:getsw(k):getQLen())
  table.insert(result, suswi:getsw(k):getQMaxLen())
  for i = 1, nsrc do
    local sw = suswi:getsw(k)
    printf("Mux '%s' Loss %d: %d %d\n", sw.name, i, sw:getLoss(i), sw:getLossVCI(i))
    -- Note: tolua maps lua table index 1 to C index 0. 
    -- Hence, we have to add 1 to the vci index in mx.yatsobj.lostVCI[i]
    table.insert(result, sw:getLoss(i), sw:getLossVCI(i))
  end
end
-- Display source counters.
for i = 1, nsrc*nsw do
  print("Source "..src[i].name.." count: "..src[i]:getCounter())
  table.insert(result, src[i]:getCounter())
end
-- Display sink counter.
for i = 1, nsrc*nsw do
  print("Sink "..snk[i].name.." count: "..snk[i]:getCounter())
  table.insert(result, snk[i]:getCounter())
end

return result