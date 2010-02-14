require "rstp"
require "stdlib"

local function printf(fmt, ...)
  print(string.format(fmt, unpack(arg)))
end

local portroles = {
  "unknown", "alternate/backup", "root", "designated"
}

local last = {
  forwarding = {},
  learning = {},
  portrole = {},
  proposal = {},
  agreement = {},
  topo_change = {},
  tcack = {}
}

local function isnew(f, w, p)
  local x = p.sender.name..".p"..p.senderport
  if not f[w] then return false end
  if not last[w][x] or last[w][x] ~= f[w] then
--    printf("### %s %s: last=%s new=%s", w, x, last[w][x] or "?", f[w])
    last[w][x] = f[w]
    return true
  end
  return false
end

local function isnew2(f, w, p) 
  if (w == "topo_change" and f[w] > 0) or
    (w == "agreement" and f[w] > 0) then
    return true 
  else
    return isnew(f,w,p) 
  end
end

local ps = {}
local function laststate(p)
  ps[p.node] = ps[p.node] or {}
  local rv = ps[p.node][p.port] or "unknown" 
  ps[p.node][p.port] = p.state
  return rv
end

function playEventCallback(what, param)
  local s = ""
  if what == "portstate" then
    printf("@@ %9.3f %s.p%d %10s -> %10s", param.time, 
	   param.node.name, param.port, laststate(param),
	   param.state)
  elseif what == "txbpdu_NO" then
    printf("## %9.3f %s.p%d ==> %s %d",
	   param.time,
	   param.sender.name, param.senderport,
	   param.receiver.name, param.seq)
  elseif what =="rxbpdu" then
    local d = yats.decode(param.bpdu, "bpdu")
    local flags = d.body.flags
--    s = string.format("bpdu-received: %s\n", pretty(flags))
    s = " "
    rid = string.format(" rid=(%s-%s)", d.body.root_id.prio, d.body.root_id.addr)
    bid = string.format(" bid=(%s-%s)", d.body.bridge_id.prio, d.body.bridge_id.addr)
    if isnew(flags, "forwarding", param) then 
      s = s .." Fwd"
    end
    if isnew(flags, "learning", param) then
      s = s.." Lrn"
    end
    if isnew2(flags, "proposal", param) then
      s = s .." Propo"
    end
    if isnew2(flags, "agreement", param) then
      s = s.." Agree"
    end
    if isnew2(flags, "topo_change", param) then
      s = s.." TcChg"
    end
    if isnew2(flags, "tcack", param) then
      s = s.. " TcAck"
    end
    if isnew(flags, "portrole", param) then
      s = s..string.format(" %q ", portroles[flags.portrole+1])
    end
    if s ~= "" then
      printf("@@ %9.3f %s.p%d ==> %s.p%d: %d %s ",
	     param.time,
	     param.sender.name, param.senderport,
	     param.receiver.name, param.receiverport,
	     param.seq, s)
    end
  elseif what == "flush" then
    printf("@@ %9.3f %s.p%d Flushing FDB on port %d. Reason: %s", 
	   param.time, param.node.name, param.port, param.port, param.reason)
  end
end
