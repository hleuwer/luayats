require "yats.rstp.play"

linkToBreak = tonumber(os.getenv("TK_LINK")) or 3
--linkToBreak = 1
NumberOfBridges = 7

-- Setting test non nil provides additional output.
--test = true
test = nil

-- Init the yats random generator.
yats.log:info("Init random generator.")
yats.sim:SetRand(10)
yats.sim:setSlotLength(1e-3)
-- logging options{"DEBUG", "INFO", "WARN", "ERROR", "FATAL"}
yats.ieeebridge.pdulog:setLevel("FATAL")
yats.ieeebridge.log:setLevel("ERROR")
-- Reset simulation time.
yats.log:info("Reset simulation time.")
yats.sim:ResetTime()

--
--sources--
--

start_delay =1
proc_time = 50

basemac={
"110000",
"220000",
"330000",
"340000",
"350000",
"360000",
"330700"
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
  start_delay=start_delay or yats.random(1,2000),
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
  start_delay=start_delay or yats.random(1,2000),
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
  start_delay=start_delay or yats.random(1,2000),
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
    rxbpdu = playEventCallback,
    portstate = playEventCallback,
    flush = playEventCallback}
end

yats.sim:run(10000, 10000)
for i = 1,7 do bridge[i]:show_port(0) end
-- Produce test result.

fwddelay = 6
maxage = 8
for _, i in ipairs{ 1, 2, 3, 4, 5, 6, 7 }
do
   assert(bridge[i]:config_stp(0, "max_age", maxage))
   assert(bridge[i]:config_stp(0, "forward_delay", fwddelay))
end

yats.sim:run(90000, 10000)

for i = 1,7 do bridge[i]:show_port(0) end
print("Now we shutdown bridge "..linkToBreak)
bridge[linkToBreak]:shutdown()

yats.sim:run(100000, 20000)

for i = 1,7 do bridge[i]:show_port(0) end
print("Now we restart bridge "..linkToBreak)
bridge[linkToBreak]:start()

yats.sim:run(100000, 20000)
for i = 1,7 do bridge[i]:show_port(0) end


