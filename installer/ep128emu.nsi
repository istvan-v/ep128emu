;NSIS Modern User Interface
;Basic Example Script
;Written by Joost Verburg

  SetCompressor /SOLID /FINAL LZMA

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "ep128emu"
  OutFile "ep128emu-2.0.6-beta.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\ep128emu2"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\ep128emu2\InstallDirectory" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "..\COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\ep128emu2"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "ep128emu2" SecMain

  SectionIn RO

  SetOutPath "$INSTDIR"

  File "..\COPYING"
  File /nonfatal "..\LICENSE.FLTK"
  File /nonfatal "..\LICENSE.Lua"
  File /nonfatal "..\LICENSE.PortAudio"
  File /nonfatal "..\LICENSE.SDL"
  File /nonfatal "..\LICENSE.dotconf"
  File /nonfatal "..\LICENSE.libsndfile"
  File "/oname=news.txt" "..\NEWS"
  File "/oname=readme.txt" "..\README"
  File "..\ep128emu.exe"
  File "C:\MinGW\bin\libsndfile-1.dll"
  File "C:\MinGW\bin\lua51.dll"
  File "..\makecfg.exe"
  File "C:\MinGW\bin\mingwm10.dll"
  File "C:\MinGW\bin\portaudio.dll.0.0.19"
  File "C:\MinGW\bin\SDL.dll"
  File "..\tapeedit.exe"

  SetOutPath "$INSTDIR\config"

  SetOutPath "$INSTDIR\demo"

  SetOutPath "$INSTDIR\disk"

  File "..\disk\disk.zip"

  SetOutPath "$INSTDIR\roms"

  File "..\roms\epfileio.rom"

  SetOutPath "$INSTDIR\snapshot"

  SetOutPath "$INSTDIR\tape"

  ;Store installation folder
  WriteRegStr HKCU "Software\ep128emu2\InstallDirectory" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ep128emu - OpenGL mode.lnk" "$INSTDIR\ep128emu.exe" '-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ep128emu - software mode.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - GL - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-opengl -colorscheme 1'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - GL - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-opengl -colorscheme 2'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - GL - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-opengl -colorscheme 3'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - Software - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl -colorscheme 1'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - Software - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl -colorscheme 2'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - Software - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl -colorscheme 3'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\README.lnk" "$INSTDIR\readme.txt"
    SetOutPath "$INSTDIR\tape"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Tape editor.lnk" "$INSTDIR\tapeedit.exe"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Reinstall configuration files.lnk" "$INSTDIR\makecfg.exe" '"$INSTDIR"'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section "Source code" SecSrc

  SetOutPath "$INSTDIR\src"

  File "..\COPYING"
  File "..\NEWS"
  File "..\README"
  File "..\SConstruct"
  File "..\*.patch"
  File "..\*.sh"

  SetOutPath "$INSTDIR\src\Fl_Native_File_Chooser"

  File "..\Fl_Native_File_Chooser\CREDITS"
  File "..\Fl_Native_File_Chooser\README.txt"
  File "..\Fl_Native_File_Chooser\*.cxx"

  SetOutPath "$INSTDIR\src\Fl_Native_File_Chooser\FL"

  File "..\Fl_Native_File_Chooser\FL\*.H"

  SetOutPath "$INSTDIR\src\config"

  File "..\config\*.cfg"

  SetOutPath "$INSTDIR\src\disk"

  File "..\disk\*.zip"

  SetOutPath "$INSTDIR\src\gui"

  File "..\gui\*.fl"
  File /x "*_fl.cpp" "..\gui\*.cpp"
  File /x "*_fl.hpp" "..\gui\*.hpp"

  SetOutPath "$INSTDIR\src\installer"

  File "..\installer\*.nsi"
  File "..\installer\*.fl"
  File "..\installer\makecfg.cpp"

  SetOutPath "$INSTDIR\src\msvc"

  File "..\msvc\*.rules"
  File "..\msvc\*.sln"
  File "..\msvc\*.vcproj"

  SetOutPath "$INSTDIR\src\msvc\include"

  File "..\msvc\include\*.h"

  SetOutPath "$INSTDIR\src\ep128emu.app"

  SetOutPath "$INSTDIR\src\ep128emu.app\Contents"

  File "..\ep128emu.app\Contents\Info.plist"
  File "..\ep128emu.app\Contents\PkgInfo"

  SetOutPath "$INSTDIR\src\ep128emu.app\Contents\MacOS"

  SetOutPath "$INSTDIR\src\ep128emu.app\Contents\Resources"

  File "..\ep128emu.app\Contents\Resources\ep128emu.icns"

  SetOutPath "$INSTDIR\src\resource"

  File "..\resource\ep128emu.rc"
  File "..\resource\*.ico"

  SetOutPath "$INSTDIR\src\roms"

  File "..\roms\*.rom"

  SetOutPath "$INSTDIR\src\src"

  File "..\src\*.cpp"
  File "..\src\*.hpp"

  SetOutPath "$INSTDIR\src\tapeutil"

  File "..\tapeutil\*.fl"
  File "..\tapeutil\tapeio.cpp"
  File "..\tapeutil\tapeio.hpp"

  SetOutPath "$INSTDIR\src\z80"

  File "..\z80\COPYING"
  File "..\z80\*.cpp"
  File "..\z80\*.hpp"

SectionEnd

Section "Associate snapshot and demo files with ep128emu" SecAssocFiles

  WriteRegStr HKCR ".ep128" "" "Ep128Emu.SnapshotFile"
  WriteRegStr HKCR ".ep128s" "" "Ep128Emu.SnapshotFile"
  WriteRegStr HKCR "Ep128Emu.SnapshotFile" "" "Ep128Emu snapshot file"
  WriteRegStr HKCR "Ep128Emu.SnapshotFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,0"
  WriteRegStr HKCR "Ep128Emu.SnapshotFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.SnapshotFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -snapshot "%1"'
  WriteRegStr HKCR ".ep128d" "" "Ep128Emu.DemoFile"
  WriteRegStr HKCR "Ep128Emu.DemoFile" "" "Ep128Emu demo file"
  WriteRegStr HKCR "Ep128Emu.DemoFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,0"
  WriteRegStr HKCR "Ep128Emu.DemoFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.DemoFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -snapshot "%1"'

SectionEnd

Section "Download ROM images" SecDLRoms

  SetOutPath "$INSTDIR\roms"

  NSISdl::download "http://ep128emu.enterpriseforever.org/roms/ep128emu_roms.bin" "$INSTDIR\roms\ep128emu_roms.bin"
  Pop $R0
  StrCmp $R0 "success" downloadDone 0
  StrCmp $R0 "cancel" downloadDone 0

  MessageBox MB_OK "WARNING: download from ep128emu.enterpriseforever.org failed ($R0), trying www.sharemation.com instead"

  NSISdl::download "http://www.sharemation.com/IstvanV/roms/ep128emu_roms.bin" "$INSTDIR\roms\ep128emu_roms.bin"
  Pop $R0
  StrCmp $R0 "success" downloadDone 0
  StrCmp $R0 "cancel" downloadDone 0

  MessageBox MB_OK "Download failed: $R0"

  downloadDone:

SectionEnd

Section "Install configuration files" SecInstCfg

  SectionIn RO

  SetOutPath "$INSTDIR"
  ExecWait '"$INSTDIR\makecfg.exe" "$INSTDIR"'

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecMain ${LANG_ENGLISH} "ep128emu binaries"
  LangString DESC_SecSrc ${LANG_ENGLISH} "ep128emu source code"
  LangString DESC_SecAssocFiles ${LANG_ENGLISH} "Associate snapshot and demo files with ep128emu"
  LangString DESC_SecDLRoms ${LANG_ENGLISH} "Download and install ROM images"
  LangString DESC_SecInstCfg ${LANG_ENGLISH} "Install configuration files"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSrc} $(DESC_SecSrc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssocFiles} $(DESC_SecAssocFiles)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDLRoms} $(DESC_SecDLRoms)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecInstCfg} $(DESC_SecInstCfg)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\LICENSE.FLTK"
  Delete "$INSTDIR\LICENSE.Lua"
  Delete "$INSTDIR\LICENSE.PortAudio"
  Delete "$INSTDIR\LICENSE.SDL"
  Delete "$INSTDIR\LICENSE.dotconf"
  Delete "$INSTDIR\LICENSE.libsndfile"
  Delete "$INSTDIR\news.txt"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\ep128emu.exe"
  Delete "$INSTDIR\libsndfile-1.dll"
  Delete "$INSTDIR\lua51.dll"
  Delete "$INSTDIR\makecfg.exe"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\portaudio.dll.0.0.19"
  Delete "$INSTDIR\SDL.dll"
  Delete "$INSTDIR\tapeedit.exe"
  Delete "$INSTDIR\config\EP_128k_EXDOS.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_NoCartridge.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_NoCartridge_FileIO.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_NoCartridge.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_NoCartridge_FileIO.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\EP_320k_EXDOS.cfg"
  Delete "$INSTDIR\config\EP_320k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\EP_320k_EXDOS_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_320k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\EP_320k_Tape.cfg"
  Delete "$INSTDIR\config\EP_320k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\EP_320k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_320k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\EP_640k_EXOS23_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_NoCartridge.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_NoCartridge_FileIO.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_NoCartridge.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_NoCartridge_FileIO.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\EP_Keyboard_HU.cfg"
  Delete "$INSTDIR\config\EP_Keyboard_US.cfg"
  Delete "$INSTDIR\config\ep128brd\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_Tape.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_128k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS231_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_Tape.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_128k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS231_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_Tape.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_128k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS22_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS23_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS231_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS_EP-PLUS.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS_EP-PLUS_TASMON.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS_FileIO_SpectrumEmulator.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS_NoCartridge.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape_EP-PLUS.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape_NoCartridge.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape_NoCartridge_FileIO.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_128k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS231_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS_NoCartridge.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape_NoCartridge.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape_TASMON.cfg"
  RMDir "$INSTDIR\config\ep128brd"
  RMDir "$INSTDIR\config\ep128esp"
  RMDir "$INSTDIR\config\ep128hun"
  RMDir "$INSTDIR\config\ep128uk"
  RMDir "$INSTDIR\config\ep64"
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR\demo"
  Delete "$INSTDIR\disk\disk.zip"
  RMDir "$INSTDIR\disk"
  RMDir "$INSTDIR\progs"
  Delete "$INSTDIR\roms\asmen15.rom"
  Delete "$INSTDIR\roms\asmon15.rom"
  Delete "$INSTDIR\roms\basic20.rom"
  Delete "$INSTDIR\roms\basic21.rom"
  Delete "$INSTDIR\roms\brd.rom"
  Delete "$INSTDIR\roms\cyrus.rom"
  Delete "$INSTDIR\roms\ep-plus.rom"
  Delete "$INSTDIR\roms\ep128emu_roms.bin"
  Delete "$INSTDIR\roms\ep_basic.rom"
  Delete "$INSTDIR\roms\epdos_z.rom"
  Delete "$INSTDIR\roms\epfileio.rom"
  Delete "$INSTDIR\roms\esp.rom"
  Delete "$INSTDIR\roms\exdos0.rom"
  Delete "$INSTDIR\roms\exdos1.rom"
  Delete "$INSTDIR\roms\exdos10.rom"
  Delete "$INSTDIR\roms\exdos13.rom"
  Delete "$INSTDIR\roms\exdos13isdos10esp.rom"
  Delete "$INSTDIR\roms\exdos13isdos10hun.rom"
  Delete "$INSTDIR\roms\exos0.rom"
  Delete "$INSTDIR\roms\exos1.rom"
  Delete "$INSTDIR\roms\exos20.rom"
  Delete "$INSTDIR\roms\exos21.rom"
  Delete "$INSTDIR\roms\exos22.rom"
  Delete "$INSTDIR\roms\exos23.rom"
  Delete "$INSTDIR\roms\exos231.rom"
  Delete "$INSTDIR\roms\exos231esp.rom"
  Delete "$INSTDIR\roms\exos231hun.rom"
  Delete "$INSTDIR\roms\exos231uk.rom"
  Delete "$INSTDIR\roms\fenas12.rom"
  Delete "$INSTDIR\roms\forth.rom"
  Delete "$INSTDIR\roms\genmon.rom"
  Delete "$INSTDIR\roms\heass10.rom"
  Delete "$INSTDIR\roms\heass10hfont.rom"
  Delete "$INSTDIR\roms\heass10uk.rom"
  Delete "$INSTDIR\roms\heassekn.rom"
  Delete "$INSTDIR\roms\hun.rom"
  Delete "$INSTDIR\roms\iview.rom"
  Delete "$INSTDIR\roms\lisp.rom"
  Delete "$INSTDIR\roms\pascal11.rom"
  Delete "$INSTDIR\roms\pasians.rom"
  Delete "$INSTDIR\roms\tasmon0.rom"
  Delete "$INSTDIR\roms\tasmon1.rom"
  Delete "$INSTDIR\roms\tasmon15.rom"
  Delete "$INSTDIR\roms\tpt.rom"
  Delete "$INSTDIR\roms\zt18.rom"
  Delete "$INSTDIR\roms\zt18ekn.rom"
  Delete "$INSTDIR\roms\zt18hfnt.rom"
  Delete "$INSTDIR\roms\zt18hun.rom"
  Delete "$INSTDIR\roms\zt18uk.rom"
  Delete "$INSTDIR\roms\zx41.rom"
  RMDir "$INSTDIR\roms"
  RMDir "$INSTDIR\snapshot"
  RMDir /r "$INSTDIR\src"
  RMDir "$INSTDIR\tape"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP

  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - OpenGL mode.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - software mode.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\GUI themes\ep128emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\GUI themes\ep128emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\GUI themes\ep128emu - GL - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\GUI themes\ep128emu - Software - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\GUI themes\ep128emu - Software - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\GUI themes\ep128emu - Software - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\README.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Tape editor.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Reinstall configuration files.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"

  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP\GUI themes"

  startMenuDeleteLoop:
    ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

    IfErrors startMenuDeleteLoopDone

    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKCU "Software\ep128emu2\InstallDirectory"
  DeleteRegKey /ifempty HKCU "Software\ep128emu2"

  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

SectionEnd

