require "yats.rstp"
require "yats.stdlib"

-- Simple printing
local function printf(fmt, ...)
  print(string.format(fmt, unpack(arg)))
end

-- Portrole names
local portroles = {
  "unknown", "alternate/backup", "root", "designated"
}

-- Last received info in form: last[what][bridge_and_port]
local last = {
  forwarding = {},
  learning = {},
  portrole = {},
  proposal = {},
  agreement = {},
  topo_change = {},
  tcack = {}
}

-- Returns true if first or changed info; false otherwise
local function isnew(f, w, p)
  local x = p.sender.name..".p"..p.senderport
  if not f[w] then return false end
  if not last[w][x] or last[w][x] ~= f[w] then
    last[w][x] = f[w]
    return true
  end
  return false
end

-- Get last state of bridge_and_port
local ps = {}
local function laststate(p)
  ps[p.node] = ps[p.node] or {}
  local rv = ps[p.node][p.port] or "unknown" 
  ps[p.node][p.port] = p.state
  return rv
end

-- Main Event Callback
function playEventCallback(what, param)
  local s = ""

  if what == "portstate" then
    printf("@@S %9.4f %s.p%d %10s -> %10s", param.time, 
	   param.node.name, param.port, laststate(param),
	   param.state)

  elseif what == "txbpdu" then
    printf("@@T %9.4f %s.p%d ==> %s.p%d: %d",
	   param.time,
	   param.sender.name, param.senderport,
	   param.receiver.name, param.receiverport, param.seq)

  elseif what =="rxbpdu" then
    local d = yats.decode(param.bpdu, "bpdu")
    local flags = d.body.flags
    s = portroles[flags.portrole + 1]
    rid = string.format(" rid=(%s-%s)", d.body.root_id.prio, d.body.root_id.addr)
    bid = string.format(" bid=(%s-%s)", d.body.bridge_id.prio, d.body.bridge_id.addr)

    if flags.forwarding == 1 then 
      s = s .." Fwd"
    end
    if flags.learning == 1 then
      s = s.." Lrn"
    end
    if flags.proposal == 1 then
      s = s .." Propo"
    end
    if flags.agreement == 1 then
      s = s.." Agree"
    end
    if flags.topo_change == 1 then
      s = s.." TcChg"
    end
    if flags.tcack == 1 then
      s = s.. " TcAck"
    end
    if isnew(flags, "portrole", param) then
      s = s..string.format(" now:%q ", portroles[flags.portrole+1])
    end
    if s ~= "" then
      printf("@@R %9.4f %s.p%d ==> %s.p%d: %d %s ",
	     param.time,
	     param.sender.name, param.senderport,
	     param.receiver.name, param.receiverport,
	     param.seq, s)
    end

  elseif what == "flush_NO" then
    -- Remove the _NO above to record flushing
    printf("@@F %9.4f %s.p%d Flushing FDB on port %d. Reason: %s", 
	   param.time, param.node.name, param.port, param.port, param.reason)
  end
end
