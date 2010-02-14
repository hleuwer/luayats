require "yats.core"
require "table"

_yats = yats.y


-- ======================================================================
-- Basic definitions.
-- ======================================================================

-- Select the default bridge type.
yats.BRIDGE_DEFAULT_TYPE = "802.1D"
-- yats.DEFBRIDGE = "802.1w"
-- yats.DEFBRIDGE = "802.1s"

yats.BRIDGE_STP = "802.1D"
yats.BRIDGE_RST = "802.1w"
yats.BRIDGE_MST = "802.1s"
yats.BRIDGE_MANAGEMENT_GROUP_ADDRESS = "01-80-C2-00-00-10"
yats.BRIDGE_CONFIG_BPDU_TYPE = 0
yats.BRIDGE_TCN_BPDU_TYPE = 128

-- Bridge BPDU 
yats.bpdu_types = {
  ['802.1D'] = {
    id = 0,
    prot_id = 0,
    type = 0
  },
  ['802.1w'] = {
  },
  ['802.1s'] = {
  }
}

--- Path Cost mapping of 802.1D bridges.
yats.bridge_path_cost_map = {
  ['802.1D'] = {
    [    4e6] = {default = 250, range = {100, 1000}},
    [   10e6] = {default = 100, range = { 50,  600}},
    [   16e6] = {default =  62, range = { 40,  400}},
    [  100e6] = {default =  19, range = { 10,   60}},
    [ 1000e6] = {default =   4, range = {  3,   10}},
    [10000e6] = {default =   2, range = {  1,    5}}
  },
  ['802.1w'] = {
  },
  ['802.1s'] = {
  }
}

-- ======================================================================
-- Utilities.
-- ======================================================================

--- Convert a MAC string expression in table and vice versa.
function yats.mac2mac(mac)
  local t, s = {}, ""
  if type(mac) == "string" then
    string.gsub(mac, "(%x+)", function(v) table.insert(t, tonumber("0x"..v)) end)
    return t, mac
  elseif type(mac) == "table" then
    local i,v
    for i,v in mac do
      s=s..string.format("%02X",v)
--      dbg(5,i,v)
      if i == 6 then 
	return mac, s 
      else 
	s = s.."-" 
      end
    end
  else
    error("MAC must be given as string or table not as '"..type(mac).."'.")
  end
end


-- Maintain a table for bridge id uniqueness.
yats.bridge_tab = {}

-- ======================================================================
-- Bridge Id Class.
-- ======================================================================
yats.bridge_id = class()

--- Constructor of 'bridge_id'.
function yats.bridge_id:new(prio, mac)
  local self = self:create()
  self.id = {}
  self.id[1] = math.floor(prio/256)
  self.id[2] = math.mod(prio, 256)
--  print(prio, mac)
  self.mac, self.smac = yats.mac2mac(mac)
--  print(self.mac, self.smac)
  for i,v in self.mac do
    self.id[2+i] = v
  end
  self.prio = prio
  setmetatable(self, {__lt = self.compare})
  return self
end

function bridge_id:compare(id)
  for i = 1,8 do
    if self[i] < id[i] then
      return true
    end
  end
end

-- ======================================================================
-- BPDU Classes.
-- ======================================================================

yats.bpdu = class()
function yats.bpdu:new(type)
  local self = self:create{
    protocol_id =  0,
    protocol_version_id = 0,
    bpdu_type = type,
  }
  return self
end

yats.config_bpdu = class(yats.bpdu)
function yats.config_bpdu:new(param)
  local self = self:create(yats.bpdu:new(yats.BRIDGE_CONFIG_BPDU_TYPE)) 
  self.flags = param.flags or {}
  self.topology_change_ack = false
  self.topology_change = false
  self.root_id = param.root_id
  self.root_path_cost = param.root_path_cost or 
    yats.bridge_path_cost_map[param.stp_type][param.linkspeed].default or 
    yats.bridge_path_cost_map[yats.BRIDGE_DEFAULT_TYPE][100e6].default
  self.bridge_id = param.bridge_id or BRIDGE_NULL_ID
  self.port_id = param.portid or -1
  self.message_age = 0
  self.hello_time = 0
  self.forward_delay = 0
  return self
end

yats.tcn_bpdu = class(yats.bpdu)
function yats.tcn_bpdu:new()
  local self = self:create(yats.bpdu:new(yats.BRIDGE_TCN_BPDU_TYPE))
  self.bpdu_type = yats.BRIDGE_TCN_BPDU_TYPE
  return self
end

-- only temporarily
yats.bpdu_template = {
  protocol_id = 0,
  protocol_version_id = 0,
  bpdu_type = 0,
  flags = {},
  topology_change = false,
  topology_change_ack = false,
  root_id = -1,
  root_path_cost = 0,
  bridge_id = nil,
  port_id = -1,
  message_age = 0,
  hello_time = 0,
  forward_delay = 0
}


-- ======================================================================
-- Class: bridge_port
-- ======================================================================
yats.bridge_port = class(yats.root)

--- Constructor of class 'bridge_port'.
function yats.bridge_port:new(param)
  local self = self:create{
    clname = "bridge_port",
  }
  -- configuration
  self.linkspeed = param.linkspeed or 100e6,
  self.bridge = param.bridge,
  config_bpdu = yats.config_bpdu:new()
  -- we don't really need a yats object for this
  return self
end

--- Show bridge_port.
function yats.bridge_port:show()
  printf("port id   : %d\n", self.port_id)
  printf("port state: %s\n", self.state)
  printf("path cost : %d\n", self.path_cost)
  printf("link speed: %d\n", self.linkspeed)
  printf("\n")
end

function yats.bridge_port:initialize()
  dbg(L4, "Initializing port %d of bridge %s\n", param.port_id, tostring(param.bridge.bridge_id))
  self:become_designated_port()
  self:set_port_state("Blocking")
  self.topology_change_acknowledge = false
  self.config_pending = false
  self.change_detection_enable = true
  self:stop_message_age_timer()
  self:forward_delay_timer()
  self:stop_hold_timer()
end

function yats.bridge_port:transmit_config()
  if self.hold_timer.active then
    self.config_pending = true
  else
    local cb = self.config_bpdu
    cb.type = BRIDGE_CONFIG_BPDU_TYPE
    cb.root_id = self.designated_root
    cb.root_path_cost = self.root_path_cost
    cb.bridge_id = self.bridge_id
    cb.port_id = self.port_id
    if self:root_bridge() then
      cb.message_age = 0
    else
      cb.message_age = self.message_age_timer[self.root_port].value + BRIDGE_MESSAGE_AGE_INCREMENT
    end
    cb.max_age = self.max_age
    cb.hello_time = self.hello_time
    cb.forward_delay = self.forward_delay
    cb.topology_change_acknowledgement = self.topology_change_acknowledge
    cb.topology_change = self.topology_change
    if cb.message_age < self.max_age then
      self.topology_change_acknowledge = false
      cb.config_pending = false
      self:send_config_bpdu(cb)
      self:start_hold_timer()
    end
end

function yats.bridge_port:enable()
  local bridge = self.bridge
  self:initialize()
  bridge:port_state_selection()
end

function yats.bridge_port:disable()
  local bridge = self.bridge
  local root = bridge:root_bridge()
  self:become_designated_port()
  self:set_port_state("Disable")
  self.topology_change_acknowledge = false
  self.config_pending = false
  self:stop_message_age_timer()
  self:stop_forward_delay_timer()
  bridge:configuration_update()
  bridge:port_state_selection()
  if self.bridge:root_bridge() and not root then
    bridge.max_age = bridge.bridge_max_age
    bridge.hello_time = bridge.bridge_hello_time
    bridge.forward_delay = bridge.bridge_forward_delay
    bridge:topology_change_detection()
    bridge:stop_tcn_timer()
    bridge:config_bpdu_generation()
    bridge:start_hello_timer()
  end
end

function yats.bridge_port:set_priority(new_port_id)
  local bridge = self.bridge
  if self:designated_port() then
    self.designated_port = new_port_id
  end
  self.port_id = new_port_id
  if bridge.bridge_id == self.designated_bridge and
    self.port_id < self.designated_port then
    self:become_designated_port()
    bridge:port_state_selection()
  end
end

function yats.bridge_port:set_path_cost(path_cost)
  self.path_cost = path_cost
  self.bridge:configuration_update()
  self.bridge:port_state_selection()
end

function yats.bridge_port:enable_change_detection()
  self.change_detection_enabled = true
end

function yats.bridge_port:disable_change_detection()
  self.change_detection_enabled = false
end


-- ======================================================================
-- Class: bridge
-- ======================================================================

yats.bridge = class(yats.root)

--- Constructor of class 'bridge'.
function yats.bridge:new(param)
  local self = self:create{
    name = autoname(param),
    clname = "bridge",
  }
  self:adjust(param)

  -- Private bridge info
  self.stp_type = param.stp_type or "802.1D"
  self.nports = param.nports or 2

  -- Standard bridge info: Configuration values
  self.bridge_id = yats.bridge_id:new(0, param.mac)
  self.bridge_max_age = param.max_age or error("max age")
  self.bridge_hello_time = param.hello_time or error("hello time")
  self.bridge_forward_delay = param.forward_delay or error("forward delay")

  -- Generate yats objects.
  t = _yats.luaxout:new()
  
  -- Generate port instances with some basic initialisations.

  self.ports = {}
  -- Inputs
  for i = 1, self.nports do
    self.ports[i] = yats.bridge_port:new{
      port_id = i,
      linkspeed = param.linkspeeds[i] or param.linkspeeds.default or 100e6,
      bridge = self
    }
    self:definp("port"..i)
  end

  -- Outputs
  self:defout(param.out)

  self:initialize(param)

  return self:finish(t)
end


function yats.bridge:initialize(param)
  dbg(L4, "Initializing bridge %s\n", tostring(self.bridge_id))
  self.designated root = self.bridge_id
  self.root_path_cost = 0
  self.root_port = self.nports
  self.max_age = self.brigde_max_age
  self.hello_time = self.bridge_hello_time
  self.forward_delay = self.bridge_forward_delay
  self.topology_change_detected = false
  self.topology_change = false
  self:stop_tcn_timer()
  self:stop_topology_change_timer()
  local port_no
  for port_no=1,self.nports do
    self.ports[port_no]:init()
  end
  self:port_state_selection()
  self:config_bpdu_generation()
  self:start_hello_timer()
end

function yats.bridge:set_bridge_priority(new_bridge_id)
  local port_no
  local root = self:root_bridge()
  for port_no = 1, self.nports do
    if self:designated_port(port_no) then
      self.ports[port_no].designated_bridge = new_bridge_id
    end
  end
  self.bridge_id = new_bridge_id
  self:configuration_update()
  self:port_state_selection()
  if self:root_bridge() and not root then
    self.max_age = self.bridge_max_age
    self.hello_time = self.bridge_hello_time
    self.forward_delay = self.bridge_forward_delay
    self:topology_change_detection()
    self:stop_tcn_timer()
    self:config_bpdu_generation()
    self:start_hello_timer()
  end
end

function self:tick()
end