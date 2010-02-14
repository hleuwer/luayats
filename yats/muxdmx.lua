-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: muxdmx.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description LuaYats - Multiplexer and Demultiplexer.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- Lua classes for multiplexers and demultiplexers.
-----------------------------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

--==========================================================================
-- Multiplexer Object 
--==========================================================================

local function mux_init(self, param)

  -- Parameter name adjustment
  self:adjust(param)
  
  -- Parameter check
  assert(param.ninp and param.ninp > 0, "mux: invalid number of inputs: "..param.ninp)
  self.ninp = param.ninp
  self.max_vci = param.maxvci or param.ninp + 1
  assert(self.max_vci > 0, "mux: invalid maxvci: "..(param.maxvci or "nil"))
  self.q:setmax((param.buff or 0) + 1)
  
  -- Init output table
  self:defout(param.out)
  
  -- Init input table
  local i
  for i = 1, self.ninp do
    self:definp("in"..i)
  end
end

_mux = mux
-- Definition of multiplexer class 'mux'.
mux = class(_mux)

--- Constructor for class 'mux'.
-- The multiplexer provides 'ninp' inputs with a single output queue with a 
-- given maximum length 'buff'. Incoming cells are enqueued by fair strategy 
-- (random choice).
-- During each tick a single cell is scheduled.
-- The multiplexer maintains the following loss counters:<br>
--   a) for the queue in total and<br>
--   b) on a per VCI level.<br>
-- @param param table - parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN"
-- <li> ninp<br>
--    Number of inputs
-- <li> buff<br>
--    Size of the output buffer
-- <li> maxvci (optional)<br>
--    Maximum value for input VCI. Default: ninp+1
-- <li> out<br>
--    Connection to successor
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table - reference to object instance.
function mux:init(param)
  self = _mux:new()
  self.name=autoname(param)
  self.clname = "mux"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false
  }
  -- Parameter initialisation
  mux_init(self, param)

  -- Finish construction
  return self:finish()
end

mux.getLossInp = mux.getLoss

--- Get summary of losses for a ranage of inputs.
-- @param from number Lower input index.
-- @param to number Upper input index.
-- @return number Loss count.
function mux:getLosses(from, to)
  local i, n = 0, 0
  for i = from,to do n = n + self:getLoss(i) end
  return n
end
mux.getLossesInp = mux.getLosses

--- Get summary of losses for a ranage of VCIs.
-- @param from number -  Lower VCI.
-- @param to number - Upper VCI.
-- @return number - Loss count.
function mux:getLossesVCI(from, to)
  local i, n = 0, 0
  for i = from,to do n = n + self:getLossVCI(i) end
  return n
end


--- Get output queue length.
-- @return number - Length of the queue.
function mux:getQLen()
  return self.q.q_len
end

--- Get maximum queue length.
-- @return number - Maximum length of queue.
function mux:getQMaxLen()
  return self.q:getmax()
end

--==========================================================================
-- Multiplexer Object with Departure First
--==========================================================================

_muxDF = muxDF
--- Definition of multiplexer class 'muxDF'.
muxDF = class(_muxDF, mux)

--- Constructor for class 'muxDF'.
-- The multiplexer provides 'ninp' inputs with a single output queue with a given
-- maximum length 'buff'. Incoming cells are enqueued by fair strategy (random choice).
-- During each tick a single cell is scheduled.
-- The multiplexer maintains loss counters.<br>
-- a) for the queue in total and<br>
-- b) on a per VCI level.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>ninp<br>
--    Number of inputs. 
-- <li>buff<br>
--    Size of the output buffer. 
-- <li>maxvci (optional)<br>
--    Maximum value for input VCI. Default: ninp+1. 
-- <li>active<br>
--    Server time distance. 
-- <li>out<br>
--    Connection to successor
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>.
-- </ul>.
-- @return table - reference to object instance.
function muxDF:init(param)
  self = _muxDF:new()
  self.name=autoname(param)
  self.clname = "muxDF"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false,
    active = false
  }

  -- Parameter initialisation
  mux_init(self, param)
  self.active = param.active or 1

  alarme(event:new(self, 1), self.active)
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--==========================================================================
-- Multiplexer Object with Arrival  First
--==========================================================================

_muxAF = muxAF
--- Definition of multiplexer class 'muxAF'.
muxAF = class(_muxAF, mux)

--- Constructor for class 'muxAF'.
-- The multiplexer provides 'ninp' inputs with a single output queue with a given
-- maximum length 'buff'. Incoming cells are enqueued by fair strategy (random choice).
-- Server time is configurable. Note that muxAF does not imply the additional delay
-- as the network object yats.mux.
-- The multiplexer maintains loss counters
-- a) for the queue in total and<br>
-- b) on a per VCI level.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>ninp<br>
--    Number of inputs. 
-- <li>buff<br>
--    Size of the output buffer. 
-- <li>maxvci (optional)<br>
--    Maximum value for input VCI. Default: ninp+1. 
-- <li>active<br>
--    Server time distance. 
-- <li>out<br>
--    Connection to successor
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>. 
-- </ul>.
-- @return table - reference to object instance.
function muxAF:init(param)
  self = _muxAF:new()
  self.name=autoname(param)
  self.clname = "muxAF"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false,
    active = false
  }

  -- Parameter initialisation
  mux_init(self, param)
  self.active = param.active or 1
  alarme(event:new(self, 1), self.active)
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--==========================================================================
-- Multiplexer with distributed server time
--==========================================================================

_muxDist = muxDist
--- Definition of multiplexer class 'muxDist'.
muxDist = class(_muxDist, mux)

--- Constructor for class 'muxDist'.
-- The multiplexer provides 'ninp' inputs with a single output queue with a given
-- maximum length 'buff'. Incoming cells are enqueued by fair strategy (random choice).
-- The service time is defined by a distribution object.
-- The multiplexer maintains loss counters<br>
-- a) for the queue in total and<br>
-- b) on a per VCI level.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>ninp<br>
--    Number of inputs. 
-- <li>buff<br>
--    Size of the output buffer. 
-- <li>maxvci (optional)<br>
--    Maximum value for input VCI. Default: ninp+1. 
-- <li>dist<br>
--    Distribution object defining server times. 
-- <li>out<br>
--    Connection to successor
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>. 
-- </ul>.
-- @return table - reference to object instance.
function muxDist:init(param)
  self = _muxDist:new()
  self.name=autoname(param)
  self.clname = "muxAF"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false,
    dist = true
  }

  -- Parameter initialisation
  mux_init(self, param)

  -- Reference to distribution object
  self.dist = param.dist

  -- Distribution table.
  local msg = GetDistTabMsg:new_local()
  local err = self.dist:special(msg, nil)
  assert(not err, err);
  self:setTable(msg:getTable())
  self.serving = 0

  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--==========================================================================
-- Multiplexer with weighted fair queuing
--==========================================================================
wfqVCQ = class(wfqpar)

function wfqVCQ:init(param)
  self = wfqpar:new()
  self.name=autoname(param)
  self.clname = "wfqVCQ"
  self.parameters = {
    delta = true,
    qsize = false
  }
  self.delta = param.delta
  self.q:setmax(param.qsize or 10)

  return self:finish()
end

_muxWFQ = muxWFQ
--- Definition of multiplexer class 'muxWFQ'.
muxWFQ = class(_muxWFQ, mux)

--- Constructor for class 'muxWFQ'.
-- The multiplexer provides 'ninp' inputs with a single output queue with a given
-- maximum length 'buff'. Incoming cells are enqueued by fair strategy (random choice).
-- The service time is defined by a distribution object.
-- The multiplexer maintains loss counters<br>
-- a) for the queue in total and<br>
-- b) on a per VCI level.
-- @param param table - parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display - Default: "objNN". 
-- <li>ninp<br>
--    Number of inputs. 
-- <li>buff<br>
--    Size of the output buffer. 
-- <li>maxvci (optional)<br>
--    Maximum value for input VCI. Default: ninp+1. 
-- <li>dist<br>
--    Distribution object defining server times. 
-- <li>out<br>
--    Connection to successor
--    Format: <code>{"name-of-successor", "input-pin-of-successor"}</code>. 
-- </ul>.
-- @return table - reference to object instance.
function muxWFQ:init(param)
  self = _muxWFQ:new()
  self.name=autoname(param)
  self.clname = "muxWFQ"
  self.parameters = {
    out = true,
    name = false,
    ninp = true,
    maxvci = false,
    buff = false,
    vcq = false
  }

  -- Parameter initialisation
  mux_init(self, param)

  if param.vcq then
    local nq = 0
    for _, v in pairs(param.vcq) do
      assert(nq > vci_max, "Too many per VC queues")
      self:addVCQ{
	vc = param.vc,
	delta = param.vcq[1] or param.vcq.delta,
	qsize = param.vcq[2] or param.vcq.qsize
      }
      nq = nq + 1
    end
  end
  self.vcqs = {}
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--- Set WFQ per VC parameter.
-- @param param table Parameter table. 
-- <ul>
-- <li>vc<br>
--    VCI. 
-- <li>delta<br>
--    weight for this VCI. 
-- <li>qsize<br>
--    size of the per VC queue. 
--</ul>.
-- @return none.
function muxWFQ:addVCQ(param)
  assert(type(param) == "table", "Wrong parameter type")
  local vc = param.vc
  assert(vc >= 0 and vc < self.max_vci,
	 string.format("VCI %d is out-of-range", vc))
  assert(self:getQueue(vc) == nil, 
	 string.format("VCI %d has already been defined", vc))
  assert(param.delta >= 1, "Invalid value for parameter 'delta'")
  assert(param.qsize >= 1, "Invalid value for parameter 'qsize'")
  local vcq = wfqVCQ{self.name.."-vcq-"..vc, vc = vc, 
    delta = param.delta, qsize = param.qsize
  }
  table.insert(self.vcqs, vcq)
  self:setQueue(vc, vcq)
  return vcq
end

--- Read per VC parameters.
-- @param vc number VCI.
-- @return Table with per VC parameters: delta and qsize.
function muxWFQ:getVCQ(vc)
  local vcq = self:getQueue(vc)
  return {
    vc = vc,
    delta = vcq.delta,
    qsize = vcq.q:getmax(),
    name = vcq.name
  }, vcq
end
--==========================================================================
-- Demultiplexer Object 
--==========================================================================

_demux = demux
--- Definition of class 'demux'.
demux = class(_demux)

--- Constructor for class 'demux'.
-- The demultiplexer distributes cells from one single input to 'nout'
-- outputs using a connection table, which is initialised using the
-- method [[signal()]].
-- @param param table - parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>maxvci<br>
--    Maximum value of output VCI. 
-- <li>nout<br>
--    Number of outputs. 
-- <li>out<br>
--    Table of successors. 
--    Format: <code>{<br> 
--              {"name-of-successor 1", "input-pin-of-successor 1"},<br>
--              {"name-of-successor 2", "input-pin-of-successor 2"},<br>
--            }.</code>
-- </ul>.
-- @return table - reference to object instance.
function demux:init(param)
  self=_demux:new()
  self.name=autoname(param)
  self.clname = "demux"
  self.parameters = {
    frametype = false,
    maxvci = true,
    nout = true,
    out = true,
  }
  self:adjust(param)
  
  -- set object vars in yats object
  if param.frametype and param.frametype == true then
    self:setRecTyp(rec_frame)
  else
    self:setRecTyp(rec_cell)
  end
  assert(param.maxvci, "demux: parameter 'maxvci' missing.")
  self.nvci = param.maxvci + 1
  assert(param.nout, "demux: parameter 'nout' missing.")
  self.noutp = param.nout
  
  -- 3. init output table
  self:set_nout(param.nout)
  self:defout(param.out)

  -- 3. init input table
  self:definp(self.clname)
  
  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--- Retrieve the demux routing table.
-- @return table Routing table as an indexed list of entries (from=N, to=N, outp=N).
function demux:getRouting()
  local i
  local t = {n=0}
  for i = 1, self.nvci-1 do
    table.insert(t, self:getRoutingEntry(i))
  end
  return t, self.nvci
end

--- Retrieve a single routing entry.
-- @param vc number Input VCI.
-- @return table - Entry in the form {from = vc, to = vc-out, outp = out-port}.
function demux:getRoutingEntry(vc)
  local t = {from = vc, to=self:getNewVCI(vc), outp=self:getOutpVCI(vc)}
  return t
end

return yats