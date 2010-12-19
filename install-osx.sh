#!/bin/bash

BASE_DIR="$HOME/Library/Application Support/ep128emu"
MAKECFG="`dirname \"$0\"`/ep128emu.app/Contents/MacOS/makecfg"

if ( ! [ -e "$BASE_DIR/roms/exos232.rom" ] ) ; then
  SAVED_CWD="`pwd`"
  mkdir -p "$BASE_DIR/roms" || exit -1
  cd "$BASE_DIR/roms" || exit -1
  curl -o ep128emu_roms.bin http://ep128emu.enterpriseforever.com/roms/ep128emu_roms.bin
  cd "$SAVED_CWD" ;
fi

if ( ! [ -e "$BASE_DIR/ep128cfg.dat" ] ) ; then
  "$MAKECFG" -f "$BASE_DIR" ;
else
  "$MAKECFG" "$BASE_DIR" ;
fi

