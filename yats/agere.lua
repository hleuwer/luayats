------------------------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: agere.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Agere Network Processor
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
------------------------------------------------------------------------------

require "yats.object"

module("yats", yats.seeall)

--==========================================================================
-- Agere Traffic Manager
--==========================================================================

-- Definition of class 'AgereTm'.
_AgereTm = AgereTm
AgereTm = class(_AgereTm)

--- Constructor of class 'AgereTm'.
--~[[ref=yats.AgereTm:new{[name=]"objname",ninp=N,[nTrafficQueue|ntq=ninp,][nPolicier|npol=ninp,][maxconn=ninp,] maxbuff_p=N, maxbuff_b=N, ShapingRate=D, out = {"next", "pin"}]].
-- @param param table - Paramter list.
-- @return Reference to TM object.
function AgereTm:init(param)
  local self = _AgereTm:new()
  self.name = autoname(param)
  self.clname = "AgereTm"
  self.parameters = {
    ninp = true, ntrafficqueue = false, npolicer = false, ntq = false, npol = false,
    maxconn = false, maxbuff_p = true, maxbuff_b = true, shapingrate = true, out = true
  }

  -- adjust parameter names to lower case
  self:adjust(param)
  
  -- parse parameters
  assert(param.ninp > 0, "AgereTm: need > 0 inputs.")
  self.ninp = param.ninp
  self.nTrafficQueue = param.ntrafficqueue or param.ntq or param.ninp
  self.nPolicer = param.npolicer or param.npol or param.ninp
  self.maxconn = param.maxconn or param.ninp
  assert(param.maxbuff_p > 0, "AgereTm: eed > 0 maxbuff packet")
  self.maxbuff_p = param.maxbuff_p
  assert(param.maxbuff_p > 0, "AgereTm: eed > 0 maxbuff packet")
  self.maxbuff_b = param.maxbuff_b
  assert(param.shapingrate > 0, "AgereTm: eed > 0 maxbuff packet")
  self.ShapingRate = param.shapingrate


  -- generate inputs
  self:set_nout(1)
  self:defout(param.out)
  for i = 1, self.ninp do 
    self:definp("in"..i) 
  end
  -- generate outputs
  local a, b =  self:finish()
  self.scheduler.name = self.name.."-sched"
  for i = 1, self.nTrafficQueue+1 do
    self.TrafficQueue[i].name = self.name.."-tq-"..i
  end
  return a, b
end

--- Set packet buffer size for CoS class.
-- @param tqid number - Traffic queue id starting at 1.
-- @param cosid number - CoS class - 1 to 8.
-- @param maxbuff_p number - Number of packets.
-- return none.
function AgereTm:TqCosSetMaxBuff_p(tqid, cosid, maxbuff_p)
  self.TrafficQueue[tqid].CosQueue[cosid].maxbuff_p = maxbuff_p
end

--- Set octet buffer size of CoS class.
-- @param tqid number - Traffic queue id starting at 1.
-- @param cosid number - CoS class - 1 to 8.
-- @param maxbuff_b number - Number of bytes.
-- return none.
function AgereTm:TqCosSetMaxBuff_b(tqid, cosid, maxbuff_b)
  self.TrafficQueue[tqid].CosQueue[cosid].maxbuff_p = maxbuff_p
end

--- Set packet buffer size for a traffic queue.
-- @param tqid number - Traffic queue id starting at 1.
-- @param maxbuff_p number - Number of packets.
-- return none.
function AgereTm:TqSetMaxBuff_p(tqid, buf)
  assert(buf > 0, self.clname..": maxbuff_p > 0 required.")
  self.TrafficQueue[tqid].maxbuff_p = buf
  assert(self.TrafficQueue[tqid].maxbuff_p < self.maxbuff_p,
	 self.clname..": max. buffer (packets) of "..tqid.." is > maxbuff of the traffic manager.")
end

--- Set octet buffer size for a traffic queue.
-- @param tqid number - Traffic queue id starting at 1.
-- @param maxbuff_b number - Number of bytes.
-- return none.
function AgereTm:TqSetMaxBuff_b(tqid, maxbuff_b)
  assert(maxbuff_b > 0, sefl.clname..": maxbuff_p > 0 required.")
  self.TrafficQueue[tqid].maxbuff_b = buf
  assert(self.TrafficQueue[tqid].maxbuff_b > self.maxbuff_b,
	 self.clname..": max. buffer (bytes) of "..tqid.." is > maxbuff of the traffic manager.")
end

--- Set SDWRR parameters: limits 1 to 3.
-- @param tqid number - Traffic queue id starting at 1.
-- @param limit1 number - 1st limit.
-- @param limit2 number - 2nd limit.
-- @param limit3 number - 3rd limit.
-- @return none.
function AgereTm:TqSetSdwrrParam(tqid, limit1, limit2, limit3)
  assert(limit1 > 0, self.clname..": limit 1 > 0 required.")
  assert(limit2 > 0, self.clname..": limit 2 > 0 required.")
  assert(limit3 > 0, self.clname..": limit 3 > 0 required.")
  self.TrafficQueue[tqid].limit1 = limit1
  self.TrafficQueue[tqid].limit2 = limit3
  self.TrafficQueue[tqid].limit3 = limit3
end

--- Set SDWRR service quantum.
-- @param sq number - Service quantum.
-- @return none.
function AgereTm:SchedSdwrrSetServiceQuantum(sq)
  assert(sq > 0, self.clname..": serviceQuantum > 0 required.")
  self.scheduler.serviceQuantum = sq
end

--- Set SDWRR maximum PDU size.
-- @param size number - PDU size.
-- @return none.
function AgereTm:SchedSdwrrSetMaxPduSize(size)
  assert(size > 0, self.clname..": maxPduSize > 0 required.")
  self.scheduler.maxPduSize = size
end

--- Set TQ shaping rate.
-- @param tqid number - Traffic queue id starting at 1.
-- @param rate number - Shaping rate in bits/s.
-- @return none.
function AgereTm:TqSetShapingRate(tqid, rate)
  assert(rate > 0, self.clname..": ShapingRate > 0 required.")
  self.TrafficQueue[tqid].ShapingRate = rate
end

--- Set TQ minimum rate.
-- @param tqid number - Traffic queue id starting at 1.
-- @param rate number - Minimum rate in bits/s.
-- @return none.
function AgereTm:TqSetMinimumRate(tqid, rate)
  assert(rate > 0, self.clname..": MinimumRate > 0 required.")
  self.TrafficQueue[tqid].MinimumRate = rate
  self.scheduler:calculateLimits()
end

--- Assign a traffic queue to a connection (VPL).
-- @param cid number - Connection id starting a 0.
-- @param tqid number - Traffic queue id starting at 1.
-- @return none.
function AgereTm:ConnSetDestinationQueue(cid, tqid)
  assert(tqid >= 0 and tqid < self.nTrafficQueue,
	 self.clname..": traffic queue id "..tqid.." is out of range.")
  self.connpar[cid+1].destinationQueue = tqid
end

--- Assign a policer to a connection (VPL).
-- @param cid number - Connection id starting a 0.
-- @param polid number - Traffic queue id starting at 1.
-- @return none.
function AgereTm:ConnSetDestinationPolicer(cid, polid)
  assert(polid >= 0 and polid < self.nPolicer,
       self.clname..": policer id "..polid.." is out of range.")
  self.connpar[cid+1].destinationPolicer = polid
end

--- Set the rates of a policer.
-- @param polid number - Index of policer starting at 1.
-- @param cir - Committed information rate.
-- @param cbs - Committed burst size.
-- @param pir - Peak information rate.
-- @param pbs - Peak burst size.
function AgereTm:PolicerSetRate(polid, cir, cbs, pir, pbs)
  self.policer[polid]:SetParam(cir, cbs, pir, pbs)
end

--- No action.
polNone = 0
--- Mark a packet.
polMark = 1
--- Drop a CIR packet.
polCIRdrop = 2
--- Drop a PIR packet.
polPIRdrop = 3

--- Set the action a policer has to perform.
-- @param polid number - Index of policer starting at 1.
-- @param action constant - Action to perform (polNone, polMark, polCIRdrop, polPIRdrop).
-- @return none.
function AgereTm:PolicerSetAction(polid, action)
  self.policer[polid]:SetAction(action)
end

return yats