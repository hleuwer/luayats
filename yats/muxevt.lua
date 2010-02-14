-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: muxevt.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description LuaYats - Event-driven Multiplexer and Demultiplexer.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- Lua classes for event driven multiplexers and demultiplexers.
-----------------------------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

--==========================================================================
-- MuxBase Object 
--==========================================================================

local function mux_init(self, param)

  -- Parameter name adjustment
  self:adjust(param)
  
  -- Parameter check
  assert(param.ninp and param.ninp > 0, "muxBase: invalid number of inputs: "..param.ninp)
  self.ninp = param.ninp
  self.max_vci = param.maxvci or param.ninp + 1
  self.max_inprio = param.maxinprio or 8
  assert(self.max_vci > 0, "muxBase: invalid maxvci: "..(param.maxvci or "nil"))
  self.q:setmax((param.buff or 0) + 1)
  if param.service then
     assert(param.service > 0, "muxBase: invalid service time: "..(param.service or "nil"))
  end
  self.serviceTime = param.service or 1
  self.needToSchedule = 1

  -- Init output table
  self:defout(param.out)
  
  -- Init input table
  for i = 1, self.ninp do
    self:definp("in"..i)
  end
end

_muxBase = muxBase
--- Definition of multiplexer class 'muxBase'.
muxBase = class(_muxBase)

--- Constructor for class 'muxBase'.
-- The multiplexer provides 'ninp' inputs with a single output queue with a given
-- maximum length 'buff'. Incoming cells are enqueued by fair strategy (random choice).
-- The class is only used as a base class for other event driven multiplexers. It
-- does not have a <code>late()</code> method for scheduling.<br>
-- The multiplexer maintains loss counters<br>
-- a) for the queue in total and<br>
-- b) on a per VCI level.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>ninp)<br>
--    Number of inputs. 
-- <li>buff)<br>
--    Size of the output buffer. 
-- <li>maxvci (optional))<br>
--    Maximum value for input VCI. Default: ninp+1. 
-- <li>service (optional))<br>
--    Service time in slots. 
-- <li>out)<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table - reference to object instance.
function muxBase:init(param)
  self = _muxBase:new()
  self.name=autoname(param)
  self.clname = "muxBase"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false,
    service = false
  }
  -- Parameter initialisation
  mux_init(self, param)
  
  -- Finish construction
  return self:finish()
end

muxBase.getLossInp = muxBase.getLoss

--- Get summary of losses for a ranage of inputs.
-- @param from number Lower input index.
-- @param to number Upper input index.
-- @return number Loss count.
function muxBase:getLosses(from, to)
  local i, n = 0, 0
  for i = from,to do n = n + self:getLoss(i) end
  return n
end
muxBase.getLossesInp = muxBase.getLosses

--- Get summary of losses for a ranage of VCIs.
-- @param from number -  Lower VCI.
-- @param to number - Upper VCI.
-- @return number - Loss count.
function muxBase:getLossesVCI(from, to)
  local i, n = 0, 0
  for i = from,to do n = n + self:getLossVCI(i) end
  return n
end

--- Get output queue length.
-- @return number - Length of the queue.
function muxBase:getQLen()
  return self.q.q_len
end

--- Get maximum length of queue.
-- @return number - Maximum length of the queue.
function muxBase:getQMaxLen()
  return self.q:getmax()
end


--==========================================================================
-- MuxPrio Object 
--==========================================================================

_muxPrio = muxPrio
--- Definition of multiplexer class 'muxPrio'.
muxPrio = class(_muxPrio, muxBase)

--- Constructor for class 'muxPrio'.
-- The multiplexer provides <code>ninp</code> inputs adn <code>nprio</code> queues. Each connection is assigned to 
-- a specific priority in the range 0 to <code>nprio-1</code>, where 0 is the lowest priority.
-- Incoming cells are enqueued by fair strategy (random choice). The service time is configurable.
-- The multiplexer operates either in work-conserving or non work-conserving manner. 
-- In work-conserving mode "async"  a cell that hits an empty queue is transmitted immediately.
-- In non work-conserving mode "sync" the server process is activated strictly synchron in 
-- fixed time intervals.<br>
-- The multiplexer maintains loss counters<br>
-- a) for the queue in total and<br>
-- b) on a per VCI level.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional))<br>
--    Name of the display. Default: "objNN". 
-- <li>ninp)<br>
--    Number of inputs. 
-- <li>buff)<br>
--    Size of the output buffer. 
-- <li>maxvci (optional))<br>
--    Maximum value for input VCI. Default: ninp+1. 
-- <li>nprio)<br>
--    Maximum number of priorities. 
-- <li>service (optional))<br>
--    Service time in slots. 
-- <li>mode)<br>
--    "async": work-conserving operation with the rate defined by 'service'. <br>
--    "sync" : non work-conserving operation with the rate defined by 'service'. <br>
-- <li>out)<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table - reference to object instance.
function muxPrio:init(param)
  self = _muxPrio:new()
  self.name=autoname(param)
  self.clname = "muxPrio"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false,
    nprio = true,
    service = false,
    mode = true
  }
  -- Parameter initialisation
  mux_init(self, param)
  assert(param.nprio > 0, "muxFrmPrio: invalid parameter nprio:" ..(param.nprio or "nil"))
  self.nprio = param.nprio
  self.serverState = self.serverIdling
  self.maxArrPrio = 0
  if self.serviceTime == 1 then
    self.syncMode = 0
  else
    assert(param.mode == "sync" or param.mode == "async", 
	   "muxFrmPrio: invalid mode:" ..(param.mode or "nil"))
    self.mode = param.mode
    if param.mode == "sync" then 
      self.syncMode = 1 
    else
      self.syncMode = 0
    end
  end
  -- Finish construction
  return self:finish()
end

--- Set maximum length of priority queue.
-- @param prio number Priority of the queue.
-- @param len number length of the queue in cells.
-- @return none.
function muxPrio:setQueueMax(prio, len)
  assert(prio >= 0 and prio < self.nprio, 
	 string.format("%s: invalid priority %s", self.name, tostring(prio)))
  self:getQueue(prio):setmax(len + 1)
end

--- Get maximum length of a priority queue.
-- @param prio number - Priority of the queue.
-- @return number - Length of the queue.
function muxPrio:getQueueMax(prio)
  assert(prio >= 0 and prio < self.nprio, 
	 string.format("%s: invalid priority %s", self.name, tostring(prio)))
  return self:getQueue(prio):getmax()
end

--- Set priority of a connection.
-- @param vci number Connection identifier.
-- @param prio number Priority.
-- @return none.
function muxPrio:setPriority(vci, prio)
  assert(vci >= 0 and vci <= self.max_vci,
	 string.format("%s: invalid vci: %s", self.name, tostring(vci)))
  assert(prio >= 0 and prio < self.nprio,
	 string.format("%s: invalid prio: %s", self.name, tostring(prio)))
  self:setPrio(vci, prio)
end

--- Get priority of a connection.
-- @param vci number Connection identifier.
-- @return Priority of the connection.
function muxPrio:getPriority(vci)
  assert(vci >= 0 and vci <= self.max_vci,
	 string.format("%s: invalid vci: %s", self.name, tostring(vci)))
  return self:getPrio(vci)
end

--- Set the service time.
-- @param delta number - Service cycle in slots.
-- @return none.
function muxPrio:setService(delta)
  self.serviceTime = val
end

--==========================================================================
-- MuxFramePrio Object 
--==========================================================================

_muxFrmPrio = muxFrmPrio
--- Definition of multiplexer class 'muxPrio'.
muxFrmPrio = class(_muxFrmPrio, muxBase)

--- Constructor for class 'muxFrmPrio'.
-- The multiplexer provides <code>ninp</code> inputs and <code>nprio</code> queues. The priority carried in the frame
-- (prioCodePoint) is interpreted as input priority. The priority of the output queue is then
-- determined by an internal mapping table, that can be manipulated using the function 
-- <code>setPriority(inprio, prio)</code>. The default mapping is 1 by 1. Zero is the lowest priority.
--<br>
-- Incoming frames are enqueued by fair strategy (random choice). The service time is configurable.
-- The multiplexer operates either in work-conserving or non work-conserving manner. 
-- In work-conserving mode "async" a frame that hits an empty queue is transmitted immediately.
-- In non work-conserving mode "sync" the server process is activated strictly synchron in 
-- fixed time intervals (frame rate scheduling).<br>
-- The multiplexer maintains loss counters per input priority, per priority , per queue and total loss.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional))<br>
--    Name of the display. Default: "objNN". 
-- <li>ninp)<br>
--    Number of inputs. 
-- <li>buff)<br>
--    Size of the output buffer. 
-- <li>maxinprio (optional))<br>
--    Maximum value for input priorities INPRIO. Default: 8. 
-- <li>nprio)<br>
--    Maximum number of output priorities (traffic classes/queues). 
-- <li>service (optional))<br>
--    Service time in slots. 
-- <li>servicerate (optional))<br>
--    Service rate in bits per slot, overrides service time
-- <li>mode)<br>
--    "async": work-conserving operation with the bitrate defined by 'servicerate'. <br>
--    "sync" : non work-conserving operation with the framerate defined by 'service'. <br>
-- <li>out)<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table - reference to object instance.
function muxFrmPrio:init(param)
  self = _muxFrmPrio:new()
  self.name=autoname(param)
  self.clname = "muxPrio"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxinprio = false,
    buff = false,
    nprio = true,
    service = false,
    servicerate = false,
    mode = true
  }
  -- Parameter initialisation
  mux_init(self, param)
  assert(param.nprio > 0, "muxPrio: invalid parameter nprio:" ..(param.nprio or "nil"))
  self.nprio = param.nprio
  self.serverState = self.serverIdling
  self.maxArrPrio = 0
  if param.servicerate then
     self.serviceRate = param.servicerate
     self.serviceTime = 1
  end
  if self.serviceTime == 1 then
    self.syncMode = 0
  else
    assert(param.mode == "sync" or param.mode == "async", 
	   "muxPrio: invalid mode:" ..(param.mode or "nil"))
    self.mode = param.mode
    if param.mode == "sync" then 
      self.syncMode = 1 
    else
      self.syncMode = 0
    end
  end
  -- Finish construction
  return self:finish()
end

--- Set maximum length of priority queue (in frames).
-- @param prio number Priority of the queue.
-- @param len number length of the queue in cells.
-- @return none.
function muxFrmPrio:setQueueMax(prio, len)
  assert(prio >= 0 and prio < self.nprio, 
	 string.format("%s: invalid priority %s", self.name, tostring(prio)))
  self:getQueue(prio):setmax(len+1)
end

--- Get maximum length of a priority queue.
-- @param prio number - Priority of the queue.
-- @return number - Length of the queue.
function muxFrmPrio:getQueueMax(prio)
  assert(prio >= 0 and prio < self.nprio, 
	 string.format("%s: invalid priority %s", self.name, tostring(prio)))
  return self:getQueue(prio):getmax()
end

--- Set priority of a priority code point.
-- @param inprio number Input Priority taken from frame.
-- @param prio number Priority.
-- @return none.
function muxFrmPrio:setPriority(inprio, prio)
  assert(inprio >= 0 and inprio <= self.max_inprio,
	 string.format("%s: invalid inprio: %s", self.name, tostring(vci)))
  assert(prio >= 0 and prio < self.nprio,
	 string.format("%s: invalid prio: %s", self.name, tostring(prio)))
  self:setPrio(inprio, prio)
end

--- Get priority of a priority code point.
-- @param inprio number Input Priority taken from frame.
-- @return Priority of the connection.
function muxFrmPrio:getPriority(inprio)
  assert(inprio >= 0 and inprio <= self.max_inprio,
	 string.format("%s: invalid inprio: %s", self.name, tostring(inprio)))
  return self:getPrio(inprio)
end

--- Set the service time.
-- @param delta number - Service cycle in slots (frame rate - non-work-conserving).
-- @return none.
function muxFrmPrio:setService(delta)
  self.serviceTime = val
end

--- Set the service rate time.
-- @param delta number - Service cycle in bits per slot (bitrate - work-conserving).
-- @return none.
function muxFrmPrio:setServiceRate(delta)
  self.serviceRate = delta
end

return yats
