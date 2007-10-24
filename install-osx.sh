#!/bin/bash

BASE_DIR="$HOME/Library/Application Support/ep128emu"
MAKECFG="`dirname \"$0\"`/ep128emu.app/Contents/MacOS/makecfg"

if ( ! [ -e "$BASE_DIR/ep128cfg.dat" ] ) ; then
  "$MAKECFG" -f "$BASE_DIR" ;
else
  "$MAKECFG" "$BASE_DIR" ;
fi

if ( ! [ -e "$BASE_DIR/roms/exos23.rom" ] ) ; then
  SAVED_CWD="`pwd`"
  mkdir -p "$BASE_DIR/roms" || exit -1
  cd "$BASE_DIR/roms" || exit -1
  curl -o ep128emu_roms.zip http://ep128emu.enterpriseforever.org/roms/ep128emu_roms.zip
  unzip -o ep128emu_roms.zip
  rm -f ep128emu_roms.zip
  cd "$SAVED_CWD" ;
fi

