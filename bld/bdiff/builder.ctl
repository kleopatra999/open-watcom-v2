# bpatch Builder Control file
# ===========================

set PROJDIR=<CWD>
set PROJNAME=bdiff

[ INCLUDE <OWROOT>/build/master.ctl ]
[ LOG <LOGFNAME>.<LOGEXT> ]

[ INCLUDE <OWROOT>/build/defrule.ctl ]

[ BLOCK <1> rel ]
    cdsay <PROJDIR>

[ BLOCK <1> rel cprel ]
#======================
#    <CCCMD> dos386/bdiff.exe     <OWRELROOT>/binw/
    <CCCMD> dos386/bpatch.exe    <OWRELROOT>/binw/

#    <CCCMD> os2386/bdiff.exe     <OWRELROOT>/binp/
    <CCCMD> os2386/bpatch.exe    <OWRELROOT>/binp/

#    <CCCMD> nt386/bdiff.exe      <OWRELROOT>/binnt/
    <CCCMD> nt386/bpatch.exe     <OWRELROOT>/binnt/

#    <CCCMD> ntx64/bdiff.exe      <OWRELROOT>/binnt64/
    <CCCMD> ntx64/bpatch.exe     <OWRELROOT>/binnt64/

[ BLOCK . . ]
#============
cdsay <PROJDIR>
