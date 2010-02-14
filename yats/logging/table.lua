-------------------------------------------------------------------------------
-- Prints logging information to console
--
-- @author Thiago Costa Ponte (thiago@ideais.com.br)
--
-- @copyright 2004-2007 Kepler Project
--
-- @release $Id: console.lua 222 2009-06-06 05:51:35Z leuwer $
-------------------------------------------------------------------------------

local logging = require "yats.logging"
function logging.table(logPattern, ringlength)
   logging.ring = {}
   logging.ringlength = ringlength
   local size = 0
   
   return logging.new(function(self, level, message)
			 table.insert(logging.ring, logging.prepareLogMsg(logPattern, os.date(), level, message, 
									  _G.yats.SimTime, _G.yats.SimTimeReal))
			 size = size + 1
			 if size > logging.ringlength then
			    table.remove(logging.ring, 1)
			 end
			 return true
		      end
		   )
end

return logging