-----------------------------------------------------------------------------------
-- @copyright Herbert Leuwer, 2003.
-- @author Herbert Leuwer.
-- @release 3.0 $Id: getopt.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description Luayats - Command line option parsing for Lua 5
--<br>
--<br><b>module: getopt</b><br>
-- <br>
-- This module provides a single function 'getopt'. It allows command 
-- line option parsing in Lua. Both short and long options are supported 
-- similar to the GNU 'getopt' library.
-----------------------------------------------------------------------------------

module("getopt", package.seeall)

_VERSION="5.0.2"

local iref={}
local refi=1

-- Build the option reference.
local function build_optref(options, long_options)
  local t = {}
  local u = {}
  local f = {}
  local s
  local i = 1
  local j = 1
  -- short options first
  repeat
    s = string.sub(options,i,i)
    u["-"..s] = s
    if s ~= "" then
      t["-"..s]=0
      if string.sub(options,i+1,i+1) == ":" then
	t["-"..s] = 1
	i = i + 1
      end
      i = i + 1
    end
  until s == ""
  -- now the long options
  for i = 1, #long_options do
    s = long_options[i][1]
    a = long_options[i][2]
    -- provide the short opts equivalent
    u["--"..s] = long_options[i][3]
    f["--"..s] = long_options[i][4]
    if a == "r" then
      t["--"..s] = 2
    elseif a == "n" then
      t["--"..s] = 0
    elseif a == "o" then
      t["--"..s] = 1
    end
  end
  return t, u, f
end

-- Add an option.
local function add_opt(tab, opt, arg, sopt, func)
   local optind = iref[opt]
   if optind then
      -- option already set
      table.remove(tab, optind)
      table.insert(tab, optind, {opt=opt, arg=arg, sopt=sopt})
   else
      table.insert(tab, refi, {opt=opt, arg=arg, sopt=sopt})
      iref[opt] = refi
      refi = refi + 1
   end
   if func then func{opt=opt, arg=arg, sopt=sopt} end
end

-- Handle error.
local function handle_err(err, flag)
  if flag then
    err = err .. " - ignored."
    io.stderr:write(err.."\n")
    return err
  else
    error(err)
  end
end

--------------------------------------------------------------
-- Parse command line options.
-- ~
-- Long options are described in a table of the following format:
-- <pre>
-- longopts = {<br>
--     {"all",     "o", "a", foo},<br>
--     {"verbose", "n", "v"},<br>
--     {"output",  "r", "?"}<br>
-- }
-- </pre>
-- The function returns 2 tables: 'opts' is an option descriptor of the form
-- <code>opts = {opt, arg, sopt}</code>, where 'opt' contains the options string, 'arg'
-- contains an eventual argument and 'sopt' contains a short option equivalent.
-- <ul>
-- <li> The first (mandatory) entry contains the name of the long option.
-- <li> The second (mandatory) entry contains a flag that mark whether an argument
--   to the options is 'o'=optional, 'n'=not required or 'r'=required.
-- <li> The third (mandatory) entry defines a short option equivalent, that is afterwards
--   found in 'opt.sopt'.
-- <li> The fourth (optional) entry defines a function that is called when an option 
--   is found. This can be used to act directly on options, e.g. to set a program flag.
--   The function receives the option descriptor as argument.
-- </ul>
-- @param arg table - Table with arguments from command line.
-- @param options string - Short options descriptor.
-- @param long_options table - Table describing long options.
-- @param ignore_err  Error handling flag.
-- @return opts, args, err: Option descriptor table, argument table and error message.
function getopt(arg, options, long_options, ignore_err)
   local cont = 1
   local i
   local err = nil
   local opts = {n=0}
   local pargs = {n=0}
   local argtab = {}
   -- Build a reference table with options from options string
   local optref, shortref, funcref = build_optref(options, long_options)
   --d  iref = {}
   --d  refi = 1
   -- Now run through args list
   local narg = #arg
   local i = 1
   while i <= narg do
      -- A '--' stops option scan
      local this = arg[i]
      local next = arg[i + 1]
      if this == "--" then
	 cont = i + 1
	 break
      end
      if string.sub(this,1,1) == "-" and string.sub(this, 2, 2) ~= "-" then
	 -- Looks like short options
	 local thisall = this
	 local j
	 -- run through concatenated options of the form: -abcd - at least one!
	 for j = 2, string.len(thisall) do
	    this = "-"..string.sub(thisall, j, j)
	    if optref[this] then
	       -- Is in reflist
	       if optref[this] == 1 then
		  -- An argument is expected: check this
		  if not next or optref[next] or next == "--" or string.sub(next,1,1) == "-" then
		     -- Ups! arg missing: error
		     err = handle_err("Option argument missing for option '"..this.."'", ignore_err)
		  else
		     if j == string.len(thisall) then
			-- seems to be o.k.
			add_opt(opts, this, next, this)
			i = i + 1
			cont = i + 1
		     else
			err = handle_err("Option argument missing for option '"..this.."'", ignore_err)
		     end
		  end
	       else
		  -- no argument expected
		  add_opt(opts, this, nil, this)
		  cont = i + 1
	       end
	    else
	       -- not in option list: error
	       err = handle_err("Invalid option '"..this.."'", ignore_err)
	       cont = i + 1
	    end
	 end
      elseif string.sub(this, 1, 2) == "--" then
	 -- Looks like a long option
	 -- Break the argument
	 local thisarg = nil
	 i1, i2= string.find(this,"=")
	 if i1 then
	    if i1 < string.len(this) then
	       thisarg = string.sub(this, i2 + 1)
	    else
	       thisarg = nil
	    end
	    this = string.sub(this, 1, i1 - 1)
	 end
	 if optref[this] then
	    if optref[this] > 0 then
	       if optref[this] == 2 and thisarg == nil then
		  err = handle_err("Option argument missing for option '"..this.."'", ignore_err)
	       else
		  -- An argument is possible
		  add_opt(opts, this, thisarg, shortref[this], funcref[this])
		  cont = i + 1
	       end
	    else
	       -- No option allowed
	       if thisarg then
		  err = handle_err("Option argument given for option '"..this.."'", ignore_err)
	       else
		  add_opt(opts, this, nil, shortref[this], funcref[this])
		  cont = i + 1
	       end
	    end
	 else
	    err = handle_err("Invalid option '"..this.."'", ignore_err)
	    cont = i + 1
	 end
      else
	 cont = i
	 break
      end
      i = i + 1
   end
   -- rest of arg 
   for i = cont, narg do
      table.insert(pargs, arg[i])
   end
   return opts, pargs, err
end

return _G.getopt