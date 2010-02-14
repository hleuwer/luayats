-----------------------------------------------------------------------------------
-- @copyright Herbert Leuwer, 2003.
-- @author Herbert Leuwer.
-- @release 3.0 $Id: stdlib.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Yats' version of a Lua Library extension.
-- <br>
-- <br><b>module: G</b><br>
-- <br>
-- The 'yats.stdlib' module defines several useful functions. All of them are in the global
-- namespace. See list of functions for details.<br>
-- <br>
-- <b>NOTE:</b>Note, that yats.stdlib is not the same as stdlib from luarocks.
-----------------------------------------------------------------------------------

local _tostring = tostring

module("_G")

---------------------------------------------------------------------------------------
-- Convert an object into a string.
-- New implementation of standard tostring to improve table conversion. 
-- The result is a string representing the input object in a way that it can be 
-- used in dostring() as a valid Lua expression.
-- @param x Object to convert.
-- @param flag Enclose strings in "'".
-- @return string - Representation of object.
---------------------------------------------------------------------------------------
function tostring2(x, flag)
  if type(x) == "table" then
    local s, sep1, sep2 = "{", "", ""
    if i == 2 then i = 121 end
    for i, v in pairs(x) do
      if type(i) == "number" then
	sep1 = sep1.."["
	sep2 = "]"
      elseif type(i) == "string" then
	 if not flag then
	    sep1 = sep1.."['"
	    sep2 = "']"
	 else
	    sep1 = sep1..""
	    sep2 = ""
	 end
      end
      if type(v) == "string" then
	local sm 
	if string.find(v, "'") then
	  sm = '"'
	else
	  sm = "'"
	end
	  s = s .. sep1 .. tostring2(i, flag) .. sep2 .. " = " .. sm .. tostring2(v, flag) .. sm
      else
	s = s .. sep1 .. tostring2(i, flag) .. sep2 .. " = " .. tostring2(v, flag)
      end
      sep1, sep2 = ", ", ""
    end
    return s .. "}"
  end
  return _tostring(x)
end

local function _pretty(t, tos, flag)
  local s = tos(t, flag)
  local last = ''
  local indent = 0
  local instring = false
  local instring1, instring2 = false, false
  if type(t) == "table" then
--    local s1 = string.gsub(s, ", ", ",")
    s1 = string.gsub(s, '([ ={},"'.."'])", 
		     function(s)
		       local x = ""
		       if s == "{" then
			 if instring == false then
			   indent = indent + 2
			   x = s.."\n"..string.rep(" ", indent) 
			 else
			   return x
			 end
		       elseif s == "}" then
			 if instring == false then
			   indent = indent - 2
			   x = "\n"..string.rep(" ", indent)..s
			 else
			   return x
			 end
		       elseif s == "," then
			 if instring == false then
			   x = s.."\n"..string.rep(" ", indent)
			 else 
			   return s
			 end
		       elseif s == "'" then
			 if instring1 == true then
			   instring1 = false
			   instring = false
			 elseif instring2 == false then
			   instring1 = true
			   instring = true
			 end
			 return s
		       elseif s == '"' then
			 if instring2 == true then
			   instring2 = false
			   instring = false
			 elseif instring1 == false then
			   instring2 = true
			   instring = true
			 end
			 return s
		       elseif s == "=" then
			 if instring == false then
			   return " = "
			 else
			   return "="
			 end
		       elseif s == " " then
			 if instring == false then
			   return ""
			 else
			   return s
			 end
		       end
		       return x
		     end
		   )
    s1 = string.gsub(s1,"%s+[\r\n]","")
    s1 = string.gsub(s1,"{%s*}","{}")
    return s1
  else
    return s
  end
end

---------------------------------------------------------------------------------------
-- Convert an object into a readable string. 
-- Numeric indices are not shown.
-- @param t Object to convert.
-- @param flag Enclose strings in "'".
-- @return string - Representation of object.
-- @see tostring2
---------------------------------------------------------------------------------------
function pretty(t, flag)
  return _pretty(t, tostring2, flag)
end

---------------------------------------------------------------------------------------
-- Convert an object into a readable string. 
-- Numeric indices are not shown.
-- @param t Object to convert.
-- @return string - Representation of object.
-- @see tostring3
---------------------------------------------------------------------------------------
function spretty(t)
  return _pretty(t, tostring3, flag)
end
--- For compatibility.
ptostring = pretty

---------------------------------------------------------------------------------------
-- Convert value into string - print table fields in alphabetic order.
-- @param value Value to convert to string.
-- @return String presentation of the given value.
---------------------------------------------------------------------------------------
function tostring3(value)
  local str = ''

  if (type(value) ~= 'table') then
    if (type(value) == 'string') then
      if (string.find(value, '"')) then
        str = '[['..value..']]'
      else
        str = '"'..value..'"'
      end
    else
      str = tostring(value)
    end
  else
    local auxTable = {}
    table.foreach(value, function(i, v)
      if (tonumber(i) ~= i) then
        table.insert(auxTable, i)
      else
        table.insert(auxTable, tostring(i))
      end
    end)
    table.sort(auxTable)

    str = str..'{'
    local separator = ""
    local entry = ""
    table.foreachi (auxTable, function (i, fieldName)
      if ((tonumber(fieldName)) and (tonumber(fieldName) > 0)) then
        entry = tostring3(value[tonumber(fieldName)])
      else
        entry = "['"..fieldName.."'] = "..tostring3(value[fieldName])
      end
      str = str..separator..entry
      separator = ", "
    end)
    str = str..'}'
  end
  return str
end

---------------------------------------------------------------------------------------
-- Hex to Decimal converions for 32 bit numbers.
-- @param s string - Presents a hex number.
-- @return number - Decimal respresentation.
-- @usage x'03ec' or x('03ec').
---------------------------------------------------------------------------------------
function x(s) 
  local k = tonumber(s,16) 
  if k>2^31-1 then 
    k=k-2^32 
  end
  return k
end

---------------------------------------------------------------------------------------
-- C-like printf. Writes to stdout.
-- @param fmt string - Format string.
-- @return none.
function printf(fmt, ...)
  io.stdout:write(string.format(fmt, unpack(arg)))
end

---------------------------------------------------------------------------------------
-- C-like printf. Writes to stderr.
-- @param fmt string - Format string.
-- @return none.
---------------------------------------------------------------------------------------
function eprintf(fmt, ...)
  io.stderr:write(string.format(fmt, unpack(arg)))
end


---------------------------------------------------------------------------------------
-- Delay execution for n (real number) s.
-- @param n Delay time in s.
-- @return none.
---------------------------------------------------------------------------------------
function delay(n)
  local t1 = clock()
  while clock() - t1 < n do
  end
end

---------------------------------------------------------------------------------------
-- Print user error message traceback. Overrides Lua's 
-- error function.
-- @param s string - Error message.
-- @param level number - Debug deepness.
-- @return never returns.
---------------------------------------------------------------------------------------
local _error = error
function error(s, level)
  level = level or 2
  _error(debug.traceback(s),level)
end

---------------------------------------------------------------------------------------
-- Split a file name into it's components.
-- Works for both, windows and linux syntax.
-- Give fname in windows with double backslash.
-- @param fname string name of the file. 
-- @param trail bool output trailing separator.
-- @return dirname and basename.
---------------------------------------------------------------------------------------
function pathcomp(fname, trail)
  local pathname = ""
  local filename = ""
  local lastpath
  fname = string.gsub(fname, "([/\\]*)([^/\\]+)", 
		      function(u, v)
			if u == "\\" then u = u .. u end
			lastpath = pathname .. u
			pathname = lastpath .. v
			filename = v
			return v
		      end)
  if trail == nil then
    lastpath = string.gsub(lastpath, "(.+)([/\\]+)$", "%1")
    lastpath = string.gsub(lastpath, "(.+)([/\\]+)$", "%1")
  end
  if lastpath == "" then lastpath = "." end
  return lastpath, filename
end

pathsplit = pathcomp
_G._tostring = _tostring

return _G