-----------------------------------------------------------------------------------
-- @title LuaYats.
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 4.0 $Id: options.lua 420 2010-01-06 21:39:36Z leuwer $
-- @description  Luayats - Graphical User Interface (GUI)
-- <br>
-- <br><b>module: gui</b><br>
-- <br>
-- This module implements the Options dialog of Luayats GUI.
-- NOTE:
-- It does not contain any definitions for direct usage.
-----------------------------------------------------------------------------------
local iup = require "iuplua"
require "iupluacontrols"
local cfg = require "yats.config"
local logging = require "yats.logging.console"
require "yats.logging.ring"
require "yats.gui.images"

module("gui", package.seeall)

-----------------------------------------------------------------------------------
-- Locals
-----------------------------------------------------------------------------------
local yes, no, on, off = "YES", "NO", "ON", "OFF"
local horizontal = "HORIZONTAL"
local loglevels = {
   DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5
}

local tinsert = table.insert

local clone

-----------------------------------------------------------------------------------
-- Clone a table.
-- @param tab table to clone.
-- @return cloned table.
-----------------------------------------------------------------------------------
function clone(tab)
   local t = {}
   for k,v in pairs(tab) do
      if type(v) == "table" then
	 t[k] = clone(v)
      else
	 t[k] = v
      end
   end
   return t
end

function copy(from, to)
   for k,v in pairs(from) do
      if type(v) == "table" then
	 copy(v, to[k])
      else
	 to[k] = v
      end
   end
end

local conf = cfg.data
local newconf = clone(conf)
local rollback = {}

-----------------------------------------------------------------------------------
-- Generate an IUP listbox for selecting logging level.
-- @param title string Title of the listbox.
-- @param section string Section in config tree.
-- @param key string Key in config tree.
-- @param tip string Help tip.
-- @return userdata IUP listbox.
-----------------------------------------------------------------------------------
local function loglist(title, section, key, tip)
   local frm = iup.frame{
      title = title,
      expand = yes,
      iup.list{
	 "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
	 dropdown = yes,
	 tip = tip,
	 value = loglevels[conf[section][key]],
	 action = function(self, value) newconf[section][key] = value end,
	 rollback = function(self, conf) self.value = loglevels[conf[section][key]] end
      }
   }
   tinsert(rollback, frm[1])
   return frm
end

-----------------------------------------------------------------------------------
-- Apply options.
-- @param opt table Options to apply.
-- @return none.
-----------------------------------------------------------------------------------
local function apply_options(opt)
   yats.log = logging[opt.yats.LogType]("SIM "..opt.yats.LogPattern)
   yats.krnlog = logging[opt.yats.LogType]("KRN "..opt.yats.LogPattern)
   yats.clog = logging[opt.yats.LogType]("CPP "..opt.yats.LogPattern)
   yats.gralog = logging[opt.yats.LogType]("GRA "..opt.yats.LogPattern)
   gui.log = logging[opt.yats.LogType]("EDT "..opt.yats.LogPattern)
   logging.setringlen(opt.yats.LogSize)

   yats.log:setLevel(opt.yats.LogLevel)
   yats.krnlog:setLevel(opt.kernel.LogLevel)
   yats.clog:setLevel(opt.kernel.LogLevel)
   yats.gralog:setLevel(opt.kernel.LogLevel)
   gui.log:setLevel(opt.gui.LogLevel)

   notebook:dialog().title = opt.gui.MainDialogTitle
   notebook:dialog().size = opt.gui.EditorWindowSize
   notebook:setfont(opt.gui.EditorFont, "current")
   iup.Refresh(notebook:dialog())

   yats.setDefaultAttributes(opt.graphics.Attributes)

   copy(opt, conf)
end

-----------------------------------------------------------------------------------
-- Rollback options in dialog according to given config tree.
-- @param dlg userdata IUP dialog handle.
-- @param opt table Configuration tree.
-- @return none.
-----------------------------------------------------------------------------------
local function rollback_options(dlg, opt)
   for _, obj in ipairs(rollback) do
      obj:rollback(opt)
   end
   iup.Refresh(dlg)
end

-----------------------------------------------------------------------------------
-- Create (but not show) a preferences dialog to edit the configuration tree.
-- @return userdata IUP dialog handle.
-----------------------------------------------------------------------------------
function options()
   local margin = "10x10"
   local gap = "5"

   -- section: conf.yats
   local optyats = iup.vbox{
      expand = yes,
      margin = margin,
      -- 1 --
      iup.toggle{
	 title = "Warn if running simulation is reset.", 
	 tip = "A warning shall be issued to the user, if the simulation is reset while it is running,",
	 value = conf.yats.WarnRunningOnReset,
	 action = function(self) newconf.yats.WarnRunningOnReset = self.value end,
	 rollback = function(self, conf) self.value = conf.yats.WarnRunningOnReset end
      },
      -- 2 --
      iup.toggle{
	 title = "Save Config on Exit", 
	 tip = "Save Configuration when leaving Luayats.",
	 value = conf.kernel.SaveConfigOnExit,
	 action = function(self) newconf.kernel.SaveConfigOnExit = self.value end,
	 rollback = function(self, conf) self.value = conf.kernel.SaveConfigOnExit end
      },
      -- 3 --
      iup.toggle{
	 title = "Save Log on Exit", 
	 tip = "Save Logging to file 'luayats.log' when Luayats exits.", 
	 value = conf.gui.SaveLogOnExit,
	 action = function(self) newconf.gui.SaveLogOnExit = self.value end,
	 rollback = function(self, conf) self.value = conf.gui.SaveLogOnExit end
      },
      -- 4 --
      iup.hbox{
	 gap = gap,
	 iup.label{title = "Documentation:", alignment = aleft},
	 iup.text{
	    expand = horizontal,
	    value = conf.yats.DocPath
	 },
	 iup.button{
	    title = "...",
	    action = function(self)
			local path
			local fdlg = iup.filedlg{
			   dialogtype = "open",
			   title = "Open Directory",
			}
			fdlg:popup(iup.CENTER, iup.CENTER)
--			newconf.yats.DocPath = fdlg.value
			path = fdlg.value or conf.yats.DocPath
			newconf.yats.DocPath = string.gsub(path, "\\","/")
			iup.GetParent(self)[2].value = path
		     end,
	 },
	 rollback = function(self, conf) self[2].value = conf.yats.DocPath end
      },
      -- 5 --
      iup.label{title = "Logging Levels:", alignment = aleft},
      -- 6 --
      iup.vbox{
	 iup.hbox{
	    loglist("User", "yats", "LogLevel", "Level for user logger 'SIM'."),
	    loglist("Kernel", "kernel", "LogLevel", "Level for kernel logger 'KRN'."),
	    loglist("C++", "kernel", "CLogLevel", "Level for C++ logger 'CPP'."),
	    loglist("Graphics", "graphics", "LogLevel", "Level for graphics output logger 'GRA'."),
	 },
	 iup.hbox{
	    loglist("GUI", "gui", "LogLevel", "Level for the GUI logger 'GUI'."),
	    loglist("STP", "Protocols", "StpLogLevel", "Level for the Spanning Tree Protocol 'STP'."),
	    loglist("PDU", "Protocols", "PduLogLevel", "Level for Protocol Data Units 'PDU'")
	 }
      },
      -- 7 --
      iup.hbox{
	 gap = gap,
	 iup.label{title = "Logging Type", alignment = aleft},
	 iup.list{
	    "ring", "console",
	    tip = "Select logging type: ring = ringbuffer, console = console window",
	    dropdown = yes,
	    editbox = yes,
	    value = conf.yats.LogType,
	    action = function(self, txt, pos, state) 
			if tonumber(state) == 1 then
			   newconf.yats.LogType = txt 
			end
		     end,
	    rollback = function(self, conf) self.value = conf.yats.LogType end
	 }
      },
      -- 8 --
      iup.hbox{
	 gap = gap,
	 iup.label{title = "Logging Pattern", alignment = aleft},
	 iup.text{
	    tip = 
[[Edit logging pattern:
Patterns:
%level    level
%simtime  simulation time in ticks
%realtime simulation time in seconds
%message  actual message text
]],
	    value = conf.yats.LogPattern,
            expand = horizontal,
	    action = function (self) newconf.yats.LogPattern = self.value end,
	    rollback = function(self, conf) self.value = conf.yats.LogPattern end
	 }
      },
      -- 9 --
      iup.hbox{
	 gap = gap,
	 iup.label{title = "Logging Ring Length:", alignment = aleft},
	 iup.text{
	    tip = "Length of logging ring.",
	    value = conf.yats.LogSize,
	    spin = yes,
	    spinvalue = conf.yats.LogSize,
	    spinmax = 100000,
	    spinmin = 0,
	    spininc = 10,
	    spin_cb = function(self, number) 
			 mate = iup.GetParent(self)[3]
			 mate.value = number
			 self.value  = number 
			 newconf.yats.LogSize = number 
		      end,
	    rollback = function(self, conf) 
			  self.value = conf.yats.LogSize 
			  self.spinvalue = self.value 
		       end
	 },
	 iup.val{
	    horizontal,
	    min = 0,
	    max = 100000,
--	    type = horizontal,
	    value = conf.yats.LogSize,
	    valuechanged_cb = function(self) 
				local num = self.value
				local mate = iup.GetParent(self)[2]
				mate.value = num
				mate.spinvalue = num
				newconf.yats.LogSize = number 
			     end,
	    rollback = function(self, conf) self.value = conf.yats.LogSize end
	 }
      }
   }
   tinsert(rollback, optyats[1])
   tinsert(rollback, optyats[2])
   tinsert(rollback, optyats[3])
   tinsert(rollback, optyats[4])
   tinsert(rollback, optyats[7][2])
   tinsert(rollback, optyats[8][2])
   tinsert(rollback, optyats[9][2])
   tinsert(rollback, optyats[9][3])

   -- section: conf.gui
   local optgui = iup.vbox{
      expand = yes,
      margin = margin,
      -- 1 --
      iup.toggle{
	 title = "Automatic Highlight", 
	 tip = "Automatically syntax highlight file when opened.", 
	 value = conf.gui.AutoHighlight,
	 action = function(self) newconf.gui.AutoHighlight = self.value end,
	 rollback = function(self, conf) self.value = conf.gui.AutoHighlight end
      },
      -- 2 --
      iup.hbox{
	 iup.label{title = "Dialog Title:", alignment = aleft},
	 iup.text{
	    tip = "Title to appear in the main dialog's title bar.",
	    expand = horizontal,
	    value = conf.gui.MainDialogTitle,
	    killfocus_cb = function(self) newconf.gui.MainDialogTitle = self.value end,
	    rollback = function(self, conf) self.value = conf.gui.MainDialogTitle end
	 },
      },
      -- 3 --
      iup.hbox{
	 iup.label{title = "Editor Font:", alignment = aleft},
	 iup.text{
	    tip = "Font used in buffers.",
	    expand = horizontal,
	    value = conf.gui.EditorFont,
	    rollback = function(self, conf) self.value = conf.gui.EditorFont end
	 },
	 iup.button{
	    title = "Select Font ...",
	    action = function(self) 
			local font = notebook:deffont() 
			newconf.gui.EditorFont = font 
			iup.GetParent(iup.GetParent(self))[3][2].value = font 
		     end,
	    rollback = function(self, conf) self.value = conf.gui.EditorFont end
	 }
      },
      -- 4 --
      iup.hbox{
	 gap = gap,
	 iup.label{title = "Startup Window Size:", alignment = aleft},
	 iup.list{
	    "CURRENT", "FULLxFULL", "HALFxHALF", "THIRDxTHIRD", "QUARTERxQUARTER",
	    tip = "Select startup window size.",
	    dropdown = yes,
	    editbox = yes,
	    value = conf.gui.EditorWindowSize,
	    action = function(self) 
			local newval
			if self.value == "CURRENT" then
			   newval = notebook:dialog().size
			else
			   newval = self.value
			end
			self.value = newval
			newconf.gui.EditorWindowSize = newval
		     end,
	    rollback = function(self, conf) self.value = conf.gui.EditorWindowSize end
	 }
      },
      -- 5 --
      iup.hbox{
	 gap = gap,
	 iup.label{title = "Maximum Undo Levels:", alignment = aleft},
	 iup.text{
	    tip = "Adjust number of undos in editor.",
	    value = conf.gui.MaxUndo,
	    spin = yes,
	    spinvalue = conf.gui.MaxUndo,
	    spinmax = 100,
	    spinmin = 1,
	    spininc = 1,
	    spin_cb = function(self, number) self.value  = number newconf.gui.MaxUndo = number end,
	    rollback = function(self, conf) self.value = conf.gui.MaxUndo self.spinvalue = self.value end
	 }
      }
   }
   tinsert(rollback, optgui[1])
   tinsert(rollback, optgui[2][2])
   tinsert(rollback, optgui[3][2])
   tinsert(rollback, optgui[4][2])
   tinsert(rollback, optgui[5][2])
   
   local tips = {
General = 
[[
CanvasType - Type of drawing canvas
   CDTYPE_DBUFFER | CDTYPE_SERVERIMG | CDTYPE_IUP
PolyType - Polygon type
   OPEN_LINES, BEZIER
PolyStep - Draw steps rather than direct point connections
   1 or 0
UsePoly - Draw polygons rather than single lines
   1 or 0
]],
Draw = 
[[
Oper - Type of drawing operation
   REPLACE, XOR, NOT_XOR, 
Color - Drawing Color
   BLACK, WHITE 
   RED, BLUE, GREEN, YELLOW, MAGENTA, CYAN, GRAY
   (also with prefix DARK_, e.g. DARK_RED).
Style - Drawing Style
   CONTINUOUS, DOTTED, DASHED, DASH_DOT, DASH_DOT_DOT
Width - Drawing Width
   in pixel
]],
Text =
[[
Oper - Type of text drawing operation
   REPLACE, XOR, NOT_XOR
Face - Face of text
   Helvetica, Times, Courier
Style - Text style
   PLAIN, BOLD, ITALIC, UNDERLINE, STRIKEOUT
Color - Text color
   BLACK, WHITE 
   RED, BLUE, GREEN, YELLOW, MAGENTA, CYAN, GRAY
   (also with prefix DARK_, e.g. DARK_RED).
Size - Text size
   in pixel
]],
Background = 
[[
Opacity - Background opacity
   TRANSPARENT, OPAQUE
Color - Background color
   BLACK, WHITE 
   RED, BLUE, GREEN, YELLOW, MAGENTA, CYAN, GRAY
   (also with prefix DARK_, e.g. DARK_RED).
]]
}
   local function attribs(section)
      local retval = iup.text{
	 expand = yes,
	 multiline = yes,
	 tip = tips[section],
	 value = "return ".. yats.pretty(conf.graphics.Attributes[section]),
	 killfocus_cb = function(self)
			   local f, err = loadstring(self.value)
			   if not f then
			      iup.Message("ERROR", "Syntax error in attribute definition")
			   else
			      newconf.graphics.Attributes[section] = f()
			   end
			end,
	 rollback = function(self, conf)
		       self.value = "return "..yats.pretty(conf.graphics.Attributes[section])
		    end
      }
      table.insert(rollback, retval)
      return retval
   end

   -- section: conf.graphics
   local optgraphics = iup.tabs{
      expand = yes,
      attribs("General"),
      tabtitle0 = "General",
      attribs("Draw"),
      tabtitle1 = "Draw",
      attribs("Text"),
      tabtitle2 = "Text",
      attribs("Background"),
      tabtitle3 = "Background"
   }

   -- Tabbed notebook with different sections
   local tabopt = iup.tabs{
      tabtitle0 = "General",
      optyats,
      tabtitle1 = "User Interface",
      optgui,
      tabtitle2 = "Graphical Output",
      optgraphics,
      expand = yes
   }
   -- Buttons
   local butcancel = iup.button{
      title = "&Close", 
      image = images.cancel,
      tip = "Hide this dialog",
      action = function(self) 
		  conf = clone(newconf)
		  local dlg = iup.GetDialog(self)
		  iup.Destroy(dlg)
		  rollback = {}
	       end
   }
   local butdefault = iup.button{
      title = "&Default", 
      image = images.undo,
      tip = "Revert values to default settings. Note: the values are not applied.",
      action = function(self)
		  newconf = cfg.default()
		  rollback_options(iup.GetDialog(self), newconf)
		  notebook:message("Configuration settings reverted to default.")
	       end
   }
   local butapply = iup.button{
      title = "&Apply", 
      image = images.check,
      tip = "Apply all configuration settings.",
      action = function(self) 
		  apply_options(newconf)
		  notebook:message("Configuration setting applied.")
	       end
   }
   local butsave = iup.button{
      title = "&Save", 
      image = images.save,
      tip = "Save settings",
      action = function(self) 
		  local fname = os.getenv("HOME").."/.luayatsrc"
		  local fout = io.open(fname, "w")
		  fout:write("return "..pretty(newconf))
		  fout:close()
		  notebook:message(string.format("Configuration setting written to %q", fname))
	       end
   }

   -- The options dialog.
   local dlg = iup.dialog{
      title = "Preferences",
      icon = images.app,
      iup.vbox{
	 margin = "x5",
	 tabopt,
	 iup.label{separator = horizontal},
	 iup.hbox{
	    iup.fill{}, butcancel, butapply, butdefault, butsave
	 }
      },
      unmap_cb = function(self) rollback = {} end,
      close_cb = function(self) rollback = {} end,
      parentdialog = notebook:dialog(),
      butapply = butapply
   }
   return dlg
end
return gui