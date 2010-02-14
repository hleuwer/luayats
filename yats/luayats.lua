#!/usr/bin/env luayats-bin
--require "profiler"
--start("prof.out")
local __require = require
local rindent = 2
function ___require(mod)
   print("#1# require "..string.rep(" ", rindent)..mod, collectgarbage("count"))
   rindent = rindent + 2
   local retval =  __require(mod)
   rindent = rindent - 2
   return retval
end
-----------------------------------------------------------------------------
-- Local utils
-----------------------------------------------------------------------------
local function eprint(fmt, ...)
   io.stderr:write(string.format(fmt.."\n", ...))
end

if not os.getenv("LUAYATSHOME") then
   eprint("LUAYATSHOME not set - stopped.")
   os.exit(1)
else
   package.path = os.getenv("LUAYATSHOME").."/?.lua;"..package.path
end

-----------------------------------------------------------------------------
-- Logging for this module.
-----------------------------------------------------------------------------
local cfg = require "yats.config"
require "yats.core"
--require "yats.graphics"
local conf = cfg.data
local logging = require "yats.logging.console"
local logging = require "yats.logging.ring"
local gui = require "yats.gui.runctrl"
-- Module private logging.
local log = logging[conf.yats.LogType]("BEG %level %simtime (%realtime) %message\n")
log:setLevel(conf.yats.LogLevel)
log:info("Logger BEG created")

require "iuplua"
require "yats.core"
require "yats.stdlib"
require "yats.info"
local getopt = require "yats.getopt"
local gui = require "yats.gui.editor"

-----------------------------------------------------------------------------
-- Locals
-----------------------------------------------------------------------------
local conf = cfg.data

-----------------------------------------------------------------------------
-- Globals
-----------------------------------------------------------------------------
_G._DORUN = false
_G._GUILOGLEVEL = nil

local function usage(arg)
--   io.stderr:write[[usage: luayats [options] [testscripts]
   eprint[[
usage: luayats [options] [testscripts]
Available options are:
 -D, --default-opts     load default options instead of rc file.
 -L, --gui-log-level=LEVEL  set GUI log level to LEVEL 
                            (DEBUG, INFO, WARN, ERROR, FATAL)
 -H, --help             print this help text
 -n                     do not start the GUI
 -r                     run last loaded test in GUI mode
 -v, --version          show Luayats version.
 -i, --info=WHAT        show Luayats binding info. WHAT defines what to show
                        n,b,s: numeric,bool,string constants
                        f: functions
                        c: classes                        
                        a: all
-d, --doc=TAGFILE       produce HTML doc of binding (default: doc/cpp/luayats.xml).
-l, --luadoc=docdir     generate Luadoc HTML anchor data base (default: doc/lua/files/yats).
-o, --out=FILE          write output to file instead of stdout.
-R, --read=FILE         read Luadoc HTML anchor data base

Notes:
 (1) When running in non-GUI mode, hit <ctrl-C> twice to stop execution.
 (2) The simulator is NOT reset in non-GUI mode.
]]
end

local function version()
   local _version = 
      yats.YATS_VERSION_MAJOR.."."..
      yats.YATS_VERSION_MINOR.."-"..
      yats.LUAYATS_VERSION_MAJOR.."."..
      yats.LUAYATS_VERSION_MINOR
   eprint("Luayats %s", _version)
   eprint("svn version $Id:")
end

-----------------------------------------------------------------------------
-- MAIN entry - Lua level
-----------------------------------------------------------------------------
local function main(arg)
   local fout
   local use_gui = true
   local longopts = {
      {"no-gui", "n", "-n"},
      {"gui-log-level", "r", "-L"},
      {"help", "n", "-H"},
      {"info", "r", "-i"},
      {"run", "n", "-r"},
      {"default-opts", "n", "-D"},
      {"version", "n", "-v"},
      {"doc", "o", "-d"},
      {"luadoc", "o", "-l"},
      {"read", "r", "-R"},
      {"out", "r", "-o"},
      {"zzz", "r", "-z"}
   }
   local opts, pargs, err = getopt.getopt(arg, "d:i:no:rvz:R:lL:DH", longopts, 1)
   _G._PROGRAMARGS = pargs
   local luadoc, outfile
   local tagfile = "doc/cpp/luayats.xml"
   local docdir = "doc/lua/files/yats"
   local luadocfile = "doc/ldocindex.lua"
   local cmd = ""
   local cmdarg = ""
   for i = 1, #opts do
      local opt = opts[i]
      if opt.sopt == "-n" then
	 use_gui = false
      elseif opt.sopt == "-H" then
	 usage(arg)
	 os.exit(0)
      elseif opt.sopt == "-D" then
	 log:debug("Loading default preferences")
	 cfg.data = cfg.default()
      elseif opt.sopt == "-L" then
	 _G._GUILOGLEVEL = opt.arg
      elseif opt.sopt == "-r" then
	 _G._DORUN = true
      elseif opt.sopt == "-v" then
	 version()
	 os.exit(0)
      elseif opt.sopt == "-i" then
	 cmdarg = opt.arg
	 cmd = "info"
      elseif opt.sopt == "-o" then
	 fout = io.open(opt.arg, "w+")
      elseif opt.sopt == "-d" then
	 if opt.arg then tagfile = opt.arg end
	 cmd = "htmldoc"
      elseif opt.sopt == "-l" then
	 if opt.arg then docdir = opt.arg end
	 cmd = "luadoc"
      elseif opt.sopt == "-R" then
	 luadoc = assert(loadfile(opt.arg))()
      elseif opt.sopt == "-Z" then
	 luadoc = assert(loadfile(opt.arg))()
	 print(pretty(luadoc))
	 os.exit(0)
      end
   end
   if cmd == "htmldoc" then
      log:debug("Generating HTML documentation from "..tagfile)
      if not luadoc then
	 luadoc = assert(loadfile(luadocfile))()
      end
      local htdoc = yats:htmlInfo(tagfile, luadoc)
      if fout then
	 fout:write(table.concat(htdoc))
	 fout:close()
      else
	 print(table.concat(htdoc))
      end
      os.exit(0)
   elseif cmd == "luadoc" then
      log:debug("Generating LUADOC database")
      local b = yats:getInfo()
      local ldoc = yats.getLuadocInfo(b, docdir)
      if fout then
--	 fout:write("return "..pretty(ldoc))
	 fout:close()
      else
--	 print("return "..pretty(ldoc))
      end
      os.exit(0)
   elseif cmd == "info" then
      yats:showInfo(cmdarg)
      os.exit(0)
   end

   if fout then fout:close() end

   -- Logging Levels
   yats.log:setLevel(string.upper(_SIMLOG or 
				  os.getenv("LUAYATS_SIMLOG") or 
				     string.upper(conf.yats.LogLevel) or 
				     "ERROR"))
   
   yats.kernel_log:setLevel(string.upper(_KNLLOG or 
					 os.getenv("LUAYATS_KNLLOG") or 
					    string.upper(conf.kernel.LogLevel) or 
					    "ERROR"))
   if use_gui then
      -- Run the GUI
      local notebook = gui.editor(nil, nil, unpack(pargs))
      iup.MainLoop()
   else
      local exitval = 0
      -- Run user scripts w/o GUI - graphics output still usable
      for i = 1, #pargs do
	 local success, retval
	 local fn, err = io.open(pargs[i],"r")
	 if not fn then
	    log:error(string.format("Cannot open file %q.", pargs[i]))
	    eprint(string.format("Cannot open file %q.", pargs[i]))
	    exitval = 1
	 else
	    local s = fn:read("*a")
	    fn:close()
	    local worker = gui.initSimulation(s, true)
	    gui.resetSimulation()
	    log:info("Simulation started without GUI support. File: "..pargs[i])
	    eprint("Simulation started without GUI support. File: "..pargs[i])
	    while true do
	       success, retval = pcall(worker)
	       if success == true then
		  if retval == "done" then
		     -- continue after complete run
		  elseif retval == "continue" then
		     -- continue after partial run
		  elseif retval == "reset" then
		     -- command: reset
		  elseif retval == "pause" then
		     -- command: pause
		     iup.Message("Information", "Script paused!\nHit ok to continue.")
		  else
		     -- script finished
		     break
		  end
	       else
		  if string.find(retval, "__ABORTED") then
		     -- Test was aborted externally
		     log:info("Simulation aborted.")
		     os.exit(exitval)
		  else
		     -- A run-time error occurred - display error message
		     eprint("Simulation aborted with runtime error.")
		     eprint("%s", retval)
		     eprint("%s", debug.traceback())
		     os.exit(1)
		  end
	       end
	    end
	 end
      end
      log:info("Simulation finished without GUI support.")
      if exitval == 0 then
	 eprint("Simulation finished.")
      end
      os.exit(exitval)
   end
end

main(arg)