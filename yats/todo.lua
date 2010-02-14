module("yats", package.seeall)

TODO = [[

FIXES:
Simulator:
    - [m] Complete Lua wrappers for all C models.
    - [m] Complete signal tracing - get content aware.

Graphical User Interface:
    - [l] Put simulation progress into titlebar when minimised.
    - [l] Provide attribute icon to measurement instruments.
    - [l] Make tray icon transparent.

Miscellanous:

Compiling and code:
    - [h] Add Destructors to all objects (done for all wrapped Lua objects).
    - [l] C++ files should have extension cpp.
    - [l] Get rid of dead code (profile it !).

========================================

IDEAS / FEATURES (all):
   - [l] Routing (OSPF, free version from Moy)
   - [l] MSTP based on verified RSTP
]]

return yats