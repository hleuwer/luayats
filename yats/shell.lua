---------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: shell.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description  IUP Shell Extension.
-- <br>
-- <br><b>module: env</b><br>
-- <br>
-- Models as Lua shell in an IUP Multiline widget.<br>
-- <br>
-- The command shell widget can be used in an IUP dialog. The
-- shell uses its own Lua environment 'shell'.<br>
-- The following methods are provided for the user:<br>
-- <ul>
-- <li> <code>shell:print(...)</code>: Prints given items separated by tab.
-- <li> <code>shell:printf(fmt, ...)</code>: Prints a formatted string.
-- <li> <code>shell:write(str)</code>: Prints the given string.
-- </ul>

module("env", package.seeall)

-- Some local variable
local yes,no = "YES", "NO"
local on,off = "ON", "OFF"

-- Extract the command from the last line.
local function getcmd(s, prompt)
   local s = s .. "\n"
   local l
   for w in string.gfind(s, "[^\r\n]+") do 
      l = w
   end
   return string.gsub(l or "", ".*"..prompt.."(.-)$", "%1")
end


-- Key handling.
local function handlekey(self, c, after)
  lin, col = self:getcaret()
  local rv = iup.DEFAULT
  
  if c == iup.K_CR then
    local cmd = getcmd(self.value, self.prompt)
    if cmd == "cls" then 
      self.value = "" 
    elseif cmd == "exit" then 
      if self.exitapp then
	local b = iup.Alarm("INFO", "Do you really want to quit luayats ?", "Yes", "No")
	if b == 1 then
	  self.exitapp()
	end
      end
    elseif cmd == "h" then
      self.Append = ""
      for i,v in ipairs(self.cmdstack.stack) do
	self.value = self.value .. string.format("%3i: %s\n", i,v) 
      end
    elseif cmd == "clear" then
      self.cmdstack:clear()
    elseif cmd == "help" then
      self:help()
    else
      cmd = string.gsub(cmd, "^(=)(.+)","print(%2)")
      if tonumber(cmd) then
	cmd = self.cmdstack.stack[tonumber(cmd)]
      end
      if string.len(cmd) > 0 then 
	self:redir(true)
	f, err = loadstring(cmd)
	if f then
	  if not _G.user then
	    local m = {}
	    setmetatable(m, {__index=_G})
	    _G.user = m
	  end
	  setfenv(f, _G.user)
	  if iup.GetGlobal("DRIVER") ~= "Motif" then
	    io.write("\n")
	  end
	  local rv, err = pcall(f)
	  if not rv then
	    io.write("\nRUNTIME ERROR: "..err)
	    io.write(debug.traceback().."\n")
	    self.cmdstack:put(cmd)
	  else
	    self.cmdstack:put(cmd)
	  end
	  self:redir(false)
	else
	  io.write("\nSYNTAX ERROR: "..err.."\n")
	  io.write(debug.traceback().."\n")
	  self:redir(false)
	  self.cmdstack:put(cmd)
	end
      else
	self:redir(true)
	if iup.GetGlobal("DRIVER") ~= "Motif" then
	  io.write("\n")
	end
	self:redir(false)
      end
    end
    iup.SetIdle(function() 
		  local lin, col = self:setcaretend()
		  self.value = self.value .. self.prompt
		  lin, col = self:setcaretend(0, 1000)
		  iup.SetIdle(nil)
		  return iup.DEFAULT
		end)
    rv = iup.DEFAULT
    
  elseif c == iup.K_UP or c == iup.K_DOWN then
    if c == iup.K_UP then
      self.cmdstack:backward()
    elseif c == iup.K_DOWN then
      self.cmdstack:forward()
    end
    local cmd = self.cmdstack:get()
    if cmd then
      iup.SetIdle(function() 
		    if iup.GetGlobal("DRIVER") == "Motif" then
		      local lin, col = self:setcaretend()
		      self:setselection(lin, self.plen, lin, col - 1)
		      self.selectedtext = cmd
		      self:setcaret(self:getcaret(0, self.plen + string.len(cmd)))
		    else
		      local lin, col = self:setcaretend()
		      self:setselection(lin, self.plen, lin, col)
		      self.selectedtext = cmd
		      self:setcaret(self:getcaret(0, self.plen + string.len(cmd)))
		    end
		    iup.SetIdle(nil)
		  end)
    end
    rv = iup.IGNORE
  elseif c == iup.K_cH then
    local l,c = self:getcaret()
    if c <= tonumber(self.plen) then 
      rv = iup.IGNORE
    else
      rv = iup.DEFAULT
    end
  elseif  c == iup.K_LEFT then
    local l,c = self:getcaret()
    if c <= tonumber(self.plen) then
      rv = iup.IGNORE
    else
      rv = iup.DEFAULT
    end
  elseif c == iup.K_RIGHT then
    rv = iup.DEFAULT
  elseif c == iup.K_HOME then
    iup.SetIdle(function()
		  local l,c = self:getcaret()
		  self:setcaret(l, self.plen)
		  iup.SetIdle(nil)
		end)
    rv = iup.IGNORE
  else
    rv = iup.DEFAULT
  end
  return rv
end

local function prehandlekey(self, c)
  if iup.GetGlobal("DRIVER") == "Motif" then
    if c == iup.K_UP or c == iup.K_DOWN then
      handlekey(self, c, self.value)
    end
  end
end

--- Constructor<br>
-- Since the shell is defined in its own environment,
-- constructor must be called in the following way:<br>
--     <code>myshell = shell.shell(param)</code>.
--@param param table Construct parameters.
--@return shell object instance.
function shell(param)
  param = param or {}
  param.expand = param.expand or "YES"
  param.font = param.font or "COURIER_NORMAL_10"
  param.prompt = param.prompt or "luayats > "
  param.exitapp = param.exitapp or nil
  param.welcome = param.welcome or "Welcome to luayats!"
  -- Browser shell object: i/o redirection overloads io.write, print
  local sh = iup.multiline{
    expand = param.expand,
    font = param.font,
    bgcolor = "255 255 255",
    action = handlekey,
    k_any = prehandlekey,
    prompt = param.prompt,
    value = param.welcome.."\n"..param.prompt,
    exitapp = param.exitapp,
    nc = 60000,
    plen = string.len(param.prompt) + 1, 
    tip = "Luayats Command Shell - use Lua syntax.",
    readonly = no,
    cmd = "",
    help = function(self)
	     local s = [[
Shell commands:
h      show command history
cls    clear window
clear  clear command history
exit   exit application
help   show this help
<n>    execute command <n> from hist
]]
	     self.value = self.value .. s
	   end,

    getcaret = function(self, loffs, coffs)
		 local lin, col
		 string.gsub(self.caret, "(%d+),(%d+)", 
			     function(u,v) 
			       lin = tonumber(u) or 1 
			       col = tonumber(v) 
			     end)
		 assert(lin and col, "shell: unable to determine caret.")
		 return lin + (loffs or 0), col + (coffs or 0)
	       end,

    setcaret = function(self, lin, col)
		 self.caret = string.format("%d,%d", lin, col)
		 return self:getcaret()
	       end,

    setcaretend = function(self, loffs, coffs)
		    local lin, col = self:setcaret(50000,10000)
		    return self:setcaret(lin + (loffs or 0), col + (coffs or 0))
		  end,

    setselection = function(self, alin, acol, zlin, zcol)
		     self.selection = string.format("%d,%d:%d,%d", alin, acol, zlin, zcol)
		   end,

    tail = function(self)
	     local t = {}
	     for s in string.gfind(self.value, "[^\n\r]+") do
	       table.insert(t, s)
	     end
	     local n1 = table.getn(t)
	     local n2, s = 0, ""
	     if n1 > 10 then n2 = n1 - 10 else n2 = 1 end
	     for i = n2, n1 do
	       s = s..t[i].."\n"
	     end
	     return "=== cut here ===\n"..s
	   end,

    write = function(self, s)
	      s = s or ""
	      self.value = self.value..s
	      if string.len(self.value) > tonumber(self.nc) then
		self.value = self:tail()..s
	      end
	    end,

    --- Print formatted text.
    -- @param fmt string Format string.
    -- @return none.
    printf = function(self, fmt, ...)
	       self:write(string.format(fmt, unpack(arg)))
	     end,
    eprintf = function(self, fmt, ...)
		self:write(string.format(fmt, unpack(arg)))
	      end,

    --- Print any items separated by tabs.
    -- @param List of items to print.
    -- @return none.
    print = function(self, ...)
	      local n = table.getn(arg)
	      for i = 1, n do
		if i < n then
		  self:write(tostring2(arg[i]).."\t")
		else
		  self:write(tostring2(arg[i]))
		end
	      end
	      self:write("\n")
	    end,
    
    remind = {},

    redir = function(self, onoff)
	      local remind = self.remind 
	      if onoff == true then
		remind.print = print
		remind.printf = printf
		remind.eprintf = eprintf
		remind.write = io.write
		_G.print = function(...) self:print(unpack(arg)) end
		_G.printf = function(...) self:printf(unpack(arg)) end
		_G.eprintf = function(...) self:eprintf(unpack(arg)) end
		io.write = function(s) self:write(s) end
	      else
		_G.print = remind.print
		_G.printf = remind.printf
		_G.eprintf = remind.eprintf
		io.write = remind.write
	      end
	    end,
    cmdstack = {
      stack = {n=0},
      hash = {},
      wp = 1,
      rp = 1,
      put = function(self, cmd)
	      -- if not string.find(cmd, "%w+") then return end
	      if self.wp > 100 then
		self.wp = 1
	      end
	      if not self.hash[cmd] then
		table.insert(self.stack, self.wp, cmd)
		self.hash[cmd] = true
		self.wp = self.wp + 1
		self.rp = self.wp
	      end
	    end,
      get = function(self)
	      return self.stack[self.rp]
	    end,
      clear = function(self)
		self.stack = {}
		self.wp = 1
		self.rp = 1
		self.hash = {}
	      end,
      forward = function(self) 
		  self.rp = self.rp + 1
		  if self.rp > table.getn(self.stack) then
		    self.rp = 1
		  end
		end,
      backward = function(self)
		   self.rp = self.rp - 1 
		   if self.rp < 1 then 
		     self.rp = table.getn(self.stack) 
		   end
		 end,
      nil
    }
  }
  return sh
end

return env