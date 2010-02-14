-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: core.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Yats programming in Lua.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- YATS (Yet Another Tiny Simulator) has been developed at University of Dresden.
-- This package provides Lua scripting capability to the Yats network simulator.<br><br>
-- <b>Classes</b><br><br>
-- <i>root</i><br>
-- Lua level root class. All node objects inherit from 'root'.
-- Note that there is no constructor, because an object of class 'root' is never instantiated
-- directly.<br><br>
-- <i>sim</i><br>
-- The simulator object. The 'sim' object is created automatically upon startup. It provides
-- methods to control the simulation execution.<br><br>
--<i>luacontrol</i><br>
-- Generic object to perform arbitrary tasks at specific times during a simulation.
-- See the constructor of object 'luacontrol' for details.
-----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------
-- Yats specialized version of the standard <code>package.seeall</code>.
-- Allows an module to see the global namespace.
-- @param module table - Module's table
-- @return none.
-----------------------------------------------------------------------------------
function yats.seeall(module)
  local t = type(module)
  assert (t == "table", "bad argument #1 to package.seeall (table expected, got "..t..")")
  local meta = getmetatable (module)
  local _index
  if not meta then
    meta = {}
    setmetatable (module, meta)
  else
    _index = meta.__index
  end
  meta.__index = function(t,k) 
--		    print("LOOKUP:", t, k, "I:", _index(t,k), "G:", rawget(_G, k))
		    return _index(t,k) or rawget(_G,k)
		 end
end

local cfg = require "yats.config"
require "yats.object"
local logging = require "yats.logging.console"
require "yats.logging.ring"
require "yats.stdlib"
require "readline"
require "yats.netlist"
require "bit"

module("yats", yats.seeall)

local conf = cfg.data

--==========================================================================
-- Logging Facility
--==========================================================================
log = logging[conf.yats.LogType]("SIM " .. conf.yats.LogPattern)
log:setLevel(conf.yats.LogLevel)
log:info(string.format("Logger SIM created: %s", os.date()))

clog = logging[conf.yats.LogType]("CPP " .. conf.yats.LogPattern)
clog:setLevel(conf.kernel.CLogLevel)
clog:info(string.format("Logger CPP created: %s", os.date()))

local klog = logging[conf.yats.LogType]("KNL ".. conf.yats.LogPattern)
klog:setLevel(conf.kernel.LogLevel)
klog:info(string.format("Logger KNL created: %s", os.date()))
kernel_log = klog
krnlog = klog
--==========================================================================
-- Utilities
--==========================================================================
-----------------------------------------------------------------------------------
--- Convert a string (embedded 0 allowed) into a printable representation.
-- @param str string - String to convert.
-- @param len number - Enforced length (optional).
-- @return String of form: dd-dd-dd-dd-...
-----------------------------------------------------------------------------------
function stringhex(str, len)
  local s = ""
  local n = len or string.len(str)
  for i = 1,n do
    s = s..string.format("%02X",string.byte(string.sub(str,i,i)))
--    if i < n then s = s .. "-" end
  end
  return s
end
local stringhex = stringhex

-----------------------------------------------------------------------------------
-- Convert a MAC address to a string.
-- @param s string - MAC address given a string (with embedded 0).
-- @return String of form: dd-dd-dd-dd-dd-dd
-----------------------------------------------------------------------------------
function mac2s(s) 
  local s = c2lua(s,6)
  return stringhex(s) 
end
local mac2s = mac2s

-----------------------------------------------------------------------------------
-- Callback or 'rec'-method.
-- This routine is called during simulation from the sending object. The data
-- is simply forwarded to the appropriate Lua object's 'rec'-method.
-- @param obj userdata Reference to a yats C node object.
-- @param pd userdata Reference to a yats C data object.
-- @param idx number Input handle.
-- @return none.
-----------------------------------------------------------------------------------
function _G.__YATSREC(obj, pd, idx)
  -- Note: The Lua objects are deleted earlier than the C-objects. Check this here.
--  log:debug(string.format("YATSREC: %s %s", obj.name, tolua.type(pd)))
  if obj then
    return obj:luarec(pd, idx)
  else
    return 0
  end
end

-----------------------------------------------------------------------------------
-- Callback for 'early'-method.
-- This routine is called during simulation from the sending object. The event
-- is simply forwarded to the appropriate Lua object's 'early'-method.
-- @param obj userdata Reference to a yats C node object.
-- @param ev userdata Reference to a yats C event object.
-- @return none.
-----------------------------------------------------------------------------------
function _G.__YATSEARLY(obj, ev)
--  log:debug(string.format("YATSEARLY: %s %s", obj.name, tolua.type(ev)))
  obj:luaearly(ev)
end

-----------------------------------------------------------------------------------
-- Callback for 'late'-method.
-- This routine is called during simulation from the sending object. The event
-- is simply forwarded to the appropriate Lua object's 'late'-method.
-- @param obj userdata Reference to a yats C node object.
-- @param ev userdata Reference to a yats C event object.
-- @return none.
-----------------------------------------------------------------------------------
function _G.__YATSLATE(obj, ev)
--  log:debug(string.format("YATSLATE: %s %s", obj.name, tolua.type(ev)))
  obj:lualate(ev)
end

--==========================================================================
-- Test environments
-- Currently not used
--==========================================================================

-- We keep environments for while for eventual inspection.
env = {
  env = {n=0}
}

function env:add(env)
  table.insert(self.env, env)
end

function env:getn()
  return self.env.n
end

function env:clear(i)
  if not i then
    self.env = {n=0}
  else
    table.remove(self.env, i)
  end
  collectgarbage("collect")
end

--==========================================================================
-- Yats internal garbage collection
--==========================================================================
garbage = {
  cur = {}, last = {},
  ncur = 0, nlast = 0,
  who = "last",
  swap = function(self)
	   self.last = self.cur
	   self.nlast = self.ncur
	   self.cur = {}
	   self.ncur = 0
	   if self.who == "current" then
	     self.who = "last"
	   else
	     self.who = "current"
	   end
	 end,
  put = function(self, obj)
	  log:debug(string.format("Object '%s' => garbage '%s'", 
				  obj.name, self.who))
	  self.cur[obj.name] = obj
	  self.ncur = self.ncur + 1
	end
}

------------------------------------------------------------------------------
-- Push objects to yats garbage.
-- We keep the last object list for postmortem debugging.
-- @return none.
------------------------------------------------------------------------------
local function deletegarbage()
   log:debug(string.format("Collect garbage '%s': %d objects", garbage.who,
			   garbage.nlast))
   for k, v in pairs(garbage.last) do
      log:debug(string.format("Delete object '%s' of type '%s'.", v.name, tolua.type(v)))
      -- Displays have a destroy method
      if v.destroy then 
	 v:destroy() 
      else
	 -- Give ownership to tolua for garbage collection
	 tolua.takeownership(v)
      end
   end
   garbage:swap()
   collectgarbage("collect")
end

sim.objectlist = {}

------------------------------------------------------------------------------
-- Connects all nodes.
-- @return none.
------------------------------------------------------------------------------
function sim:connect()
  local objn
  local obj
  if self.connected then return end
  for objn, obj in pairs(self.objectlist) do
     log:debug(string.format("Connect object '%s' %s", obj.name, type(obj.connect)))
     obj:connect()
  end
  self.connected = true
end


sim.ResetTime = sim.ResetTime_
--
-- Reset simulation time.
-- @return none.
--
local function resTime()
  sim.ResetTime_()
end
restime = resTime

------------------------------------------------------------------------------
-- Reset simulation time.
-- All living objects are informed about time reset.
-- @return none. 
------------------------------------------------------------------------------
function sim:resetTime()
  -- inform all objects that time is reset
  for objn, obj in pairs(self.objectlist) do
    obj:restim()
  end
  _sim:ResetTime_()
end
sim.ResetTime = sim.resetTime

sim._SetSlotLength = sim.SetSlotLength

function sim:setSlotLength(sl)
  _sim:_SetSlotLength(sl)
end
sim.SetSlotLength = sim.setSlotLength

------------------------------------------------------------------------------
-- Set simulation slot length.
-- @param sl number - slot length in seconds.
-- @return none.
------------------------------------------------------------------------------
function setSlotLength(sl)
  sim:setSlotLength(sl)
end

------------------------------------------------------------------------------
-- Calculate ticks from given time.
-- @param t number - time
-- @return slot length in ticks.
------------------------------------------------------------------------------
function time2slot(t)
  return t / SlotLength
end

sim._SetRand = sim.SetRand

------------------------------------------------------------------------------
-- Set the seed value for yats random number generator.
-- @param n number - seed value.
------------------------------------------------------------------------------
function sim:setRand(n) 
  _sim:_SetRand(n) 
end
sim.SetRand = sim.setRand
sim.setrand = sim.SetRand

sim._GetRand = sim.GetRand
------------------------------------------------------------------------------
-- Generate a random number
-- @return random number
------------------------------------------------------------------------------
function sim:getRand() 
  return _sim:_GetRand() 
end
sim.GetRand = sim.getRand

------------------------------------------------------------------------------
-- Set yats seed value for random number generation.
-- Same as @see sim:setRand
-- @param n number - some number to init the generator.
-- @return none.
------------------------------------------------------------------------------
function randomseed(n)
  return _sim:_SetRand(n)
end

------------------------------------------------------------------------------
-- Generate a random number withing given limits.
-- @param l number - lower limit
-- @param u number - upper limit
-- @return random number
------------------------------------------------------------------------------
function random(l, u)
  local r = _sim:getRand() / 32768
  if l then
    if u then
      return math.floor(r*(u-l+1))+1
    else
      return math.floor((r*l)+l)
    end
  else
    return r
  end
end

------------------------------------------------------------------------------
-- Insert an element into object list.
-- This method should not be used in user scripts
-- @param obj table - Lua object reference.
-- @return none.
------------------------------------------------------------------------------
function sim:insert(obj)
--  print(string.format("INSERT: %s %s", obj.name, tostring(obj)))
  assert(self.objectlist[obj.name] ~= obj, 
	 string.format("Object '%s' of type '%s' is already present.",
		       obj.name, tolua.type(obj)))
  assert(not self.objectlist[obj.name], 
	 string.format("Object '%s' already defined.", obj.name))
  self.objectlist[obj.name] = obj
end

------------------------------------------------------------------------------
-- Delete an element from object list.
-- This method should not be used in user scripts
-- @param obj string - Lua object name.
-- @return none.
------------------------------------------------------------------------------
function sim:delete(obj)
  local nam
  if type(obj) == "string" then
    -- name given
    nam = obj
  elseif type(obj) == "table" then
    -- lua reference given
    nam = obj.name
  elseif type(obj) == "userdata" then
    -- yats reference given
    nam = obj.name
  end
  self.objectlist[nam] = nil
end

------------------------------------------------------------------------------
-- Retrieve an element from object list.
-- @param name string - Name of the object.
-- @return table - Reference to the object.
------------------------------------------------------------------------------
function sim:getObj(name)
  return self.objectlist[name]
end
sim.getobj = sim.getObj

------------------------------------------------------------------------------
-- List objects.
-- @param s string - If not nil the list is printed to STDOUT, where each line 
--                   is preprended by 'prefix'. If nil only a string containing 
--                   the list is returned.
-- @param prefix string - See above.
-- @return string - List of objects as table.
------------------------------------------------------------------------------
function sim:list(s, prefix)
  local i,v 
  local t = {}
  if s then print(s) end
  for i,v in ipairs(self.objectlist) do
    if i ~= "n" then
      if s then
	print(prefix..self:getobj(i).name)
      else
	table.insert(t, self:getobj(i).name)
      end
    end
  end
  return t
end
sim.listObjects = sim.list

sim._run = sim.run
------------------------------------------------------------------------------
-- Run simulation for 'slots' ticks.
-- @param slots number - Number of slots in ticks.
-- @param dots number - Display a dot every 'dots' ticks.
-- @return none.
------------------------------------------------------------------------------
function sim:run(slots, dots)
   self:deletegarbage()
   local curslot = 0
   local slots = slots or 0
   local dots = dots or slots * 2
   local delta = yats.deltaSlot
   yats._slots = slots
   yats._dots = dots
   yats.log:debug(string.format("sim.run: slots=%d dots=%d delta=%d", slots, dots, delta))
   if not self.connected then
      self:connect()
   end
   while curslot < slots do
      local delta = yats.deltaSlot
      _sim:_run(delta, dots)
      local cmd = gui.command:get()
      if cmd == "cmd_go" then
	 yats.log:debug("sim.run: paused")
	 coroutine.yield("pause")
      elseif cmd == "cmd_reset" then
	 yats.log:debug("sim.run: reset")
	 coroutine.yield("reset")
      else
	 -- no log here, because we would flush the log otherwise
	 coroutine.yield("continue")
      end
      curslot = curslot + delta
   end
   return "continue"
   -- Continue script
end

sim.Run = sim.run

------------------------------------------------------------------------------
-- Clean pending events in early and late timeslot tables.
-- @param del boolean - Delete the event items, if del is true.
-- @return none.
------------------------------------------------------------------------------
function sim:flushEvents(del)
  log:debug("Flushing events.")
  if del == true then
    local c = flushevents(1)
--    print("### flush ", del, c)
  else
    local c = flushevents(0)
--    print("### flush ", del, c)
  end
end

------------------------------------------------------------------------------
-- Disconnect everything.
-- We don't really disconnect stuff, because we will need to delete the objects anyway and
-- re-create them. The function changes only simulator's state.
-- @return none.
------------------------------------------------------------------------------
function sim:disconnect()
  self.connected = false
end

sim._stop = sim.stop
------------------------------------------------------------------------------
-- Stop Simulation (break running simulation loop (started by sim:run()).
-- @return none.
------------------------------------------------------------------------------
function sim:stop()
  stopflag = true
  _sim:_stop()
end

------------------------------------------------------------------------------
-- Reset Simulation (do not execute any further run (started by sim:run()).
-- The event queue is flushed by default.
-- @param no_flush boolean - Do not flush the event queue.
-- @return none.
------------------------------------------------------------------------------
function sim:reset(no_flush)
   log:debug("Reset Simulation")
   sim:resetTime()
   self:disconnect()
   if not no_flush or no_flush == false then
      self:flushEvents(true)
   end
   self:pushgarbage()
end

------------------------------------------------------------------------------
-- Collect all active objects into current garbage table.
-- Object are not deleted directly. They are put into a garbage table instead.
-- This allows the observation of object internals after a simulation run has
-- finished.
-- Static objects remain in the objectlist and can be re-used.
-- This method should normally not be used in user scripts.
-- @return none.
------------------------------------------------------------------------------
function sim:pushgarbage()
   local t = {}
   log:debug("Push objects into garbage")
   for k, v in pairs(self.objectlist) do
      if not v.static then
	 garbage:put(v)
      else
	 table.insert(t, v)
      end
      if v.hide then
	 v:hide()
      end
   end
   for k, v in pairs(t) do
      log:debug(string.format("Keeping object %s", v.name or "???"))
      self:insert(v)
   end
   self.objectlist = {}
   log:debug(string.format("%d objects moved to garbage %q", garbage.ncur, garbage.who))
end

------------------------------------------------------------------------------
--- Delete objects in last garbage table.
-- @return none.
------------------------------------------------------------------------------
function sim:deletegarbage()
   log:debug(string.format("Delete garbage 'last': %d objects", garbage.nlast))
   for k, v in pairs(garbage.last) do
      log:debug(string.format("Delete object '%s' of type '%s'.", v.name, tolua.type(v)))
      -- Delete the object.
      if v.destroy then 
	 v:destroy() 
      end
      -- Give ownership to tolua for garbage collection
      tolua.takeownership(v)
   end
   -- Put current garbage into last garbage table for later deletion.
   -- Provide a new current garbage table.
   garbage:swap()
end

local name_index = 0
------------------------------------------------------------------------------
-- Automatically generate a name or take it from 'param'.
-- 'autoname' is also provided in the global environment for convenience.
-- @param param table - Parameter table given to object constructor.
-- @return Generated name as string.
------------------------------------------------------------------------------
function autoname(param)
  param = param or {}
  name_index = name_index + 1
  local name = param.name or param.NAME or param[1] or "autoobj"..name_index
  return name
end

_G.autoname = autoname

local nc_index = 0
------------------------------------------------------------------------------
-- Not connected pseudo object.
-- This function implicitly instantiates a sink object with name 'nc<index>'.
-- The index is incremented automatically.
-- The function is given for convenience reasons in user scripts.
-- @return Table containing the string 'nc<index>'
------------------------------------------------------------------------------
function nc()
  nc_index = nc_index + 1
  local sink = sink{"nc"..nc_index}
  return {"nc"..nc_index}
end
_G.nc = nc

--==========================================================================
-- Root Class: This the mother of all network objects in the Lua layer.
--==========================================================================
------------------------------------------------------------------------------
-- Finish object definition.
-- Call this routine to finish object definition.
-- It performs common stuff, that is  necessary in network object definition.
-- @param t userdata - Reference to a 'yats' object.
-- @return luaobj, yatsobj, name: References to Lua object, yats object and 
--                                the object's name.
------------------------------------------------------------------------------
function root:finish()
  assert(self.clname, "Property 'clname' of '"..self.name.."' not set.")
  -- Sanity checks before finishing.
  assert((self.outputs == nil) or 
	 (self.noutp == nil) or 
	   (table.getn(self.outputs) == self.noutp), 
	 "Property 'noutp=" .. (self.noutp or 'nil') .. "' of '"..self.name ..
	   "' does not match number of elements in output table: "..
	   (table.getn(self.outputs or {}) or 'nil')..".")
  -- Mark static events with a magic number to ensure that they are
  -- not deleted during simulation reset.
  if self.std_evt then
    self.std_evt.stat = 12345678
    self.dyn = 0
  end

  -- Keep a reference to name strings, because they are otherwise garbage collected.
  if type(self) == "userdata" then
    self._name = self.name
    self._title = self.title
  end

  -- Act on parameters in yats object
  if self.act then self:act() else print("act not found for "..self.name) end
    
  -- Insert lua object into global object list (contained in sim)
  sim:insert(self)

  return self, self.name
end

------------------------------------------------------------------------------
-- Generic connect function for each network object.
-- This function is automatically called by <code>sim:connect()</code>.
-- @return none.
------------------------------------------------------------------------------
function root:connect()
  log:debug("Connecting '"..self.name.."'")
  -- Set number of outputs if necessary
  local peer, shand, objn, out
  local outs = self.outputs
  if not outs then 
    log:debug(string.format("'%s' no output", self.name))
    return 
  end
  local n = table.getn(outs)
  -- run over all elements in output list
  for i = 1,n do
    out = outs[i]
    objn = out[1]
    pin = out[2]
    peer = sim:getobj(objn)
    -- Late binding of input name: we take the object's class name.
    if not pin then
      if not peer then debug.debug() end
      assert(peer, string.format("Invalid 'nil' peer '%s' of '%s'\n", objn, self.name))
      log:info(string.format("auto-assign pin '%s' of object '%s'", 
			     peer.clname, peer.name))
      pin = peer.clname
    end
    assert(peer, "peer instance '"..objn.."' not found.")
    shand, peer = peer:handle(self, pin)
    assert(shand, "pin '"..pin.."' in peer instance '"..peer.name.."' not found.")
    if self.set_output then
      log:debug(string.format("set-con: '%s' out-%d (%d pins) to '%s.%s' with handle [%d] ... ", 
			      self.name, i, n, peer.name, pin, shand))
      self:set_output(peer, shand)
    elseif self.add_output then 
      log:debug(string.format("add-con: '%s' out-%d (%d pins) to '%s.%s' with handle [%d] ... ", 
			      self.name, i, n, peer.name, pin, shand))
      self:add_output(i, peer, shand)
    end
  end
end

------------------------------------------------------------------------------
-- Generic handle function.
-- It provides the answer to an output's connect input. The returned handle is
-- used when the predecessor's calls the object's <code>rec</code> function.
-- @param peer string - pin name of input as known by peer output.
-- @return number - handle of input.
------------------------------------------------------------------------------
function root:handle(peer, pin)
  local i, mypin
  for i = 1, table.getn(self.inputs) do
    mypin = self.inputs[i]
    if pin == mypin then
      return i-1, self
    end
  end
  return nil
end

------------------------------------------------------------------------------
-- Define a input for an object.
-- Produces a runtime error if the input is already defined.
-- @param name string - Name of the input pin.
-- @return none.
------------------------------------------------------------------------------
function root:definp(name)
  name = name or self.clname
  self.inputs = self.inputs or {n=0}
  for i,v in pairs(self.inputs) do
    assert(v ~= name, string.format("'%s': input '%s' already defined.", self.name, name))
  end
  table.insert(self.inputs, name)
end

------------------------------------------------------------------------------
-- Define a output for an object.
-- Note: If ot is a function, defout executes this function, which then
-- must return a valid output table format.
-- @param ot table or string or function - output list or single output.
-- @return none.
------------------------------------------------------------------------------
function root:defout(ot)
  self.outputs = self.outputs or {}
  if type(ot) == "function" then
    ot = ot(self)
  end
  if type(ot[1]) == "string" then
    table.insert(self.outputs, ot)
  elseif type(ot[1]) == "table" then
    self.outputs = ot
  end
end

------------------------------------------------------------------------------
-- Signal to an object.<br>
--  The arguments are specific to the class that is addressed.
-- <i>Demux</i>: Routing table as a list of n-tuples. With each call multiple entries can be defined.
-- It is possible to call this method multiple times.<br>
-- Each n-tuple is either a list of numbers or a structure with named members:<br>
-- from = VCI-in, to = VCI-out, out = output index (port).
-- <code>demux:signal{{from1, to1, out1},{from2, to2, out2}, ...}</code>.<br>
-- <code>demux:signal{{from = N, to = N, out = N}, {from2, to2, out2}}</code>.<br>
-- @param ... arglist - List of n-tuples with n = 3.<br>
-- @return none.
------------------------------------------------------------------------------
function root:signal(...)
  assert(self.clname, "class cannot receive signals.")
  if self.clname == "demux" then
     log:debug(string.format("Signalling to '%s': %s", 
			     self.name, tostring(arg)))
    for i = 1, #arg do
      local msg = WriteVciTabMsg:new_local()
      msg.old_vci = arg[i].from or arg[i][1]
      msg.new_vci = arg[i].to or arg[i][2]
      msg.outp = arg[i].out or arg[i][3]
      self:special(msg, nil)
    end
  end
end

------------------------------------------------------------------------------
-- Adjust object's parameter table for easier handling.
-- All keys are transformed to lower case words.
-- @param p table - parameter table.
-- @return p table - adjusted parameter table
------------------------------------------------------------------------------
function root:adjust(p)
  local i,v
  for i,v in pairs(p) do
    local index = string.lower(i)
    assert((not self.parameters) or index == "1"  or (self.parameters[index] ~= nil), 
	   "Unknown parameter '"..tostring2(i).."' for class '"..self.clname.."'.")
    p[index] = p[i]
  end
  -- check for mandatory parameters: not really needed because this should happen in 
  -- constructor.
  return p
end

------------------------------------------------------------------------------
-- Get successor reference for a specific output.
-- @param index number - Output index starting from 0. This parameter is optional.
-- @return Reference to successor C object.
------------------------------------------------------------------------------
function root:getSuc(index)
  if not self.get_suc then
    return nil, "no successor"
  else
    if self.nout then 
      return self:get_suc(index)
    else
      return self:get_suc()
    end
  end
end

------------------------------------------------------------------------------
-- Get successor input handle for a specific output.
-- @param index number - Output index starting from 0. This parameter is optional.
-- @return Reference to successor input handle.
------------------------------------------------------------------------------
function root:getSHandle(index)
  if not self.get_shand then
    return nil, "no successor"
  else
    if index then
      return self:get_shand(index)
    else
      return self:get_shand()
    end
  end
end

------------------------------------------------------------------------------
-- Get complete successor info (reference + handle) for a specfiic output.
-- @param index number - Output index starting from 0. This parameter is optional.
-- @return suc, shand - Reference to successor and the handle.
------------------------------------------------------------------------------
function root:getNext(index)
  if index then
    if index <= self.nout then
      return self:getSuc(index), self:getSHandle(index)
    else
      return nil, nil, "No successor at this output."
    end
  else
    return self:getSuc(), self:getSHandle()
  end
end

------------------------------------------------------------------------------
-- Iterator delivering successor objects of given object.
-- @param obj - This object.
-- @return func, table, nil Iterator list.
------------------------------------------------------------------------------
function root.successors(obj)
   local t = {}
   if obj.nout then
      for i = 1, obj.nout do
	 table.insert(t, i, {suc = obj:getSuc(i), shand = obj:getSHandle(i)})
      end
      return next, t, nil
   else
      local suc = obj:getSuc()
      if suc then
	 table.insert(t, 1, {suc = obj:getSuc(), shand = obj:getSHandle()})
      end
      return next, t, nil
   end
end

function root:nocollect(flag)
  return self
end
function root:__nocollect(flag)
  flag = flag or false
  if flag == true then
    self.nodelete = true
  end
  return self
end

--==========================================================================
-- Class 'luacontrol.
--==========================================================================

--- Definition of class 'luacontrol'.
luacontrol = class(lua1out)

--- Constructor of class 'luacontrol'.<br>
-- 'luacontrol' provides a means to influence yats behavior during simulation.
-- This achieved by instantiating an instance of 'luacontrol' in the simulation
-- configuration file. An 'action' file given as parameter during object 
-- instantiation defines actions that are executed at specific time slots.
-- There are action for the early phase and actions for the late phase of
-- a simulation time slot.<br><br>
-- 
-- Format of an <i>action file</i>:
-- <pre>
-- actions = {early = early_action_table, late = late_action_table}.<br>
-- <br>
-- <code>early</code> and <code>late]</code> are <i>action tables</i> defining the actions,
-- which are executed during early and late timeslot.<br>
-- <br>
-- An action table defines a timeslot and an action to perform:<br>
-- <code>action_table = {slot, action, [arg=]{arg1, arg2, ..}, cycle=10000 [, ncycle=10]}.</code><br>
-- <br>
-- 'slot' defines the absoulte simulation time slot at which the action shall occur.
-- </pre>
-- An action is one of the following:<br>
-- <ul>
--   <li>A string containing Lua code. The string is simply executed.
--   <li>A function which is executed. Return values are not evaluated.<br>
--         <code>action[3]</code> or <code>arg</code> optionally define arguments for the function.
--   <li> A table describing a sequence of actions.
-- </ul>
-- The action can be executed periodically by declaring a cycle, which defines
-- the period in timeslots. If 'ncycle' is given, the periodic action is performed
-- ncycle times.
-- @param param table - Parameter table.
-- @return table - Reference to object instance.
function luacontrol:init(param)
  self = lua1out:new()
  self.name = autoname(param)
  self.clname = "luacontrol"
  self.parameters = {
    out = false,
    file = false,
    actions = false,
    static = false
  }
  self._early_index = 1
  self._late_index = 1
  self:adjust(param)
  if param.file then
    dofile(param.file)
  end
  self.static = param.static
  self.actions = actions or param.actions
  self:definp(self.clname)
  self:defout({})
  self.events = {eev={}, lev={}}
  local last, delta
  for i,action in ipairs(self.actions.early) do
    local ev = event:new(self, i)
    table.insert(self.events.eev, ev)
    ev.action = action
    if action.cycle then
      action.count = action.ncycle
    end
    alarme(ev, action[1])
  end
  
  for i,action in ipairs(self.actions.late) do
    local ev = event:new(self, i)
    table.insert(self.events.lev, ev)
    ev.action = action
    if action.cycle then
      action.count = action.ncycle
    end
    alarml(ev, action[1])
  end
  return self:finish()
end

function luacontrol:doaction(action, ev, when)
   local slot = action[1]
   local cmd = action[2]
   if type(cmd) == "string" then
      assert(loadstring(cmd))()
   elseif type(cmd) == "function" then
      cmd(unpack(action.arg or action[3] or {self, action, ev, when}))
   elseif type(cmd) == "table" then
      local i,v
      for i,v in ipairs(cmd) do
	 self:doaction(v, ev, when)
      end
   end
end

function luacontrol:lualate(ev)
  local action = ev.action
  if action.cycle and (not action.count or action.count > 0) then
    if action.count then
      action.count = action.count - 1
    end
    alarml(ev, action.cycle)
  end
  self:doaction(action, ev, "late")
end

function luacontrol:luaearly(ev)
  local action = ev.action
  if action.cycle and (not action.count or action.count > 0) then
    if action.count then
      action.count = action.count - 1
    end
    alarme(ev, action.cycle)
  end
  self:doaction(action, ev, "early")
end

local prompt = "luayats > "
-- Yats command line interpreter.
-- Runs a subshell. On the command line prompt you can enter any valid
-- Lua code. To enable cli in luacontrol actions simply enter cli as an action.
-- @param prompt string - Command line prompt.
-- @return none.
function cli(...)
  printf("\nWelcome! Simulation time is %d ticks. Hit <ctrl-d> or type 'exit' to quit.\n", SimTime)
  s = readline.readline(prompt)
  while s do
    if s == "exit" then break end
    s = string.gsub(s, "^(=)(.+)","print(%2)")
    local f, err = loadstring(s)
    if f then
      f()
      readline.add_history(s)
    else
      io.stderr:write(err.."\n")
      readline.add_history(s)
    end
    s = readline.readline(prompt)
  end
  printf("\n")
end

--=-----------------------------------------------------------------------------
-- PDU decoding
--=-----------------------------------------------------------------------------
local byte = string.byte
local sub = string.sub
local function sbyte(str, i)
  return byte(sub(str, i, i))
end
local function sword(str, i)
  return sbyte(str, i)*256 + sbyte(str, i+1)
end
local function slong(str, i)
  return sword(str,i)*65536 + sword(str,i+2)
end

--=-----------------------------------------------------------------------------
-- Rapid Spanning Tree BPDU
--=-----------------------------------------------------------------------------
local fmt = function(s, fmt, ...) return  s .. "  " .. string.format(fmt .."\n", unpack(arg)) end
local portroles = {
  "unknown", "alternate/backup", "root", "designated"
}
local function sprint_bpdu(t)
--  s = fmt("", "RAW: %s", t.raw)
  local s = ""
  s = fmt(s, "HEADER:")
  s = fmt(s, "  dest-mac: %s", t.mac.dst_mac)
  s = fmt(s, "  src-mac : %s", t.mac.src_mac)
  s = fmt(s, "  length  : %d", t.eth.len8023)
  s = fmt(s, "  dsap    : 0x%02X", t.eth.dsap)
  s = fmt(s, "  ssap    : 0x%02X", t.eth.ssap)
  s = fmt(s, "  llc     : 0x%02X", t.eth.llc)
  s = fmt(s, "BPDU:")
  s = fmt(s, "  protocol id: 0x%02X", t.hdr.protocol_id)
  s = fmt(s, "  version    : 0x%02X", t.hdr.version_id)
  s = fmt(s, "  type       : %s", t.bpdu_type)
  s = fmt(s, "  FLAGS:")
  local b = t.body
  s = fmt(s, "   topo_change: %d", b.flags.topo_change)
  s = fmt(s, "   proposal   : %d", b.flags.proposal)
  s = fmt(s, "   portrole   : %s", portroles[b.flags.portrole+1])
  s = fmt(s, "   learning   : %d", b.flags.learning)
  s = fmt(s, "   forwarding : %d", b.flags.forwarding)
  s = fmt(s, "   agreement  : %d", b.flags.agreement)
  s = fmt(s, "   tcack      : %d", b.flags.tcack)
  if string.find(t.bpdu_type, "RSTP") then
    s = fmt(s, "  root id      : %04x-%s", b.root_id.prio, b.root_id.addr)
    s = fmt(s, "  path cost    : %d", b.path_cost)
    s = fmt(s, "  port id      : 0x%04X", b.port_id)
    s = fmt(s, "  message age  : %d", b.message_age/256)
    s = fmt(s, "  max age      : %d", b.max_age/256)
    s = fmt(s, "  forward delay: %d", b.forward_delay/256)
  elseif string.find(t.bpdu_type, "TOPOLOGY-CHANGE") then
    -- Topology change: nothing to print
  else
    -- shouldn't reach here
    log:fatal("Shouldn't have reached to this point")
  end
  return s
end

local function _sprint_bpdu(t)
  return pretty(t, true)
end

local function decode_bpdu(str)
  local p = 0
  local t = {
    raw = "",
    mac = {},
    eth = {},
    hdr = {},
    body = {}
  }
  t.raw = stringhex(str)
  -- MAC header
  t.mac.dst_mac = mac2s(sub(str, 1, 6))
  t.mac.src_mac = mac2s(sub(str, 7, 12))
  p = p + 12
  -- LLC header
  t.eth.len8023 = sword(str, p+1)
  t.eth.dsap = sbyte(str,p+3)
  t.eth.ssap = sbyte(str,p+4)
  t.eth.llc = sbyte(str, p+5)
  p = p + 5
  -- BPDU
  t.hdr.protocol_id = sword(str, p+1)
  t.hdr.version_id = sbyte(str,p+3)
  if sbyte(str, p+4) == 0 then
    -- Config BPDU
    t.bpdu_type = "CONFIG (0x00)"
  elseif sbyte(str, p+4) == 2 then
    -- RSTP BPDU
    t.bpdu_type = "RSTP (0x02)"
    local flags = sbyte(str, p+5)
    t.body.flags = {}
    t.body.flags.topo_change = bit.band(flags, 1)
    t.body.flags.proposal = bit.band(bit.rshift(flags,1), 1)
    t.body.flags.portrole = bit.band(bit.rshift(flags, 2), 3)
    t.body.flags.learning = bit.band(bit.rshift(flags, 4), 1)
    t.body.flags.forwarding = bit.band(bit.rshift(flags, 5), 1)
    t.body.flags.agreement = bit.band(bit.rshift(flags, 6), 1)
    t.body.flags.tcack = bit.band(bit.rshift(flags, 7), 1)
  elseif sbyte(str, p+4) == 128 then
    -- Topology Change BPDU
    t.bpdu_type = "TOPOLOGLY-CHANGE (0x80)"
  end

  if string.find(t.bpdu_type, "RSTP") then
    -- Configuration
    t.body.root_id = {
      prio = sword(str, p+6),
      addr = mac2s(sub(str, p+8, p+13))
    }
    t.body.path_cost = slong(str, p+14)
    t.body.bridge_id = {
      prio = sword(str, p+18),
      addr = mac2s(sub(str, p+20, p+25))
    }
    t.body.port_id = sword(str, p+26)
    t.body.message_age = sword(str, p+28)
    t.body.max_age = sword(str, p+30)
    t.body.hello_time = sword(str, p+32)
    t.body.forward_delay = sword(str, p+34)
  elseif string.find(t.bpdu_type, "TOPOLOGY-CHANGE") then
    -- Topology change: nothing more to decode
  else
    assert(false, "decode_bpdu(): unkown BPDU type")
  end
  setmetatable(t, {__tostring = sprint_bpdu})
  return t
end

--- Decode a frame given in a string.
-- @param frame string The frame to decode.
-- @param typ string Command string which denotes the type of the frame.
-- @return Table representation of the frame.
function decode(frame, typ)
  if typ == "bpdu" then
    return decode_bpdu(frame)
  else
    return nil, "Unknown frame type."
  end
end

return yats