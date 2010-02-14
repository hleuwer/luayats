-----------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: n23.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - N23 Credit Based Flow control scheme in Lua
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- This module implements the N23 Credit Based Flow Control scheme. 
-- All logic is implemented in Lua. So it's not as fast as you might expect.
-----------------------------------------------------------------------------------
require "yats.core"
require "yats.object"

module("yats", yats.seeall)

----------------------------------------------------------
-- Logging for this module
----------------------------------------------------------
local log = logging.console("PTP %level %simtime (%realtime) %message\n")
log:setLevel("DEBUG")


----------------------------------------------------------
-- Constants
----------------------------------------------------------
local EVCUP, EVAPP, EVREC, EVSEND, EVMEAS = 1, 2, 3, 4, 5
local RADIO_IDLE_LEN = 8
----------------------------------------------------------
-- Helpers
----------------------------------------------------------
local fmt = string.format

-- Time to slots
function t2s(t)
  return t / yats.SlotLength
end

-- Slots to time
function s2t(s)
  return s * yats.SlotLength
end

-- Rate to slots
function r2s(r, n)
   return (n * 8) / (r * yats.SlotLength)
end

-- Wrapper for a data or frame queue. 
-- We need this to avoid garbage collection of userdata
-- objects allocated via Lua and enqueued in a C-level queue.
local cqueue = class()

function cqueue.init(self)
   self.q = uqueue:new()
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

local cells, seq = {}, 1
local function newcell(vci)
  local pd = cell:new(vci)
  pd.seq = seq
  seq = seq + 1
  cells[pd] = true 
  return pd
end

local function delcell(pd)
  if cells[pd] then
    cells[pd] = nil
    pd:delete()
  else
    log:debug("already deleted")
  end
end

local function lock(c) cells[c] = true end
local function unlock(c) cells[c] = nil end

---------------------------------------------------------------
-- PTP - TC Layer
---------------------------------------------------------------
tcptp = class(lua1out)

-- Constructor
function tcptp:init(param)
   local self = lua1out:new()
   self.name = autoname(param)
   self.clname = "tcptp"
   self.params = {
      name = false,
      rate = true,
      segsize = true
   }
   self:adjust(param)
   self.rate = param.rate
   self.segsize = param.segsize
   self.cycle = r2s(param.rate, param.segsize)
   self.q = cqueue()
   self.ptpseq = 1
   self.counter = 0
   self.sendtoline = false
   -- Pins
   self:definp("in")
   self:defout(param.out)
   -- Events
   self.sendev = event:new(self, EVSEND)
   self.sendev.stat = 12345678
   self.recev = event:new(self, EVREC)
   self.recev.stat = 12345678
   return self:finish()
end

-- Receive
function tcptp:luarec(pd, idx)
   if idx == 0 then
      tolua.cast(pd, "cell")
      self.inp_buf = pd
      pd.ptpseq = self.ptpseq
      self.ptpseq = self.ptpseq + 1
      alarml(self.recev, 0)
   end
end

-- Early Events
function tcptp:luaearly(ev)
   if ev.key == EVSEND then
      if self.q:getlen() > 0 then
	 local pd = self.q:dequeue()
	 tolua.cast(pd, "cell")
	 log:debug(fmt("%s: segment send: chnl=%d len=%d pno=%d fno=%d sno=%d ls=%s",
		       self.name, pd.vci, pd.len, pd.ptpseq, pd.fno, pd.seqno, tostring(pd.last)))
	 local suc, shand = self:getNext()
	 local rv = suc:rec(pd, shand)
      else
	 -- No more data - stop sending
	 self.sendtoline = false
      end
      if self.sendtoline == true then
	 -- Reschedule sender if necessary
	 alarme(self.sendev, self.cycle)
      end
   end
end

-- Late Event
function tcptp:lualate(ev)
   if ev.key == EVREC then
      -- Data received
      local pd = self.inp_buf
--      print("#1#", pd.connID, pd.fno, pd.seqno)
      log:debug(fmt("%s: segment receive: channel=%d fno=%d sno=%d ls=%s",
		    self.name, pd.vci, pd.fno, pd.seqno, tostring(pd.last)))
      self.q:enqueue(pd)
      self.counter = self.counter + 1
      -- Start sending if not yet running
      if self.sendtoline == false then
	 self.sendtoline = true
	 alarme(self.sendev, 1)
      end
   end
end

---------------------------------------------------------------
-- Scheduler
---------------------------------------------------------------
scheduler = class(lua1out)

-- Constructor
function scheduler:init(param)
  local self = lua1out:new()
  self.name = autoname(param)
  self.clname = "scheduler"
  self.params = {
     name = false,
     cycle = false,
     segsize = false,
     n2 = true,
     n3 = true,
     chno = true,
     chtype = false
  }

  self:adjust(param)

  self.cycle = param.cycle or 1
  self.segsize = param.segsize or 128
  self.n2 = param.n2
  self.n3 = param.n3
  self.chtype = param.chtype or "packet"
  self.chno = param.chno
  self.state = "send"
  self.vs = 0
  self.q = cqueue()
  self.counter = 0
  -- ival1: counts received credit segments 
  self.ival1 = 0
  self.seqno = 1
  self.sendtoline = false

  -- Initial credit: N2 + N3
  self.credit_balance = self.n2 + self.n3
  log:info(fmt("%s: init: cb=%d n2=%d n3=%d cycle=%d", self.name, self.credit_balance, self.n2, self.n3,
	 self.cycle))

  -- Pins
  self:definp("in")
  self:definp("credit")
  self:defout(param.out)
  
  -- Events
  self.sndev = event:new(self, EVSEND)
  self.sndev.stat = 12345678
  self.recev = event:new(self, EVREC)
  self.recev.stat = 12345678
  return self:finish()
end

-- Receive
function scheduler:luarec(pd, idx)
  if idx == 0 then
     ----------------------------------------------
     -- data frame
     ----------------------------------------------
     tolua.cast(pd, "frame")
     self.inp_buf = pd
     pd.seqno = self.seqno
     self.seqno = self.seqno + 1
     log:debug(fmt("%s: frame rec: fno=%d\n", self.name, pd.seqno))
     -- Lock the data to avoid GC on it
     lock(pd)
     alarml(self.recev, 0)
  elseif idx == 1 then
     ----------------------------------------------
     -- credit update
     ----------------------------------------------
     tolua.cast(pd, "cell")
     self.ival1 = self.ival1 + 1
     -- Update credit balance
     self.credit_balance = self.n2 + self.n3 - (self.vs - pd.credit)
     log:debug(fmt("%s: credit update: cred=%d cbal=%d n2=%d n3=%d vs=%d seq=%d", 
		   self.name,  pd.credit,  
		   self.credit_balance, 
		   self.n2, self.n3, self.vs, pd.seq))
     -- Delete this item
     delcell(pd)
  else
     -------------------------------------------------------
     -- shouldn't go here !
     -------------------------------------------------------
     error("What is this ?")
  end
end

-- Early Event
function scheduler:luaearly(ev)
   if ev.key == EVSEND then
      ----------------------------------------------
      -- SEND event
      ----------------------------------------------
      -- Do we have credit to send data?
      if self.credit_balance > 0 then
	 -- yes: do we also have data available?
	 if self.q:getlen() > 0 then
	    -- yes: get data and send
	    local pd = self.q:dequeue()
	    tolua.cast(pd, "cell")
	    -- For delay measurement
	    pd.tsend = SimTime
	    -- Get successor and send
	    log:debug(fmt("%s: data segment send: cb=%d vs=%d len=%d fno=%d sno=%d ls=%s",
			  self.name, self.credit_balance, self.vs, pd.len, pd.fno, pd.seqno, 
			  tostring(pd.last)))
	    local suc, shand = self:getNext()
	    local rv = suc:rec(pd, shand)
	    -- Update the credit balance and counter Vs
	    self.credit_balance = self.credit_balance - pd.len
	    self.vs = self.vs + pd.len
	 else
	    if self.chtype == "modem" then
	       -- insert idle cell as long as we have credit to do so
	       local pd = newcell(self.chno)
	       -- For delay measurement
	       pd.tsend = SimTime
	       pd.last = true
	       pd.seqno = -1
	       pd.fno = -1
	       pd.len = RADIO_IDLE_LEN
	       -- Get successor and send
	       log:debug(fmt("%s: idle segment send: cb=%d vs=%d len=%d fno=%d sno=%d ls=%s",
			     self.name, self.credit_balance, self.vs, pd.len, pd.fno, pd.seqno, 
			     tostring(pd.last)))
	       local suc, shand = self:getNext()
	       local rv = suc:rec(pd, shand)
	       -- Update the credit balance and counter Vs
	       self.credit_balance = self.credit_balance - pd.len
	       self.vs = self.vs + pd.len
--	       delcell(pd)
	    else
	       -- no data in non-modem configuration => stop transmit
	       self.sendtoline = false
	    end
	 end
      end
      -- Reschedule if required
      if self.sendtoline == true then
	 alarme(self.sndev, self.cycle)
      end
   else
      error("What is this ?")
   end
end

-- Late Event
function scheduler:lualate(ev)
   if ev.key == EVREC then
      local pd = self.inp_buf
      log:debug(fmt("%s: frame receive: connid=%d flen=%d fno=%d", self.name, pd.connID, pd.frameLen, pd.seqno))
      local len = pd.frameLen
      local seqno = 1
      -- Segment frame and enqueue single cells
      while len > 0 do
	 local seg = newcell(pd.connID)
	 seg.seqno = seqno
	 seg.fno = pd.seqno
	 seqno = seqno + 1
	 if len > self.segsize then
	    seg.len = self.segsize
	    seg.last = false
	 else
	    seg.len = len
	    seg.last = true
	 end
	 len = len - seg.len
	 self.q:enqueue(seg)
      end
      self.counter = self.counter + 1
      if self.sendtoline == false then
	 self.sendtoline = true
	 alarme(self.sndev, 1)
      end
      -- forget the original frame
      unlock(pd)
      pd:delete()
   else
      error("What is this ?")
   end
end

---------------------------------------------------------------
-- Receiver
---------------------------------------------------------------
receiver  = class(lua1out)

-- Constructor
function receiver:init(param)
  local self = luaxout:new()
  self.name = autoname(param)
  self.clname = "receiver"
  self.params = {
     name = false,
     rtt = true,
     rate = true,
     n2 = true,
     chunksize = true,
     meantime = false,
     watermark = true,
  }
  self:adjust(param)

  self.rtt = param.rtt
  self.n2 = param.n2
  self.watermark = param.watermark
  self.chunksize = param.chunksize

  self.ival0 = 0
  self.lost = 0
  self.counter = 0
  self.vr = 0
  self.ur = 0
  self.seq = 1
  self.buf = {}
  self.underrun = 0
  self.ival3 = 0
  self.oldival3 = 0
  self.ival4 = 0
  self.rate = param.rate
  self.tupdate = self.n2 * 8/ self.rate
  self.n3 = self.rtt * self.rate / 8
  self.cupcycle = t2s(self.tupdate)
  self.appcycle = r2s(self.rate, self.chunksize)
  self.meantime = param.meantime or 100e-9
  self.meancycle = t2s(param.meantime)
  self.meanrate = 0
  self.meanrateold = 0
  self.qlen = 0
  self.sendtoline = false
  self.ival3old = 0
  self.maxbuf = 0
  log:debug(fmt("%s init ratemeas: meantime=%.4E, meancycle=%d",
	       self.name, self.meantime, self.meancycle)) 
  self.nmax = self.n2 + self.n3
  log:info(fmt("%s init scheduler: n2=%d n3=%d nmax=%d tupdate=%.4E rate=%.4E meantime=%.4E",
	       self.name, self.n2, self.n3, self.nmax, self.tupdate, self.rate, self.meantime))
  self.q = cqueue()
  self:definp("in")
  self:set_nout(table.getn(param.out))
  self:defout(param.out)

  -- 2 events for line and credit update
  self.cupev = event:new(self, EVCUP)
  self.cupev.stat = 12345678
  self.appev = event:new(self, EVAPP)
  self.appev.stat = 12345678
  self.recev = event:new(self, EVREC)
  self.recev.stat = 12345678
  self.measev = event:new(self, EVMEAS)
  self.measev.stat = 12345678

  -- Cyclic timer for credit update
  alarme(self.cupev, self.cupcycle)
  -- Cyclic timer for rate measurement
  alarme(self.measev, self.meancycle)

  return self:finish()
end

-- Receiver
function receiver:luarec(pd, idx)
   -- Check on overflow
   if self.ival0 >= self.nmax then
      log:warn(fmt("%s: overflow bf=%d nmax=%d n2=%d n3=%d", 
		   self.name, self.ival0, self.nmax, self.n2, self.n3))
      -- Overflow
      self.lost = self.lost + 1
      unlock(pd)
      pd:delete()
   else
      -- Put segment into input buffer
      self.inp_buf = pd
      tolua.cast(pd, "cell")
      pd.treceive = SimTime
      self.ival3 = self.ival3 + pd.len
      -- Process in late slot
      alarml(self.recev, 0)
   end
   return yats.ContSend
end

-- Early Event
function receiver:luaearly(ev)
  if ev.key == EVCUP then
     ------------------------------------------------
     -- CUP event - send the credit
     ------------------------------------------------
     local pd = newcell(self.vci)
     pd.credit = self.vr
     pd.seq = self.seq
     log:debug(fmt("%s: credit send cred=%d s=%d", self.name, pd.credit, pd.seq))
     -- Get successor and send on output 0 (credit out)
     local suc, shand = self:getNext(1)
     suc:rec(pd, shand)
     -- Reschedule Credit
     alarme(self.cupev, self.cupcycle)

  elseif ev.key == EVAPP then
     ------------------------------------------------
     -- Line event - Do we have data ?
     ------------------------------------------------
     if self.q:getlen() > 0 then
	local chunk = self.q:dequeue()
	tolua.cast(pd, "cell")
	self.qlen = self.qlen - chunk.len
	local x = self.last
	self.last = chunk.lastseg
	self.fno = chunk.fno
	self.sno = chunk.sno
	self.cno = chunk.seqno
	-- update buffer fill level
	self.ival0 = self.ival0 - chunk.len
	-- update Vr
	self.vr = self.vr + chunk.len
	log:debug(fmt("%s: chunk send: bf=%d vr=%d ur=%d cnt=%d fno=%d sno=%d cno=%d ls=%s lc=%s", 
		      self.name, self.ival0, self.vr, self.ur, self.counter, 
		       chunk.fno, chunk.sno, chunk.seqno, tostring(chunk.lastseg), tostring(chunk.last)))
	-- Get successor and send on output 1 (data out)
	local suc, shand = self:getNext(2)
	-- Unlock this segment
	unlock(chunk)
	suc:rec(chunk, shand)
     else
	-- FIFO is now empty! stop sending
	self.sendtoline = false
	log:debug(fmt("%s: stop sending", self.name))
	if self.last == false then
	   log:error(fmt("%s: underrun at frame=%d segment=%d chunk=%d", 
			 self.name, self.fno, self.sno, self.cno))
	   self.underrun = self.underrun + 1
	end
	self.last = nil
     end
     -- Reschedule Line
     if self.sendtoline == true then
	alarme(self.appev, self.appcycle)
     end
     
  elseif ev.key == EVMEAS then
     ------------------------------------------------
     -- Measurement event
     ------------------------------------------------
     self.meanrate = (self.ival3 - self.ival3old) * 8 / self.meantime
     self.ival3old = self.ival3
     self.ival4 = (self.meanrateold + self.meanrate) / 2
     self.meanrateold = self.meanrate
     log:debug(fmt("%s: meanrate: meanold=%.4E mean=%.4E ival3=%d",
		  self.name, self.meanrateold, self.meanrate, self.ival3))
     alarme(self.measev, self.meancycle)
  end
end

-- Late Event
function receiver:lualate(ev)
   if ev.key == EVREC then
      local pd = self.inp_buf
      local len = pd.len
      -- Update buffer fill level
      self.ival0 = self.ival0 + len
      if self.ival0 > self.maxbuf then
	 self.maxbuf = self.ival0
      end
      -- Update Ur
      self.ur = self.ur + len
      -- Count segment
      self.counter = self.counter + 1
      log:debug(fmt("%s: segment receive: bf=%d len=%d ur=%d vr=%d fno=%d sno=%d last=%s dt=%d", 
		    self.name, self.ival0, len, self.ur, self.vr, pd.fno, pd.seqno, tostring(pd.last),
	      pd.treceive - pd.tsend))
      local cno = 1
      while len > 0 do
	 local chunk = newcell(pd.vci)
	 chunk.lastseg = pd.last
	 chunk.fno = pd.fno
	 chunk.sno = pd.seqno
	 chunk.seqno = cno
	 cno = cno + 1
	 if len > self.chunksize then
	    chunk.len = self.chunksize
	    chunk.last = false
	 else
	    chunk.len = len
	    chunk.last = true
	 end
	 len = len - chunk.len
	 self.qlen = self.qlen + chunk.len
	 self.q:enqueue(chunk)
      end
      if self.sendtoline == false then
	 if self.qlen > self.watermark or pd.last == true then
	    self.sendtoline = true
	    alarme(self.appev, 1)
	 end
      end
      -- Delete the segment once we have chunked it.
      delcell(pd)
  end
end

--------------------------------------------------------------
-- Other methods
--------------------------------------------------------------
function receiver:setlinerate(rate, recalc)
   log:info(fmt("%s: old linerate: rate=%.3E n2=%d n3=%d nmax=%d tupdate=%.3E", 
		self.name, self.rate, self.n2, self.n3, self.nmax, self.tupdate))
   self.rate = rate
   local n3 = self.rtt * self.rate / 8
   local n2 = self.tupdate * self.rate / 8
   self.appcycle = r2s(self.rate, self.chunksize)
   log:info(fmt("%s: new linerate: rate=%.3E n2=%d n3=%d nmax=%d tupdate=%.3E", 
		self.name, self.rate, n2, n3, self.nmax, self.tupdate))
   return n2, n3
end

function receiver:setnmax(n)
   log:info(fmt("%s: new nmax: %d", self.name, n))
   self.nmax = n
end

function receiver:getnmax(n)
   return self.nmax
end

function receiver:setn2(n)
   local _n = self.n2
   self.n2 = n
   self.tupdate = self.n2 * 8 / self.rate
   self.cupcycle = t2s(self.tupdate)
   log:info(fmt("%s: new n2: n2=%d (%d) tupdate=%.4E cupcycle=%d", self.name, self.n2, _n, self.tupdate, self.cupcycle))
end

function receiver:align(rate, updatecup)
   self.n3 = self.rtt * rate / 8
   if not updatecup then
      self.n2 = self.tupdate * rate / 8
      self.nmax = self.n2 + self.n3
   else
      self.n2 = self.nmax - self.n3
      self.tupdate = self.n2 * 8 / rate
      self.cupcylce = t2s(self.tupdate)
   end
   log:info(fmt("%s: align to new rate: update CUP=%s rate=%.4E n2=%d tupdate=%.4E", self.name, tostring(updatecup), 
		self.rate, self.n2, self.tupdate))
   return self.n2, self.n3
end

function scheduler:setn2n3(n2, n3)
   local _n2, _n3 = self.n2, self.n3
   self.n2 = n2
   self.n3 = n3
   log:info(fmt("%s: new n2+n3: n2=%d (%d) n3=%d (%d)\n", self.name, self.n2, _n2, self.n3, _n3))
end

function scheduler:getn2n3()
   return self.n2, self.n3
end

function scheduler:getcreditcount()
   return self.ival1
end


