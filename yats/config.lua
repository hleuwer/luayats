-----------------------------------------------------------------------------------
-- @title LuaYats.
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: config.lua 424 2010-01-13 20:47:31Z leuwer $
-- @description Luayats - Configuration
-- <br>
-- <br><b>module: yats.config</b><br>
-- <br>
-- This module defines utilities for Luayats configuration administration
-----------------------------------------------------------------------------------

local iup = require "iuplua"
require "yats.stdlib"

--local logging = require "yats.logging.console"
local logging = require "yats.logging.ring"

module("config", package.seeall)

local on, off, yes, no = "ON", "OFF", "YES", "NO"

driver = iup.GetGlobal("DRIVER")

--iup.SetLanguage("ENGLISH")

--- Module private logging.
--local log = logging.console("CFG %level %simtime (%realtime) %message\n")
local log = logging.ring("CFG %level %simtime (%realtime) %message\n")
log:setLevel("DEBUG")
log:info(string.format("Logger CFG created: %s", os.date()))


data = {}
rcname = ""
saved = false

local _rcname = ".luayatsrc"
--- 
-- Configuration file names
-- @class table
-- @name cfg_fnames
-- @field 1   ./.luayatsrc
-- @field 6   $HOME/.luayatsrc
-- @field 2   $PWD/.luayatsrc
-- @field 3   Lua variable LUAYATSRC,
-- @field 4   $LUAYATSRC
-- @field 5   $LUAYATSHOME/.luayatsrc
local cfg_fnames = {
  "./".._rcname,
  os.getenv("HOME").."/".._rcname,
--  os.getenv("PWD").."/".._rcname,
  LUAYATSRC,
  os.getenv("LUAYATSRC"),
  os.getenv("LUAYATSHOME").."/".._rcname
}

--- Get numerical size from string widthxheight.
-- @param s string size.
-- @return width,height.
function getScreenSize(s)
  local xs, ys
  string.gsub(s, "(%d*)x(%d*)", function(x,y) xs=tonumber(x) ys=tonumber(y) end)
  return xs, ys
end
get_size = getScreenSize

--- Set size string from numerical values.
-- @param width number width.
-- @param height number height.
-- @return string representation widthxheight.
function setScreenSize(width, height)
  return string.format("%dx%d", width,height)
end
set_size = setScreenSize

--- Read Configuration.
-- @return Configuration table.
function read()
   for _,v in pairs(cfg_fnames) do
      log:debug(string.format("Checking preferences in '"..v.."'"))
      local fc = io.open(v, "r")
      if fc then 
	 rcname = v
	 fc:close()
	 local f,err = loadfile(rcname)
	 if f then
	    log:info("Loading preferences from '"..rcname.."'")
	    data = f()
	    return data
	 else
	    iup.Message("ERROR", "Syntax error in configuration script\n\n"..err)
	    log:fatal("Aborted due to error in configuration script.\nDid you see the message showing the erorr? If not try again :-)")
	    os.exit()
	 end
      end
   end
   log:warn("No preference file found. Using default and saving in homedir.")
   rcname = os.getenv("HOME").."/".._rcname
   local data = default()
   save(data)
   return data
end

function capabilities()
   local driver = iup.GetGlobal("DRIVER")
   if driver ~= "Win32" then
      return {
	 formatText = yes,
	 canHighlight = yes,
	 canPrint = no
      }
   else
      return {
	 formatText = no,
	 canHighlight = no,
	 canPrint = no
      }
   end
end

--- Get the default configuration table.
-- @return Default configuration.
function default()
   if driver ~= "Win32" then
      log:debug("Set default preferences for driver "..driver.." (non-Windows).")
      return {
	 yats = {
	    WarnRunningOnReset = "yes",
	    LogType = "ring",
	    LogLevel = "WARN",
	    LogPattern = "%level %simtime (%realtime) %message",
	    DocPath = "/usr/local/share/luayats/doc/index.html",
	    BrowserPath = "/opt/firefox/firefox",
	    LogSize = 10000
	 },
	 gui = {
	    EditorWindowSize = "HALFxHALF",
	    EditorFont="Monospace, 9",
	    AutoHighlight = "off",
	    MainDialogTitle = "LUAYATS Network Simulator",
	    LogLevel = "WARN",
	    MaxUndo = 5,
	    SaveLogOnExit = "off",
	    MaxRecent = 5
	 },
	 graphics = {
	    LogLevel = "WARN",
	    Attributes = {
	       General = {
		  CanvasType = "CDTYPE_DBUFFER",
		  PolyType = "OPEN_LINES",
		  PolyStep = 0,
		  UsePoly = 1,
	       },
	       Draw = {
		  Oper = "REPLACE",
		  Color = "BLACK",
		  Style = "CONTINUOUS",
		  Width = 1,
	       },
	       Text = {
		  Oper = "REPLACE",
		  Face = "Helvetica",
		  Style = "PLAIN",
		  Color = "BLACK",
		  Size = 8,
	       },
	       Background = {
		  Opacity = "TRANSPARENT",
		  Color = "WHITE",
	       }
	    },
	    Paper = "A4",
	    Layout = "portrait",
	    Printfile = {
	       Type = "pdf",
	       Name = "luayats-printout",
	       Path = "./"
	    },
	    Resolution = 75,
	    Margin = {
	       Left = 25,
	       Right = 20,
	       Top = 25,
	       Bottom = 25
	    }
	 },
	 kernel = {
	    LogLevel = "WARN",
	    CLogLevel = "WARN",
	    ShowDots = "yes",
	    SaveConfigOnExit = "on"
	 },
	 Protocols = {
	    StpLogLevel = "WARN",
	    PduLogLevel = "WARN"
	 },
	 Recent = {
	 }
      }
   else
      log:debug("Set default preferences for driver "..driver.." (Windows).")
       return {
	 yats = {
	    WarnRunningOnReset = "yes",
	    LogType = "ring",
	    LogLevel = "WARN",
	    LogPattern = "%level %simtime (%realtime) %message",
	    DocPath = "c:/cygwin/usr/local/share/luayats/doc/index.html",
	    BrowserPath = "",
	    LogSize = 10000
	 },
	 gui = {
	    EditorWindowSize = "HALFxHALF",
	    EditorFont="Monospace, 9",
	    AutoHighlight = "off",
	    MainDialogTitle = "LUAYATS Network Simulator",
	    LogLevel = "WARN",
	    MaxUndo = 5,
	    SaveLogOnExit = "off",
	    MaxRecent = 5
	 },
	 graphics = {
	    LogLevel = "WARN",
	    Attributes = {
	       General = {
		  CanvasType = "CDTYPE_DBUFFER",
		  PolyType = "OPEN_LINES",
		  PolyStep = 0,
		  -- We donot use polygons in Windows - sometimes unstable
		  UsePoly = 0,
	       },
	       Draw = {
		  Oper = "REPLACE",
		  Color = "BLACK",
		  Style = "CONTINUOUS",
		  Width = 1,
	       },
	       Text = {
		  Oper = "REPLACE",
		  Face = "Helvetica",
		  Style = "PLAIN",
		  Color = "BLACK",
		  Size = 8,
	       },
	       Background = {
		  Opacity = "TRANSPARENT",
		  Color = "WHITE",
	       }
	    },
	    Paper = "A4",
	    Layout = "portrait",
	    Printfile = {
	       Type = "pdf",
	       Name = "luayats-printout",
	       Path = "./"
	    },
	    Resolution = 75,
	    Margin = {
	       Left = 25,
	       Right = 20,
	       Top = 25,
	       Bottom = 25
	    }
	 },
	 kernel = {
	    LogLevel = "WARN",
	    CLogLevel = "WARN",
	    ShowDots = "yes",
	    SaveConfigOnExit = "on"
	 },
	 Protocols = {
	    StpLogLevel = "WARN",
	    PduLogLevel = "WARN"
	 },
	 Recent = {
	 }
      }
   end
end

--- Save current configuration.
-- @param cfg table - Configuration table.
-- @return none.
function save(cfg)
  if saved == true then
    -- We only save on the fly, if preferences have not been edited manually.
    log:info("Preferences already saved.")
  else
    local f = io.open(rcname,"w+")
    if f then
      local s = "return "..pretty(cfg)
      f:write(s)
      f:close()
      log:info("Current preferences saved in file '"..rcname.."'")

    else
      error("Cannot save preferences in file '"..rcname.."'.")
    end
  end
end

--- Read a specific value from configuration.
-- @param folder string - configuration group/folder
-- @param key string - key for value.
-- @return none
function get(folder, key, ignoremissing)
   if (data[folder] and data[folder][key]) then
      log:debug(string.format("Read '%s.%s' = %q (%s)", 
			      folder, key, tostring(data[folder][key]), type(data[folder][key])))
      if key == "max_editors" then
	 return data[folder][key]
      else
	 return data[folder][key]
      end
   else
      if not ignoremissing then
	 local s = string.format("Config entry '%s.%s' not found. Using default", folder, key)
	 log:error(s)
	 error(s)
      end
   end
   return nil
end

--- Set a specfic value in the configuration.
-- @param folder string - configuration group/folder.
-- @param key string - configuration key
-- @param value any - configuration value
-- @return none
function set(folder, key, value)
  data[folder][key] = value
end

--
-- Read the configuration
--
data = read()
if data == nil then
  rcname = "luayatsrc"
  data = default()
end

capa = capabilities()

return config