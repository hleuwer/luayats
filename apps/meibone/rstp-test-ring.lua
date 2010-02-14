--------------------------------------------------------------------
-- Template for an RSTP test case: ring with n bridges
-- 
-- $Id: rstp-test-ring.lua 222 2009-06-06 05:51:35Z leuwer $
--------------------------------------------------------------------
require "yats.core"
require "yats.stdlib"
require "yats.rstp"

--
-- Configurations
--

-- linkToBreak = tonumber(os.getenv("TK_LINK") or 6)
linkToBreak = 6
NumberOfBridges = 7
start_delay = 1
proc_time = 10
fwddelay = 5
maxage = 7

-- Logging and Tracing
local logfull = true

-- PDUs and state changes
logstate = logfull or true
logpdu = logfull or false
-- the bridge
trcbridge = logfull or false 
-- the STP within bridge
trcstpm = logfull or false   
-- the port within bridge
trcport = logfull or false   

basemac={
  string.char(1,1,0,0,0,0),
  string.char(2,2,0,0,0,0),
  string.char(3,3,0,0,0,0),
  string.char(3,4,0,0,0,0),
  string.char(3,5,0,0,0,0),
  string.char(3,6,0,0,0,0),
  string.char(3,7,0,0,0,0),
}

priorities = {
  {[0] = 10},
  {[0] = 12},
  {[0] = 12},
  {[0] = 12},
  {[0] = 12},
  {[0] = 12},
  {[0] = 12}
}

--
-- Logging
--
-- HELP: logging options{"DEBUG", "INFO", "WARN", "ERROR", "FATAL"}
yats.ieeebridge.pdulog:setLevel("INFO")
yats.ieeebridge.log:setLevel("INFO")

--
-- Basic Simulator Initialisation
--
-- * Init the yats random generator.
yats.log:info("Init random generator.")
yats.sim:SetRand(10)
yats.sim:setSlotLength(1e-3)

-- * Reset simulation time.
yats.log:info("Reset simulation time.")
yats.sim:ResetTime()

-- Event callback
local item = 1
local headerlist = {}
local bpdulist = {}

function evcb(what, param)
  if what == "portstate" then
    if string.lower(param.state) == "forwarding" then
      print(string.format("%d (%s) USER CALLBACK portstate: bridge '%s' port %d of vid %d now %s",
			  param.tick,
			  param.time,
			  param.node.name, param.port, param.vid, "forwarding"))
    end
  elseif what =="rxbpdu" then
    local sender = param.sender
    local receiver = param.receiver
    headerlist[item] = string.format("%d: %8d %s.p%d =====> %s.p%d", item,
				     param.tick, sender.name, param.senderport,
				     receiver.name, param.receiverport)
    bpdulist[item] = tostring(yats.decode(param.bpdu, "bpdu"))
    item = item + 1
  elseif what == "flush" then
    print(string.format("%d (%s) USER CALLBACK flush: bridge '%s' port %d of vid %d flushed with type %s and reason %s",
			param.tick,
			param.time,
			param.node.name, param.port, param.vid, param.typ, param.reason))
  end
end
--
-- Node definitions
--

bridge = {}

local i=1
bridge[i] = yats.bridge{
  "bridge["..i.."]",
  process_speed = proc_time,
  nport=2, 
  basemac=basemac[i],
  vlans = priorities[i],
  memberset = {[0]= {1,1,1,1}},
  start_delay=start_delay or yats.random(1,2000),
  out = {
    {"bridge[7]","in"..2},
    {"bridge["..(i+1).."]","in"..1}
  }
}

for i = 2,6 do
  bridge[i] = yats.bridge{
    "bridge["..i.."]", 
    process_speed = proc_time,
    nport=2, 
    basemac=basemac[i],
    vlans = priorities[i],
    memberset = {[0]= {1,1,1,1}},
    start_delay=start_delay or yats.random(1,2000),
    out = {
      {"bridge["..(i-1).."]","in"..2},
      {"bridge["..(i+1).."]","in"..1}
    }
  }
end

i = 7
bridge[i] = yats.bridge{
  "bridge["..i.."]", 
  process_speed = proc_time,
  nport=2, 
  basemac=basemac[i],
  vlans = priorities[i],
  memberset = {[0]= {1,1,1,1}},
  start_delay=start_delay or yats.random(1,2000),
  out = {
    {"bridge["..(i-1).."]","in"..2},
    {"bridge[1]","in"..1}
  }
}

--
-- Connection management
--
yats.log:debug("connecting")
yats.sim:connect()

yats.sim:run(10, 10000)

--
-- Set Tracing options
--
bridge[5]:Trace(trcbridge):stpmTrace(0, trcstpm)
bridge[6]:Trace(trcbridge):stpmTrace(0, trcstpm)
bridge[5]:portTrace("all", 0, 2, trcport)
bridge[5]:portTrace("all", 0, 1, trcport)
bridge[6]:portTrace("all", 0, 1, trcport)

bridge[5]:portPduLog("rt", {1,2}, logpdu)
bridge[6]:portPduLog("rt", {1,2}, logpdu)

bridge[5]:stateLog({1,2}, logstate)
bridge[6]:stateLog({1,2}, logstate)

--
-- Event callbacks
--

bridge[5]:setCallback{rxbpdu = evcb, portstate = evcb, txbpdu = evcb, flush = evcb}
bridge[6]:setCallback{rxbpdu = evcb}
--
-- Produce test result.
--
for i = 1, 7 do
   assert(bridge[i]:config_stp(0, "max_age", maxage))
   assert(bridge[i]:config_stp(0, "forward_delay", fwddelay))
end


yats.sim:run(99999, 10000)

print("= PATH 1 first convergence ===============================")
for i=1,7 do bridge[i]:show_port(0) end
print("= PATH 1 first convergence ==============================")


print("Now we plug out link "..linkToBreak)

if (linkToBreak < NumberOfBridges)
then
   bridge[linkToBreak]:linkDown(2)
   bridge[linkToBreak+1]:linkDown(1)
else
   bridge[linkToBreak]:linkDown(2)
   bridge[1]:linkDown(1)
end


yats.sim:run(100000, 20000)

print("= PATH 2 after failure ===============================")
for i=1,7 do bridge[i]:show_port(0) end
print("= PATH 2 after failure ===============================")

print("Now we plug in link "..linkToBreak)

if (linkToBreak < NumberOfBridges)
then
   bridge[linkToBreak]:linkUp(2)
   bridge[linkToBreak+1]:linkUp(1)
else
   bridge[linkToBreak]:linkUp(2)
   bridge[1]:linkUp(1)
end

yats.sim:run(100000, 20000)

print("= PATH 3 after recovery ===============================")
for i=1,7 do bridge[i]:show_port(0) end
print("= PATH 3 after recovery  ===============================")

iup.SetLanguage("ENGLISH")
local sel = 1
repeat 
  sel = iup.ListDialog(1, "BPDU Trace", item-1, headerlist, sel, 40, 20, {})
  if sel ~= -1 then
     iup.Message("BPDU #"..sel, headerlist[sel+1].."\n\n"..bpdulist[sel+1])
  end
until sel == -1

return {}