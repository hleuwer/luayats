-----------------------------------------------------------------------------------
-- @title LuaYats.
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 4.0 $Id: runctrl.lua 435 2010-02-13 17:07:19Z leuwer $
-- @description  Luayats - Graphical User Interface (GUI)
-- <br>
-- <br><b>module: gui</b><br>
-- <br>
-- This module implements the simulation run control of Luayats.
-----------------------------------------------------------------------------------

local iup = require "iuplua"
-- Note: We need to load cdlua before iupluacontrols, because otherwise
--       we might end up with cdlua being loaded twice, once by iupluacontrols
--       via dll dependency for /usr/local/bin/cdlua51.dll and once via require from
--       /usr/local/lib/lua/5.1/cdlua51.dll. This leads to memory conflicts (cdluapdf.dll
--       crashes).
require "cdlua"
require "iupluacontrols"
local yats = require "yats.core"
local config = require "yats.config"
local conf = config.data
require "yats.stdlib"

local start, stop = "start", "stop"
local busy, arrow = "BUSY", "ARROW"
local yes, no, on, off = "YES", "NO", "ON", "OFF"
local workerentry, workerexit = yats.workerentry, yats.workerexit

local _tostring = _G.tostring

module("gui", package.seeall)

command = {
   val = nil,
   get = function(self) 
	    local retval = self.val 
	    self:cls() 
	    return retval 
	 end,
   set = function(self, cmd) 
	    log:debug("command.set: "..cmd)
	    self.val = cmd 
	 end,
   cls = function(self) 
	    self.val = nil 
	 end
}

local state = "IDLE"

function getState()
   return state
end

function newState(newstate)
   local oldstate = state
   if newstate == oldstate then 
      return oldstate, newstate 
   end
   log:debug(string.format("%s => %s", state, newstate))
   state = newstate
   return oldstate, newstate
end

local function cursor(what)
   notebook:dialog().cursor = what
end

-----------------------------------------------------------------------------------
-- Enable simulation controls.
-- @param state Command 'ON' or 'OFF'
-- @return none.
-----------------------------------------------------------------------------------
function enableSimControls(state)
   log:debug("enabling ctrl button/menu: "..state)
   local item = menuitems.simul.run
   local item2 = menuitems.simul.reset
   if state == "ON" then
      item.active = yes
      item.bmap.active = yes
      item2.active = no
      item2.bmap.active = no
   elseif state == "OFF" then
      item.active = yes
      item.bmap.active = yes
      item2.active = no
      item2.bmap.active = no
   end
end

-----------------------------------------------------------------------------------
-----------------------------------------------------------------------------------
function setSimControls(state)
--   log:debug("setting ctrl button/menu: "..state)
   local item = menuitems.simul.run
   local item2 = menuitems.simul.reset
   if state == "stop" then
      item.title = "&Stop"
      item.bmap.image = images.stop
      item.titleimage = useimg(images.stop)
      item2.active = yes
      item2.bmap.active = yes
   elseif state == "start" then
      item.title = "&Go"
      item.titleimage = useimg(images.run)
      item.bmap.image = images.run
      item2.active = no
      item2.bmap.active = no
   end
end

local npoll, npoll_last = 0, 0
local tlast
local function adjustDelta(ev)
   npoll = npoll + 1
   local tnow = os.time()
   local dt = os.difftime(tnow, tlast)
   if dt > 2 then
      if npoll > npoll_last then
	 ev.action.cycle = ev.action.cycle + 100
      else
	 ev.action.cycle = ev.action.cycle - 100
      end
      yats.deltaSlot = ev.action.cycle
      npoll_last = npoll
      npoll = 0
      tlast = tnow
   end
end

-----------------------------------------------------------------------------------
-----------------------------------------------------------------------------------
local function intercept(self, action, ev, when)
   adjustDelta(ev)
   iup.LoopStep()
end

local newslots, newdots

-----------------------------------------------------------------------------------
-- Initializes a simulation.
-- The function wraps the user's script into a Lua coroutine and assignes a 
-- user defined environment to the loaded script.
-- The simulation is halted for a few milliseconds in order to poll the GUI.
-- @param script Script text to be executed.
-- @param nogui Indicate whether this function is called via the GUI or not.
-- @return none.
-----------------------------------------------------------------------------------
function initSimulation(script, nogui)
   state = "IDLE"

   -- Load the script
   local func, err = assert(loadstring(script))
   
   -- Create an environment _G.user for the script
   if true then
      local env = {}
      _G.user = setmetatable(env, {__index = _G})
      setfenv(func, env)
      log:debug(string.format("environment created %q", tostring(env)))
   end

   -- Create a coroutine and an activation function
   -- for execution
   local worker = coroutine.wrap(
      function()
	 _G._SIMCO = coroutine.running()
	 yats.deltaSlot = 1000
	 runctrl = yats.luacontrol{
	    "runctrl",
	    actions = {
	       early = {},
	       late = {
		  {
		     yats.deltaSlot, 
		     intercept,
		     arg = nil, 
		     cycle=yats.deltaSlot
		  }
	       }
	    }
	 }
	 workerentry()
	 local a, b = func()
	 workerexit()
	 return a, b
      end)
   log:debug(string.format("worker created %q", tostring(worker)))


   -- Instantiate a luacontrol object to service the GUI
   newState("RUN")
   if not nogui then
      notebook:simtime(function() return yats.SimTimeReal, yats.SimTime end)
   end
   tstart = os.time()
   return worker
end

function continueSimulation(slots, dots)
   yats.sim:run(slots, dots)
end

-----------------------------------------------------------------------------------
-- Runs the simulation from a script text.
-- The controller maintains simulation states 'IDLE', 'RUN', 'PAUSE', 'FINISH',
-- 'ERROR', 'ABORTED'.
-- The script is run by calling the script running corouinte cyclically.
-- @param script Script to execute.
-- @return none.
-----------------------------------------------------------------------------------
function runSimulation(script)
   local success, retval
   
   local worker = initSimulation(script)
   cursor(busy)
   setSimControls(stop)
   resetSimulation()
   -- Start the script: we do that protected in order to
   -- catch errors.
   log:info("Simulation started.")
   while true do
      if state == "IDLE" then
	 -------------------------------------------------------------
	 -- IDLE state: 
	 -- poll GUI and wait for go
	 -------------------------------------------------------------
	 iup.LoopStep()
	 if command:get() == "cmd_go" then
	    notebook:simtime(function() return yats.SimTimeReal, yats.SimTime end)
	    newState("RUN")
	 end
      elseif state == "RUN" then
	 -------------------------------------------------------------
	 -- RUN state: 
	 -- Start script and let it poll the GUI via luacontrol.
	 -- Events are catched via getCommand() from luacontrol
	 -- or via worker return whoever catches the event first.
	 -- Accepted commands from GUI
	 --  'cmd_go'   ==> goto state 'pause'
	 -- Accepted erturn values from worker
	 --  'continue' ==> stay in 'run'
	 --  'finish'   ==> goto state 'finish'
	 --  'pause'    ==> goto state 'pause'
	 -- If the script ends the worker 
	 -------------------------------------------------------------
	 success, retval = pcall(worker)
	 if success == true then
	    if retval == "continue" then
	       -- continue after partial run
	       newState("RUN")
	    elseif retval == "done" then
	       -- continue after complete run
	       newState("RUN")
	    elseif retval == "pause" then
	       -- command: pause
	       setSimControls("start")
	       cursor(arrow)
	       newState("PAUSE")
	    elseif retval == "reset" then
	       -- command: reset
	       setSimControls("start")
	       cursor(arrow)
	       newState("RESET")
	    else
	       -- script ended finally: catch return value
	       yats.scriptReturnValue = retval
	       setSimControls("start")
	       cursor(arrow)
	       newState("FINISH")
	    end
	 else
	    if string.find(retval, "__ABORTED__") then
	       -- script aborted regular
	       newState("ABORT")
	    else
	       -- sript stopped with error
	       newState("ERROR")
	    end
	 end

      elseif state == "PAUSE" then
	 -------------------------------------------------------------
	 -- PAUSE state:
	 -- We stay here and poll the GUI until we receie a command
	 -- to continue. The we return sto state 'run'.
	 -------------------------------------------------------------
--	 setSimControls("start")
	 notebook:simtime(function() return yats.SimTimeReal, yats.SimTime end)
	 iup.LoopStep()
	 iup.Flush()
	 if command:get() == "cmd_go" then
	    cursor(busy)
	    setSimControls("stop")
	    newState("RUN")
	 end

      elseif state == "RESET" then
	 -------------------------------------------------------------
	 -- RESET state: 
	 -------------------------------------------------------------
	 local b
	 if string.upper(conf.yats.WarnRunningOnReset) == yes then
	    b = iup.Alarm("SIMULATION", "Simulation is running.\nReset now?", "Yes", "No")
	 else
	    b = 1
	 end
	 if b == 1 then
	    yats.sim:stop()
	    yats.sim:reset()
	    newState("IDLE")
	    break
	 else
	    newState("RUN")
	 end

      elseif state == "ABORT" then
	 -------------------------------------------------------------
	 -- ABORT state: 
	 -------------------------------------------------------------

      elseif state == "FINISH" then
	 -------------------------------------------------------------
	 -- FINISH state: 
	 -------------------------------------------------------------
	 iup.LoopStep()
	 cursor(arrow)
	 setSimControls(start)
	 newState("IDLE")
	 break

      elseif state == "ERROR" then
	 -------------------------------------------------------------
	 -- ERROR state: 
	 -------------------------------------------------------------
	 error(retval)
	 newState("IDLE")
      end
   end
   log:info("Simulation finished.")
end

-----------------------------------------------------------------------------------
-- Reset the simulator.
-- @param none.
-- @return none. 
-----------------------------------------------------------------------------------
function resetSimulation()
   yats.sim:reset()
end

-----------------------------------------------------------------------------------
-- Sort a list into a list of key,value pairs.
-- @param tin Input list.
-- @return key, value table.
-----------------------------------------------------------------------------------
local function sortlist(tin)
   local tout = {}
   local t = {}
   for k,v in pairs(tin) do
      table.insert(t, {key=k, val=v})
   end
   table.sort(t, function(u,v)
		    if type(u.val) == type(v.val) then
		       return _tostring(u.key) < _tostring(v.key)
		    else
		       return type(u.val) < type(v.val) 
		    end
		 end)
   return t
end

-----------------------------------------------------------------------------------
-- Add a list of items to the tree table.
-- @param tab Tree table.
-- @param list List of items.
-- @return Tree table.
-----------------------------------------------------------------------------------
local function addlist(tab, list)
   local items = sortlist(list)
   for _, item in ipairs(items) do
      local name, obj = item.key, item.val
      if type(item.val) == "userdata" or type(item.val) == "table" then
	 if item.key ~= ".self" then
	    table.insert(tab, {branchname = item.key, addexpanded = yes, userid = item.val})
	 end
      else
	 if type(item.val) == "string" then
	    table.insert(tab, {leafname = string.format("%s: %q", item.key, item.val), userid = list})
	 elseif type(item.val) == "function" then
	    
	 else
	    table.insert(tab, {leafname = string.format("%s: %s", item.key, tostring(item.val)), userid = list})
	 end
      end
   end
   return tab
end

-----------------------------------------------------------------------------------
-- Opens an object and create members tree.
-- @param tree Object tree handle.
-- @param id Id of selected node.
-- @param obj Handle of object associated with the node.
-- @return none.
-----------------------------------------------------------------------------------
local function objopen(tree, id, obj)
   local to = {}
   local _, t = toluaex.cmembers(obj)
   to = addlist(to, t) 
   if tolua.getpeer(obj) and true then
      local mt = getmetatable(obj)
      if mt.class then
	 to = addlist(to, {mt.class})
      else
	 to = addlist(to, tolua.getpeer(obj))
      end
   end
   iup.TreeSetValue(tree, to, id)
end


-----------------------------------------------------------------------------------
-- Get information from object tree node.
-- @param tree Tree handle.
-- @param id Node id.
-- @return Table with key, value info on success, nil + err on failure.
-----------------------------------------------------------------------------------
local function objgetval(tree, id)
   local t = {}
   t.uid = iup.TreeGetUserId(tree, id)
   t.fullkey = tree["title"..id]
   t.key = string.gsub(t.fullkey,"([^:]+):.*", "%1")
   t.id = id
   if t.key then
      t.val = t.uid[t.key]
      return t
   else
      return nil, "noval"
   end
end

-----------------------------------------------------------------------------------
-- Set new value in object tree node.
-- @param tree Tree handle.
-- @param id Node id.
-- @param t Info table.
-----------------------------------------------------------------------------------
local function objsetval(tree, id, t)
   local uid = iup.TreeGetUserId(tree, id)
   tree["title"..id] = string.format("%s: %s", t.key, tostring(t.val))
   uid[t.key] = t.val
end

-----------------------------------------------------------------------------------
-- Object browser branchopen callback.
-- Shows details of an object.
-- @param tree Tree handle.
-- @param id Node id.
-- @return none.
-----------------------------------------------------------------------------------
function objbranchopen_cb(tree, id) 
   local uid = iup.TreeGetUserId(tree, id)
   if tonumber(tree["childcount"..id]) == 0 then
      -- need to open this branch
      if type(uid) == "userdata" then
	 objopen(tree, id, uid)
      elseif type(uid) == "table" then
	 -- native members
	 local to = addlist({}, uid)
	 iup.TreeSetValue(tree, to, id)
      end
   end
end

-----------------------------------------------------------------------------------
-- Object browser branchclose callback.
-- Does nothing.
-- @param tree Tree handle.
-- @param id Node id.
-- @return none.
-----------------------------------------------------------------------------------
function objbranchclose_cb(tree, id)
end

-----------------------------------------------------------------------------------
-- Object browser branchopen callback.
-- Change values of object properties.
-- @param tree Tree handle.
-- @param id Node id.
-- @return none.
-----------------------------------------------------------------------------------
function objexecuteleaf_cb(tree, id)
   local t = objgetval(tree, id)
   local oldval = t.val
   local val = iup.Scanf("Modifier\n"..t.key.."%32.32%s\n", tostring(oldval))
   if val then
      if type(oldval) == "boolean" then
	 t.val = x == "true"
      elseif type(oldval) == "number" then
	 t.val = tonumber(val)
      end
      objsetval(tree, id, t)
   end
end


-----------------------------------------------------------------------------------
-- Object browser right click callback.
-- @param tree Tree handle.
-- @param id Node id.
-- @return none.
-----------------------------------------------------------------------------------
function objrightclick_cb(tree, id)
end

-----------------------------------------------------------------------------------
-- Object browser selection callback.
-- Extend the branch node and show it's first level children.
-- @param tree Tree handle.
-- @param id Node id.
-- @return none.
-----------------------------------------------------------------------------------
function objselection_cb(tree, id, status)
   if status == 1 then
      if tree["kind"..id] == "BRANCH" then
	 objbranchopen_cb(tree, id)
      end
   end
end


-----------------------------------------------------------------------------------
-- Create initial object and environment tree.
-- @return Table representing object tree.
-----------------------------------------------------------------------------------
function buildObjectTree()
   local to = {branchname = "Objects"}
   local te = {branchname = "Globals"}
   local t = {branchname = "Last Run", to, te}
   -- Objects
   if yats.sim.objectlist then
      to = addlist(to, yats.sim.objectlist)
   end
   -- Globals
   if _G.user then
      te = addlist(te, _G.user)
   end
   return t
end
return gui