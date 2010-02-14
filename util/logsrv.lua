#!/usr/local/bin/lua
-----------------------------------------------------------------------------
-- TCP sample: Little program to dump lines received at a given port
-- LuaSocket sample files
-- Author: Diego Nehab
-- RCS ID: $Id: logsrv.lua 222 2009-06-06 05:51:35Z leuwer $
-----------------------------------------------------------------------------
local socket = require("socket")
host = host or "*"
port = port or 5000
local con = false
if arg then
	host = arg[1] or host
	port = tonumber(arg[2]) or port
end
print("Binding to host '" ..host.. "' and port " ..port.. "...")
s = socket.try(socket.bind(host, port))
i, p  = socket.try(s:getsockname())
print("Waiting connection from talker on " .. i .. ":" .. p .. "...")
while true do
  c = socket.try(s:accept())
  if not con then
    print("Connected - now receiving")
    con = true
  end
  l, e = c:receive()
  while not e do
    print(l)
    l, e = c:receive()
  end
end
