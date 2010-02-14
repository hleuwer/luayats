-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: user.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - User Objects.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- User Lua classes.
-----------------------------------------------------------------------------------

require "yats.muxevt"

module("yats", yats.seeall)

--==========================================================================
-- Marker object
--==========================================================================

_marker = marker
--- Definition of 'marker' object.
marker = class(_marker)


--- Constructor of 'marker' object.
-- Marks a received data object according to given parameters.
-- @param param table - Parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> domark (optional)<br>
--    Flag to enable marking. Default: true. 
-- <li> clp (optional)<br>
--    Value to use in marking, either 0 or 1. Default: do not mark, if not given. 
-- <li> out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table - Reference to object instance.
function marker:init(param)
  local self = _marker:new()
  self.name = autoname(param)
  self.clname = "marker"
  self.parameters = {
    domark = false, clp = false, out = true
  }
  self:adjust(param)

  assert(param.domark == 1 or param.domark == 0, self.clname..": 'domark' must be 1 or 0.")
  local domark = param.domark or true
  if domark == true then
    self.domark = 1
  else
    self.domark = 0
  end
  assert(param.clp == 1 or param.clp == 0, self.clname..": 'clp' must be 1 or 0.")
  self.markclp = param.clp or -1
  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- Frame Marker Object.
--==========================================================================

_framemarker = framemarker
--- Definition of class 'framemarker'.
framemarker = class(_framemarker)

--- Constructor of class 'framemarker'.
-- Marks a frame according to given parameters.
-- @param param  table - Parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> domark (optional)<br>
--    Flag to enable marking. Default: true. 
-- <li> clp (optional)<br>
--    Value to use in marking, either 0 or 1. Default: do not mark, if not given. 
-- <li> maxsize (optional)<br>
--    Max. size of a frame in bytes. Default: 10000 bytes. 
-- <li> vlanid (optional)<br>
--    VLAN ID of the frame to mark. Default: -1. 
-- <li> vlanpriority or vlanprio (optional)<br>
--    VLAN priority field to set. Default: -1. 
-- <li> dropprecedence (optional)<br>
--    Drop precedence (DiffServ) to set. Default: -1. 
-- <li> internaldropprecedence (optional)<br>
--    Internal drop precedence to set. Default: -1. 
-- <li> out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}.
-- </ul>
-- @return table - Reference to object instance.
function framemarker:init(param)
  self = _framemarker:new()
  self.name = autoname(param)
  self.clname = "framemarker"
  self.parameters = {
    domark = false, clp = false, maxsize = false, vlanid = false, 
    vlanpriority = false, dropprecedence = false,
    internaldropprecedences = false, out = true
  }
  self:adjust(param)
  
  assert(not param.domark or param.domark == 1 or param.domark == 0, self.clname..": 'domark' must be 1 or 0.")
  local domark = param.domark or true
  if domark == true then
    self.domark = 1
  else
    self.domark = 0
  end
  assert(not param.clp or param.clp == 1 or param.clp == 0, self.clname..": 'clp' must be 1 or 0.")
  self.markclp = param.clp or -1
  
  assert(not param.maxsize or param.maxsize > 0, self.clname..": maxsize > 0 required.")
  self.maxsize = param.maxsize or 10000;
  self.vlanId = param.vlanid or -1
  self.vlanPriority = param.vlanpriority or param.vlanprio or -1
  self.dropPrecendence = param.dropprecedence or -1
  self.internalDropPrecedence = param.internaldropprecedence or - 1
  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- Data to Frame
--==========================================================================

_dat2fram = dat2fram
--- Definition of class 'dat2fram' (Data to Frame Converter).
dat2fram = class(_dat2fram)

--- Constructor for class 'dat2fram'.
-- Receives a cell and uses this to send a new frame of len 'flen'. The cell is only used
-- as trigger. Various data frame attributes can be set on the fly. Note that the object
-- takes no care about inconsistent address settings. It simply copies the configured values
-- into the frame.
-- @param param table - Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li> flen<br>
--    Frame length to produce 
-- <li> dmac<br>
--    Destination MAC address 
-- <li> smac<br>
--    Source MAC address <br>
-- <li> connid (optional)<br>
--    Connection id to identify the frame connection 
-- <li> pcp (optional)<br>
--    Priority Code Point (generic priority, e.g. for internal usage) 
-- <li> vlanid (optional)<br>
--    VLAN ID (only tag) 
-- <li> tpid (optional)<br>
--    TPID for vlan tagged frames 
-- <li> vlanprio (optional)<br>
--    VLAN priority 
-- <li> dscp (optional)<br>
--    DiffServ service class 
-- <li> out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table - Reference to object.
function dat2fram:init(param)
  local self = _dat2fram:new()
  self.name = autoname(param)
  self.clname = "dat2fram"
  self.parameters = {
     flen = true, connid = false, vlanid = false,
     dmac = false, smac = false,
     tpid = false, vlanprio = false, pcp = false, dscp = false, 
     out = true
  }
  self:adjust(param)

  assert(param.flen > 0, "dat2fram: frame length > 0 required")
  self.flen = param.flen
  self.connID = param.connid or 0
  self.pcp = param.pcp or 0
  self.vlanprio = param.vlanprio or 0
  self.dscp = param.dscp or 0
  self.dmac = param.dmac or 0
  self.smac = param.smac or 0
  self.tpid = param.tpid or 0
  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- data2frs
--==========================================================================

_data2frs = data2frs
--- Definition of class 'data2frs' (Data to Frame Converter with SeqNo).
data2frs = class(_data2frs)

--- Constructor for class 'data2frs'.
-- Receives a cell and uses this to send a new frame of len 'framesize'. A
-- sequence number is assigned to the frame.
-- The cell is only used as trigger.
-- @param param table - Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN" 
-- <li> framesize<br>
--    Frame length to produce 
-- <li> maxsnr<br>
--    Maximum sequeunce number
-- <li> out<br>
--    Connection to successor 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table - Reference to object.
function data2frs:init(param)
  local self = _data2frs:new()
  self.name = autoname(param)
  self.clname = "data2frs"
  self.parameters = {
    framesize = true, maxsnr = true, out = true
  }
  self:adjust(param)

  self.framesize = param.framesize
  self.maxframe = param.maxsnr

  self.framenr = 0

  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- measframe
--==========================================================================

_measframe = measframe
--- Definition of class 'measframe'.
measframe = class(_measframe)

--- Constructor for class 'measframe'.
-- This object measures and calculates the mean "goodput" of traffic passing
-- the element.<br>
-- The goodput per packet is defined by the packet length divided by the
-- packet's transmission time. For a constant linerate or (throughput), the
-- mean goodput decreases with increasing delay of packets.<br>
-- The inverse of goodput is also calculated.<br>
-- <br>
-- <i>EXPORT</i>: Goodput5, Goodput5Invers<br>
-- @param param table - Parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}
-- </ul>.
-- @return table - Reference to object instance.
function measframe:init(param)
  local self = _measframe:new()
  self.name = autoname(param)
  self.clname = "measframe"
  self.parameters = {
	out = true
  }
  self:adjust(param)


  self.meanGoodput5 = 0.0;
  self.meanGoodput5Invers = 0.0;
  self.counter = 0;


  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
  
end
--==========================================================================
-- Tracer object
--==========================================================================

--- Definition of class 'tracer'.
tracer = class(setTrace)

--- Constructor of class 'tracer'.
-- This device sets the trace pointer of a data object.
-- In each incoming data object, the pointer traceOrigPtr is set to the 
-- name of the trace object. Additionally, the trace sequence number is copied
-- into the data object and incremented.
-- All subsequent network objects will generate trace messages for the marked 
-- objects.
-- NOTE: Only works if DATA_OBJECT_TRACE has been defined in src/kernel/defs.h
--	 Otherwise no action is performed.
--@param param table Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> vci or connid<br>
--    Connection identifier for cells (vci) or frames (connid). If not set
--    All data objects are traced. 
-- <li> out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}.
-- </ul>
--@return table - Reference to device.
function tracer:init(param)
  local self = setTrace:new()
  self.name = autoname(param)
  self.clname = "tracer"
  self.parameters = {
    vci = false, connid = false, out = true
  }
  self:adjust(param)
  if param.vci then
    self.id = param.vci
    self.inputType = CellType
  elseif param.connid then
    self.id = param.connid
    self.inputType = FrameType
  else
    self.inputType = DataType
  end
  self.seqNo = 0
  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- lbframe object
--==========================================================================

_lbframe = lbframe
--- Definition of 'lbframe' object.
lbframe = class(_lbframe)


--- Constructor of 'lbframe' object.
function lbframe:init(param)
  local self = _lbframe:new()
  self.name = autoname(param)
  self.clname = "lbframe"
  self.parameters = {
	   bitrate = true, mcr = true, mbs = true, mfs = true,
	   tag = true, vci = false,  out = true
  }
  self:adjust(param)

  assert(param.bitrate > 0, self.name..": The BITRATE must be > 0.0")
  local lb_bitrate = param.bitrate

  assert(param.mcr > 0, self.name..": MCR must be > 0.0")
  assert(param.mcr <= lb_bitrate, self.name..": The MCR must be <= BITRATE")
  local lb_scr = param.mcr

  assert(param.mbs >= 1, self.name..": The MBS must be >= 1.0")
  local lb_mbs = param.mbs

  assert(param.mfs >= 1, self.name..": The MFS must be >= 1.0")
  self.lb_mfs = param.mfs
  
  self.lb_tag = param.tag
  
  self.vci = param.vci or NILVCI

  self.inp_type = AAL5CellType	

  self.lb_inc = lb_bitrate / lb_scr
  self.lb_max = lb_mbs * lb_bitrate / lb_scr

  self.lb_dec = 1.0
  self.lb_siz = 0.0
  self.last_time = 0

  self.vci_first = 1
  self.cellnumber = 0
  self.vci_clp = 0

  self:definp(self.clname)
  self:defout(param.out)
  return self:finish()
end

--==========================================================================
-- fork object
--==========================================================================

--- Definition of 'fork' object.
fork = class(dfork)

--- Constructor of 'fork' object.
-- This object replicates an input data item to n outputs.
-- 
-- @param param table - Parameter list
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> nout Number of outputs
-- <li> out<br>
--    Table of successors. 
--    Format: <code>{<br> 
--              {"name-of-successor 1", "input-pin-of-successor 1"},<br>
--              {"name-of-successor 2", "input-pin-of-successor 2"},<br>
--            }</code>
-- </ul>.
-- @return table - Reference to object instance.
function fork:init(param)
  local self = _fork:new()
  self.name = autoname(param)
  self.clname = "fork"
  self.parameters = {
    nout = true, out = true
  }
  self:adjust(param)

  self:definp(self.clname)
  self:set_nout(param.nout)
  self:defout(param.out)

  return self:finish()
end

--==========================================================================
-- tickctrl object
--==========================================================================
local _tickctrl = tickctrl
--- Definition of 'tickctrl' object.
tickctrl = class(_tickctrl)

--- Constructor of 'tickctrl' object.
-- This object emulates the behaviour of an operating system, where a defined
-- number of processes contend for processing time in a round robin fashion. 
-- The active time interval of the object is probed from a distribution 
-- function, which is defined externally using on of the available distribution
-- objects (for instance geometrical or binominal distribution).
-- The length of the actual active time interval for the object is then 
-- calculated considering the contending other processes:
-- next_time =  '(contproc + 1) * tick_from_distribution + 1. 
--<br> 
-- Input data is always enqueued at arrival, also during inactive state. Queue overflow
-- is avoided using the start/stop protocol, which is activated at a given threshold
-- 'xoff'. Once the buffer filling decreases below the limit 'xon' a start message
-- is sent to the predecessor, which results in new data arriving. 
--
-- If the system becomes empty during the active time interval it tries to restarts
-- it's feeding node. Data items that arrive are served at the beginning of the 
-- next time interval.
--<br> 
-- The object also reacts upon start/stop initiated by it's successor. 
--<br>
--<br>INPUTS: 'in' for data, 'start' for the start/stop protocol. 
--<br>EXPORT: 'QLen' and 'Count'.
--@param param table Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> buf<br>
--    Maximum size of the internal buffer. 
-- <li> bstart<br>
--    Buffer threshold, when to wake up the predecessor (start/stop protocol). 
-- <li> offint<br>
--    Time interval, in which 'going off' is detected. 
-- <li> contproc<br>
--    Number of contending processes (round robin cycle). 
-- <li> phase<br>
--    Phase to start processing (asynchronous behaviour of different objects). 
-- <li> out<br>
--    Connection to successor and predecessor. 
--    Format:
--<br>   {{"name-of-successor", "input-pin-of-successor"},
--     {"name-of-predecessor", "start-pin-of-predecessor"}}.
-- </ul>
--@return table - Reference to device.

function tickctrl:init(param)
   local self = _tickctrl:new()
   self.name = autoname(param)
   self.clname = "tickctrl"
   self.parameters = {
      dist = true, xoff = false, xon = false, offint = false,
      phase = false, contproc = false, out = true
   }
   self:adjust(param)

   self.q_max = param.xoff or 0
   self.q_start = param.xon or 0
   self.off_int = param.offint or 1
   self.contproc = param.contproc or 0
   self.phase = param.phase or -1 -- -1 to signal not defined 
   self.dist = param.dist

   -- Distribution table
   local msg = GetDistTabMsg:new_local()
   local err = self.dist:special(msg, nil)
   assert(not err, err)
   self:setTable(msg:getTable())

   -- Outputs
   self:set_nout(table.getn(param.out))
   self:defout(param.out)

   -- Input
   self:definp("in")
   if self.q_max > 0 then
      self:definp("start")
   end

   -- Finish with C++ act() call
   return self:finish()
end

--==========================================================================
-- eth_bridge object
--==========================================================================
_ethbridge = ethbridge
--- Definition of 'ethbridge' object.
ethbridge = class(_ethbridge)

--- Constructor of 'ethbridge' object.
-- This object emulates the behavior of a transparent ethernet bridge according
-- to IEEE 802.1D. 
--<br>
-- Forwarding: <br>
-- The source address of incoming frames is learned in the filtering
-- database which has a configurable size. The whole MAC table is divided into a
-- configurable number of independent databases. The database used is determined
-- by the input port. The FdB can be populated by the user using the function
-- addMac().
--<br>
-- Queuing:<br>
-- Each port has a set of nprios output queues which are scheduled in strict
-- priority fashion. The queue is selected using either the vlanPriority field
-- of the incoming frame or by the default priority of the input port. 
--<br>
--<br>INPUTS: 'p1', 'p2', ..., 'pn' for n data ports
--<br>EXPORT: 'QLen', 'LossTot', 'Loss' per port, 'LossInp', LossINPRIO and 'Count'.
--@param param table Parameter table
-- <ul>
-- <li> name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li> buf (optional)<br>
--    Maximum size of the internal queues. 
-- <li> nports <br>
--    Number of ports. 
-- <li> ndb (optional)<br>
--    Number of databases in FdB (default = 1). 
-- <li> ninprio (optional)<br>
--    Number input priorities (default = 8). 
-- <li> nprio (optional) <br>
--    Number of strict priority output queues per port (default = 4). 
-- <li> agetime (optional)<br>
--    Ageing time of FdB entries. 
-- <li> servicerate (mandatory)<br>
--    Rate in bits per second  at which the queues of a single port 
--    are scheduled. By default all output ports have the same servicerate. 
-- <li> out<br>
--    Connection to successor and predecessor. 
--    Format:
--<br>   {{"name-of-successor", "input-pin-of-successor"},<br>
--     {"name-of-predecessor", "start-pin-of-predecessor"}}.
-- </ul>
--@return table - Reference to device.

function ethbridge:init(param)
   assert(param.nports and param.nports > 1, "bridge needs at least 2 ports")
   local self = _ethbridge:new(param.nports)
   self.name = autoname(param)
   self.clname = "ethbridge"
   self.parameters = {
      nentries = false, nports = true, ndb = false,
      servicerate = true, ninprio = false,
      nprio = false, agetime = true, out = true
   }
   self:adjust(param)
   self.numdb = param.ndb or 1
   self.numentries = param.nentries or 256
   self.numprios = param.nprio or 4
   self.numinprios = param.ninprio or 8
   self.agetime = param.agetime 
   for i = 1, self.numports do
      self:definp("p"..i)
   end
   self.omux = {}
   for i = 1, self.numports do
      local omux = yats.muxFrmPrio{
	 self.name.."-omux"..i, ninp = self.numports, nprio=self.numprios, buff = 80,
	 servicerate = param.servicerate, mode = "async",
	 out = param.out[i]
      }
      self.omux[i] = omux
      self:setMux(i, omux)
   end
   return self:finish()
end

--
-- Overload the generic connect function and use priority mux outputs instead
-- of ethbridge's.
--
function ethbridge:connect()
   for p = 1, self.numports do
      self:getMux(p):connect()
   end
end

--- Set queue maximum size.
-- @param port number - Port index starting with 1.
-- @param prio number - Output priority.
-- @param len number -  Length of the queue.
-- @return none.
function ethbridge:setQueueMax(port, prio, len)
   assert(prio >= 0 and prio < self.numprios, 
	  string.format("%s: invalid priority %s", self.name, tostring(prio)))
   return self:getMux(port):getQueue(prio):setmax(len+1)
end

--- Get queue maximum size.
-- @param port number - Port index starting with 1.
-- @param prio number - Output priority.
-- @return number - Length of queue.
function ethbridge:getQueueMax(port, prio)
   assert(prio >= 0 and prio < self.numprios, 
	  string.format("%s: invalid priority %s", self.name, tostring(prio)))
   return self:getMux(port):getQueue(prio):getmax()
end

--- Set in-to-out priority mapping.
-- Input priority is mapped to output
-- traffic class.
-- @param port number - Port index starting with 1.
-- @param inprio number - Input priority.
-- @param prio number - Output priority.
-- @return none.
function ethbridge:setPriority(port, inprio, prio)
   assert(inprio >= 0 and inprio <= self:getMux(port).max_inprio,
	  string.format("%s: invalid inprio: %s", self.name, tostring(inprio)))
   assert(prio >= 0 and prio < self.numprios,
	  string.format("%s: invalid prio: %s", self.name, tostring(prio)))
   return self:getMux(port-1):setPrio(inprio, prio)
end

--- Get in-to-out priority mapping.
-- Input priority is mapped to output
-- traffic class.
-- @param port number - Port index starting with 1.
-- @param inprio number - Input priority.
-- @return number - Output priority.
function ethbridge:getPriority(port, inprio)
   assert(inprio >= 0 and inprio <= self:getMux(port).max_inprio,
	  string.format("%s: invalid inprio: %s", self.name, tostring(inprio)))
   return self:getMux(port-1):getPrio(inprio)
end

--- Set output ports interface speed.
-- @param port number - Port index starting with 1.
-- @param delta number - Port rate in bits per slot.
-- @return number - Output priority.
function ethbridge:setServiceRate(port, delta)
   self:getMux(port-1).serviceRate = delta
end

--- Get output ports interface speed.
-- @param port number - Port index starting with 1.
-- @return number - Output priority.
function ethbridge:getServiceRate(port)
   return self:getMux(port-1).serviceRate
end

--- Set input ports default priority.
-- @param port number - Port index starting with 1.
-- @param inprio number - Input priority.
-- @return none.
function ethbridge:setDefaultPrio(port, inprio)
   assert(inprio >= 0 and inprio <= self:getMux(port).max_inprio,
	  string.format("%s: invalid inprio: %s", self.name, tostring(inprio)))
   self.defprio[port-1] = prio
end

--- Get input ports default priority.
-- @param port number - Port index starting with 1.
-- @return none.
function ethbridge:getDefaultPrio(port)
   return self.defprio[port-1]
end
--- Add a static unicast entry into FdB.
-- @param db number - Database index starting with 0.
-- @param port number - Port index starting with 1.
-- @param mac number - MAC address.
-- @param age number - Age of this entry; -1 means locked (no ageing).
-- @return none.
function ethbridge:addmac(db, mac, portvec, age)
   local pvec = 0
   local mask = 1
   local age = age or self.agetime
   assert(string.len(portvec) < self.numports + 1, "invalid port vector")
   for i = 1, string.len(portvec) do
      local bit = string.sub(portvec, -i, -i)
      if bit == "1" then
	 pvec = pvec + mask
      end
      mask = mask * 2
   end
   self.fdb:add(db, mac, pvec, age)
end

--- Purge an entry in the FdB.
-- @param db number - Database index starting with 0.
-- @param mac number - MAC address.
-- @return none.
function ethbridge:purgemac(db, mac)
   self.fdb:purge(db, mac)
end

--- Flush a complete database in the FdB.
-- @param db number - Database index starting with 0.
-- @return none.
function ethbridge:flush(db)
   self.fdb:flush(db)
end

--- Lookup an entry in the FdB.
-- @param db number - Database index starting with 0.
-- @param mac number - MAC address.
-- @return none.
function ethbridge:lookup(db, mac)
   local pvec, macentry = self.lookup(db, mac)
   local mask = 1
   for i = 1, self.numports do
      if math.mod(pvec, 2) == 1 then
	 retval = retval .. "1"
      else
	 retval = retval .. "0"
      end
      pvec = pvec / 2
   end
end

return yats
