-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: rstp.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description LuaYats - Spanning Tree Protocol.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- Lua classes for Rapid Spanning Tree. It is based on rstplib from Alex Rozin.
-- The library was modified in order to run multiple spanning tree instances
-- in the single threaded environment of luayats.
-- This module provides wrapper functions for a convenient instantiation and
-- simulation of spanning tree instances in luayats.
-----------------------------------------------------------------------------------

require "yats.core"
require "yats.logging.console"
require "yats.object"
require "toluaex"
local cfg = require "yats.config"
conf = cfg.data

module("yats", yats.seeall)

local seq_no = 0
local EVQUEUE = 1
local EVPROCESS = 2
local EVCLOCK = 3
local EVSTART = 4
local EVSEND = 5


local mac2s = mac2s
local stringhex = stringhex

-- Wrapper for a data or frame queue. 
-- We need this to avoid garbage collection of userdata
-- objects allocated via Lua and enqueued in a C-level queue.
local cqueue = class()

function cqueue.init(self, max)
  self.q = queue:new(max)
  self.elems = {}
  return self
end

function cqueue:enqueue(pd)
  self.elems[pd] = true
  self.q:enqueue(pd)
end

function cqueue:dequeue()
  local pd = self.q:dequeue()
  self.elems[pd] = nil
  return pd
end

function cqueue:getlen()
  return self.q:getlen()
end

-- Private Logger.
local log = logging[conf.yats.LogType]("STP "..conf.yats.LogPattern)
local pdulog = logging[conf.yats.LogType]("PDU "..conf.yats.LogPattern)

-- We got the log level from the command line
log:setLevel(conf.Protocols.StpLogLevel)
pdulog:setLevel(conf.Protocols.PduLogLevel)
log:info(string.format("Logger STP created: %s", os.date()))
pdulog:info(string.format("Logger PDU created: %s", os.date()))

ieeebridge.log = log
ieeebridge.pdulog = pdulog
local pdulog = pdulog

--- Definition of class 'bridge'.
bridge = class(luaxout)


--- Constructor of class 'bridge'.
-- Instantiates state machines for the RSTP protocol.
-- @param param table Parameter table
-- <ul>
-- <li>name (optional)<br>
--    Name of the bridge. Default: "objNN". 
-- <li>nport<br>
--    Number of ports. 
-- <li>basemac<br>
--    Base MAC address of the bridge given as
--    Lua string. Embedded zeros are allowed. 
--    e.g. string.char(1,2,0,0,0,0). 
-- <li>start_delay (optional)<br>
--    Start delay in slots for the protocol. Default 1 tick. 
-- <li>process_speed (optional)<br>
--    Processing speed in seconds for BPDU handling. Default: 1 ms tick. 
-- <li>priorities (optional)<br>
--    Bridge priorities for different VLANs.Default: [0] = 32768. 
-- <li>memberset (optional)<br>
--    Defines the VLAN port membersets in the following form:<br>
--    {[VID] = PORTMASK, VID = PORTMASK}, where PORTMASK is a table
--    {p1, p2, p3, ..., pn} representing the port BITMASK. A bit set
--    to 1 enables the port in the port memberset, a 0 disables the port. 
-- <li>rxlog (optional)<br>
--    A list of ports for BPDU receive logging. Default: empty = no logging. 
-- <li>txlog (optional)<br>
--    A list of ports for BPDU transmit logging. Default: empty = no logging. 
-- <li>statelog (optional)<br>
--    A list of ports for state change logging. Default: empty = no logging. 
-- <li>vidlog (optional)<br>
--    A list of vlan ids for logging. Default: all. 
-- <li>callback (optional)<br>
--    A list of user defined event callback functions. Default: empty = no callback. 
-- <li>out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return userdata Reference to object instance.
function bridge:init(param)
  self = luaxout:new()
  self.name=autoname(param)
  self.clname = "rstp"
  self.parameters = {
    name = false,
    nport = true,
    basemac = true,
    start_delay = false,
    process_speed = false,
    vlans = false,
    priorities = false,
    memberset = false,
    statelog = false,
    vidlog = false,
    rxlog = false,
    txlog = false,
    callback = false,
    out = true
  }

  self:adjust(param)
  self.nport = param.nport
  self.speed, self.duplex = {},{}
  self.vlans = param.priorities or param.vlans or {[0]=32768}
  if param.memberset then
    self.memberset = param.memberset
  else
    self.memberset = {[0]={}}
    for i = 1, self.nport do
      table.insert(self.memberset[0], 1)
    end
  end
  -- Physical port parameters: speed and duplex mode
  for i =1,self.nport do
    if param.speed then
      table.insert(self.speed, i, param.speed[i] or 100)
    else
      table.insert(self.speed, i, 100)
    end
    if param.duplex then
      table.insert(self.duplex, i, param.duplex[i] or 1)
    else
      table.insert(self.duplex, i, 1)
    end
  end

  -- BPDU logging
  self.pdurxlog = {}
  self.pdutxlog = {}
  for _, v in ipairs(param.rxlog or {}) do
    assert(v <= self.nport, "invalid port index in portlist")
    self.pdurxlog[v] = true
  end
  for _, v in ipairs(param.txlog or {}) do
    assert(v <= self.nport, "invalid port index in portlist")
    self.pdutxlog[v] = true
  end
  
  -- State logging
  self.statelog = {}
  for _,v in ipairs(param.statelog or {}) do
    assert(v <= self.nport, "invalid port index in portlist")
    self.statelog[v] = true
  end

  -- VID logging mask
  self.vidlog = {}
  if not param.vidlog then
     for k,v in ipairs(self.vlans) do
      self.vidlog[k] = true
    end
  else
     for _,v in ipairs(param.vidlog) do
      assert(self.vlans[v], "invalid vlan index in vidlist")
      self.vidlog[v] = true
    end
  end
  self.basemac = param.basemac
  self.start_delay = param.start_delay
  self.process_speed = param.process_speed or (1e-3/SlotLength)
  if self.process_speed < 1 then
    self.process_speed = 1
  end

  -- check event callbacks
  self.callback = {}
  if param.callback then
    for k,v in pairs(param.callbacks or {}) do
      assert(type(v) == "function", "invalid value for event callback")
    end
    self.callback = param.callback
  end

  -- unplugging: init to everything plugged in
  self.unplug = {}
  for i = 1, param.nport do
     self.unplug[i] = {rx = false, tx = false}
  end
  -- init outputs
  self:set_nout(table.getn(param.out))
  self:defout(param.out)
  self.sendstate = "idle"

  -- init inputs
  for i = 1, param.nport do
     self:definp("in"..i)
  end

  -- input buffer and queue
--  self.q = queue:new(10)
--  self.q = bqueue(self)
  self.q = cqueue(10)
  self.inbuf = {n=0}
  self.loss={n=0}

  -- output queue
  self.outq={}
  for i = 1, self.nport do
    self.outq[i] = cqueue(20)
  end

  -- RSTP instance
  self.rstp = ieeebridge.rstp_bridge:new(self, param.nport, self.basemac)
  log:debug(string.format("RSTP created with %d ports", param.nport))
  self.rstp:setcallback("cb_learning", bridge.cb_learning)
  self.rstp:setcallback("cb_forwarding", bridge.cb_forwarding)
  self.rstp:setcallback("cb_portstate", bridge.cb_portstate)
  self.rstp:setcallback("cb_hardware_mode", bridge.cb_hardware_mode)
  self.rstp:setcallback("cb_tx_bpdu", bridge.cb_tx_bpdu)
  self.rstp:setcallback("cb_flush_fdb", bridge.cb_flush_fdb)

  -- init speed (pathcost) and duplex mode
  for i = 1, self.nport do
    self.rstp:set_speed(i, self.speed[i])
    self.rstp:set_duplex(i, self.duplex[i])
  end

  -- init one second timer
  self.cycle = 1 / SlotLength
  self.evtimer = event:new(self, EVCLOCK)
  self.evtimer.stat = 12345678

  -- input processing status
  self.inpstate = "idle"
  self.procstate = "idle"

  -- init input queue event
  self.evqueue = event:new(self, EVQUEUE)
  self.evqueue.stat = 12345678
  self.evprocess = event:new(self, EVPROCESS)
  self.evprocess.stat = 12345678

  -- init transmit event
  self.evsend = event:new(self, EVSEND)
  self.evsend.stat = 12345678

  -- finish definition before starting the bridge
  local rv = self:finish()

  -- start start delay timer
  self.started = false
  if (not self.start_delay) or (self.start_delay == 0) then
    log:debug(string.format("%s direct start", self.name))
    bridge.start(self)
  else
    self.evstart = event:new(self, EVSTART)
    self.evstart.stat = 12345678
    alarme(self.evstart, self.start_delay)
  end

  -- finished.
  return rv
end

local function set2bitmap(set)
  local s = ""
  for i,v in ipairs(set) do
    if set[i] > 0 then
      s = s .. "1"
    else
      s = s .. "0"
    end
  end
  return s
end

--- Start the bridge.
-- This routine initialises the RSTP protocoland starts a 
-- per bridge one second timer. In this way each bridge
-- instance has it's own notion of time.
function bridge:start()
  -- start the bridge
  for vid, prio in pairs(self.vlans) do
    log:debug(string.format("Starting bridge %s vlan=%d prio=%d memberset=%s",
			    self.name, vid, prio, set2bitmap(self.memberset[vid])))
    local err = self.rstp:start(vid, prio, set2bitmap(self.memberset[vid]))
    assert(err == nil, err)
  end  
  -- start one second timer
  alarme(self.evtimer, self.cycle)

  -- enable ports
  for i = 1,self.nport do
    self.rstp:enable_port(i, true)
  end
  self.started = true
end

--- Shutdown the bridge.
function bridge:shutdown()
  local err = self.rstp:shutdown()
  unalarme(self.evtimer)
  assert(not err, err)
end

--- Receive a data item.
-- Note, that each port can reside on a shared LAN segment. Hence
-- we may receive more BPDU's than inputs during a single time slot.
-- Must not be called by user scripts.
-- @param pd userdata Data item, that carries the actual payload.
-- @param idx number Input port number.
-- @return none.
function bridge:luarec(pd, idx)
  if self.unplug[idx+1].rx == true then
    -- Port has been unplugged - discard the frame
    log:debug(string.format("%s discard seq=%d port=%d vid=%d unplugged",
			    self.name, pd.seq, idx+1, pd.vid))
    pd.bpdu = nil
    pd:delete()
  else
     -- Port is plugged in - process the frame
    pd.portno = idx+1
    local len = pd:pdu_len()
    table.insert(self.inbuf, pd)
    --    log:debug(string.format("%s receive seq=%d port=%d vid=%d len=%d data=%s",
    --			    self.name, pd.seq, idx+1, pd.vid, len, stringhex(pd.bpdu)))
    log:debug(string.format("%s.p%d luarec: receive FROM %s.p%d seq=%d port=%d vid=%d len=%d data=%s",
			    self.name, idx+1, pd.from.name, pd.fromport, pd.seq, idx+1, pd.vid, len, stringhex(pd.bpdu)))
    pdulog:debug(string.format("%s.p%d ==> %s.p%d\n%s", 
			       pd.from.name, pd.fromport, self.name, idx+1,
			       tostring(decode(pd.bpdu, "bpdu"))))
    -- Start process inputs in late time slot 
    -- Make sure to send only 1 event per time slot
    if self.inpstate == "idle" then
      self.inpstate = "enqueue"
      alarml(self.evqueue, 0)
    end
  end
end

--- Late event handler.
-- Must not be called by user scripts.
-- @param ev userdata Event that caused the callback.
-- @return none.
function bridge:lualate(ev)
   --  log:debug(string.format("%s late timer (key=%d) at %s sec (%d)", 
   --			  self.name, ev.key, SimTimeReal, SimTime))
   local old_qlen = self.q:getlen()
   local inbuf = self.inbuf
   local n = table.getn(self.inbuf)
   repeat
      local p
      -- select a random input frame
      if (n > 1) then
	 p = yats.random(1,n)
      else
	 p = 1
      end
      -- enqueue the frame
      local pd = inbuf[p]
      local len = pd:pdu_len()
      log:debug(string.format("%s lualate: enqueue seq=%d port=%d vid=%d len=%d n=%d pd=%s",
			      self.name, pd.seq, pd.portno, pd.vid, len, n, tostring2(pd)))
      if self.q:enqueue(pd) == 0 then
	 -- queue is full: drop and count
	 log:error(string.format("%s eqneue failed.", self.name)) 
	 self.loss[pd.portno] = self.loss[pd.portno] + 1
	 pd:delete()
      end
      -- replace processed frame by another one to avoid replica.
      inbuf[p] = inbuf[n]
      -- one less in input
      table.remove(inbuf, n)
      n = n - 1
   until n == 0

   -- we are done: stop enqueuing but stay ready for new input data
   self.inpstate = "idle"
   
   -- serve one frame after processing time
   if self.procstate == "idle" then
     alarme(self.evprocess, self.process_speed)
     self.procstate = "process"
   end
end

--- Early event handler.
-- Must not be called by user scripts.
-- @param ev userdata Event that caused the callback.
-- @return none.
function bridge:luaearly(ev)
  if ev.key == EVSTART then
    -- Start delay
    log:debug(string.format("%s delayed start", self.name))
    self:start()
  elseif ev.key == EVCLOCK then
    -- Timer event
--    log:debug(string.format("%s early timer (key=%d) at %s sec (%d)", 
--			    self.name, ev.key, SimTimeReal, SimTime))
    self.rstp:one_second()
    alarme(ev, self.cycle)

  elseif ev.key == EVPROCESS then
--    log:debug(string.format("%s early timer (key=%d) at %s sec (%d)", 
--			    self.name, ev.key, SimTimeReal, SimTime))
    -- BPDU reception 
    -- get the next BPDU
    local pd = self.q:dequeue()
    tolua.cast(pd, "frame")
    -- decapsulate the BPDU
    local portno = pd.portno
    local bpdu = pd.bpdu
    local vid = pd.vid
    local len = pd:pdu_len()

    if self.pdurxlog[portno] == true and self.vidlog[vid] == true then
      pdulog:info(string.format("%s luaearly: receive seq=%d port=%d vid=%d len=%d data=%s",
				 self.name, pd.seq, portno, pd.vid, len, stringhex(pd.bpdu)))
      pdulog:info(string.format("\n%s", tostring(decode(pd.bpdu, "bpdu"))))
--      pdulog:debug(string.format("%s receive seq=%d port=%d vid=%d len=%d\nbpdu = %s",
--				self.name, pd.seq, portno, pd.vid, len, 
--				tostring(decode(pd.bpdu, "bpdu"))))
    end

    log:debug(string.format("%s process seq=%d port=%d vid=%d len=%d", 
			    self.name, pd.seq, portno, vid, len))
    if self.callback.rxbpdu then
      self.callback.rxbpdu("rxbpdu", {
			     node = self,
			     sender = pd.from,
			     senderport = pd.fromport,
			     receiver = self,
			     receiverport = portno,
			     vid = vid,
			     tick = SimTime,
			     time = SimTimeReal,
			     bpdu = pd.bpdu,
			     seq = pd.seq
			   })
    end
    -- we can now delete the container
    pd:delete()

    -- hand-over to STP instance - we do that at a certain speed.
    self.rstp:rx_bpdu(vid, portno, bpdu, len)

    -- Re-schedule if more data available
    if self.q:getlen() > 0 then
      alarme(ev, self.process_speed)
    else
      self.procstate = "idle"
    end
  elseif ev.key == EVSEND then
    -- Send max. one BPDU per port
    local more = 0
    for i = 1, self.nport do
      local q = self.outq[i]
      if q:getlen() > 0 then
	local pd = q:dequeue()
	if pd then
	  if self.pdutxlog[i] == true and self.vidlog[pd.vid] == true then
	    pdulog:info(string.format("%s transmit OUT seq=%d port=%d vid=%d len=%d data=%s", 
				      self.name, pd.seq, i, pd.vid, pd.len, stringhex(pd.bpdu)))
	  end
	  pd.suc:rec(pd, pd.shand)
	  if q:getlen() > 0 then
	    more = more + 1
	  end
	end
      end
    end
    -- Re-schedule transmitter if there are more BPDUs to transmit
    if more > 0 then
      alarme(ev, 1)
    else
      self.sendstate = "idle"
    end
  end
end

--- Callback from RSTP.
-- Start learning addresses - influences the bridge's ingress processing path.
-- @param rstp userdata Reference to bridge instance.
-- @param portno number The port for which this callback is valid.
-- @param vid number The VLAN ID for which this callback is valid (STP instance).
-- @param ena boolean True/false means learning on/off.
-- @return none.
function bridge.cb_learning(rstp, portno, vid, ena)
   log:info(string.format("%s cb_learning port=%d vid=%d ena=%d", rstp:get_parent().name, portno, vid, ena))
end

--- Callback from RSTP.
-- Start to forward data - influences the bridge's ingress and egress processing path.
-- @param rstp userdata Reference to bridge instance.
-- @param portno number The port for which this callback is valid.
-- @param vid number The VLAN ID for which this callback is valid (STP instance).
-- @param ena boolean True/false means: forwarding on/off.
-- @return none.
function bridge.cb_forwarding(rstp, portno, vid, ena)
  log:info(string.format("%s cb_forwarding port=%d vid=%d ena=%d", rstp:get_parent().name, portno, vid, ena))
end

--- Readable fdb flush type.
ieeebridge.s_flush_fdb_type = {
   "all_ports_exclude_this", "only_the_port"
}

--- Callback from RSTP.
-- Start to forward data - influences the bridge's ingress and egress processing path.
-- @param rstp userdata Reference to bridge instance.
-- @param portno number The port for which this callback is valid.
-- @param vid number The VLAN ID for which this callback is valid (STP instance).
-- @param ena boolean True/false means: forwarding on/off.
-- @return none.
function bridge.cb_flush_fdb(rstp, portno, vid, typ, reason)
  local self = rstp:get_parent()
  if self.statelog[portno] == true and self.vidlog[vid] then
    log:info(string.format("%s cb_flush_fdb port=%d vid=%d typ=%s reason=%s", 
			   rstp:get_parent().name, portno, vid, ieeebridge.s_flush_fdb_type[typ+1], reason))
  end
  if self.callback.flush then
    self.callback.flush("flush", {
			  node = self,
			  tick = SimTime,
			  time = SimTimeReal,
			  vid = vid,
			  typ = ieeebridge.s_flush_fdb_type[typ+1],
			  typ_n = typ,
			  port = portno,
			  reason = reason
			})
  end
end

--- Readable hardware mode.
ieeebridge.s_hardware_mode = {
  "STP disabled", "STP enabled"
}
--- Readable Spanning tree mode.
ieeebridge.s_stpmode = {
  "disabled", "enabled"
}
--- Readable Port state in short form.
ieeebridge.ss_portstate = {
  "Dis", "Blk", "Lrn", "Fwd", "Non", "Unk"
}
--- Readable Port state in long form.
ieeebridge.s_portstate = {
  "Disabled", "Discarding", "Learning", "Forwarding", "NoSTP", "Unknown"
}

--- Callback from RSTP.
-- Indicates change in the port state.
-- @param rstp userdata Reference to bridge instance.
-- @param portno number The port for which this callback is valid.
-- @param vid number The VLAN ID for which this callback is valid (STP instance).
-- @param state Indicates the state set by STP protocol instance.
-- @return none.
function bridge.cb_portstate(rstp, portno, vid, state)
  local self = rstp:get_parent()
  if self.statelog[portno] == true and self.vidlog[vid] == true then
    log:info(string.format("%s cb_portstate port=%d vid=%d state=%s (%d)", 
			   self.name, portno, vid, ieeebridge.s_portstate[state+1], state))
  end
  if self.callback.portstate then 
    self.callback.portstate("portstate", {
			      node = self,
			      tick = SimTime, 
			      time = SimTimeReal, 
			      vid = vid, 
			      port = portno, 
			      state_n = state,
			      state = ieeebridge.s_portstate[state+1]})
  end
end

--- Callback from RSTP.
-- Enable/disable STP - indicates to hardware, whether STP was enabled or disabled.
-- @param rstp userdata Reference to bridge instance.
-- @param vid number The VLAN ID for which this callback is valid (STP instance).
-- @param mode number The mode adjusted - 0=disable, 1 = enable.
function bridge.cb_hardware_mode(rstp, vid, mode)
  local self = rstp:get_parent()
  if self.statelog[portno] == true and self.vidlog[vid] == true then
    log:info(string.format("%s cb_hardware_mode vid=%d mode=%s (%d)", 
			   self.name, vid, ieeebridge.s_hardware_mode[mode+1], mode))
  end
end

--- Callback from RSTP.
-- Transmit a BPDU - sends a BPDU from STP protocol instance to the data plane.
-- @param rstp userdata Reference to bridge instance.
-- @param portno number The port for which this callback is valid.
-- @param vid number The VLAN ID for which this callback is valid (STP instance).
-- @param state Indicates the state set by STP protocol instance.
-- @return none.
function bridge.cb_tx_bpdu(rstp, portno, vid, bpdu, len)
  -- encapsulate the bpdu and send it
  -- get the peer partner of this link
  local self = rstp:get_parent()
  local suc, shand, err = self:getNext(portno) 
  if suc then
    if rstp:get_parent().unplug[portno].tx == true then
      -- unplugged: delete the frame
      log:debug(string.format("%s discard port=%d unplugged",
			      rstp:get_parent().name, portno))
    else
      -- plugged: transmit the frame
      local pd = frame:new(len)
      pd.bpdu = bpdu
      pd.from = rstp:get_parent()
      pd.fromport = portno
      pd.seq = seq_no
      seq_no = seq_no + 1
      pd.portno = portno
      pd.vid = vid
      pd.len = len
      if self.pdutxlog[portno] == true and self.vidlog[vid] == true then
	pdulog:info(string.format("%s transmit IN seq=%d port=%d vid=%d len=%d data=%s", 
				  self.name, pd.seq, portno, vid, len, stringhex(bpdu)))
	pdulog:debug(string.format("\n%s", tostring(decode(bpdu, "bpdu"))))
      end
      if self.pdutxlog[i] == true and self.vidlog[pd.vid] == true then
	pdulog:info(string.format("%s transmit OUT seq=%d port=%d vid=%d len=%d data=%s", 
				  self.name, pd.seq, i, pd.vid, pd.len, stringhex(pd.bpdu)))
      end
      -- Enqueue the BPDU into port's output queue
      local bridge = rstp:get_parent()
      local q = bridge.outq[portno]
      pd.suc = suc
      pd.shand = shand
      if self.callback.txbpdu then
	self.callback.txbpdu("txbpdu", {
			       sender = pd.from,
			       senderport = pd.fromport,
			       receiver = suc,
			       receiverport = shand + 1,
			       vid = vid,
			       tick = SimTime,
			       time = SimTimeReal,
			       bpdu = pd.bpdu,
			       seq = pd.seq
			     })
      end
      bridge.outq[portno]:enqueue(pd)
      if bridge.sendstate == "idle" then
	bridge.sendstate = "run"
	alarme(bridge.evsend, 1)
      end
    end
  else
    log:debug(string.format("%s discard port=%d not connected.",
			    rstp:get_parent().name, portno))
  end
--  collectgarbage()
end


local ud2t = toluaex.udata2table
local function __ud2t(ud)
  local t={}
  for k,v in pairs(ud['.get']) do
    local x = ud[k]
    if type(x) == "userdata" then
      t[k] = ud2t(x)
   else
      if type(x) == "string" then
	 t[k] = string.sub(x, 1, string.len(x)).."("..string.len(x)..")"
      else
	 t[k] = x
      end
    end
  end
  return t
end

--- Get STP state.
-- @param vid number Indicates the STP instance (with per VLAN STP).
-- @return Table with STP state.
function bridge:get_stpstate(vid)
  local statec = ieeebridge.stpmstate:new()
  local state = self.rstp:get_stpmstate(vid, statec.val)
  if state then
    return ud2t(state)
  else
    return nil
  end
end

--- Get Port state.
-- @param vid number Indicates the STP instance (with per VLAN STP).
-- @param portno number Indicates the port.
-- @return Table with port state for the STP instance.
function bridge:get_portstate(vid, portno)
   local statec = ieeebridge.portstate:new()
   statec.val.port_no = portno
   local state = self.rstp:get_portstate(vid, statec.val)
   if state then
     return  ud2t(state)
   else
     return nil
   end
end

---
-- Configuration items (names) for STP configuration.
-- The following fields for parameter <code>what</code> can be used in 
-- <code>yats.bridge:config_stp(vid, what, val)</code>.
-- @class table
-- @name stp_cfg_fields
-- @field 1 "stp_disable" (type: number 0(disable) or 1(enable))
-- @field 2 "bridge_priority" (type: number 0 to 32768)
-- @field 3 "max_age" - (type: number = time in s)
-- @field 4 "hello_time" - (type: number = time in s)
-- @field 5 "forward_delay" (type: number = time in s)
-- @field 6 "force_version" (type:  string)
-- @field 7 "hold_count" - (type: number = time in s, range: 1 to 10)
local stp_cfg_fields = {
   "stp_enable",
   "bridge_priority", 
   "max_age",
   "hello_time",
   "forward_delay",
   "force_version",
   "nil",
   "nil",
   "hold_count"
}
bridge.stp_cfg_fields = stp_cfg_fields
---
-- Configuration items (names) for port configuration.
-- @class table
-- @name port_cfg_fields
-- @field 1 "admin_port_path_cost" (type: number)
-- @field 2 "port_priority" (type: number)
-- @field 3 "admin_point2point" (type: number: 1 or 0)
-- @field 4 "admin_edge" (type: number 1 or 0)
-- @field 5 "migrate_check" (no value)
-- @field 6 "admin_non_stp" (type: number 1 or 0)
local port_cfg_fields = {
   "nil",
   "admin_port_path_cost",  
   "port_priority",
   "admin_point2point",
   "admin_edge",
   "migrate_check",
   "admin_non_stp"
}
bridge.port_cfg_fields = port_cfg_fields

-- Bridge configuration error messages.
local error_messages = {
   "STP_OK",                         
   "STP_Cannot_Find_Vlan",
   "STP_Imlicite_Instance_Create_Failed",
   "STP_Small_Bridge_Priority",
   "STP_Large_Bridge_Priority",
   "STP_Small_Hello_Time",
   "STP_Large_Hello_Time",
   "STP_Small_Max_Age",
   "STP_Large_Max_Age",
   "STP_Small_Forward_Delay",
   "STP_Large_Forward_Delay",
   "STP_Forward_Delay_And_Max_Age_Are_Inconsistent",
   "STP_Hello_Time_And_Max_Age_Are_Inconsistent",
   "STP_Vlan_Had_Not_Yet_Been_Created",
   "STP_Port_Is_Absent_In_The_Vlan",
   "STP_Big_len8023_Format",
   "STP_Small_len8023_Format",
   "STP_len8023_Format_Gt_Len",
   "STP_Not_Proper_802_3_Packet",
   "STP_Invalid_Protocol",
   "STP_Invalid_Version",
   "STP_Had_Not_Yet_Been_Enabled_On_The_Vlan",
   "STP_Cannot_Create_Instance_For_Vlan",
   "STP_Cannot_Create_Instance_For_Port",
   "STP_Invalid_Bridge_Priority",
   "STP_There_Are_No_Ports",
   "STP_Cannot_Compute_Bridge_Prio",
   "STP_Another_Error",
   "STP_Nothing_To_Do",
   "STP_LAST_DUMMY"
}
bridge.error_messages = error_messages

function bridge:errstr(err)
   if err == 0 or err == nil then
      return true, nil, err
   else
      return false, self.error_messages[err + 1], err
   end
end
  
--- Configure Spanning Tree FSM.
-- @param vid number Indicates the STP instance (with per VLAN STP).
-- @param what string Parameter name.
-- @param val any Parameter value.
-- @return error code.
function bridge:config_stp(vid, what, val)
   local index
   assert(self.started == true, 
	  string.format("Bridge `%s'  must start before configuration.", self.name))
   local cfg = ieeebridge.stpcfg:new()
   for k,v in ipairs(self.stp_cfg_fields) do
      if v == what then
	 index = what
	 mask = 2 ^ (k-1)
	 break
      end
   end
   cfg.val.field_mask = mask
   cfg.val[index] = val
   return self:errstr(self.rstp:set_stpm_cfg(vid, self.rstp.enabled_ports, cfg.val))
end

--- Configure Spanning Tree port FSMs.
-- @param portlist table List of ports to configure in the form: {1, 4, 2, etc}.
-- @param what string Parameter name.
-- @param val any Parameter value.
-- @return Error code and error message
function bridge:config_port(vid, portlist, what, val)
   local index, mask
   assert(self.started == true, 
	  string.format("Bridge `%s'  must start before configuration.", self.name))
   local cfg = ieeebridge.portcfg:new()
   for k,v in ipairs(self.port_cfg_fields) do
      if v == what then
	 index = what
	 mask = 2 ^ (k-1)
	 break
      end
   end
   cfg.val.field_mask = mask
   cfg.val[index] = val
   local s = 1
   local bm = 0
   for i,v in ipairs(portlist) do
      bm = bm + 2 ^ (v-1)
   end
   cfg:setbm(bm)
   -- cfg.val.port_bmp = bm
   -- return self:errstr(self.rstp:set_port_cfg(vid, self.rstp.enabled_ports, cfg.val))
   return self:errstr(self.rstp:set_port_cfg(vid, cfg.val))
end

--- Plug-in a bridge port.
-- @param portno number Bridge port to plug-in again, starting from 1.
-- @param dir string Direction: t=transmit, r=receive, a | rt = bidir.
--                              If omitted, bidirectional is assumed.
-- @return none.
function bridge:plug_in(portno, dir)
  dir = dir or "a"
  log:info(string.format("%s plug_in port=%d dir=%s", self.name, portno, dir))
  if string.find(dir, "[ra]") then
    self.unplug[portno].rx = false
  end
  if string.find(dir, "[ta]") then
    self.unplug[portno].tx = false
  end
end

--- Plug-out a bridge port.
-- Once plugged out, the STP doesn't send nor receive any BPDU any more.
-- @param portno number Bridge port to plug-out, starting from 1.
-- @param dir string Direction: t=transmit, r=receive, a | rt = bidir.
--                              If omitted, bidirectional is assumed.
-- @return none.
function bridge:plug_out(portno, dir)
  dir = dir or "a"
  log:info(string.format("%s plug_out port=%d dir=%s", self.name, portno, dir))
  if string.find(dir, "[ra]") then
    self.unplug[portno].rx = true
  end
  if string.find(dir, "[ta]") then
    self.unplug[portno].tx = true
  end
end

--- Notification Link UP.
-- @param portno number Bridge port with link up event.
-- @return none.
function bridge:linkUp(portno)
  assert(self.started == true, 
	 string.format("Bridge `%s'  must start before configuration.", self.name))
  self.rstp:linkUp(portno)
end

--- Notification Link DOWN.
-- @param portno number Bridge port with link up event.
-- @return none.
function bridge:linkDown(portno)
  assert(self.started == true, 
	 string.format("Bridge `%s'  must start before configuration.", self.name))
  self.rstp:linkDown(portno)
end

--- Modify link speed of a port.
-- @param portno number Bridge port with modified link speed.
-- @param speed number Speed in Mbit/s
-- @return none.
function bridge:speed(portno, speed)
  self.rstp:set_speed(portno, speed)
  self.rstp:change_port_speed(portno, speed)
end

--- Enable a bridge port.
-- Notify spanning trees of a functional port.
-- @param portno number Bridge port to enable.
-- @return none.
function bridge:enable_port(portno)
  log:info(string.format("%s: enable_port %d", self.name, portno))
  self.rstp:enable_port(portno, true)
end

--- Disable a bridge port.
-- Notify spanning trees of a non-functional port.
-- @param portno number Bridge port to disable.
-- @return none.
function bridge:disable_port(portno)
  log:info(string.format("%s: disable_port %d", self.name, portno))
  self.rstp:enable_port(portno, false)
end

local function inspectPort(obj, what, ...)
  if not arg or table.getn(arg) == 0 then 
    return next, {}, nil 
  end
  if what == "*n" then 
    return "PORT STATE Port "..arg[1] 
  end
  local statec = ieeebridge.portstate:new()
  statec.val.port_no = arg[1]
  local state = obj.rstp:get_portstate(arg[2] or 0, statec.val)
  local t = {
    { key = "port number", val = state.port_no},
    { key = "state", val = state.state},
    { key = "role", val = state.role},
    { key = "uptime", val = state.uptime},
    { key = "path cost", val = state.path_cost},
    { key = "designated bridge", 
      val = string.format("%04x-%6s", sprint_brid(state.designated_bridge)),
      modifier = nil },
    { key = "designated cost", val = state.designated_cost, modifier = nil},
    { key = "designated port", val = state.designated_port, modifier = nil},
    { key = "designated root", 
      val = string.format("%04x-%6s", sprint_brid(state.designated_root)) },
    { key = "oper_point2point", val = state.oper_point2point, modifier = nil},
    { key = "oper edge", val = state.oper_edge, modifier = nil},
    { key = "oper port path cost", val = state.oper_port_path_cost, modifier = nil},
    { key = "config bpdu receive count", val = state.rx_cfg_bdpu_cnt},
    { key = "rstp bpdu receive count", val = state.rx_rstp_bdpu_cnt},
    { key = "tcn bpdu receive count", val = state.rx_tcn_bdpu_cnt},
    { key = "topology changes", val = state.tc},
    { key = "bpdu transmit count", val = state.txCount}
  }
  return next, t, nil
end

--- Iterator which delivers information of the bridge.
-- Besides per bridge info, the iterator also returns more iterators
-- describing the ports of the bridge.
--@param what string - What to inspect: *n = name, *d = data.
--@return iterator.
function bridge:inspect(what, ...)
  local vid = arg[1] or 0
  if what == "*n" then 
    return "STP STATE" 
  end
  local statec = ieeebridge.stpmstate:new()
  local state = self.rstp:get_stpmstate(vid, statec.val)
  local rootport
  if state.root_port > 0 then rootport = state.root_port else rootport = "none" end
  local t = {
    { key = "name", val = self.name, modifier= nil},
    { key = "instance", val = state.vlan_id, modifier = nil},
    { key = "instance name", val = state.vlan_name, modifier = nil},
    { key = "bridge id", val = string.format("%04x-%s", sprint_brid(state.bridge_id)) ,
      modifier = nil
    },
    { key = "bridge priority", val =  state.bridge_id.prio, 
      modifier = function(v)
		   if not tonumber(v) then return nil, "Invalid type" end
		   return self:config_stp(0, "bridge_priority", tonumber(v))
		 end},
    { key = "designated root", val = string.format("%04x-%s", sprint_brid(state.designated_root)),
      modifier = nil
    },
    { key = "root port", val = rootport, modifier = nil},
    { key = "time since topology change", val = state.timeSince_Topo_Change,
      modifier = nil
    },
    { key = "max age", val = state.max_age, 
      modifier = function(v)
		   if not tonumber(v) then return nil, "Invalid type" end
		   return self:config_stp(0, "max_age", tonumber(v))
		 end
    },
    { key = "hello time", val = state.hello_time,
      modifier = function(v)
		   if not tonumber(v) then return nil, "Invalid type" end
		   return self:config_stp(0, "hello_time", tonumber(v))
		 end
    },
    { key = "forward delay", val = state.forward_delay,
      modifier = function(v)
		   if not tonumber(v) then return nil, "Invalid type" end
		   return self:config_stp(0, "forward_delay", tonumber(v))
		 end
    },
    { key = "topology change count", val = state.Topo_Change_Count, modifier = nil},
    { key = "root path cost", val = state.root_path_cost, modifier = nil},
  }
  for i = 1, self.nport do
    table.insert(t, { key = "PORT STATE", val = inspectPort, modifier = nil, arg={i, 0}})
  end
  return next, t, nil
end

--- Convert a bridge id to printable strings
-- @param id userdata - Reference to a bridge id.
-- @return prio and hex presentation of mac address (base mac).
function sprint_brid(id)
--  print(id, type(id), tolua.type(id))
  local prio, addr = brid2lua(id)
--  print("###", prio, stringhex(addr))
  return prio, stringhex(addr)
end

local sprint_brid = sprint_brid

--- Show STP state machine info in shell.
-- @param vid number - Spanning tree instance.
-- @return none.
function bridge:show_stp(vid)
  local statec = ieeebridge.stpmstate:new()
  local state = self.rstp:get_stpmstate(vid, statec.val)
  print()
  printf("Bridge: `%s' at time: %s sec (%d)\n", self.name, SimTimeReal, SimTime)
  printf("  Instance (VLAN):  %d `%s'\n", state.vlan_id, state.vlan_name)
  printf("  State:            %s (%d)\n", ieeebridge.s_stpmode[state.stp_enabled+1], state.stp_enabled)
  printf("  BridgeId:         %0x-%s\n", sprint_brid(state.bridge_id))
  printf("  Bridge Priority:  %d (0x%0x)\n", state.bridge_id.prio, state.bridge_id.prio)
  printf("  Designated Root:  %0x-%s\n", sprint_brid(state.designated_root))
  if state.root_port > 0 then x = state.root_port else x = "none" end
  printf("  Root Port:        %s\n", x)
  printf("  Last Topol. Chg.: %d sec ago\n", state.timeSince_Topo_Change)
  printf("  MaxAge:           %d\n", state.max_age)
  printf("  HelloTime:        %d\n", state.hello_time)
  printf("  Forward Delay:    %d\n", state.forward_delay)
  printf("  Topol. Chg. Cnt:  %d\n", state.Topo_Change_Count)
  printf("  Root Path Cost:   %d\n", state.root_path_cost)
end

--- Show port info in shell.
--@param vid number - Spanning tree instance.
--@param port_index number - Port number to show.
--@return none.
function bridge:show_port(vid, port_index)
  local statec = ieeebridge.stpmstate:new()
  local state = self.rstp:get_stpmstate(vid, statec.val)
  if not state then
    printf("Bridge: `%s' at time: %s sec (%d)\n", self.name, SimTimeReal, SimTime)
    printf("  Bridge is disabled\n")
  else
    printf("Bridge: `%s' at time: %s sec (%d)\n", self.name, SimTimeReal, SimTime)
    local prio, addr = brid2lua(state.bridge_id)
    printf("  BridgeId: %0x-%s\n", prio, stringhex(addr)) 
    if not port_index then
      for i = 1,self.nport do
	local statec = ieeebridge.portstate:new()
	statec.val.port_no = i
	local state = self.rstp:get_portstate(vid, statec.val)
	if state then
	  local s = "  "
	  if state.oper_point2point ~= 0 then s = s.." " else s = s.."*" end
	  if state.oper_edge ~= 0 then s = s.."E" else s = s.." " end
	  if state.oper_stp_neigb ~= 0 then s = s.."s" else s = s.." " end
	  s = s..string.format("%04x %3s ", state.port_id, ieeebridge.ss_portstate[state.state + 1])
	  if state.designated_root.prio > 0 then
	    s = s..string.format("%04x-%s", sprint_brid(state.designated_root))
	    s = s..string.format(" %04x-%s", sprint_brid(state.designated_bridge))
	    s = s..string.format(" %04x %c", state.designated_port, state.role)
	  end
	  print(s)
	else
	  print("     not present")
	end
      end
    else
      local statec = ieeebridge.portstate:new()
      statec.val.port_no = port_index
      local state = self.rstp:get_portstate(vid, statec.val)
      local t
      if state then
	t = ud2t(state)
      else
	t = nil
      end
      print(pretty(t))
    end
  end
end

--- Get spanning tree internals into a Lua table.
-- @param vid number - VLAN id.
-- @return table with spanning tree internals.
function bridge:get_stpm(vid)
  local statec = ieeebridge.stpmstate:new()
  local state = self.rstp:get_stpmstate(vid, statec.val)
  if state then
    return ud2t(state)
  else
    return nil
  end
end

--- Get port internals into a Lua table.
-- @param vid number - VLAN id.
-- @param port_index number - Port number.
-- @return table with port internals.
function bridge:get_port(vid, port_index)
  local statec = ieeebridge.portstate:new()
  statec.val.port_no = port_index
  local state = self.rstp:get_portstate(vid, statec.val)
  if state then
    return ud2t(state)
  else
    return nil
  end
end

--- Valid state machine names.
stp_mach_names = {
  topoch = true,
  migrate = true,
  p2p = true,
  pcost = true,
  info = true,
  roletrns = true,
  sttrans = true,
  transmit = true,
  portrec = true,
  brdec = true,
  rolesel = true,
  all = true
}

--- Control state machine tracing on a port.
-- Valid state machine names:
-- "topoch", "migrate", "p2p", "pcost", "info",
-- "roletrns", "sttrans", "transmit", "portrec",
-- "brdec", "rolesel", "all".
-- @param name string - Name of state machine.
-- @param vid number - VLAN id.
-- @param port_index number - Port number.
-- @param enadis number - Enable (1) or disable (0).
-- @return Reference to bridge.
function bridge:portTrace(name, vid, port_index, enadis)
  if type(enadis) == "boolean" then
    if enadis == true then enadis = 1 else enadis = 0 end
  end
  assert(stp_mach_names[name], string.format("State machine %s is unknown", name))
  self.rstp:set_port_trace(name, vid, port_index, enadis)
  return self
end

--- Control BPDU logging on a port.
-- @param dir string - 'r'=receive, 't'=transmit, 'rt'= both.
-- @param portlist table - List of ports to control.
-- @param enadis boolean - true or false.
-- @return Reference to bridge.
function bridge:portPduLog(dir, portlist, enadis)
  local enadis = enadis or false
   for _, v in ipairs(portlist or {}) do
    assert(v <= self.nport, "invalid port index in portlist")
    if string.find(dir, "r")  then
      self.pdurxlog[v] = enadis
    end
    if string.find(dir, "t") then
      self.pdutxlog[v] = enadis
    end
  end
  return self
end

--- Control state logging on a port.
-- @param portlist table - List of ports to control.
-- @param enadis boolean - Trace level:
--                         0,false:off, 1,true: normal, 2: extended.
-- @return Reference to bridge.
function bridge:stateLog(portlist, enadis)
  enadis = enadis or false
   for _,v in ipairs(portlist or {}) do
    assert(v <= self.nport, "invalid port index in portlist")
    self.statelog[v] = enadis
  end
  return self
end

--- Control VLAN logging.
-- @param vidlist table - List of vlans to control.
-- @param enadis boolean - Trace level:
--                         0,false: off, 1,true: normal, 2:extended.
-- @return Reference to the bridge.
function bridge:vidLog(vidlist, enadis)
  enadis = enadis or false
   for _,v in ipairs(vidlist or {}) do
    assert(self.vlans[v], "invalid vlan index in vidlist")
    self.vidlog[v] = enadis
  end
  return self
end

--- Control bridge Tracing.
-- @param enadis boolean - Trace level:
--                         0,false:off, 1,true: normal, 2: extended.
-- @return Reference to bridge.
function bridge:Trace(enadis)
  enadis = enadis or false
  if type(enadis) == "boolean" then
    if enadis == true then enadis = 1 else enadis = 0 end
  end
  self.debug = enadis
  return self
end

--- Control STP Tracing.
-- @param vid number - VLAN ID.
-- @param enadis boolean - Trace level:
--                         0,false:off, 1,true: normal, 2: extended.
-- @return Reference to bridge.
function bridge:stpmTrace(vid, enadis)
  enadis = enadis or false
  if type(enadis) == "boolean" then
    if enadis == true then enadis = 1 else enadis = 0 end
  end
  self.rstp:set_stpm_trace(vid, enadis)
  return self
end

--- Define user event callback functions.
-- Using this function it is possible to have a user defined function 
-- called in case of certain STP events. The callback receives a table with 
-- values from the simulator. The following values are always present: 
-- <code>node, port, vid, tick, time</code> 
-- @param cblist table - List of callback functions for the following
--                       events
-- <ul>
-- <li>rxbpdu - Reception of BPDU (when leaving the input queue).  
--   Additional params: <code>sender, receiver, senderport, receiverport, bpdu</code>. 
-- <li>txbpdu - Transmission of a BPDU (when entering the output queue). 
--   Additional params: <code>sender, receiver, senderport, receiverport, bpdu</code>. 
-- <li>portstate - State of a port changes. 
--   Additional params: <code>state and state_n</code>. 
-- <li>flush - Fdb is flushed. 
--   Additional params: <code>typ, typ_n, reason</code>. 
-- </ul>.
-- @return Reference to bridge.
function bridge:setCallback(cblist)
  self.callback = cblist
  return self
end
-- ================================================================
-- MIB
-- ================================================================
--[[
dot1d = {
   BaseBridgeAddress = {
      get = function(self) return self.basemac end
      set = nil,
   },
   BaseNumPorts = {
      get = function(self) return self.nport end,
      set = nil,
   },
   BasePortTable = {
      get = function(self) 
	       for i = 1, self.nport do
		  local t = {}
		  table.insert(t, self.dot1d.BasePortEntry.get(self, i))
	       end
	    end,
      set = nil
   },
   BasePortEntry = {
      get = function(self, index)
	       local t = {}
	       t.BasePort = index
	       t.BasePortIfIndex = index
	       t.BasePortCircuit = "0.0"
	       t.BasePortExceededDiscards = "todo"
	       t.BasePortMtuExceededDiscards = "todo"
	    end,
      set = nil
   },
   StpProtocolSpecification = {
      get = function(self) return 3, "ieee802.1d" end,
      set = nil
   },
   StpPriority = {
      get = function(self) 
	       local state = self:get_stpstate()
	       return state.bridge_id.prio
	    end,
      set = function(self, val)
	       return self:putmib(ieeebridge.BR_CFG_PRIO, val)
	    end
   },
   StpTimeSinceTopologyChange = {
      get = function(self)
	       local state = self:get_stpstate()
	       return state.bridge_id.timeSince_Topo_Change
	    end,
      set = nil
   },
   StpTopChanges = {
      get = function(self)
	       local state = self:get_stpstate()
	       return state.bridge_id.Topo_Change_Count
	    end,
      set = nil
   },
   StpDesignatedRoot = {get = nil, set = nil}
   StpRootCost = {}
   StpRootPort = {}
   StpMaxAge = {}
   StpHelloTime = {}
   StpHoldTime = {}
   StpForwardDelay = {}
   StpBridgeMaxAge = {}
   StpBridgeHelloTime = {}
}
]]

return yats
