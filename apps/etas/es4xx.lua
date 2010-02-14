require "yats"
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
local EVSAMPLE, EVREC, EVSEND, EVSCHED = 1, 2, 3, 4

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
-- objects that have been allocated via Lua and are enqueued 
-- in a C-level queue, where they are temporarily not visible
-- to Lua objects. 
-----------------------------------------
-- Limited queue
-----------------------------------------
local cqueue = class()

function cqueue.init(self, len)
   self.q = queue:new(len)
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

-----------------------------------------
-- Unlimited queue
-----------------------------------------
local ucqueue = class()

function ucqueue.init(self)
   self.q = uqueue:new()
   self.elems = {}
   return self
end

function ucqueue:enqueue(pd)
  self.elems[pd] = true
  self.q:enqueue(pd)
end

function ucqueue:dequeue()
  local pd = self.q:dequeue()
  self.elems[pd] = nil
  return pd
end

function ucqueue:getlen()
  return self.q:getlen()
end

---------------------------------------------------------------
-- ES4xx data source
---------------------------------------------------------------
es4xx = class(lua1out)

-----------------------------------------
-- Constructor
-----------------------------------------
function es4xx:init(param)
   local self = lua1out:new()
   self.name = autoname(param)
   self.clname = "ex4xx"
   self.params = {
      name = false,
      samprate = true,
      produce = true,
      winsize = true,
      cycle = true,
      qlen = true
   }
   self:adjust(param)
   -- Parameter processing
   self.samprate = param.samprate -- sampling rate
   self.produce = param.produce   -- data producing function
   self.cycle = param.cycle       -- send cycle, work conserving on eth
   self.winsize = param.winsize
   self.qlen = param.qlen

   -- Flow control window: initially fully open
   self.wincount = self.winsize

   -- The queue
   self.q = cqueue(self.qlen)

   -- Counters
   self.sampcount = 0
   self.incount = 0
   self.outcount = 0
   self.loss = 0
   self.seqno = 1
   
   -- Pins
   self:definp("in")
   self:definp("fc")
   self:defout(param.out)

   -- Events
   self.sendev = event:new(self, EVSEND)
   self.sendev.stat = 12345678
   self.recev = event:new(self, EVREC)
   self.recev.stat = 12345678
   self.sampev = event:new(self, EVSAMPLE)
   self.sampev.stat = 12345678

   -- Done
   return self:finish()
end

-----------------------------------------
-- Receive a frame from frame generator
-----------------------------------------
function es4xx:luarec(pd, idx)
   print("REC:", pd, idx)
   if idx == 0 then
      ---------------------------------
      -- data input
      ---------------------------------
      tolua.cast(pd, "cell")
      -- assign seq # and put into input buffer
      self.inp_buf = pd
      pd.seqno = self.seqno
      self.seqno = self.seqno + 1
      self.incount = self.incount + 1
      -- notify Enqueue Engine in late event handler
      alarml(self.recev, 0)
   elseif idx == 1 then
      ---------------------------------
      -- flow control ack
      ---------------------------------
      tolua.cast(pd, "cell")
      self.wincount = self.wincount + self.winsize/2
   end
end

-----------------------------------------
-- Early Events: Scheduling
-----------------------------------------
function es4xx:luaearly(ev)
   if ev.key == EVSEND then
      if self.wincount > 0 then
	 -- window open
	 if self.q:getlen() > 0 then
	    local pd = self.q:dequeue()
	    tolua.cast(pd, "cell")
	    local suc, shand = self:getNext()
	    -- send it in early time slot; close window a bit
	    self.wincount = self.wincount - 1
	    self.outcount = self.outcount + 1
	    local rv = suc:rec(pd, shand)
	    alarme(self.sendev, self.cycle)
	 end
      end
   end
end

-----------------------------------------
-- Late Events: fired by frame reception or sample
-----------------------------------------
function es4xx:lualate(ev)
   if ev.key == EVREC then
      -- frame receive event
      local pd = self.inp_buf
      -- put data if available; use data producing function
      if self.putdata then
	 pd.data = {ts=yats.TimeReal, self.produce()}
	 self.putdata = false
      end

      -- enqueue the data
      self.q:enqueue(pd)

      -- notify sender: starts sender if window was closed
      alarme(self.sendev, 1)
      
   elseif ev.key == EVSAMPLE then
      -- announce sample
      self.putdata = true
      self.sampcount = self.sampcount + 1
      -- next sample
      alarml(self, samplev, self.sampcycle)
   end
end

---------------------------------------------------------------
-- driver/socket
---------------------------------------------------------------
sock = class(lua1out)

-----------------------------------------
-- Constructor
-----------------------------------------
function sock:init(param)
   local self = lua1out:new()
   self.name = autoname(param)
   self.clname = "sock"
   self.params = {
      name = false,
      winsize = true, -- window size
      cycle = true,   -- cycle funtion
      qlen = true 
   }
   self:adjust(param)

   -- Service rate
   self.count = 0
   self.winsize = param.winsize
   self.wincount = self.winsize/2
   self.qlen = param.qlen
   self.ackcount = 0
   self.discard = 0

   -- Queue
   self.q = cqueue(self.qlen)

   -- Pins
   self:definp("in")
   self:defout(param.out)

   -- Events
   self.recev = event:new(self, EVREC)
   self.recev.stat = 12345678
   self.schedev = event:new(self, EVSCHED)
   self.schedev.stat = 12345678

   -- Done
   return self:finish()
end

-----------------------------------------
-- Receive event
-----------------------------------------
function sock:luarec(pd)
   if idx == 0 then
      -- frame received
      tolua.cast(pd, "cell")
      -- store in input buffer and notify enqueue engine
      self.inp_buf = pd
      alarml(self.recev, 0)
   end
end

-----------------------------------------
-- Early event: send output and/or dequeue+consume data
-----------------------------------------
function sock:luaearly(ev)
   if ev.key == EVSCHED then
      if self.q:getlen() > 0 then
	 local pd = self.q:dequeue()
	 tolua.cast(pd, "cell")
	 self.count = self.count + 1
	 -- check if sequence num is as expected
	 if  self.expected_seqno then
	    if pd.seqno ~= self.expected_seqno then
	       self.loss = self.loss + 1
	    end
	 end

	 -- Set expected seqno
	 self.expected_seqno = self.seqno + 1

	 -- Check if we have data
	 if pd.data then
	    self.datacount = self.datacount + 1
	 else
	    self.emptycount = self.emptycount + 1
	 end
	 
	 -- Check ack sending: send after half window has been received
	 self.wincount = self.wincount - 1
	 if self.wincount == 0  then
	    self.wincount = self.winsize/2
	    self.ackcount = self.ackcount + 1
	    local suc, shand = self:getNext()
	    local rv = suc:rec(pd, shand)
	 end
	 -- consume the data
	 pd:delete()
      end
      alarme(self.schedev, self:cycle())
   end
end

-----------------------------------------
-- Late event enqueue data
-----------------------------------------
function sock:lualate(ev)
   if ev.key == EVREC then
      -- frame receive notification from input
      local pd = self.inp_buf
      -- enqueue
      if not self.q:enqueue(pd) then
	 self.discard = self.discard + 1
	 pd:delete()
      end
   end
end