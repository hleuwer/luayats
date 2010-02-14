-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: tcpip.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - TCPIP classes.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- Lua classes for TCP/IP simulation.
-----------------------------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

--==========================================================================
-- CBR Frame Source Object
--==========================================================================
_cbrframe = cbrframe
--- Definition of 'cbrframe' source class.
cbrframe = class(_cbrframe)

--- Constructor for class 'cbrframe'.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>delta<br>
--    Gap in ticks between frames. 
-- <li>len<br>
--    Length of the frames. 
-- <li>start_time (optional)<br>
--    Start time in ticks. Default: self.delta. 
-- <li>end_time (optional)<br>
--    End time in ticks. Default: -self.delta (never). 
-- <li>bytes (optional)<br>
--    Bytes to send. Default: -self.len (unlimited). 
-- <li>connid<br>
--    A generic connection id. 
-- <li>out<br>
--    Connection to successor; 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table - Reference to object instance.
function cbrframe:init(param)
  local self = _cbrframe:new()
  self.name=autoname(param)
  self.clname = "cbrframe"
  self.parameters = {
    delta = true, len = true, start_time = false, end_time = false,
    bytes = false, connid = false, out = true
  }
  self:adjust(param)

  -- 2. set object vars in yats object
  self.delta = param.delta
  self.pkt_len = param.len
  local tim = param.start_time or param.delta
  if tim == 0 then
    tim = 50 * math.mod(random(0,32767), 10000)
  end
  self.StartTime = tim
  self.EndTime = param.end_time or (0 - param.delta)
  self.bytes = param.bytes or (0 - param.len)
  self.connID = param.connid or 0

  -- 3. init output table
  self:defout(param.out)
  
  -- 3. init input table
  self:definp("ctrl")

  -- 4 to 6 can be summarised in a utility method finish()
  return self:finish()
end

--==========================================================================
-- TCPIP Sender Object
--==========================================================================
local function b2i(val)
  if val == true then return 1 else return 0 end
end
_tcpipsend = tcpipsend
--- Definition of 'tcpipsend'  class.
tcpipsend = class(_tcpipsend)

--- Constructor for class 'tcpipsend'.
-- Data received at the input is sent via TCP protocol to a receiving
-- object of class 'tcpiprecv'. The object tcpipsend implements the
-- start/stop protocol and implements the following TCP algorithms: 
--<br> - Slow start and congestion avoidance.
--<br>  - Silly window avoidance (RFC1122, section 4.2.3.4).
--<br>  - Nagle's algorithm (can be turned off, default: on) (RFC1122, section 4.2.3.4).
--<br>  - Karn's algorithm (no RTT measurement during retransmission).
--<br>  - Fast retransmission and recovery (can be turned off, default: on).
--<br>  - RTT measurement with TS option according to RFC 1323 (can be turned off,
--<br>    default: on)
--<br>.
-- Per packet sent, a processing delay of PROCTIM is included. The exact value is
-- a random value between PROCTIM and 1.1 times PROCTIM. This avoids
-- synchronisation effects between different TCP connections.
-- <br>
-- Timer for zero window probe: if the ACK opening the window again is lost,
-- then the connection falls asleep forever.
-- <br> 
-- The node has 2 inputs and 1 output. Input 0 receive the data and input 1 
-- provides a restart input for the start/stop protocol. The output carries the TCP
-- packets.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the object. Default: "objNN". 
-- <li>buf<br>
--    Upper bound of send buffer in bytes. 
-- <li>bstart (optional)<br>
--    Lower bound for start/stop protocol in bytes. Default is 4096 bytes
--    below upper bound or 0 if upper bound is < 4096 bytes. 
-- <li>mtu (optional)<br>
--    Maximum transmission unit in bytes. Default: 1500 bytes. 
-- <li>ts (optional)<br>
--    Timestamp option RFC1323 on/off. Default: true (on). 
-- <li>nagle (optional)<br>
--    Nagle algorithm on/off. Default: true (on). 
-- <li>phef (optional)<br>
--    Phase effect compensation on/off (proctim). Default: false (off). 
-- <li>fretr (optional)<br>
--    Fast retransmit/recover on/off. Default: true (on). 
-- <li>bitrate (optional, deprecated)<br>
--    Transmission bitrate in bit/s. Default: 149.76 Mbit/s (STM-1). 
-- <li>proctim (optional)<br>
--    Processing time in s. Default: 0.3 ms. 
-- <li>tick (optional)<br>
--    Time of TCP tick in s. Default: 500 ms. 
-- <li>rtomin (optional)<br>
--    Minimum duration of retransmission timer in s. Default: 1.5 s. 
-- <li>oqwm (optional)<br>
--    Output queue watermark for new data. The watermark  defines the 
--    threshold of the output buffer below which no new data is taken
--    from the input buffer any more. Default: 2 packets. 
-- <li>logretr (optional)<br>
--    Logging of retransmission to STDOUT on/off. Default: false (off). 
-- <li>rec<br>
--    Name of TCP receiver instance. 
-- <li>out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table - Reference to object instance.
function tcpipsend:init(param)
  local self = _tcpipsend:new()
  self.name=autoname(param)
  self.clname = "tcpipsend"
  self.parameters = {
    buf = true, bstart = false, mtu = false, nagle = false, phef = false, ph_ef = false,
    ts = false, fretr = false, bitrate = false, proctim = false, tick = false, rtomin = false,
    oqwm = false, logretr = false, rec = true, out = true
  }
  self:adjust(param)

  assert(param.buf > 0, self.name .. ": parameter 'buf' must be > 0.")
  self.max_input = param.buf
  if param.bstart then
    assert(param.bstart > 0, self.name .. ": parameter 'bstart' is invalid.")
    self.q_start = param.bstart
  else
    if self.max_input < 4096 then
      self.q_start = 0
    else
      self.q_start = self.max_input - 4096
    end
  end
  
  if param.mtu then
    assert(param.mtu > 0, self.name .. ": parameter 'mtu' must be > 0")
    assert(param.mtu < 65536, self.name .. ": parameter 'mtu' must be lower than 65536")
    self.MTU = param.mtu
  else
    self.MTU = 1500
  end

  self.do_timestamp = b2i(param.ts or true)
  self.nagleOff = b2i(param.nagle or false)
  self.ph_efOn = b2i(param.phef or param.ph_ef or false)
  self.doFastRetr = b2i(param.fretr or true)
  assert((not param.bitrate) or (param.bitrate > 0), 
	 self.name .. ": parameter 'bitrate' must be > 0")
  self.bitrate = (param.bitrate or 148.76 * 1e6)
  assert((not param.proctim) or (param.proctim > 0),
	 self.name .. ": parameter 'proctim' must be > 0")
  self.proc_time = self:secs_to_slots(param.proctim or 0.3/1000)
  assert((not param.tick) or (param.tick > 0),
	 self.name .. ": parameter 'tick' must be > 0")
  self.tick = param.tick or 0.5
  assert((not param.rtomin) or (param.rtomin > 0),
	 self.name .. ": parameter 'rtomin' must be > 0")
  self.rto_lb = param.rtomin or 1.5
  
  assert((not param.oqvm) or (param.oqvm > 0),
	 self.name ..": parameter 'oqwm' must be > 0.")
  self.procqThresh = param.oqwm or 2
  self.calcSendStopped = b2i(false)
  
  self.doLogRetr = b2i(param.logretr or false)
  
  self.rec_name = param.rec

  -- Outputs
  self:set_nout(table.getn(param.out))
  self:defout(param.out)

  -- Inputs
  self:definp("data")
  self:definp("start")
  self:definp("ack")

  return self:finish()
end

local _connect = root.connect
function tcpipsend:connect()
  self.ptrTcpRecv = sim:getobj(self.rec_name)
  _connect(self)
  self:connectact(self.ptrTcpRecv)
end

--==========================================================================
-- TCPIP Receiver Object
--==========================================================================
_tcpiprec = tcpiprec
--- Definition of 'tcpipsend'  class.
tcpiprec = class(_tcpiprec)
tcpiprecv = tcpiprec

--- Constructor for class 'tcpiprecv'.
-- The tcpiprecv object implements a TCPIP receiver. It operates in 
-- cooperation with the object tcpipsend. It advertises it's window size during 
-- connection setup to the sender. 
--<br> The object implements the start/stop prototol at the output. The successor 
-- may stop the receiver which results in a closing window towards the tcpip 
-- sender. If a data object is received at the tcpip's 'start' input it resumes 
-- data transfer to the successor.
-- <br>
-- The node has 2 inputs and 2 outputs. The input 'data' receives the TCP 
-- frames. Input 'start' receives  messages to resume data transfer to the
-- successor.
--<br> Output 0 forwards the data and output 1 sends acknowledge packets to
-- the TCPIP sender.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>wnd<br>
--    Receive window size. 
-- <li>ackdel (optional)<br>
--    Delay of delayed ACKs in ms. Default: 200 ms. 
-- <li>phef (optional)<br>
--    Phase effect compensation on/off (proctim). Default: false (off). 
-- <li>iackdel (optional)<br>
--    Delay of immediate ACKs in s. Default: same as proctim. 
-- <li>proctim (optional)<br>
--    Processing time in s. Default: 0.3 ms. 
-- <li>keepalive (optional)<br>
--    Keepalive timer in s. Default: no timer. 
-- <li>out<br>
--    Connection to successor. 
--    Format: {{"name-of-data-successor", "input-pin-of-data-successor"}
--             {"name-of-tcpisend", "InpAck"}}. 
-- </ul>.
-- @return table - Reference to object instance.
function tcpiprec:init(param)
  local self = _tcpiprec:new()
  self.name=autoname(param)
  self.clname = "tcpiprec"
  self.parameters = {
    wnd = true, proctim = false, ackdel = false, out = true, keepalive = false,
    iackdel = false
  }

  self:adjust(param)
  assert(param.wnd > 0, self.name .. ": parameter 'wnd' must be > 0.")
  self.max_win = param.wnd
  self.wnd = self.max_win
  assert((not param.proctim) or (param.proctim > 0),
	 ": parameter 'proctim' must be > 0.")
  self.proctim_secs = param.proctim or 0.3 / 1000
  assert((not param.ackdel) or (param.ackdel > 0),
	 ": parameter 'ackdel' must be > 0.")
  self.ackdel_secs = (param.ackdel or 200 / 1000)
  self.ph_efOn = b2i(param.phef or false)
  assert((not param.iackdel) or (param.iackdel >= 0),
	 ": parameter 'iackdel' must be >= 0.")
  self.iackdel_secs = param.iackdel or self.proctim_secs
  assert((not param.keepalive) or (param.keepalive >= 0),
	 ": parameter 'keepalive' must be >= 0.")
  self.keepalive_secs = param.keepalive or 0

  -- Outputs
  self:set_nout(table.getn(param.out))
  self:defout(param.out)

  -- Inputs
  self:definp("data")
  self:definp("start")

  return self:finish()
end

--==========================================================================
-- TERMSTRSTP Object
--==========================================================================
_termStartStop = termStartStop
--- Definition of 'tcpipsend'  class.
termStartStop = class(_termStartStop)

--- Constructor for class 'termStartStop'.
-- The termStartStop object simply forwards data and terminates
-- the start/stop protocol of Luayats. If the node has been stopped
-- by its successor, but the node at the input continues to send,
-- a buffer overflow occurs. Data transmission resumes, when a 
-- data object is received on input 'start'.
-- @param param table - Parameter list
-- <ul>
-- <li>name (optional)<br>
--    Name of the display. Default: "objNN". 
-- <li>nbuf<br>
--    Buffer size. Default is 100. 
-- <li>out<br>
--    Connection to successor. 
--    Format: {"name-of-successor", "input-pin-of-successor"}. 
-- </ul>.
-- @return table - Reference to object instance.
function termStartStop:init(param)
  local self = _tcpipsend:new()
  self.name=autoname(param)
  self.clname = "termStartStop"
  self.parameters = {
  }
  self:adjust(param)
  self.q:setmax(param.nbuf or 100)

  -- Outputs
  self:defout(param.out)

  -- Inputs
  self:definp("in")
  self:definp("start")

  return self:finish()
end

return yats

