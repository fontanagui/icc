#!/bin/sh
raiz=$(CDPATH='' cd -- "$(dirname -- "$0")/../usr" && pwd)
export BROYDEN_LIKWID_RAIZ="$raiz"
export LUA_PATH="$raiz/share/lua/?.lua;;"
export LUA_CPATH="$raiz/lib/x86_64-linux-gnu/?.so;;"
export LD_LIBRARY_PATH="$raiz/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$raiz/bin/lua5.3" -e '
    local lk = require("likwid")
    local raiz = os.getenv("BROYDEN_LIKWID_RAIZ")
    lk.groupfolder = raiz .. "/share/likwid/perfgroups"
    lk.pinlibpath = raiz .. "/lib/x86_64-linux-gnu/liblikwidpin.so.5.4"
    lk.setGroupPath(lk.groupfolder)
' "$raiz/bin/${0##*/}" "$@"
