;NSIS Modern User Interface
;Basic Example Script
;Written by Joost Verburg

  SetCompressor /SOLID /FINAL LZMA

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Use "makensis.exe /DWIN64" to create x64 installer
  ;NOTE: this script requires INetC (nsis.sourceforge.net/Inetc_plug-in)
  !ifndef WIN64
  ;Name and file
  Name "ep128emu"
  OutFile "ep128emu-2.0.11_beta-x86.exe"
  ;Default installation folder
  InstallDir "$PROGRAMFILES\ep128emu2"
  !else
  Name "ep128emu (x64)"
  OutFile "ep128emu-2.0.11_beta-x64.exe"
  InstallDir "$PROGRAMFILES64\ep128emu2"
  !endif

  RequestExecutionLevel admin

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\ep128emu2\InstallDirectory" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_COMPONENTSPAGE_NODESC

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "..\COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
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

Section "ep128emu binaries" SecMain

  SectionIn RO

  SetOutPath "$INSTDIR"

  File "..\COPYING"
  File "..\licenses\LICENSE.FLTK"
  File "..\licenses\LICENSE.Lua"
  File "..\licenses\LICENSE.PortAudio"
  File "..\licenses\LICENSE.SDL"
  File "..\licenses\LICENSE.dotconf"
  File "..\licenses\LICENSE.libsndfile"
  File "/oname=news.txt" "..\NEWS"
  File "/oname=readme.txt" "..\README"
  File "..\ep128emu.exe"
  File "..\makecfg.exe"
  File "..\tapeedit.exe"
  !ifndef WIN64
  File "C:\mingw32\bin\libgcc_s_dw2-1.dll"
  File "C:\mingw32\bin\libsndfile-1.dll"
  File "C:\mingw32\bin\libstdc++-6.dll"
  File "C:\mingw32\bin\lua51.dll"
  File "C:\mingw32\bin\lua53.dll"
  File "C:\mingw32\bin\portaudio_x86.dll"
  File "C:\mingw32\bin\SDL.dll"
  !else
  File "C:\mingw64\bin\libgcc_s_seh-1.dll"
  File "C:\mingw64\bin\libsndfile-1.dll"
  File "C:\mingw64\bin\libstdc++-6.dll"
  File "C:\mingw64\bin\lua51.dll"
  File "C:\mingw64\bin\lua53.dll"
  File "C:\mingw64\bin\portaudio_x64.dll"
  File "C:\mingw64\bin\SDL.dll"
  !endif

  SetOutPath "$INSTDIR\config"

  SetOutPath "$INSTDIR\demo"

  SetOutPath "$INSTDIR\disk"

  File "..\disk\disk.zip"
  File "..\disk\ide126m.vhd.bz2"
  File "..\disk\ide189m.vhd.bz2"

  SetOutPath "$INSTDIR\files"

  SetOutPath "$INSTDIR\roms"

  File "..\roms\epfileio.rom"
  File "..\roms\tvcfileio.rom"
  File "..\util\epimgconv\iview\iview.rom"

  SetOutPath "$INSTDIR\snapshot"

  SetOutPath "$INSTDIR\tape"

  ;Store installation folder
  WriteRegStr HKLM "Software\ep128emu2\InstallDirectory" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    SetShellVarContext all

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes"
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator"
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator"
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ep128emu - OpenGL mode.lnk" "$INSTDIR\ep128emu.exe" '-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\ep128emu - software mode.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - GL - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-opengl -colorscheme 1'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - GL - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-opengl -colorscheme 2'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - GL - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-opengl -colorscheme 3'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - Software - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl -colorscheme 1'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - Software - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl -colorscheme 2'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\GUI themes\ep128emu - Software - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-no-opengl -colorscheme 3'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - GL - default theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -opengl' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - GL - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -opengl -colorscheme 1' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - GL - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -opengl -colorscheme 2' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - GL - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -opengl -colorscheme 3' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - Software - default theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -no-opengl' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - Software - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -no-opengl -colorscheme 1' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - Software - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -no-opengl -colorscheme 2' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Spectrum emulator\zx128emu - Software - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-zx -no-opengl -colorscheme 3' "$INSTDIR\ep128emu.exe" 1
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - GL - default theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -opengl' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - GL - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -opengl -colorscheme 1' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - GL - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -opengl -colorscheme 2' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - GL - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -opengl -colorscheme 3' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - Software - default theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -no-opengl' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - Software - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -no-opengl -colorscheme 1' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - Software - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -no-opengl -colorscheme 2' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\CPC emulator\cpc6128emu - Software - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-cpc -no-opengl -colorscheme 3' "$INSTDIR\ep128emu.exe" 2
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - GL - default theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -opengl' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - GL - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -opengl -colorscheme 1' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - GL - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -opengl -colorscheme 2' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - GL - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -opengl -colorscheme 3' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - Software - default theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -no-opengl' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - Software - Win2000 theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -no-opengl -colorscheme 1' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - Software - plastic theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -no-opengl -colorscheme 2' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\TVC emulator\tvc64emu - Software - Gtk+ theme.lnk" "$INSTDIR\ep128emu.exe" '-tvc -no-opengl -colorscheme 3' "$INSTDIR\ep128emu.exe" 3
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\README.lnk" "$INSTDIR\readme.txt"
    SetOutPath "$INSTDIR\tape"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Tape editor.lnk" "$INSTDIR\tapeedit.exe"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Reinstall configuration files.lnk" "$INSTDIR\makecfg.exe" '"$INSTDIR"'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

Section "ep128emu source code" SecSrc

  SetOutPath "$INSTDIR\src"

  File "..\COPYING"
  File "..\NEWS"
  File "..\README"
  File "..\SConstruct"
  File "..\*.patch"
  File "..\*.sh"
  File "..\*.spec"

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
  File "..\disk\*.bz2"

  SetOutPath "$INSTDIR\src\ep128emu.app"

  SetOutPath "$INSTDIR\src\ep128emu.app\Contents"

  File "..\ep128emu.app\Contents\Info.plist"
  File "..\ep128emu.app\Contents\PkgInfo"

  SetOutPath "$INSTDIR\src\ep128emu.app\Contents\MacOS"

  SetOutPath "$INSTDIR\src\ep128emu.app\Contents\Resources"

  File "..\ep128emu.app\Contents\Resources\ep128emu.icns"

  SetOutPath "$INSTDIR\src\gui"

  File "..\gui\*.fl"
  File /x "*_fl.cpp" "..\gui\*.cpp"
  File /x "*_fl.hpp" "..\gui\*.hpp"

  SetOutPath "$INSTDIR\src\installer"

  File "..\installer\*.nsi"
  File "..\installer\*.fl"
  File "..\installer\makecfg.cpp"

  SetOutPath "$INSTDIR\src\licenses"

  File "..\licenses\LICENSE.*"

  SetOutPath "$INSTDIR\src\msvc"

  File "..\msvc\*.rules"
  File "..\msvc\*.sln"
  File "..\msvc\*.vcproj"

  SetOutPath "$INSTDIR\src\msvc\include"

  File "..\msvc\include\*.h"

  SetOutPath "$INSTDIR\src\resource"

  File "..\resource\*.rc"
  File "..\resource\*.ico"
  File "..\resource\*.png"
  File "..\resource\*.desktop"
  File "..\resource\Makefile"

  SetOutPath "$INSTDIR\src\roms"

  File "..\roms\*.rom"
  File "..\roms\*.s"

  SetOutPath "$INSTDIR\src\src"

  File "..\src\*.c"
  File "..\src\*.cpp"
  File "..\src\*.h"
  File "..\src\*.hpp"

  SetOutPath "$INSTDIR\src\tapeutil"

  File "..\tapeutil\*.fl"
  File "..\tapeutil\tapeio.cpp"
  File "..\tapeutil\tapeio.hpp"

  SetOutPath "$INSTDIR\src\util"

  File "..\util\*.patch"

  SetOutPath "$INSTDIR\src\util\dtf"

  File "..\util\dtf\*.cpp"
  File "..\util\dtf\*.s"

  SetOutPath "$INSTDIR\src\util\epcompress"

  SetOutPath "$INSTDIR\src\util\epcompress\src"

  File "..\util\epcompress\src\*.cpp"
  File "..\util\epcompress\src\*.hpp"

  SetOutPath "$INSTDIR\src\util\epcompress\uncompress"

  File "..\util\epcompress\uncompress\*.ext"
  File "..\util\epcompress\uncompress\*.rom"
  File "..\util\epcompress\uncompress\*.s"

  SetOutPath "$INSTDIR\src\util\epcompress\z80_asm"

  File "..\util\epcompress\z80_asm\*.f"
  File "..\util\epcompress\z80_asm\*.py"
  File "..\util\epcompress\z80_asm\*.s"

  SetOutPath "$INSTDIR\src\util\epimgconv"

  SetOutPath "$INSTDIR\src\util\epimgconv\iview"

  File "..\util\epimgconv\iview\*.com"
  File "..\util\epimgconv\iview\*.ext"
  File "..\util\epimgconv\iview\*.lua"
  File "..\util\epimgconv\iview\*.rom"

  SetOutPath "$INSTDIR\src\util\epimgconv\iview\old"

  File "..\util\epimgconv\iview\old\*.com"
  File "..\util\epimgconv\iview\old\*.ext"
  File "..\util\epimgconv\iview\old\*.hea"
  File "..\util\epimgconv\iview\old\*.s"

  SetOutPath "$INSTDIR\src\util\epimgconv\iview\src"

  File "..\util\epimgconv\iview\src\*.s"

  SetOutPath "$INSTDIR\src\util\epimgconv\src"

  File "..\util\epimgconv\src\*.fl"
  File /x "*_fl.cpp" "..\util\epimgconv\src\*.cpp"
  File /x "*_fl.hpp" "..\util\epimgconv\src\*.hpp"
  File "..\util\epimgconv\src\*.s"

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

Section "Associate Spectrum .TZX and .Z80 files with ep128emu" SecAssocZX

  WriteRegStr HKCR ".tzx" "" "Ep128Emu.TZXFile"
  WriteRegStr HKCR "Ep128Emu.TZXFile" "" "Ep128Emu Spectrum tape file"
  WriteRegStr HKCR "Ep128Emu.TZXFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,1"
  WriteRegStr HKCR "Ep128Emu.TZXFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.TZXFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -zx "tape.imageFile=%1"'
  WriteRegStr HKCR ".z80" "" "Ep128Emu.Z80File"
  WriteRegStr HKCR "Ep128Emu.Z80File" "" "Ep128Emu Spectrum snapshot file"
  WriteRegStr HKCR "Ep128Emu.Z80File\DefaultIcon" "" "$INSTDIR\ep128emu.exe,1"
  WriteRegStr HKCR "Ep128Emu.Z80File\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.Z80File\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -zx -snapshot "%1"'

SectionEnd

Section "Associate CPC .CDT and .SNA files with ep128emu" SecAssocCPC

  WriteRegStr HKCR ".cdt" "" "Ep128Emu.CDTFile"
  WriteRegStr HKCR "Ep128Emu.CDTFile" "" "Ep128Emu CPC tape file"
  WriteRegStr HKCR "Ep128Emu.CDTFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,2"
  WriteRegStr HKCR "Ep128Emu.CDTFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.CDTFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -cpc "tape.imageFile=%1"'
  WriteRegStr HKCR ".sna" "" "Ep128Emu.SNAFile"
  WriteRegStr HKCR "Ep128Emu.SNAFile" "" "Ep128Emu CPC snapshot file"
  WriteRegStr HKCR "Ep128Emu.SNAFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,2"
  WriteRegStr HKCR "Ep128Emu.SNAFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.SNAFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -snapshot "%1"'

SectionEnd

Section /o "Associate CPC .DSK files with ep128emu" SecAssocDiskCPC

  WriteRegStr HKCR ".dsk" "" "Ep128Emu.CPCDiskFile"
  WriteRegStr HKCR "Ep128Emu.CPCDiskFile" "" "Ep128Emu CPC disk image"
  WriteRegStr HKCR "Ep128Emu.CPCDiskFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,2"
  WriteRegStr HKCR "Ep128Emu.CPCDiskFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.CPCDiskFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -cpc "floppy.a.imageFile=%1"'

SectionEnd

Section /o "Associate TVC .DSK files with ep128emu" SecAssocDiskTVC

  WriteRegStr HKCR ".dsk" "" "Ep128Emu.TVCDiskFile"
  WriteRegStr HKCR "Ep128Emu.TVCDiskFile" "" "Ep128Emu TVC disk image"
  WriteRegStr HKCR "Ep128Emu.TVCDiskFile\DefaultIcon" "" "$INSTDIR\ep128emu.exe,3"
  WriteRegStr HKCR "Ep128Emu.TVCDiskFile\shell" "" "open"
  WriteRegStr HKCR "Ep128Emu.TVCDiskFile\shell\open\command" "" '"$INSTDIR\ep128emu.exe" -tvc "floppy.a.imageFile=%1"'

SectionEnd

Section "Download and install ROM images" SecDLRoms

  SetOutPath "$INSTDIR\roms"

  INetC::get "http://ep128.hu/Emu/ep128emu_roms-2.0.11.bin" "$INSTDIR\roms\ep128emu_roms-2.0.11.bin"
  Pop $R0
  StrCmp $R0 "OK" downloadDone 0
  StrCmp $R0 "Cancelled" downloadDone 0

  MessageBox MB_OK "WARNING: download from ep128.hu failed ($R0), trying enterpriseforever.com instead"

  INetC::get "https://enterpriseforever.com/letoltesek-downloads/egyeb-misc/?action=dlattach;attach=16928" "$INSTDIR\roms\ep128emu_roms-2.0.11.bin"
  Pop $R0
  StrCmp $R0 "OK" downloadDone 0
  StrCmp $R0 "Cancelled" downloadDone 0

  MessageBox MB_OK "Download failed: $R0"

  downloadDone:

SectionEnd

Section "Install configuration files" SecInstCfg

  SectionIn RO

  SetOutPath "$INSTDIR"
  ExecWait '"$INSTDIR\makecfg.exe" "-c" "$INSTDIR"'

SectionEnd

Section "Install epimgconv and other utilities" SecUtils

  SetOutPath "$INSTDIR"

  File /nonfatal "..\dtf.exe"
  File /nonfatal "..\epcompress.exe"
  File /nonfatal "..\epimgconv.exe"
  File /nonfatal "..\epimgconv_gui.exe"
  File /nonfatal "..\iview2png.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    SetShellVarContext all

    ;Create shortcuts
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Image converter utility.lnk" "$INSTDIR\epimgconv_gui.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecMain ${LANG_ENGLISH} "ep128emu binaries"
  LangString DESC_SecSrc ${LANG_ENGLISH} "ep128emu source code"
  LangString DESC_SecAssocFiles ${LANG_ENGLISH} "Associate snapshot and demo files with ep128emu"
  LangString DESC_SecAssocZX ${LANG_ENGLISH} "Associate Spectrum .TZX and .Z80 files with ep128emu"
  LangString DESC_SecAssocCPC ${LANG_ENGLISH} "Associate CPC .CDT and .SNA files with ep128emu"
  LangString DESC_SecAssocDiskCPC ${LANG_ENGLISH} "Associate CPC .DSK files with ep128emu"
  LangString DESC_SecAssocDiskTVC ${LANG_ENGLISH} "Associate TVC .DSK files with ep128emu"
  LangString DESC_SecDLRoms ${LANG_ENGLISH} "Download and install ROM images"
  LangString DESC_SecInstCfg ${LANG_ENGLISH} "Install configuration files"
  LangString DESC_SecUtils ${LANG_ENGLISH} "Install epimgconv and other utilities"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSrc} $(DESC_SecSrc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssocFiles} $(DESC_SecAssocFiles)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssocZX} $(DESC_SecAssocZX)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssocCPC} $(DESC_SecAssocCPC)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssocDiskCPC} $(DESC_SecAssocDiskCPC)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssocDiskTVC} $(DESC_SecAssocDiskTVC)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDLRoms} $(DESC_SecDLRoms)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecInstCfg} $(DESC_SecInstCfg)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecUtils} $(DESC_SecUtils)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  SetShellVarContext all

  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\LICENSE.FLTK"
  Delete "$INSTDIR\LICENSE.Lua"
  Delete "$INSTDIR\LICENSE.PortAudio"
  Delete "$INSTDIR\LICENSE.SDL"
  Delete "$INSTDIR\LICENSE.dotconf"
  Delete "$INSTDIR\LICENSE.libsndfile"
  Delete "$INSTDIR\news.txt"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\dtf.exe"
  Delete "$INSTDIR\ep128emu.exe"
  Delete "$INSTDIR\epcompress.exe"
  Delete "$INSTDIR\epimgconv.exe"
  Delete "$INSTDIR\epimgconv_gui.exe"
  Delete "$INSTDIR\iview2png.exe"
  Delete "$INSTDIR\libgcc_s_dw2-1.dll"
  Delete "$INSTDIR\libgcc_s_seh-1.dll"
  Delete "$INSTDIR\libsndfile-1.dll"
  Delete "$INSTDIR\libstdc++-6.dll"
  Delete "$INSTDIR\lua51.dll"
  Delete "$INSTDIR\lua53.dll"
  Delete "$INSTDIR\makecfg.exe"
  Delete "$INSTDIR\portaudio_x64.dll"
  Delete "$INSTDIR\portaudio_x86.dll"
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
  Delete "$INSTDIR\config\cpc\CPC_128k.cfg"
  Delete "$INSTDIR\config\cpc\CPC_128k_AMSDOS.cfg"
  Delete "$INSTDIR\config\cpc\CPC_576k.cfg"
  Delete "$INSTDIR\config\cpc\CPC_576k_AMSDOS.cfg"
  Delete "$INSTDIR\config\cpc\CPC_64k.cfg"
  Delete "$INSTDIR\config\cpc\CPC_64k_AMSDOS.cfg"
  Delete "$INSTDIR\config\ep128brd\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128brd\EP2048k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128brd\EP2048k_EXOS24_EXDOS_utils.cfg"
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
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS231_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS232_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS232_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128brd\EP_640k_EXOS24_SDEXT_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP2048k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP2048k_EXOS24_EXDOS_utils.cfg"
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
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS231_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS232_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS232_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128esp\EP_640k_EXOS24_SDEXT_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP2048k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP2048k_EXOS24_EXDOS_utils.cfg"
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
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS231_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS232_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS232_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128hun\EP_640k_EXOS24_SDEXT_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP2048k_EXOS231_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP2048k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP2048k_EXOS24_EXDOS_utils.cfg"
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
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS231_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS232_EXDOS.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS232_EXDOS_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS232_IDE_utils.cfg"
  Delete "$INSTDIR\config\ep128uk\EP_640k_EXOS24_SDEXT_utils.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS_FileIO.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS_NoCartridge.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_EXDOS_TASMON.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape_FileIO.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape_NoCartridge.cfg"
  Delete "$INSTDIR\config\ep64\EP_64k_Tape_TASMON.cfg"
  Delete "$INSTDIR\config\tvc\TVC_32k_V12.cfg"
  Delete "$INSTDIR\config\tvc\TVC_32k_V22.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k_V12.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k_V22.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k_V22_FileIO.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k_V22_SDEXT.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V12.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V12_FileIO.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V12_SDEXT.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V12_VTDOS.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V22.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V22_FileIO.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V22_SDEXT.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V22_VTDOS.cfg"
  Delete "$INSTDIR\config\tvc\TVC_64k+_V22_VTDOS_CART.cfg"
  Delete "$INSTDIR\config\zx\ZX_128k.cfg"
  Delete "$INSTDIR\config\zx\ZX_128k_FileIO.cfg"
  Delete "$INSTDIR\config\zx\ZX_16k.cfg"
  Delete "$INSTDIR\config\zx\ZX_16k_FileIO.cfg"
  Delete "$INSTDIR\config\zx\ZX_48k.cfg"
  Delete "$INSTDIR\config\zx\ZX_48k_FileIO.cfg"
  Delete "$INSTDIR\config\zx\ZX_48k_GW03.cfg"
  RMDir "$INSTDIR\config\cpc"
  RMDir "$INSTDIR\config\ep128brd"
  RMDir "$INSTDIR\config\ep128esp"
  RMDir "$INSTDIR\config\ep128hun"
  RMDir "$INSTDIR\config\ep128uk"
  RMDir "$INSTDIR\config\ep64"
  RMDir "$INSTDIR\config\tvc"
  RMDir "$INSTDIR\config\zx"
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR\demo"
  Delete "$INSTDIR\disk\disk.zip"
  Delete "$INSTDIR\disk\ide126m.vhd.bz2"
  Delete "$INSTDIR\disk\ide189m.vhd.bz2"
  RMDir "$INSTDIR\disk"
  RMDir "$INSTDIR\files"
  RMDir "$INSTDIR\progs"
  Delete "$INSTDIR\roms\asmen15.rom"
  Delete "$INSTDIR\roms\asmon15.rom"
  Delete "$INSTDIR\roms\basic20.rom"
  Delete "$INSTDIR\roms\basic21.rom"
  Delete "$INSTDIR\roms\brd.rom"
  Delete "$INSTDIR\roms\cpc464.rom"
  Delete "$INSTDIR\roms\cpc6128.rom"
  Delete "$INSTDIR\roms\cpc664.rom"
  Delete "$INSTDIR\roms\cpc_amsdos.rom"
  Delete "$INSTDIR\roms\cyrus.rom"
  Delete "$INSTDIR\roms\edcw.rom"
  Delete "$INSTDIR\roms\ep-plus.rom"
  Delete "$INSTDIR\roms\ep128emu_roms.bin"
  Delete "$INSTDIR\roms\ep128emu_roms-2.0.10.bin"
  Delete "$INSTDIR\roms\ep128emu_roms-2.0.11.bin"
  Delete "$INSTDIR\roms\ep_basic.rom"
  Delete "$INSTDIR\roms\epd17l12.rom"
  Delete "$INSTDIR\roms\epd17z12.rom"
  Delete "$INSTDIR\roms\epdos_z.rom"
  Delete "$INSTDIR\roms\epd19hft.rom"
  Delete "$INSTDIR\roms\epd19uk.rom"
  Delete "$INSTDIR\roms\epfileio.rom"
  Delete "$INSTDIR\roms\esb.rom"
  Delete "$INSTDIR\roms\esp.rom"
  Delete "$INSTDIR\roms\exdos0.rom"
  Delete "$INSTDIR\roms\exdos1.rom"
  Delete "$INSTDIR\roms\exdos10.rom"
  Delete "$INSTDIR\roms\exdos13.rom"
  Delete "$INSTDIR\roms\exdos13isdos10esp.rom"
  Delete "$INSTDIR\roms\exdos13isdos10hun.rom"
  Delete "$INSTDIR\roms\exdos14isdos10uk.rom"
  Delete "$INSTDIR\roms\exdos14isdos10uk-brd.rom"
  Delete "$INSTDIR\roms\exdos14isdos10uk-esp.rom"
  Delete "$INSTDIR\roms\exdos14isdos10uk-hfont.rom"
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
  Delete "$INSTDIR\roms\exos232.rom"
  Delete "$INSTDIR\roms\exos232esp.rom"
  Delete "$INSTDIR\roms\exos232hun.rom"
  Delete "$INSTDIR\roms\exos232uk.rom"
  Delete "$INSTDIR\roms\exos24brd.rom"
  Delete "$INSTDIR\roms\exos24es.rom"
  Delete "$INSTDIR\roms\exos24hu.rom"
  Delete "$INSTDIR\roms\exos24uk.rom"
  Delete "$INSTDIR\roms\fenas12.rom"
  Delete "$INSTDIR\roms\forth.rom"
  Delete "$INSTDIR\roms\genmon.rom"
  Delete "$INSTDIR\roms\heass10.rom"
  Delete "$INSTDIR\roms\heass10hfont.rom"
  Delete "$INSTDIR\roms\heass10uk.rom"
  Delete "$INSTDIR\roms\heassekn.rom"
  Delete "$INSTDIR\roms\hun.rom"
  Delete "$INSTDIR\roms\ide.rom"
  Delete "$INSTDIR\roms\ide12.rom"
  Delete "$INSTDIR\roms\iview.rom"
  Delete "$INSTDIR\roms\lisp.rom"
  Delete "$INSTDIR\roms\pascal11.rom"
  Delete "$INSTDIR\roms\pascal12.rom"
  Delete "$INSTDIR\roms\pasians.rom"
  Delete "$INSTDIR\roms\paszians.rom"
  Delete "$INSTDIR\roms\sdext05.rom"
  Delete "$INSTDIR\roms\tasmon0.rom"
  Delete "$INSTDIR\roms\tasmon1.rom"
  Delete "$INSTDIR\roms\tasmon15.rom"
  Delete "$INSTDIR\roms\tpt.rom"
  Delete "$INSTDIR\roms\tvc12_ext.rom"
  Delete "$INSTDIR\roms\tvc12_sys.rom"
  Delete "$INSTDIR\roms\tvc21_ext.rom"
  Delete "$INSTDIR\roms\tvc21_sys.rom"
  Delete "$INSTDIR\roms\tvc22_ext.rom"
  Delete "$INSTDIR\roms\tvc22_sys.rom"
  Delete "$INSTDIR\roms\tvc_dos11c.rom"
  Delete "$INSTDIR\roms\tvc_dos11d.rom"
  Delete "$INSTDIR\roms\tvc_dos12c.rom"
  Delete "$INSTDIR\roms\tvc_dos12d.rom"
  Delete "$INSTDIR\roms\tvcfileio.rom"
  Delete "$INSTDIR\roms\tvc_sdext.rom"
  Delete "$INSTDIR\roms\tvcupm_c.rom"
  Delete "$INSTDIR\roms\tvcupm_d.rom"
  Delete "$INSTDIR\roms\zt18.rom"
  Delete "$INSTDIR\roms\zt18ekn.rom"
  Delete "$INSTDIR\roms\zt18hfnt.rom"
  Delete "$INSTDIR\roms\zt18hun.rom"
  Delete "$INSTDIR\roms\zt18uk.rom"
  Delete "$INSTDIR\roms\zt19ekn.rom"
  Delete "$INSTDIR\roms\zt19hfnt.rom"
  Delete "$INSTDIR\roms\zt19hun.rom"
  Delete "$INSTDIR\roms\zt19uk.rom"
  Delete "$INSTDIR\roms\zx128.rom"
  Delete "$INSTDIR\roms\zx41.rom"
  Delete "$INSTDIR\roms\zx41uk.rom"
  Delete "$INSTDIR\roms\zx48.rom"
  Delete "$INSTDIR\roms\zx48gw03.rom"
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
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - GL - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - GL - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - Software - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - Software - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - Software - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator\zx128emu - Software - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - GL - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - GL - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - Software - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - Software - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - Software - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc464emu - Software - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - GL - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - GL - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - Software - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - Software - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - Software - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\CPC emulator\cpc6128emu - Software - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - GL - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - GL - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - Software - default theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - Software - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - Software - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\TVC emulator\tvc64emu - Software - Gtk+ theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\ep128emu - GL - plastic theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Image converter utility.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\README.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Tape editor.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Reinstall configuration files.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"

  RMDir "$SMPROGRAMS\$MUI_TEMP\Spectrum emulator"
  RMDir "$SMPROGRAMS\$MUI_TEMP\CPC emulator"
  RMDir "$SMPROGRAMS\$MUI_TEMP\TVC emulator"

  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP\GUI themes"

  startMenuDeleteLoop:
    ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

    IfErrors startMenuDeleteLoopDone

    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKLM "Software\ep128emu2\InstallDirectory"
  DeleteRegKey /ifempty HKLM "Software\ep128emu2"

  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

SectionEnd

