require "yats.rstp.play"

linkToBreak = tonumber(os.getenv("TK_LINK")) or 3
NumberOfBridges = 7

-- Run times
local sec = 10000
local runtimes = {
  sec * 1,      --   1s init: 1 s
  sec * 9,      --  10s change max age
  sec * 91,     -- 101s failure
  sec * 100.0,    -- 201s ok
  sec * 300     -- 301s finish
}

-- Setting test non nil provides additional output.
--test = true
test = nil

-- Init the yats random generator.
yats.log:info("Init random generator.")
yats.sim:SetRand(10)
yats.sim:setSlotLength(1/sec)
-- logging options{"DEBUG", "INFO", "WARN", "ERROR", "FATAL"}
yats.ieeebridge.pdulog:setLevel("FATAL")
yats.ieeebridge.log:setLevel("ERROR")
-- Reset simulation time.
yats.log:info("Reset simulation time.")
yats.sim:ResetTime()

--
--sources--
--

local function start_delay(i)
  return 1
end
proc_time = 3

basemac={
  string.char(1,1,0,0,0,0),
  string.char(2,2,0,0,0,0),
  string.char(3,3,0,0,0,0),
  string.char(3,4,0,0,0,0),
  string.char(3,5,0,0,0,0),
  string.char(3,6,0,0,0,0),
  string.char(3,3,0,7,0,0)
}

priorities = {
  {[0] = 10},
  {[0] = 12},
  {[0] = 11},
  {[0] = 12},
  {[0] = 12},
  {[0] = 12},
  {[0] = 12}
}


bridge = {}

i=1
  bridge[i] = yats.bridge{
  "bridge["..i.."]", 
  process_speed = proc_time,
  nport=2, 
  basemac=basemac[i],
  vlans = priorities[i],
  memberset = {[0]= {1,1,1,1}},
  start_delay=start_delay(i),
  out = {
	  {"bridge[7]","in"..2},
	  {"bridge["..(i+1).."]","in"..1},
  }
}

for _,i in ipairs{2, 3, 4, 5, 6}
do
  bridge[i] = yats.bridge{
  "bridge["..i.."]", 
  process_speed = proc_time,
  nport=2, 
  basemac=basemac[i],
  vlans = priorities[i],
  memberset = {[0]= {1,1,1,1}},
  start_delay=start_delay(i),
  out = {
	  {"bridge["..(i-1).."]","in"..2},
	  {"bridge["..(i+1).."]","in"..1}
  }
}
end

i=7
  bridge[i] = yats.bridge{
  "bridge["..i.."]", 
  process_speed = proc_time,
  nport=2, 
  basemac=basemac[i],
  vlans = priorities[i],
  memberset = {[0]= {1,1,1,1}},
  start_delay=start_delay(i),
  out = {
     {"bridge["..(i-1).."]","in"..2},
     {"bridge[1]","in"..1}
   }
}


-- Connection management
yats.log:debug("connecting")
yats.sim:connect()
for i = 1, 7 do 
  bridge[i]:stateLog({1,2}, true)
  bridge[i]:setCallback{
    txbpdu = playEventCallback,
    rxbpdu = playEventCallback,
    portstate = playEventCallback,
    flush = playEventCallback
  }
end

yats.sim:run(runtimes[1])

bridge[3]:portTrace("all", 0, 1, true)
bridge[3]:portTrace("all", 0, 2, true)
bridge[4]:portTrace("all", 0, 1, true)
bridge[4]:portTrace("all", 0, 2, true)

yats.sim:run(runtimes[2])
for i = 1,7 do bridge[i]:show_port(0) end

-- Produce test result.

fwddelay = 6
maxage = 8
for _, i in ipairs{ 1, 2, 3, 4, 5, 6, 7 }
do
   assert(bridge[i]:config_stp(0, "max_age", maxage))
   assert(bridge[i]:config_stp(0, "forward_delay", fwddelay))
end

yats.sim:run(runtimes[3], 10000)
for i = 1,7 do bridge[i]:show_port(0) end


print("Now we plug out link "..linkToBreak)

if (linkToBreak < NumberOfBridges)
then
   bridge[linkToBreak]:linkDown(2)
   bridge[linkToBreak+1]:linkDown(1)
else
   bridge[linkToBreak]:linkDown(2)
   bridge[1]:linkDown(1)
end


yats.sim:run(runtimes[4])
for i = 1,7 do bridge[i]:show_port(0) end

print("Now we plug in link "..linkToBreak)

if (linkToBreak < NumberOfBridges)
then
   bridge[linkToBreak]:linkUp(2)
   bridge[linkToBreak+1]:linkUp(1)
else
   bridge[linkToBreak]:linkUp(2)
   bridge[1]:linkUp(1)
end
 
yats.sim:run(runtimes[5])
for i = 1,7 do bridge[i]:show_port(0) end


