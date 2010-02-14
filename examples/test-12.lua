require "yats.stdlib"
require "yats.core"
require "yats.misc"
require "yats.graphics"

ncycles = 30000000
maxval = 500
x,y,dx,dy = 0, 0, 200, 100
dx2 = 200
-- Init the yats random generator.
print("Init random generator.")
yats.sim:SetRand(10)

-- Reset simulation time.
print("Reset simulation time.")
yats.sim:ResetTime()
--
-- Sources
--

-- CBR Source
scbr = yats.cbrsrc{"scbr", vci = 1, delta = 10, out = {"xcbr"}}

-- BS Source
sbs = yats.bssrc{"sbs",vci = 2,  ex = 10, es = 90, delta = 1, out = {"xbs"}} 

-- BS Source deterministic
sbs2 = yats.bssrc{"sbs2",vci = 8,  ex = 10, es = 90, 
  des = 1, dex = 1, delta = 1, out = {"xbs2"}} 

-- GEO Source
sgeo = yats.geosrc{"sgeo", vci = 3, ed = 10, out = {"xgeo"}}

-- LIST Source
slist = yats.listsrc{"slist", vci = 4, --ntim = 6, 
  delta = {10, 20, 20, 30, 30, 30, 40, 40, 50, 80},
  rep = true,
  out = {"xlist"}
}

-- DIST Source - geometric distribution
dgeo = yats.distrib{"dgeo", dist=yats.distrib.geometric, distargs={e=10}}
sdgeo = yats.distsrc{"sdgeo", vci = 5, dist = dgeo, out = {"xdgeo"}}

-- DIST Source - binomial distribution
dbin = yats.distrib{"dbin", dist=yats.distrib.binomial, distargs={n=20, p=0.5}}
sdbin = yats.distsrc{"sdbin", vci = 6, dist = dbin, out = {"xdbin"}}

-- DIST Source - tabular
dtab = yats.distrib{"dtab", 
  dist={
    {1, 0.8}, {10, 0.1}, {100, 0.1}
  }
}
sdtab = yats.distsrc{"sdtab", vci = 7, dist = dtab, out = {"xdtab"}}

--
-- IAT measurements and histograms
--
xcbr = yats.meas3{"xcbr", vci = {1,1}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hcbr = yats.histo{"hcbr", title = "CBR IAT", 
  val = {xcbr, "IAT", 1},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xbs = yats.meas3{"xbs", vci = {2,2}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hbs = yats.histo{"hbs", title = "BS IAT", 
  val = {xbs, "IAT", 2},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xgeo = yats.meas3{"xgeo", vci = {3,3}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hgeo = yats.histo{"hgeo", title = "GEO IAT", 
  val = {xgeo, "IAT", 3},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xlist = yats.meas3{"xlist", vci = {4,4}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hlist = yats.histo{"hlist", title = "LIST IAT", 
  val = {xlist, "IAT", 4},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xdgeo = yats.meas3{"xdgeo", vci = {5,5}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hdgeo = yats.histo{"hdgeo", title = "DIST GEO IAT", 
  val = {xdgeo, "IAT", 5},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xdbin = yats.meas3{"xdbin", vci = {6,6}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hdbin = yats.histo{"hdbin", title = "DIST BIN IAT", 
  val = {xdbin, "IAT", 6},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xdtab = yats.meas3{"xdtab", vci = {7,7}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hdtab = yats.histo{"hdtab", title = "DIST TAB IAT", 
  val = {xdtab, "IAT", 7},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
xbs2 = yats.meas3{"xbs2", vci = {8,8}, 
  iat = {0,100}, iatdiv = 1, out = nc()
}
hbs2 = yats.histo{"hbs2", title = "BS DET IAT", 
  val = {xbs2, "IAT", 8},
  win={x,y,dx2,dy},
  delta = 1000, display = false
}
--
-- Meters 
--
mcbr = yats.meter{"mcbr", title = "CBR", val = {scbr, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mbs = yats.meter{"mbs", title = "BS", val = {sbs, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mbs2 = yats.meter{"mbs2", title = "BS DET", val = {sbs2, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mgeo = yats.meter{"mgeo", title = "GEO", val = {sgeo, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mlist = yats.meter{"mlist", title = "LIST", val = {slist, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mdgeo = yats.meter{"mdgeo", title = "DIST-GEO", val = {sdgeo, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mdbin = yats.meter{"mdbin", title = "DIST-BIN", val = {sdbin, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}
mdtab = yats.meter{"mdtab", title = "DIST-TAB", val = {sdtab, "Count"}, 
  mode = yats.DiffMode,
  win={x,y,dx,dy},
  nvals = 100, maxval = 500,
  delta = 1000, update = 50, display = false
}

disp = yats.bigdisplay {
  "disp", title="SOURCES", nrow = 4, ncol = 4, width = 200, height = 100, 
  instruments = {
    {mcbr,  hcbr,  mbs,   hbs},
    {mbs2,  hbs2,  mgeo,  hgeo},
    {mlist, hlist, mdgeo, hdgeo},
    {mdbin, hdbin, mdtab, hdtab}
  }
}
yats.sim:connect()
yats.sim:run(ncycles)
printf("CBR     : %d\n", scbr.counter)
printf("BS      : %d\n", sbs.counter)
printf("BS DET  : %d\n", sbs2.counter)
printf("GEO     : %d\n", sgeo.counter)
printf("LIST    : %d\n", slist.counter)
printf("DIST GEO: %d\n", sdgeo.counter)
printf("DIST BIN: %d\n", sdbin.counter)
printf("DIST TAB: %d\n", sdtab.counter)
--[[
print("=== GEO ===============================================")
dis_geo:show()
print("=== BIN ===============================================")
dis_bin:show()
print("=== TAB ===============================================")
dis_tab:show()
]]

result = {}
for _, m in pairs{hcbr, hbs, hbs2, hgeo, hlist, hdgeo, hdbin, hdtab} do
  for i = 1, m.nvals do
--    printf("%s %d %d\n", m.name, i, m:getVal(i))
    table.insert(result, m:getVal(i-1))
  end
end
return result
