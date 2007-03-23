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
  OutFile "ep128emu-2.0.0-beta2.exe"

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
  File /nonfatal "..\LICENSE.PortAudio"
  File /nonfatal "..\LICENSE.dotconf"
  File /nonfatal "..\LICENSE.libsndfile"
  File "/oname=readme.txt" "..\README"
  File "..\ep128emu.exe"
  File "D:\MinGW\bin\libsndfile-1.dll"
  File "..\makecfg.exe"
  File "D:\MinGW\bin\mingwm10.dll"
  File "D:\MinGW\bin\portaudio.dll.0.0.19"
  File "..\tapeedit.exe"

  SetOutPath "$INSTDIR\config"

  SetOutPath "$INSTDIR\demo"

  SetOutPath "$INSTDIR\disk"

  File "..\disk\disk.zip"

  SetOutPath "$INSTDIR\progs"

  SetOutPath "$INSTDIR\roms"

  File "..\roms\epfileio.rom"

  SetOutPath "$INSTDIR\tape"

  ;Store installation folder
  WriteRegStr HKCU "Software\ep128emu2\InstallDirectory" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ep128emu - OpenGL mode.lnk" "$INSTDIR\ep128emu.exe" '-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ep128emu - software mode.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\README.lnk" "$INSTDIR\readme.txt"
    SetOutPath "$INSTDIR\tape"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Tape editor.lnk" "$INSTDIR\tapeedit.exe"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Reinstall configuration files.lnk" "$INSTDIR\makecfg.exe" '"$INSTDIR"'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

  ExecWait '"$INSTDIR\makecfg.exe" "$INSTDIR"'

SectionEnd

Section "Source code" SecSrc

  SetOutPath "$INSTDIR\src"

  File "..\COPYING"
  File "..\README"
  File "..\SConstruct"
  File "..\*.cpp"
  File "..\*.hpp"

  SetOutPath "$INSTDIR\src\config"

  File "..\config\*.cfg"

  SetOutPath "$INSTDIR\src\disk"

  File "..\disk\*.zip"

  SetOutPath "$INSTDIR\src\gui"

  File "..\gui\*.fl"
  File "..\gui\gui.cpp"
  File "..\gui\gui.hpp"
  File "..\gui\main.cpp"

  SetOutPath "$INSTDIR\src\installer"

  File "..\installer\*.nsi"
  File "..\installer\*.fl"
  File "..\installer\makecfg.cpp"

  SetOutPath "$INSTDIR\src\roms"

  File "..\roms\*.rom"

  SetOutPath "$INSTDIR\src\tapeutil"

  File "..\tapeutil\*.fl"
  File "..\tapeutil\tapeio.cpp"
  File "..\tapeutil\tapeio.hpp"

  SetOutPath "$INSTDIR\src\z80"

  File "..\z80\COPYING"
  File "..\z80\*.cpp"
  File "..\z80\*.hpp"

SectionEnd

Section "Download ROM images" SecDLRoms

  SetOutPath "$INSTDIR\roms"

  Push ""
  Push "tasmon1.rom"
  Push "tasmon0.rom"
  Push "exos1.rom"
  Push "exos0.rom"
  Push "exdos1.rom"
  Push "exdos0.rom"
  Push "ep_basic.rom"

  downloadLoop:

    Pop $0
    StrCmp $0 "" downloadLoopDone 0
    NSISdl::download "http://www.sharemation.com/IstvanV/roms/$0" "$INSTDIR\roms\$0"
    Pop $R0
    StrCmp $R0 "success" downloadLoop 0
    StrCmp $R0 "cancel" downloadLoop 0
    MessageBox MB_OK "Download failed: $R0"
    Goto downloadLoop

  downloadLoopDone:

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecMain ${LANG_ENGLISH} "ep128emu binaries"
  LangString DESC_SecSrc ${LANG_ENGLISH} "ep128emu source code"
  LangString DESC_SecDLRoms ${LANG_ENGLISH} "Download and install ROM images"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSrc} $(DESC_SecSrc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDLRoms} $(DESC_SecDLRoms)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\ep128emu.exe"
  Delete "$INSTDIR\libsndfile-1.dll"
  Delete "$INSTDIR\makecfg.exe"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\portaudio.dll.0.0.19"
  Delete "$INSTDIR\tapeedit.exe"
  Delete "$INSTDIR\config\EP_Keyboard_US.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape.cfg"
  Delete "$INSTDIR\config\EP_256k_Tape.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS.cfg"
  Delete "$INSTDIR\config\EP_256k_EXDOS.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\EP_256k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\EP_256k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\EP_256k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\EP_256k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\EP_64k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_256k_Tape_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_64k_EXDOS_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_128k_EXDOS_FileIO_TASMON.cfg"
  Delete "$INSTDIR\config\EP_256k_EXDOS_FileIO_TASMON.cfg"
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR\demo"
  RMDir "$INSTDIR\disk"
  RMDir "$INSTDIR\progs"
  Delete "$INSTDIR\roms\ep_basic.rom"
  Delete "$INSTDIR\roms\epfileio.rom"
  Delete "$INSTDIR\roms\exdos0.rom"
  Delete "$INSTDIR\roms\exdos1.rom"
  Delete "$INSTDIR\roms\exos0.rom"
  Delete "$INSTDIR\roms\exos1.rom"
  Delete "$INSTDIR\roms\tasmon0.rom"
  Delete "$INSTDIR\roms\tasmon1.rom"
  RMDir "$INSTDIR\roms"
  RMDir /r "$INSTDIR\src"
  RMDir "$INSTDIR\tape"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP

  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - OpenGL mode.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - software mode.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\README.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Tape editor.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Reinstall configuration files.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"

  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"

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

