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

function logging.console(logPattern)

    return logging.new(  function(self, level, message)
                            io.stdout:write(logging.prepareLogMsg(logPattern, os.date(), level, message, 
								  _G.yats.SimTime, _G.yats.SimTimeReal).."\n")
                            return true
                        end
                      )
end

return logging