---------------------------------------------------------------
-- @title LuaYats.
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: dummy.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Dummy node
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- A dummy node to test node objects written in Lua.
-- Use this as an example for Lua network objects.
---------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

luadummy = class(lua1out)

--- Constructor for the dummy object.
-- @param param table - parameter list.
-- @return table - reference to object instance.
function luadummy:init(param)
  local self = lua1out:new()
  self.name = autoname(param) 
  self.clname = "luadummy"
  self.evcounter = {0,0}
  self:adjust(param)
  -- Define inputs and outputs
  self:definp(self.clname)
  self:defout(param.out)

  -- We use the generic lua1out as underlying Yats object.
  -- print("LUADUMMY: assign C object")

  -- Set 2 different cyclic alarms.
  alarme(event:new(self, 1), 1000)
  alarme(event:new(self, 2), 1500)

  -- That's it.
  print(self)
  return self:finish()
end


local hit
--- The receive method of the object.
-- @param pd userdata - Received data.
-- @param idx number - Input handle as delivered during connect.
function luadummy:luarec(pd, idx)
  -- Find the suce
  local suc, shand = self:getNext()
  -- We need to cast the received data to the expected type.
  pd = tolua.cast(pd, "cell")

  -- Action: we just count the data
  self.counter = self.counter + 1
  if not hit then
    print("\nHey - I received data from a C object:")
    print("data type = "..pd.type)
    print("vci = "..pd.vci)
    print("pdu len = "..pd:pdu_len())
    print("counter = "..self.counter)
    print("suc.name = "..suc.name)
    print("suc.rec = "..tostring(suc.rec))
    hit = true 
  end
  -- Forward the event to our successor. Note that the following line
  -- directly calls the C class rec method of the successor.
  local rv = suc:rec(pd, shand)
  return rv
end

--- Early event.
-- @parmam ev userdata - Received event.
-- @return none.
function luadummy:luaearly(ev)
  -- We don't need to cast because there is no class hierarchy in events.
  -- Increment an event counter (per event).
  self.evcounter[ev.key] = self.evcounter[ev.key] + 1
  if ev.key == 1 then 
    -- Some control output.
--    print("event 1 "..ev.time)
    -- Restart the timer.
    alarme(ev, 1000)
  else
    -- Some control output.
--    print("    event 2 "..ev.time)
    -- Restart the timer.
    alarme(ev, 1500)
  end
end

--- Late event.
-- @param ev userdata - Received event.
-- @return none.
function luadummy:lualate(ev)
end

function luadummy:get_counter()
  return self.counter
end

function luadummy:get_evcounter()
  return self.evcounter
end

return yats