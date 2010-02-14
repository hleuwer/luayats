---------------------------------------------------------------
-- @copyright GNU Public License.
-- @author Herbert Leuwer, Backnang.
-- @release 3.0 $Id: graphics.lua 433 2010-02-13 06:08:00Z leuwer $
-- @description LuaYats - Graphic Objects.
-- <br>
-- <br><b>module: yats</b><br>
-- <br>
-- <b>Overview</b><br>
-- This module provides two types of graphical displays: a dialog with a single
-- display called <i>display</i> and a dialog with an array of displays called
-- <i>bigdisplay</i>
-- Each single display is associated with an measurement instrument.<br>
-- Displays are very fast due the fact that the display has direct access to
-- measurement data via internal pointers.<br><br>
-- <b>Attributes</b><br>
-- The appearence of all graphical displays can be controlled via attributes.
-- Assuming an instance of an instrument <i>instr</i> with an associated C-level
-- object <i>cinstr</i> in your Lua simulation script, the appearance of the
-- graphical display can be controlled using attributes, which affects either 
-- text, graphics or the windows background.
-- <pre>
--    instr.display:setattrib("textcolor", yats.CD_BLUE)<br>
--    or directly:<br>
--    instr.display.cinstr.textcolor = yats.CD_BLUE,
-- </pre>
-- Attributes can be retrieved via
-- <pre>
--    var = instr.display:getattrib("textcolor")<br>
--    or directly:<br>
--    var = instr.display.cinstr.textcolor 
-- </pre>
-- Note, that color related attributes take only effect, if the <i>linecolor</i> attribute
-- for the instrument is not set (left to default).<br>
-- <br>
-- The following attributes are defined:<br><br>
-- 
-- <b>Window background attributes:</b>
-- <ul>
-- <li>bgcolor - Color of window background: see <i>Colors</i> below.<br>
-- <li>bgopacity - Opacity of the window: CD_TRANSPARENT (default), CD_OPAQUE.<br>
-- </ul>
-- <b>Text attributes:</b>
-- <ul>
-- <li>textcolor - Text color: see Colors below.
-- <li>texttype  - Text font: see CD library documentation.
-- <li>textface  - Text face: see CD library documentation.
-- <li>textsize - Text size in pixel: see CD library documentation.
-- <li>textoper - Text drawing operation: see CD library documentation.
-- </ul>
-- <b>Graphics attributes:</b>
-- <ul>
-- <li>drawcolor - Color of lines: see CD documentation.
-- <li>drawstyle - Style of lines: see CD documentation.
-- <li>drawwidth - Width of lines: a number indicating the width of lines in pixel.
-- <li>drawoper - Drawing operation for graphics: see see CD documentation.
-- <li>usepoly  - 0=Draw bars, 1=Draw vertexes of a polygon (default).
-- <li>polytype - Interpolation: CD.OPEN_LINES (default), CD.BEZIER.
-- <li>polystep - Step Mode: <ul>
-- <li>             0=linear connection of vertices
-- <li>             1=stepwise (horizontal first) connection of vertices, 
-- <li>            -1=stepwise (vertical first) connection of vertices.</ul>
-- <li>canvastype - Type of drawing canvas:<ul>
-- <li>             CDTYPE_IUP draws directly into the canvas.<br>
-- <li>             CDTYPE_DBUFFER uses a double buffer.<br>
-- <li>             CDTYPE_SERVERIMG (default, most flickerfree) draws into a background server
--                  image and copies the image into the visible canvas. It cannot be zoomed.</ul>
-- </ul>
-- <b>Colors:</b><br>
-- see CD library documentation for valid color values.<br>
-- <br>
-- The attribute table does not need to be complete. Missing values are 
-- automatically replaced by their default values.<br>
-- <br>
-- <b>Printing, Plotting and Snapshots</b><br>
-- A graphical display provides means to print, plot and save it's contents. 
-- <br>
-- Data can be saved via snapshot. Right click on the display opens a menu
-- for printing, plotting and saving snapshots. The simulation will halt as long as the 
-- menu is open.<br><br>
-- <b>References</b><br>
-- <a href="http://www.tecgraf.puc-rio.br/cd">CD Canvas Draw - 2D graphics library</a>

-- $Id: graphics.lua 433 2010-02-13 06:08:00Z leuwer $

require "iuplua"
require "iupluacd"
local cd = require "cdlua"
-- Cannot load cdluapdf after cdlua ==> segfault. Why ????
if iup.GetGlobal("DRIVER") ~= "Win32" then
   require "cdluapdf"
else
   cd = require "cdluapdf"
end
local logging = require "yats.logging.console"
local config = require "yats.config"
local gui = require "yats.gui.images"
local conf = config.data

require "iupluacontrols"
require "iuplua_pplot"

module("yats", yats.seeall)

--
-- Module local logging
--
local log = logging[conf.yats.LogType]("GRA "..conf.yats.LogPattern)
log:setLevel(conf.graphics.LogLevel)
log:info(string.format("Logger GRA created: %s", os.date()))
gralog = log
--
-- Some utility functions
--
local paperformats = {
  A4 = {
    cdval = cd.A4,
    orient = {
      portrait = {w = 210, h = 297},
      landscape = {w = 297, h = 210}
    }
  },
  A3 = {
    cdval = cd.A3,
    orient = {
      portrait = {w = 297, h = 420},
      landscape = {w = 420, h = 297}
    }
  }
}

local defmargin = {
  left = 25, right = 20,
  top = 25, bottom = 25
}

local snapshot_delim = "\t"

--
-- Write an instrument measurement snapshot to a file.
--
local function snapshot2file(win)
  local b = 2
  local err = 0
  local fnam
  while b == 2 do
    while err == 0 do
      iup.SetLanguage("ENGLISH")
      fnam, err = iup.GetFile("./*.txt")
      if err == -1 then 
	-- cancelled
	return 
      elseif err == 0 then
	-- file exists
	b = iup.Alarm("WARNING", "File exists! Overwrite ?",
		      "Yes", "No", "Cancel")
	if b == 1 then 
	  break 
	elseif b == 3 then
	  return
	end
      elseif err == 1 then
	b = nil
	break
      end
    end
  end
  if not string.find(fnam, ".txt") then
    fnam = fnam .. ".txt"
  end
  log:info("Storing values of "..win.title.." into file '"..fnam.."'.")
  win:store2file(fnam, snapshot_delim)
end


------------------------------------------------------------------------------
-- Default attributes
-- @class table
-- @name defaultattrib
-- @field drawoper blabla
------------------------------------------------------------------------------
local defaultattrib
function setDefaultAttributes(cf)
   local attr = {
      drawoper = cd[cf.Draw.Oper],
      drawcolor = cd[cf.Draw.Color],
      drawstyle = cd[cf.Draw.Style],
      drawwidth = cf.Draw.Width,
      textoper = cd[cf.Text.Oper],
      texttypeface = cf.Text.Face,
      textstyle = cd[cf.Text.Style],
      textcolor = cd[cf.Text.Color],
      textsize = cf.Text.Size,
      bgopacity = cd[cf.Background.Opacity],
      bgcolor = cd[cf.Background.Color],
      usepoly = cf.General.UsePoly,
      polytype = cd[cf.General.PolyType],
      polystep = cf.General.PolyStep,
      canvastype = yats[cf.General.CanvasType]
   }
   defaultattrib = attr
   return attr
end
setDefaultAttributes(conf.graphics.Attributes)

local xs, ys = config.get_size(iup.GetGlobal("SCREENSIZE"))
if xs  < 1000 then
  defaultattrib.textsize = 8
end

--
-- Set non existing components with default values.
--
local function adjust_attrib(attrib)
   if not attrib then 
      attrib = defaultattrib 
   else
      for k,v in pairs(defaultattrib) do
	 if attrib[k] == nil then 
	    attrib[k]=v 
	 end
      end
   end
   return attrib
end


--
-- A plot dialog to show a single plot with axes and scaling
--
local function plot_display(instr, canvas)
   local function makeit()
      local plot = iup.pplot{
	 title = instr.title,
	 font = "Helvetica, 8",
	 marginbottom="60",
	 marginleft="60",
	 axs_xlabel="Ticks",
	 axs_ymin = 0,
	 axs_yautomin= "No",
	 grid = "Yes",
	 gridlinestyle = "DOTTED",
      }
      return plot
   end
   local function drawit(plot)
      iup.PPlotBegin(plot, 0)
      local scale = 1
      if instr.delta > 100 then
	 plot.axs_xlabel = "kTicks"
	 scale = 1000
      elseif instr.delta > 10000 then
	 plot.axs_xlabel = "MTicks"
	 scale = 1000*1000
      end
      if instr.clname == "meter" then
	 plot.axs_ylabel = "value"
	 for i = 1, instr.nvals do
	    iup.PPlotAdd(plot, i*instr.delta/scale, instr:getVal(i-1))
	 end
      elseif instr.clname == "histo2" or instr.clname == "histo" then
	 plot.axs_ylabel = "frequency"
	 for i = 1, instr.nvals do
	    iup.PPlotAdd(plot, i*instr.delta/scale, instr:getVal(i-1))
	 end
      end
      iup.PPlotEnd(plot)
   end

   local plot = makeit()
   drawit(plot)
   local dlg
   local activeprint = "Yes"
   if iup.GetGlobal("DRIVER") ~= "Win32" then
      activeprint = "No"
   end
   local menu = iup.menu {
      iup.submenu {
	 title = "Plot", key = "P",
	 iup.menu {
	    iup.item {
	       title = "Print ...", key = "P",
	       active = activeprint,
	       action = function(this)
			   local mplot = makeit()
			   drawit(mplot)
			   local cv = cd.CreateCanvas(cd.PRINTER, "luayats -d")
			   if not cv then return end
			   mplot.marginleft, mplot.margintop = cv:MM2Pixel(25, 20)
			   -- print format: 4/3
			   local bottom,_ = 297-20-(210-45)
			   mplot.marginright, mplot.marginbottom = cv:MM2Pixel(20, bottom*4/3)
			   iup.PPlotPaintTo(mplot, cv)
			end
	    },
	    iup.item {
	       title = "Redraw", key = "R",
	       action = function(this)
			   plot.REDRAW = true
			end
	    },
	    iup.item {
	       title = "Close", key = "C",
	       action = function(this)
			   dlg:destroy()
			end
	    }
	 }
      }
   }
   dlg = iup.dialog {
      menu = menu,
      plot,
      title="Plot of '"..instr.title.."'",
      size="QUARTERxQUARTER",
      plot = plot
   }
   dlg:show()

end

--
-- A short popup menu for graphic displays.
--
local function print_display(instr)
   iup.Message("INFORMATION", "Printing of single displays not yet implemented!")
end

--
-- A short popup menu for graphic displays.
--
local function make_canvas_menu(instr)
   local activeprint = "Yes"
   if iup.GetGlobal("DRIVER") ~= "Win32" then
--      activeprint = "No"
   end
   local menu = iup.menu{
      iup.item{
	 title = "Print", key="P",
	 active = activeprint,
	 action = function(this)
		     print_display(this.instr)
		  end,
      },
      iup.item{
	 instr = instr,
	 title = "Plot", key="T",
	 action = function(this)
		     this.instr:drawWin(1)
		     plot_display(this.instr)
		  end,
      },
      iup.item{
	 -- we need a reference to the attached instrument for the snapshot at runtime
	 -- this is reset to nil if instrument is detached.
	 instr = instr,
	 title = "Snapshot", key="S", 
	 action=function(this) 
		   snapshot2file(this.instr)
		end,
      }
   }
   return menu
end


--
-- Detach an instrument from a display
--
local function detach_instr(canvas)
   local instr = canvas.instr
   if instr then
      log:debug(string.format("unmap instrument %q", instr.name))
      instr:unmap()
      canvas.instr = nil
      return instr
   else
      return nil
   end
end

--
-- Attach an instrument to a display
--
local function attach_instr(canvas, instr, attrib) 
   local prev
   log:debug(string.format("map instrument %q attached:%s", 
			   instr.name, tostring(instr.attached)))
   prev = detach_instr(canvas)
   canvas.instr = instr

   local bcanvas = cd.CreateCanvas(cd.IUP, canvas)
   if attrib.canvastype == yats.CDTYPE_DBUFFER then
      instr.bcanvas = bcanvas
      instr.canvas = cd.CreateCanvas(cd.DBUFFER, bcanvas)
      log:debug(string.format("DBUFFER canvas created: %s", tostring(instr.canvas)))
   else
      instr.bcanvas = bcanvas
      instr.canvas = bcanvas
      log:debug(string.format("Using IUP canvas directly: %s", tostring(instr.canvas)))
   end
   instr.canvas:Activate()

   instr.drawoper = attrib.drawoper
   instr.drawstyle = attrib.drawstyle
   instr.drawwidth = attrib.drawwidth
   instr.textoper = attrib.textoper
   instr.textsize  = attrib.textsize
   instr.textcolor = attrib.textcolor
   instr.usepoly = attrib.usepoly
   instr.polytype = attrib.polytype
   instr.polystep = attrib.polystep
   instr.cvtype = attrib.canvastype
   
   instr.bgcolor = attrib.bgcolor
   instr.canvas:SetBackground(attrib.bgcolor)
   
   instr.drawcolor = attrib.drawcolor
   instr.canvas:SetForeground(attrib.drawcolor)

   instr.canvas:WriteMode(attrib.drawoper)

   instr.bgopacity = attrib.bgopacity
   instr.canvas:BackOpacity(attrib.bgopacity)
   
   instr.texttypeface = attrib.texttypeface
   instr.textstyle = attrib.textstyle
   instr.textsize = attrib.textsize
   instr.canvas:Font(attrib.texttypeface, attrib.textstyle, attrib.textsize)

   instr.canvas_mapped = true

   instr:setViewport(0, instr.width, 0, instr.height);

   log:debug(string.format("canvas mapped: canvas=%s instr.canvas=%s",
			   tostring(canvas), tostring(instr.canvas)))
   return prev
end
--
-- Attach an instrument to a display
--
local function _attach_instr(canvas, instr, attrib) 
   local wid, prev
   log:debug(string.format("map instrument %q attached:%s", instr.name, tostring(instr.attached)))
   prev = detach_instr(canvas)
   canvas.instr = instr
   -- Crashes under Cygwin. But we do not really need this attribute.
--   _wid = iup.GetAttribute(canvas, "WID")
   local cdcanvas = assert(instr:map(canvas, wid, 
				     instr.width, instr.height, instr.xpos, instr.ypos, 
				     attrib.texttypeface, 
				     attrib.textstyle, 
				     attrib.textsize, 
				     attrib.textcolor, 
				     attrib.textoper, 
				     attrib.drawcolor, 
				     attrib.drawoper, 
				     attrib.drawstyle, 
				     attrib.drawwidth, 
				     attrib.bgcolor,	
				     attrib.bgopacity,
				     attrib.usepoly,
				     attrib.polytype,
				     attrib.polystep,
				     attrib.canvastype
				  ), "Cannot map canvas")
--   instr.attached = true
   print("#1#", cdcanvas, iup.GetAttribute(canvas, "CD_CANVAS"))
   instr:setViewport(0, instr.width, 0, instr.height);
   log:debug(string.format("canvas mapped: wid=%s canvas=%s cdcanvas=%s",
			   tostring(wid), tostring(canvas), tostring(cdcanvas)))
   return prev
end

--
-- Get numerical size from IUP size string.
--
local function get_size(s)
  local xs, ys
  string.gsub(s, "(%d*)x(%d*)", function(x,y) xs=x ys=y end)
  return xs, ys
end

--
-- Create a display canvas.
--
local function canvas(instr, width, height, attrib)
   local cv = iup.canvas{
      rastersize=string.format("%dx%d", width, height),
      instr = instr,
      attrib = attrib,
      cliprect = string.format("%d %d %d %d", 0, 0, width, height),

      action__ = function(this, x, y) end,
      -- NOTE: We get crashes when moving the window.
      -- Workaround: Do not define the action callback and live
      --             with ugly graphics while moving window
      action_inactive_see_above = function(this, x, y)
		  local instr = this.instr
		  if instr then
		     instr.width, instr.height = get_size(this.rastersize)
		     instr:drawWin(1)
		  end
	       end,
      map_cb = function(this)
		  this:attachinstr()
		  iup.Flush()
	       end,
--      NOTE: We get crashes when killing the dialog which contained this canvas
--      Workaround: unmap during display destroy
--      date: 2009-11-30 leu
--      unmap_cb = function(this)
--		    return this:detachinstr()
--		 end,
      resize_cb = function(this)
		     local instr = this.instr
		     if instr then
			instr:activateCanvas()
		     end
		  end,
      button_cb = function(this, but, pressed, x, y, status)
		     if but == iup.BUTTON3 then
			log:debug("Right Button pressed")
			if this.menu[2].instr then
			   this.menu:popup(iup.MOUSEPOS, iup.MOUSEPOS)
			end
		     end
		  end,
      attachinstr = function(this, instr, attrib)
		       local instr = instr or this.instr
		       this.instr = instr
		       local attrib = attrib or this.attrib
		       this.attrib = attrib
		       if instr then
			  prev = attach_instr(this, instr, attrib)
		       end
		       this.menu = make_canvas_menu(instr)
		       return prev
		    end,
      
      detachinstr = function(this)
		       return detach_instr(this)
		    end
   }
   if instr then
      instr.cv = cv
   end
   return cv
end

--
-- API
--

--- Get graphic default attributes.
-- @return Default attributes as table.
function defaultattributes()
  return defaultattrib
end

--- Definition of class 'display'.
--@name display
display = class(root)

--- Create a display class instance.
-- Creates a graphical display and optionally associates an instrument to it. The display
-- is automatically shown on the screen.<br>
-- Note, that by default all instruments automatically also create a display implicitly. 
-- The method <b>attach()</b> can be used to attach a measurement instrument to a display.<br>
-- Title, size, and position are handled in the following precedence:<br>
-- (1) parameter value, (2) associated instrument, (3) some reasonable default.
-- @param param Parameter table.
--<ul>
--<li> name (optional)<br>
--     Name of the display. Default: `objNN'.
--<li> title (optional)<br>
--     Title of the display. Default: see description. 
--<li> width, height (optional)<br>
--     Size of the display. Default: see description. 
--<li> xpos, ypos (optional)<br>
--     Position on screen. Default: see description. 
--<li> attrib (optional)<br>
--     Drawing attributes. Default: values as returned by yats.defaultattrib(). 
--<li> instrument (optional)<br>
--     A reference to an instrument previously defined. A nil value creates an empty display.
--</ul>
-- @return Reference to display instance.
-- @see yats.bigdisplay:init.
function display:init(param)
   local self = root:new()
   self.name = autoname(param)
   self.clname = "display"
   self.parameters = {
      title = false, width = false, height = false, xpos = false, ypos = false, attrib = false,
      instrument = false
   }
   self:adjust(param)
   self.param = param
   log:debug("Creating display '"..self.name.."'.")
   if param.instrument then
      local cinstr = param.instrument
      self.xpos = param.xpos or cinstr.xpos or iup.CENTER
      self.ypos = param.ypos or cinstr.ypos or iup.CENTER
      self.attrib = param.attrib or cinstr.attrib or defaultattrib
      self.title = param.title or cinstr.title or self.clname..":unknown"
      if param.width then cinstr.width = param.width end
      if param.height then cinstr.height = param.height end
      self.cv = canvas(cinstr, cinstr.width, cinstr.height, self.attrib)
   else
      self.xpos = param.xpos or iup.CENTER
      self.ypos = param.ypos or iup.CENTER
      self.attrib = param.attrib or defaultattrib
      self.title = param.title or self.clname..":unknown"
      self.cv = iup.canvas {
	 rastersize=string.format("%dx%d", param.width or 100, param.height or 100)
      }
   end
   
   -- We use an IUP dialog with a canvas for drawing
   local resize = "YES"
   if self.attrib.canvastype == CDTYPE_SERVERIMG then 
      resize = "NO" 
   end
   self.dg = iup.dialog{
	 self.cv,
      drawme = function(this)
		  local instr = self.cv.instr
		  if instr then
		     instr:drawWin(1)
		  end
	       end,
      move_cb = function(this) this:drawme() end,
      getfocus_cb = function(this) this:drawme() end,
      killfocus_cb = function(this) this:drawme() end,
      enterwindow_cb = function(this) this:drawme() end,
      leavewindow_cb = function(this) this:drawme() end,
      title = self.title,
      icon = gui.images.app,
      resize = resize
   }
   local a = self:finish()
   self.dg:showxy(self.xpos, self.ypos)
   return a
end

--- Attach an instrument to a display.
-- @param instr table -  Reference to instrument (Lua ref to histogram or meter).
-- @param attrib table -  Optionally use given attributes.
-- @param title string - Optionally set title.
-- @return Reference to previous attached instrument or nil, if none was attached.
function display:attach(instr, attrib, title)
   local canvas = self.cv
   local attrib = attrib or self.attrib
   log:debug(string.format("Attach instrument %q of type %q to display %q",
			   instr.name, tolua.type(instr), self.name))
   local prev = attach_instr(canvas, instr, attrib)
   self.dg.title = title or instr.title
   if prev then
      return prev
   else
      return nil
   end
end

--- Detach an instrument from a display
-- @return Reference to previous attached instrument and to the canvas
--         associated with the display.
function display:detach()
   local canvas = self.cv
   local instr = detach_instr(canvas)
   log:debug(string.format("Detached instrument %q of type '%s' in display '%s'",
			   instr.name, tolua.type(instr), self.name))
   return instr
end

--- Show the display.
-- This function is automatically called when the instrument is instantiated
-- and can be used to re-show the display after hiding it via script.
-- @return none.
function display:show()
  self.dg:show()
end

--- Show the display.
-- This function is automatically called when the instrument is instantiated
-- and can be used to re-show the display after hiding it via script.
-- @param x number - Horizontal position on the screen.
-- @param y number - Vertical position on the screeen.
-- @return none.
function display:showxy(x, y)
  self.dg:showxy(x, y)
end

--- Hide the display.
-- This function can be used to hide a display from the simulation script.
-- @return none.
function display:hide()
  self.dg:hide()
end

--- Set a drawing attribute for the display.
-- @param attr string - Name of the attribute.
-- @param val any - Value of the attribute.
-- @return none.
function display:setAttribute(attr, val)
   local instr = self.canvas.instr
   cinstr[attrib] = val
end

--- Get value of a displays drawing attribute.
-- @param attr string - Name of the drawing attribute.
-- @return Value of this drawing attribute.
function display:getAttribute(attr)
   local cinstr = self.canvas.instr
   return cinstr[attr]
end
display.setattrib = display.setAttribute
display.getattrib = display.getAttribute

--- Destroy a display.
-- This function is automatically called when the associated instrument is
-- is deleted during simulation script reset.
-- @return none.
function display:destroy()
   log:debug(string.format("destroy display %q %q", self.name, tostring(self.instr)))
   if self.cv then
      local instr = self:detach()
   end
   iup.Destroy(self.dg)
end


--==========================================================================
-- A big display containing multiple drawing areas
--==========================================================================

--- Definition of class 'bigdisplay'.
--@see yats.bigdisplay:init.
bigdisplay = class(root)

--- Constructor for class 'bigdisplay'.
-- A big display displays a number of graphic outputs in a single window.
-- Once measurements instruments are created, an instrument can be assigned to
-- a canvas in a big display. It is possible to create an empty display and
-- afterwards attach instruments to the big display using the <b>attach()</b> method.
--@param param table Parameter table.
--<ul><li> name (optional)<br>
--    Name of the display. Default: "objNN".
--<li> title (optional)<br>
--    Title of the display. Default: "class-name:unknown".
--<li> nrow, ncol<br>
--    Number of rows and columns.
--<li> width, height (optional)<br>
--    Size of the display. Default: see description.
--<li> xpos, ypos (optional)<br>
--    Position on screen. Default: see description.
--<li> attrib (optional)<br>
--    Drawing attributes. Default: as return by yats.defaultattrib().
--<li> instruments<br>
--   Table with references to instruments previously defined.
--   The table is organised in the same way as the display.
--   Optional, an empty display is created in case of nil value.
-- </ul>
--@return Reference to bigdisplay object.
function bigdisplay:init(param)
   local self = root:new()
   self.name = autoname(param)
   self.clname = "bigdisplay"
   self.parameters = {
      title = false, nrow = true, ncol = true, width = false, 
      height = false, attrib = false, instruments = true
   }
   local frame
   self:adjust(param)
   log:debug("Creating a bigdisplay '"..self.name.."'.")
   self.title = param.title or self.clname..":unknown"
   local l, c, cm = 0, 0, 0
   for i, v in ipairs(param.instruments) do
      l = l + 1
      c = 0
      for j, w in ipairs(v) do
	 c = c + 1
      end
      if c > cm then cm = c end
   end
   self.nrow = param.nrow or l
   self.ncol = param.ncol or cm
   self.width = param.width or 100
   self.height = param.height or 100
   self.xpos = param.xpos or iup.CENTER
   self.ypos = param.ypos or iup.CENTER
   self.attrib = param.attrib 
   local vbox = iup.vbox{expand = "YES"}
   self.instruments = {}
   self.frames = {}

   for i = 1,self.nrow do
      self.instruments[i] = {}
      self.frames[i] = {}
      local frame
      local hbox = iup.hbox{margin="2x2"}
      for j = 1,self.ncol do
	 k = (i-1)*self.ncol+j
	 if param.instruments[i] and param.instruments[i][j] then
	    local instr = param.instruments[i][j]
	    self.instruments[i][j] = instr
	    log:debug("Mapping instrument `"..instr.name.."'["..instr.title.."].")
	    -- create a canvas for drawing
	    local attrib = self.attrib or instr.attrib or defaultattrib
	    attrib = adjust_attrib(attrib)
	    local cv = canvas(instr, self.width, self.height, attrib)
	    cv.scrollbar="no"
	    -- create a frame which holds a single display canvas
	    frame = iup.frame{
	       title = string.format("%s", instr.title),
	       expand = "no",
	       cv
	    }
	 else
	    local cv = canvas(nil, self.width, self.height)
	    cv.scrollbar="no"
	    -- create a frame which holds a single display canvas
	    frame = iup.frame{
	       title = string.format("%s", "none"),
	       expand = "no",
	       cv
	    }
	 end
	 iup.Append(hbox, frame)
	 self.frames[i][j] = frame
      end
      iup.Append(vbox, hbox)
   end

   -- Create the main dialog
   local parentdialog 
   if gui then parentdialog = gui.dlg end

   -- Printing works only for MS Windows
   local activeprint = "Yes"
   if iup.GetGlobal("DRIVER") ~= "Win32" then
      activeprint = "No"
   end
   self.dlg = iup.dialog{
      menu = iup.menu {
	 iup.submenu {
	    title = "Display", key="D",
	    iup.menu {
	       iup.item {
		  title = "Print ...", key = "P",
		  active = activeprint,
		  action = function() 
			      self:print(nil, nil, nil, conf.graphics.Resolution or 100, "printer")
			   end
	       },
	       iup.item {
		  title = "Print to PDF", key = "D",
		  action = function() 
			      self:print(nil, nil, nil, 0, "pdf")
			   end
	       },
	       iup.item {
		  title = "Print to Postscript", key = "S",
		  action = function() 
			      self:print(nil, nil, nil, 0, "ps")
			   end
	       },
	       iup.item {
		  title = "Exit", key = "X",
		  action = function() 
			      if gui.mnu_exit then gui.mnu_exit() else os.exit() end
			   end
	       }
	    }
	 },
--	 unmap_cb = function(self) print("#10#", "unmapping", self.title) end
      },
      drawme = function(this)
		  for i = 1,self.nrow do
		     for j = 1, self.ncol do
			local instr = self.instruments[i][j]
			if instr then
			   instr:drawWin(1)
			end
		     end
		  end
	       end,
      active = function(this) this:drawme() end,
      move_cb = function(this) this:drawme() end,
      getfocus_cb = function(this) this:drawme() end,
      killfocus_cb = function(this) this:drawme() end,
      enterwindow_cb = function(this) this:drawme() end,
      icon = gui.images.app,
      parentdialog = parentdialog,
      title = param.title,
      resize="no",
      shrink="no",
      vbox
   }
   -- 'bigdisplay' doesn't have an associated C-level object
   self.dlg:showxy(self.xpos, self.ypos)
   self.shown = true
   return self:finish()
end


local function getArea(paper, layout, margin)
  local margin = margin
  local paper = paper
  local layout = layout
  local fmt = paperformats[paper].orient[layout]
  local width, height
  width = fmt.w - margin.Left - margin.Right
  height = fmt.h - margin.Top - margin.Bottom
  return width, height, fmt.w, fmt.h
end

--- Print a bigdisplay.
-- The instruments are printed with the given resolution. Multiple pages
-- may be printed. The layout is slightly different from the screen layout
-- for better readability.<br>
-- NOTE: Currently only  "ps" mode (postscript file) and layout = "portrait"
-- are supported.
-- @param paper string - Type of paper: "A4", A3".
-- @param layout string - Layout: "portrait", "landscape".
-- @param margin table - <code>{left=<n>, right=<n>, top=<n>, bottom=<n>}</code>.
-- @param res number - Resolution in pixel / inch.
-- @param filetype - Type of printfile: "ps" or "pdf" or "printer".
function bigdisplay:print(paper, layout, margin, res, filetype, filename)

   -- Viewport Scaling factor
   local sx, sy = 1, 1

   -- A drawing offset for the text
   local offs = 75

   -- Parameter defaults
   local paper = paper or conf.graphics.Paper
   local layout = layout or conf.graphics.Layout
   local margin = margin or conf.graphics.Margin
   local res = res or conf.graphics.Resolution
   if res == 0 then res = nil end
   local cpf = conf.graphics.Printfile
   local pf = {}
   if filetype == "printer" then
      pf.Type = "printer"
      pf.Name = "NOFILE"
      pf.Path = ""
   elseif filetype == "pdf" or filetype == "ps" then
      pf.Type = filetype
      pf.Path = ""
      local fname, status = iup.GetFile(cpf.Path.."*."..pf.Type)
      pf.Name = fname
      if status == -1 then
	 -- cancelled: stop printing
	 return
      elseif status == 0 then
	 b = iup.Alarm("WARNING", "File exists! Overwrite?",
		       "Yes", "No", "Cancel")
	 if b ~= 1 then
	    return 
	 end
      end
   end
   
   -- Resolution in dots/inch
   -- Determine drawing area from paper and margins
   local w, h, pw, ph = getArea(paper, layout, margin)
   local hres, vres = res, res
   if not res then
      hres = (self.width * self.ncol) * 25.4 / w
      vres = ((self.height + offs) * self.nrow) * 25.4 / h
      if vres > hres then 
	 res = vres 
      else
	 res = hres
      end
      log:debug("No resolution given:")
      log:debug(string.format("  calculated from paper size [dots/inch]: hres=%d vres=%d res=%d (selected)", hres, vres, res))
   end
   
   -- Create the PS file canvas
   local pfile, cv
   if string.lower(pf.Type) == "ps" then
      pfile = pf.Path .. pf.Name .. "." .. pf.Type
      pfile = pfile .. " -s"..tostring(math.ceil(res))
      pfile = pfile .. " -p"..tostring(paperformats[paper].cdval)
      -- We handle the margins during drawing
      pfile = pfile .. " -l0"
      pfile = pfile .. " -t0"
      pfile = pfile .. " -r0"
      pfile = pfile .. " -b0"
      if layout == "landscape" then
	 pfile = pfile .. " -o"
      end
      cv = assert(cd.CreateCanvas(cd.PS, pfile), "cannot create PS canvas")
      lmarg, bmarg = cv:MM2Pixel(margin.Left, margin.Bottom)
      
   elseif string.lower(pf.Type) == "pdf" then
      pfile = pf.Path .. pf.Name .. "." .. pf.Type
--      pfile = "luayats-printout.pdf"
      pfile = pfile .. " -s"..tostring(math.ceil(res))
      pfile = pfile .. " -p"..tostring(paperformats[paper].cdval)
      if layout == "landscape" then
	 pfile = pfile .. " -o"
      end
      cv = assert(cd.CreateCanvas(cd.PDF, pfile), "cannot create PDF canvas")
      -- PDF does not honour margin during canvas creation
      lmarg, bmarg = cv:MM2Pixel(margin.Left, margin.Bottom)

   elseif iup.GetGlobal("DRIVER") == "Win32" and string.lower(pf.Type) == "printer" then
      pfile = "luayats -d"
      cv = cd.CreateCanvas(cd.PRINTER, pfile)
      if not cv then return end
      lmarg, bmarg = cv:MM2Pixel(margin.Left, margin.Bottom)
      -- resolution of printer in dots/inch and dots/mm
      local ix, iy = cv:MM2Pixel(25.4, 25.4)
--      local mx, my = cv:MM2Pixel(1, 1)
      sx = ix / hres
      sy = ix / vres
   end
   log:debug(string.format("Printing : %s", pfile))

   -- Visible area in pixel
   local xmax, ymax = cv:MM2Pixel(w, h)
   -- Paper area in pixel
   local pxmax, pymax = cv:MM2Pixel(pw, ph)
   
   -- Debugging stuff
   log:debug(string.format("scale: %.3g %.3g", sx, sy))
   log:debug(string.format("pix/mm: %d %d", cv:MM2Pixel(1,1)))
   log:debug(string.format("dots/inch: %d %d", cv:MM2Pixel(25.4, 25.4)))
   log:debug(string.format("mm/pix: %.3g %.3g",cv:Pixel2MM(1,1)))
   log:debug(string.format("resolution [pix/inch]: hres=%d vres=%d res=%d", hres, vres, res))
   log:debug(string.format("resolution [pix/mm]: hres=%d vres=%d res=%d", hres/25.4, vres/25.4, res/25.4))
   log:debug(string.format("canvas size [mm]   : x=%d, y=%d", cv:Pixel2MM(self.width, self.height)))
   log:debug(string.format("canvas size [pix]  : xmax=%d ymax=%d pxmax=%d pymax=%d", xmax, ymax, pxmax, pymax))
   log:debug(string.format("total size [mm]    : x=%d, y=%d", cv:Pixel2MM(self.width * self.ncol, self.height * self.nrow)))
   log:debug(string.format("real size [mm]     : w=%d h=%d pw=%d ph=%d", w, h, pw, ph))
   -- End debugging stuff

   -- 
   local x0, y0 = 0, ymax - (self.height - offs)
   
   -- columns per page = min(xmax/width, ncol)
   local colpage = math.floor(xmax / ((self.width * sx)))
   if colpage > self.ncol then colpage = self.ncol end

   -- rows per page = min(ymax/height, nrow)
   local rowpage = math.floor(ymax / ((self.height + offs) * sy))
   if rowpage > self.nrow then rowpage = self.nrow end
   
   -- number of pages in horizontal (H) and vertical (V) direction
   local pageH = math.ceil(self.ncol / colpage)
   local pageV = math.ceil(self.nrow / rowpage)
   local npages = pageH * pageV
   
   -- Debugging
   log:debug(string.format("printing: rowpage=%d(%f) colpage=%d(%f) pageH=%d pageV=%d npages=%d", 
			   rowpage, ymax/(self.height + offs), colpage, xmax/self.width, 
			  pageH, pageV, npages))
   
   -- Create a table of pages
   local pages = {}
   local k = 1
   local i, j = 1, 1
   for u = 1, pageH do
      for v = 1, pageV do
	 pages[k] = {}
	 pages[k].i1 = i
	 pages[k].j1 = j
	 pages[k].i2 = i + rowpage - 1
	 if pages[k].i2 > self.nrow then pages[k].i2 = self.nrow end
	 pages[k].j2 = j + colpage - 1
	 if pages[k].j2 > self.ncol then pages[k].j2 = self.ncol end
	 i = i + rowpage
	 k = k + 1
      end
      i = 1
      j = j + colpage
   end
   
   -- Run through all pages
  for p, v in ipairs(pages) do
     -- Origin for page: left, bottom - 10 mm
     local xp, yp = cv:MM2Pixel(10, 10)
     cv:Origin(lmarg, bmarg - yp)
     -- Only for test purposes:
     -- cv:Rect(0, xmax, 0, ymax)
     cv:Font("Helvetica", cd.PLAIN, 8)
     if filetype == "printer" then
	cv:Text(0, 5, string.format("Page %d [printer / Date: %s]", p, os.date()))
     else
	cv:Text(0, 5, string.format("Page %d [File: %s / Date: %s]", p, pf.Path..pf.Name.."."..pf.Type, os.date()))
     end
     -- Origin for page: 0, -10 mm
     cv:Origin(-lmarg, -bmarg)
     for i = v.i1, v.i2 do
	for j = v.j1, v.j2 do
	   local r, c = math.mod(i-1, rowpage) + 1, math.mod(j-1, colpage) + 1
	   local x0, y0 = self.width * (c-1) * sx + lmarg, (self.height + offs) * r * sy - bmarg
	   -- set new origin
	   cv:Origin(x0, ymax - y0 - (self.height - offs + 20) * sy)
	   local instr = self.instruments[i][j]
	   cv:LineWidth(1)
	   if instr ~= nil then
	      cv:Font("Helvetica", cd.BOLD_ITALIC, 8)
	      cv:Text(5, (self.height + 5) * sy, 
		      string.format("%s  [%d,%d]", instr.title, i, j))
	      cv:Rect(0, self.width * sx, 0, self.height * sy)
	      local ocv = instr:setCanvas(cv)
	      instr:setViewport(0, self.width * sx, 0, self.height * sy)
	      instr:setScale(sx, sy)
	      instr:drawWin(0,0,0)
	      instr:setScale(1, 1)
	      instr:setCanvas(ocv)
	   end
	   -- set origin back
	   cv:Origin(-x0, -(ymax - y0 - (self.height - offs + 20) * sy))
	end
     end
     if p < npages then
	cv:Flush()
     end
  end
  cd.KillCanvas(cv)
  self:redraw()
end

--- Redraw all bigdisplay instruments.
-- @param none.
-- @return none.
function bigdisplay:redraw()
  for i = 1, self.nrow do
    for j = 1, self.ncol do
      local instr = self.instruments[i][j]
      if instr then
	instr:setViewport(0, instr.width, 0, instr.height)
	instr:drawWin()
      end
    end
  end
end

--- Attach an instrument to a bigdisplay canvas.
-- The attach method can be used to map an instruments output to an empty canvas or
-- to change the instrument associated with the canvas in the given (row,col) position.
-- @usage yats.bigdisplay:attach()
-- @param instr table - Reference to an instrument (Lua ref to histogram or meter).
-- @param row number - Row within the big display.
-- @param col number - Column within the big display.
-- @return Reference to previously attached instrument or nil if none was attached.
function bigdisplay:attach(instr, row, col)
   local frame = self.frames[row][col]
   log:debug(string.format("attach instrument %q of type %q at (%d,%d) to display %q", 
			   instr.name, tolua.type(instr), row, col, self.name))
   local cv = frame[1]
   local prev = attach_instr(cv, instr, self.attrib)
   frame.title = instr.title
   if prev then
      return prev
   else
      return nil
   end
end

--- Detach instrument from position (row, col).
-- @param row number - row.
-- @param col number - column.
-- @return Reference to the instrument.
function bigdisplay:detach(row, col)
   local frame = self.frames[row][col]
   local cv = frame[1]
   local instr = detach_instr(cv)
   frame.title = "none"
   self.instruments[row][col] = nil
   if instr then
      return instr
   else
      return nil
   end
end

--- Show the big display.
-- @return self - reference to object.
function bigdisplay:show()
   if self.shown == true then return end
   self.dlg:showxy(self.xpos, self.ypos)
   if self.shown == true then return end
   return self
end

--- Show a big display at position (x,y).
-- @param x number - Horizontal position.
-- @param y number - Vertical position.
-- @return self - reference to object.
function bigdisplay:showxy(x, y)
   if self.shown == true then return end
   log:debug("Showing dialog.")
   self.dlg:showxy(x, y)
   self.shown = true
   return self
end

--- Hide a big display.
-- @return none.
function bigdisplay:hide()
   self.shown = nil
   self.dlg:hide()
   return self
end

--- Set graphics attributes for a single display in a big display.
-- @param attrib string - Graphic attribute name.
-- @param val any - Graphic attribute value.
-- @param row number - Row of the display (optional).
-- @param col number - Column of the display (optional).
-- @return Previous value.
function bigdisplay:setAttribute(attrib, val, row, col)
   if row == nil or col == nil then
      local prev = {}
      for i = 1, self.nrow do
	 prev[i] = {}
	 for j = 1, self.ncol do
	    local instr = self.instruments[i][j]
	    if instr then
	       prev[i][j] = instr[attrib] 
	       instr[attrib] = val
	    end
	 end
      end
      return prev
   else
      local instr = self.instruments[row][col]
      if instr then
	 local prev = instr[attrib]
	 instr[attrib] = val
	 return prev
      end
      return nil, "Instrument not existent."
   end
end

--- Set graphics attributes for a single display in a big display.
-- @param attrib string - Graphic attribute name.
-- @param val any - Graphic attribute value.
-- @param row number - Row of the display (optional).
-- @param col number - Column of the display (optional).
-- @return Previous value.
function bigdisplay:getAttribute(attrib, row, col)
   if row == nil or col == nil then
      local prev = {}
      for i = 1, self.nrow do
	 prev[i] = {}
	 for j = 1, self.ncol do
	    local instr = self.instruments[i][j]
	    if instr then
	       prev[i][j] = instr[attrib] 
	    end
	 end
      end
      return prev
   else
      local instr = self.instruments[row][col]
      local prev = self.instr[attrib]
      instr[attrib] = val
      return prev
   end
end
bigdisplay.setattrib = bigdisplay.setAttribute
bigdisplay.getattrib = bigdisplay.getAttribute

--- Delete a big display.
-- @return none.
function bigdisplay:destroy()
   log:debug(string.format("Destroying display '%s'", self.name))
   for i = 1, self.nrow do
      for j = 1, self.ncol do
	 local instr = self.instruments[i][j]
	 if instr then
	    log:debug(string.format("Detaching instrument '%s' of type '%s' in display '%s'",
				    instr.name, tolua.type(instr), self.name))
	    detach_instr(instr.cv)
	    self.instruments[i][j] = nil
	    instr.cv = nil
	 end
      end
   end
   iup.Destroy(self.dlg)
end

--==========================================================================
-- Histogram Objects
--==========================================================================

_histo = histo
-- Definition of class 'histo'.
histo = class(_histo)

--- Constructor for class 'histo'.
-- Displays a histogram of a distribution, e.g. the number of times a 
-- certain queue length is reached. 
-- @param param table - parameter values
-- <ul>
-- <li>name (optional)
--    Name of the display. Default: "objNN"
-- # title (optional)
--    Title of the display. Default: "class-name:unknown"
-- # val
--    Table with references to exported values of another object. 
--    Format: { object-reference, "variable-name" [, index]}
-- # win
--    Table with window geometry. 
--    Format: {xpos, ypos, width, height}
-- # maxfreq (optional)
--    Normalisation. `maxfreq' corresponds to the window height
--    Default: autoscale to 1.25 * largest frequency
-- # delta (optional)
--    Drawing interval between two samples (in ticks). Default: 100
-- # attrib (optional)
--    Drawing attributes. Optional - default: see default attributes 
-- # display (optional)
--    Create a display if set to true
--<</pre>>.
-- @return table reference to object instance.
function histo:init(param)
  local self = _histo:new()
  self.name = autoname(param)
  self.clname = "histo"
  self.parameters = {
    val = true, title = false, win = true, maxfreq = false,
    delta = false, nvals = true, attrib = false, display = false
  }
  self:adjust(param)
  
  self.exp_obj = param.val[1]
  self.exp_varname = param.val[2]
  self.exp_idx = param.val[3] or -1
  self.exp_idx2 = param.val[4] or -1
  self.title = param.title or self.name
  self.xpos = param.win.xpos or param.win[1]
  self.ypos = param.win.ypos or param.win[2]
  self.width = param.win.width or param.win[3]
  self.height = param.win.height or param.win[4]
  self.delta = param.delta or 100
  self.nvals = self.nvals or 0
  if param.maxfreq then
    self.maxfreq = param.maxfreq 
    self.autoscale = 0
  else
    self.autoscale = 1
  end
  local a = self:finish()
  -- Associate a display with the histogram
  if param.display == nil or param.display == true then
    self.attrib = param.attrib or defaultattrib
    self.display = display{
      self.name..":display", instrument = self, attrib = self.attrib
    }
  else
    self.display = "no"
  end
  return a
end

_histo2 = histo2
--- Definition of class 'histo2'.
--@see yats.histo2:init.
histo2 = class(_histo2)

--- Constructor for class 'histo2'.
-- Displays a histogram of the frequency of certain events, e.g. the number of times a 
-- certain queue length is reached. 
-- # y-axis: frequency - normalised to a given value or to 1.25 of the maximum measured value.
-- # x-axis: values.
-- @param param table - parameter values
-- <<pre>>
-- # name (optional)
--    Name of the display. Default: "objNN"
-- # title (optional)
--    Title of the display. Default: "class-name:unknown"
-- # val
--    Table with references to exported values of another object
--    Format: { object-reference, "variable-name" [, index]}
-- # val
--    Reference to a scalar or to a one/two dimensional array of  measurement values,
--    which must be exported by another object
--    Format: {objref, "varname", index1 [, index2]}
-- # nvals 
--    Number of values to plot
-- # maxfreq (optional)
--    Normalisation. `maxfreq' corresponds to the window height
--    Default: autoscale to 1.25 * largest frequency
-- # delta
--    Drawing interval between two samples (in ticks). Default: 100
-- # attrib (optional)
--    Drawing attributes. Default: see default attributes
-- # win (optional)
--    Window geometry. Format: {xpos, ypos, width, height}
-- # update (optional)
--    Redraw period. Display is updated every `update-th' sample
-- # display
--    Create a display if set to true
-- <</pre>>.
-- @return table reference to object instance
-- @see yats.histo:init.
function histo2:init(param)
  local self = _histo2:new()
  self.name = autoname(param)
  self.clname = "histo2"
  self.parameters = {
    val = true, win = false, maxfreq = false, nvals = true, delta = false, 
    title = false, update = false, attrib = false, display = false
  }
  self:adjust(param)
  self.exp_obj = param.val[1]
  self.exp_varname = param.val[2]
  self.exp_idx = param.val[3] or -1
  self.exp_idx2 = param.val[4] or -1
  self.title = param.title or self.name
  self.xpos = param.win.xpos or param.win[1]
  self.ypos = param.win.ypos or param.win[2]
  self.width = param.win.width or param.win[3]
  self.height = param.win.height or param.win[4]
  self.delta = param.delta or 1
  self.nvals = param.nvals or 0
  if param.maxfreq then
    self.maxfreq = param.maxfreq 
    self.autoscale = 0
  else
    self.autoscale = 1
  end
  self.nvals = param.nvals
  self.update = param.update or 1
  local a = self:finish()
  -- Associate a display with the histogram
  if param.display == nil or param.display == true then
    self.attrib = param.attrib or defaultattrib
    self.display = display{
      self.name..":display", instrument = self, attrib = self.attrib
    }
  else
    self.display = "no"
  end
  return a
end

--==========================================================================
-- Meter
--==========================================================================
_meter = meter
--- Definition of class 'meter'.
--@see yats.meter:init.
meter = class(_meter)

drawmode = {
  draw = 1,
  ylast = 2,
  ylastdescr = 4,
  yfull = 8,
  xval = 16,
  yall = 15,
  xyall = 31
}

linecolor = {
  black = 1,
  red = 2,
  green = 3,
  yellow = 4
}

--- Constructor for class 'meter'.
-- Sliding time history of a value. The meter is updated during the late slot phase.
-- Thus, variabls manipulating during the late phase cannot be displayed exactly
-- with a resolution of 1 tick, because the processing order between variable manipulation
-- and display is undefined.
-- @param param table - Parameter table
-- <<pre>>
-- # name (optional)
--    Name of the display. Default: "objNN"
-- # title (optional)
--    Title of the display. Default: "class-name:unknown"
-- # val
--    Value to be displayed
-- # win (optional)
--    Window geometry. Format: {xpos, ypos, width, height}
-- # nvals 
--    Number of values to plot
-- # maxval
--    Normalisation. `maxval' corresponds to window height
-- # delta 
--    Drawing interval between two samples (in ticks). Default: 100
-- # update (optional)
--    Redraw period. Display is updated every `update-th' sample
-- # mode
--    yats.AbsMode: display the sample value itself
--    yats.DiffMode: display difference between subsequent samples
-- # linemode (optional)
--    0: draw points; 1: draw lines. Default: 1
-- # linecolor (optional)
--    Color of lines. 1: black(default), 2: red, 3: green, 4: yellow
-- # drawmode (optional)
--    bit0: drawing, 
--~   bit1: last y-value, 
--~   bit2: description of last value, 
--~   bit3: full y-value, bit4: x-value
--~   Default: all bits set = 31
-- # dodrawing (optional)
--    1: only value
--~   2: only drawing
--~   3: value and drawing
--~   4: value (only y) and drawing   
-- # atttrib
--    Drawing attributes. Default: see default attributes
-- # display
--    Create a display if set to true
--<</pre>>.
-- @return table - Reference to object instance.
function meter:init(param)
  local self = _meter:new()
  self.name = autoname(param)
  self.clname = "meter"
  self.parameters = {
    val = true, win = true, dodrawing = false, linemode = false, linecolor = false, 
    nvals = true, maxval = true, delta = true, update = false, drawmode = false, 
    title = false, mode = true, attrib = false, display = false
  }
  self:adjust(param)
  self.exp_obj = param.val[1]
  self.exp_varname = param.val[2]
  self.exp_idx = param.val[3] or -1
  self.exp_idx2 = param.val[4] or -1
  self.title = param.title or self.name
  assert(param.mode, "Unknown mode.")
  self.display_mode = param.mode or AbsMode
  self.xpos = param.win.xpos or param.win[1]
  self.ypos = param.win.ypos or param.win[2]
  self.width = param.win.width or param.win[3]
  self.height = param.win.height or param.win[4]
  param.dodrawing = (param.dodrawing or 3)
  assert(param.dodrawing > 0 and param.dodrawing < 5, "meter: 'dodrawing' must be 1,2,3,4")
  self.do_drawing = param.dodrawing
  local h = {2,1,15,31}
  self.drawmode = param.drawmode or h[param.dodrawing]
--  self.drawmode = param.drawmode or 31
  param.linemode = param.linemode or 1
  assert(param.linemode > -1 and param.linemode < 2, "meter: 'linemode' must be 0 or 1")
  self.linemode = param.linemode
  self.linecolor = param.linecolor or 1
  self.nvals = param.nvals
  self.maxval = param.maxval
  self.delta = param.delta 
  self.update = param.update or 1 
  local a = self:finish()
  -- Associate a display with the histogram
  if param.display == nil or param.display == true then
    self.attrib = param.attrib or defaultattrib
    self.display = display{
      self.name..":display", instrument = self, attrib = self.attrib
    }
  else
    self.display = "no"
  end
  return a
end

return yats