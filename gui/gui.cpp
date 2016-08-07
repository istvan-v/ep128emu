
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2016 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "gui.hpp"
#include "guicolor.hpp"
#include "ep128vm.hpp"
#include "zx128vm.hpp"

#include <typeinfo>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN   1
#  include <windows.h>
#else
#  include <unistd.h>
#  include <pthread.h>
#endif

void Ep128EmuGUI::init_()
{
  flDisplay = (Ep128Emu::FLTKDisplay_ *) 0;
  emulatorWindow = dynamic_cast<Fl_Window *>(&display);
  flDisplay = dynamic_cast<Ep128Emu::FLTKDisplay_ *>(&display);
#ifdef WIN32
  mainThreadID = uintptr_t(GetCurrentThreadId());
#else
  mainThreadID = uintptr_t(pthread_self());
#endif
  displayMode = 0;
  exitFlag = false;
  errorFlag = false;
  oldWindowWidth = -1;
  oldWindowHeight = -1;
  oldDisplayMode = 0;
  oldDemoStatus = -1;
  oldSpeedPercentage = 100;
  oldPauseFlag = true;
  oldTapeSampleRate = -1L;
  oldTapeSampleSize = -1;
  oldFloppyDriveLEDState = 0U;
  oldTapeReadOnlyFlag = false;
  oldTapePosition = -2L;
  functionKeyState = 0U;
  tapeButtonState = 0;
  oldTapeButtonState = -1;
  demoRecordFileName = "";
  demoRecordFile = (Ep128Emu::File *) 0;
  quickSnapshotFileName = "";
  updateDisplayEntered = false;
  debugWindowShowFlag = false;
  debugWindowOpenFlag = false;
  browseFileWindowShowFlag = false;
  browseFileStatus = 1;
  browseFileWindow = (Fl_Native_File_Chooser *) 0;
  windowToShow = (Fl_Window *) 0;
  diskConfigWindow = (Ep128EmuGUI_DiskConfigWindow *) 0;
  displaySettingsWindow = (Ep128EmuGUI_DisplayConfigWindow *) 0;
  keyboardConfigWindow = (Ep128EmuGUI_KbdConfigWindow *) 0;
  soundSettingsWindow = (Ep128EmuGUI_SoundConfigWindow *) 0;
  machineConfigWindow = (Ep128EmuGUI_MachineConfigWindow *) 0;
  debugWindow = (Ep128EmuGUI_DebugWindow *) 0;
  aboutWindow = (Ep128EmuGUI_AboutWindow *) 0;
  savedSpeedPercentage = 0U;
  std::string defaultDir_(".");
  snapshotDirectory = defaultDir_;
  demoDirectory = defaultDir_;
  soundFileDirectory = defaultDir_;
  configDirectory = defaultDir_;
  loadFileDirectory = defaultDir_;
  tapeImageDirectory = defaultDir_;
  diskImageDirectory = defaultDir_;
  romImageDirectory = defaultDir_;
  debuggerDirectory = defaultDir_;
  screenshotDirectory = defaultDir_;
  screenshotFileName = "";
  guiConfig.createKey("gui.snapshotDirectory", snapshotDirectory);
  guiConfig.createKey("gui.demoDirectory", demoDirectory);
  guiConfig.createKey("gui.soundFileDirectory", soundFileDirectory);
  guiConfig.createKey("gui.configDirectory", configDirectory);
  guiConfig.createKey("gui.loadFileDirectory", loadFileDirectory);
  guiConfig.createKey("gui.tapeImageDirectory", tapeImageDirectory);
  guiConfig.createKey("gui.diskImageDirectory", diskImageDirectory);
  guiConfig.createKey("gui.romImageDirectory", romImageDirectory);
  guiConfig.createKey("gui.debuggerDirectory", debuggerDirectory);
  guiConfig.createKey("gui.screenshotDirectory", screenshotDirectory);
  browseFileWindow = new Fl_Native_File_Chooser();
  Fl::add_check(&fltkCheckCallback, (void *) this);
}

void Ep128EmuGUI::updateDisplay_windowTitle()
{
  if (oldPauseFlag) {
    std::sprintf(&(windowTitleBuf[0]), "ep128emu 2.0.9.2 (paused)");
  }
  else {
    std::sprintf(&(windowTitleBuf[0]), "ep128emu 2.0.9.2 (%d%%)",
                 int(oldSpeedPercentage));
  }
  mainWindow->label(&(windowTitleBuf[0]));
}

void Ep128EmuGUI::updateDisplay_windowMode()
{
  if (((displayMode ^ oldDisplayMode) & 2) != 0) {
    if ((displayMode & 2) != 0)
      mainWindow->fullscreen();
    else
      mainWindow->fullscreen_off(32, 32,
                                 config.display.width, config.display.height);
  }
  resizeWindow(config.display.width, config.display.height);
  Fl::redraw();
  Fl::flush();
  int     newWindowWidth = mainWindow->w();
  int     newWindowHeight = mainWindow->h();
  if ((displayMode & 1) == 0) {
    statusDisplayGroup->resize(newWindowWidth - 360,
                               (newWindowWidth >= 745 ?
                                0 : (newWindowHeight - 30)),
                               360, 30);
    statusDisplayGroup->show();
    mainMenuBar->resize(0, 2, 300, 26);
    mainMenuBar->show();
    if (typeid(vm) != typeid(ZX128::ZX128VM)) {
      diskStatusDisplayGroup->resize(345, 0, 30, 30);
      diskStatusDisplayGroup->show();
    }
  }
  else {
    statusDisplayGroup->hide();
    mainMenuBar->hide();
    diskStatusDisplayGroup->hide();
  }
  oldWindowWidth = -1;
  oldWindowHeight = -1;
  oldDisplayMode = displayMode;
  if ((displayMode & 1) == 0)
    emulatorWindow->cursor(FL_CURSOR_DEFAULT);
  else
    emulatorWindow->cursor(FL_CURSOR_NONE);
  mainWindow->redraw();
  mainMenuBar->redraw();
  diskStatusDisplayGroup->redraw();
  statusDisplayGroup->redraw();
}

void Ep128EmuGUI::updateDisplay_windowSize()
{
  int     newWindowWidth = mainWindow->w();
  int     newWindowHeight = mainWindow->h();
  if ((displayMode & 1) == 0) {
    int   h = newWindowHeight - (newWindowWidth >= 745 ? 30 : 60);
    emulatorWindow->resize(0, 30, newWindowWidth, h);
    if ((displayMode & 2) == 0) {
      config.display.width = newWindowWidth;
      config.display.height = h;
    }
    statusDisplayGroup->resize(newWindowWidth - 360,
                               (newWindowWidth >= 745 ?
                                0 : (newWindowHeight - 30)),
                               360, 30);
    mainMenuBar->resize(0, 2, 300, 26);
    diskStatusDisplayGroup->resize(345, 0, 30, 30);
  }
  else {
    emulatorWindow->resize(0, 0, newWindowWidth, newWindowHeight);
    if ((displayMode & 2) == 0) {
      config.display.width = newWindowWidth;
      config.display.height = newWindowHeight;
    }
  }
  oldWindowWidth = newWindowWidth;
  oldWindowHeight = newWindowHeight;
  mainWindow->redraw();
  mainMenuBar->redraw();
  diskStatusDisplayGroup->redraw();
  statusDisplayGroup->redraw();
}

void Ep128EmuGUI::updateDisplay(double t)
{
#ifdef WIN32
  uintptr_t currentThreadID = uintptr_t(GetCurrentThreadId());
#else
  uintptr_t currentThreadID = uintptr_t(pthread_self());
#endif
  if (currentThreadID != mainThreadID) {
    Ep128Emu::Timer::wait(t * 0.5);
    return;
  }
  if (browseFileWindowShowFlag | debugWindowShowFlag) {
    if (browseFileWindowShowFlag) {
      browseFileStatus = browseFileWindow->show();
      browseFileWindowShowFlag = false;
    }
    if (debugWindowShowFlag) {
      debugWindow->show();
      debugWindowShowFlag = false;
    }
  }
  if (windowToShow) {
    windowToShow->show();
    windowToShow = (Fl_Window *) 0;
  }
  if (flDisplay->haveFramesPending())
    t = t * 0.5;
  if (updateDisplayEntered) {
    // if re-entering this function:
    Fl::wait(t);
    return;
  }
  updateDisplayEntered = true;
  Ep128Emu::VMThread::VMThreadStatus  vmThreadStatus(vmThread);
  if (vmThreadStatus.threadStatus != 0) {
    exitFlag = true;
    if (vmThreadStatus.threadStatus < 0)
      errorFlag = true;
  }
  if (displayMode != oldDisplayMode) {
    updateDisplay_windowMode();
  }
  else if (mainWindow->w() != oldWindowWidth ||
           mainWindow->h() != oldWindowHeight) {
    updateDisplay_windowSize();
  }
  if (vmThreadStatus.isPaused != oldPauseFlag) {
    oldPauseFlag = vmThreadStatus.isPaused;
    updateDisplay_windowTitle();
  }
  int   newDemoStatus = (vmThreadStatus.isRecordingDemo ?
                         2 : (vmThreadStatus.isPlayingDemo ? 1 : 0));
  if (newDemoStatus != oldDemoStatus) {
    Fl_Menu_Item& m = getMenuItem(0);   // "File/Stop demo (Alt+K)"
    if (newDemoStatus == 0) {
      if (oldDemoStatus == 2)
        closeDemoFile(false);
      demoStatusDisplay1->hide();
      demoStatusDisplay2->hide();
      m.deactivate();
    }
    else {
      demoStatusDisplay1->show();
      demoStatusDisplay2->show();
      if (newDemoStatus == 1) {
        demoStatusDisplay2->labelcolor(Fl_Color(6));
        demoStatusDisplay2->label("P");
      }
      else {
        demoStatusDisplay2->labelcolor(Fl_Color(1));
        demoStatusDisplay2->label("R");
      }
      m.activate();
    }
    oldDemoStatus = newDemoStatus;
    mainWindow->redraw();
  }
  if (vmThreadStatus.tapeSampleRate != oldTapeSampleRate ||
      vmThreadStatus.tapeSampleSize != oldTapeSampleSize ||
      vmThreadStatus.tapeReadOnly != oldTapeReadOnlyFlag) {
    oldTapeSampleRate = vmThreadStatus.tapeSampleRate;
    oldTapeSampleSize = vmThreadStatus.tapeSampleSize;
    oldTapeReadOnlyFlag = vmThreadStatus.tapeReadOnly;
    if (vmThreadStatus.tapeSampleRate < 1L || vmThreadStatus.tapeSampleSize < 1)
      tapeInfoDisplay->label("Tape: none");
    else {
      char  tmpBuf[256];
      std::sprintf(&(tmpBuf[0]), "Tape: %ldHz %dbit %s",
                   vmThreadStatus.tapeSampleRate, vmThreadStatus.tapeSampleSize,
                   (vmThreadStatus.tapeReadOnly ? "RO" : "RW"));
      tapeInfoDisplay->copy_label(&(tmpBuf[0]));
    }
    mainWindow->redraw();
  }
  if (tapeButtonState != oldTapeButtonState) {
    oldTapeButtonState = tapeButtonState;
    switch (tapeButtonState) {
    case 1:
      tapeStatusDisplay->labelcolor(Fl_Color(6));
      tapeStatusDisplay->label("P");
      break;
    case 2:
      tapeStatusDisplay->labelcolor(Fl_Color(1));
      tapeStatusDisplay->label("R");
      break;
    default:
      tapeStatusDisplay->labelcolor(Fl_Color(6));
      tapeStatusDisplay->label("");
      break;
    }
    tapePositionDisplay->redraw();
    tapeStatusDisplay->redraw();
  }
  long  newTapePosition = long(vmThreadStatus.tapePosition * 10.0 + 0.25);
  newTapePosition = (newTapePosition >= 0L ? newTapePosition : -1L);
  if (newTapePosition != oldTapePosition) {
    oldTapePosition = newTapePosition;
    if (newTapePosition >= 0L) {
      char  tmpBuf[256];
      int   h, m, s, ds;
      ds = int(newTapePosition % 10L);
      s = int((newTapePosition / 10L) % 60L);
      m = int((newTapePosition / 600L) % 60L);
      h = int((newTapePosition / 36000L) % 100L);
      std::sprintf(&(tmpBuf[0]), "%2d:%02d:%02d.%d ", h, m, s, ds);
      tapePositionDisplay->copy_label(&(tmpBuf[0]));
    }
    else
      tapePositionDisplay->label("-:--:--.- ");
    tapePositionDisplay->redraw();
    tapeStatusDisplay->redraw();
  }
  uint32_t  newFloppyDriveLEDState = vmThreadStatus.floppyDriveLEDState;
  if (newFloppyDriveLEDState != oldFloppyDriveLEDState) {
    oldFloppyDriveLEDState = newFloppyDriveLEDState;
    static const Fl_Color ledColors_[16] = {
      FL_BLACK,      Fl_Color(92),  FL_GREEN,      Fl_Color(87),
      Fl_Color(128), Fl_Color(92),  FL_GREEN,      Fl_Color(87),
      FL_BLACK,      Fl_Color(92),  FL_GREEN,      Fl_Color(87),
      Fl_Color(128), Fl_Color(128), Fl_Color(128), Fl_Color(128)
    };
    driveALEDDisplay->color(ledColors_[newFloppyDriveLEDState & 0x0FU]);
    driveALEDDisplay->redraw();
    driveBLEDDisplay->color(ledColors_[(newFloppyDriveLEDState >> 8) & 0x0FU]);
    driveBLEDDisplay->redraw();
    driveCLEDDisplay->color(ledColors_[(newFloppyDriveLEDState >> 16) & 0x0FU]);
    driveCLEDDisplay->redraw();
    driveDLEDDisplay->color(ledColors_[(newFloppyDriveLEDState >> 24) & 0x0FU]);
    driveDLEDDisplay->redraw();
  }
  if (statsTimer.getRealTime() >= 0.5) {
    statsTimer.reset();
    int32_t newSpeedPercentage = int32_t(vmThreadStatus.speedPercentage + 0.5f);
    if (newSpeedPercentage != oldSpeedPercentage) {
      oldSpeedPercentage = newSpeedPercentage;
      updateDisplay_windowTitle();
    }
  }
  Fl::wait(t);
  updateDisplayEntered = false;
}

void Ep128EmuGUI::errorMessage(const char *msg)
{
  Fl::lock();
  while (errorMessageWindow->shown()) {
    Fl::unlock();
    updateDisplay();
    Fl::lock();
  }
  if (msg)
    errorMessageText->copy_label(msg);
  else
    errorMessageText->label("");
  errorMessageWindow->set_modal();
  windowToShow = errorMessageWindow;
  while (true) {
    Fl::unlock();
    try {
      updateDisplay();
    }
    catch (...) {
    }
    Fl::lock();
    if (windowToShow != errorMessageWindow && !errorMessageWindow->shown()) {
      errorMessageText->label("");
      Fl::unlock();
      break;
    }
  }
}

Fl_Menu_Item& Ep128EmuGUI::getMenuItem(int n)
{
  const char  *s = (char *) 0;
  switch (n) {
  case 0:
    s = "File/Stop demo (Alt+K)";
    break;
  case 1:
    s = "File/Record audio/Stop";
    break;
  case 2:
    s = "File/Record video/Stop";
    break;
  case 3:
    s = "Machine/Speed/No limit (Alt+W)";
    break;
  case 4:
    s = "Options/Process priority/Idle";
    break;
  case 5:
    s = "Options/Process priority/Below normal";
    break;
  case 6:
    s = "Options/Process priority/Normal";
    break;
  case 7:
    s = "Options/Process priority/Above normal";
    break;
  case 8:
    s = "Options/Process priority/High";
    break;
  default:
    throw Ep128Emu::Exception("internal error: invalid menu item number");
  }
  return *(const_cast<Fl_Menu_Item *>(mainMenuBar->find_item(s)));
}

int Ep128EmuGUI::getMenuItemIndex(int n)
{
  const Fl_Menu_Item  *tmp = &(getMenuItem(n));
  return int(tmp - mainMenuBar->menu());
}

void Ep128EmuGUI::createMenus()
{
  Ep128Emu::setWindowIcon(mainWindow,
                          (typeid(vm) == typeid(Ep128::Ep128VM) ?
                           0 : (typeid(vm) == typeid(ZX128::ZX128VM) ? 1 : 2)));
  Ep128Emu::setWindowIcon(diskConfigWindow->window, 1);
  Ep128Emu::setWindowIcon(displaySettingsWindow->window, 2);
  Ep128Emu::setWindowIcon(keyboardConfigWindow->window, 3);
  Ep128Emu::setWindowIcon(machineConfigWindow->window, 3);
  Ep128Emu::setWindowIcon(aboutWindow->window, 10);
  Ep128Emu::setWindowIcon(errorMessageWindow, 12);
  mainMenuBar->add("File/Configuration/Load from ASCII file (Alt+Q)",
                   (char *) 0, &menuCallback_File_LoadConfig, (void *) this);
  mainMenuBar->add("File/Configuration/Load from binary file (Alt+L)",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Configuration/Save as ASCII file",
                   (char *) 0, &menuCallback_File_SaveConfig, (void *) this);
  mainMenuBar->add("File/Configuration/Save",
                   (char *) 0, &menuCallback_File_SaveMainCfg, (void *) this);
  mainMenuBar->add("File/Configuration/Revert",
                   (char *) 0, &menuCallback_File_RevertCfg, (void *) this);
  mainMenuBar->add("File/Save snapshot (Alt+S)",
                   (char *) 0, &menuCallback_File_SaveSnapshot, (void *) this);
  mainMenuBar->add("File/Load snapshot (Alt+L)",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Set file name",
                   (char *) 0, &menuCallback_File_QSFileName, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Save (Ctrl+F9)",
                   (char *) 0, &menuCallback_File_QSSave, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Load (Ctrl+F10)",
                   (char *) 0, &menuCallback_File_QSLoad, (void *) this);
  mainMenuBar->add("File/Record demo",
                   (char *) 0, &menuCallback_File_RecordDemo, (void *) this);
  mainMenuBar->add("File/Stop demo (Alt+K)",
                   (char *) 0, &menuCallback_File_StopDemo, (void *) this);
  mainMenuBar->add("File/Load demo (Alt+L)",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Record audio/Start...",
                   (char *) 0, &menuCallback_File_RecordSound, (void *) this);
  mainMenuBar->add("File/Record audio/Stop",
                   (char *) 0, &menuCallback_File_StopSndRecord, (void *) this);
  mainMenuBar->add("File/Record video/Start...",
                   (char *) 0, &menuCallback_File_RecordVideo, (void *) this);
  mainMenuBar->add("File/Record video/Stop",
                   (char *) 0, &menuCallback_File_StopAVIRecord, (void *) this);
  mainMenuBar->add("File/Save screenshot (F12)",
                   (char *) 0, &menuCallback_File_Screenshot, (void *) this);
  mainMenuBar->add("File/Quit (Shift+F12)",
                   (char *) 0, &menuCallback_File_Quit, (void *) this);
  mainMenuBar->add("Machine/Speed/No limit (Alt+W)",
                   (char *) 0, &menuCallback_Machine_FullSpeed, (void *) this);
  mainMenuBar->add("Machine/Speed/10%",
                   (char *) 0, &menuCallback_Machine_Speed_10, (void *) this);
  mainMenuBar->add("Machine/Speed/25%",
                   (char *) 0, &menuCallback_Machine_Speed_25, (void *) this);
  mainMenuBar->add("Machine/Speed/50%",
                   (char *) 0, &menuCallback_Machine_Speed_50, (void *) this);
  mainMenuBar->add("Machine/Speed/100% (Alt+E)",
                   (char *) 0, &menuCallback_Machine_Speed_100, (void *) this);
  mainMenuBar->add("Machine/Speed/200%",
                   (char *) 0, &menuCallback_Machine_Speed_200, (void *) this);
  mainMenuBar->add("Machine/Speed/400%",
                   (char *) 0, &menuCallback_Machine_Speed_400, (void *) this);
  mainMenuBar->add("Machine/Tape/Select image file (Alt+T)",
                   (char *) 0, &menuCallback_Machine_OpenTape, (void *) this);
  mainMenuBar->add("Machine/Tape/Play (Alt+P)",
                   (char *) 0, &menuCallback_Machine_TapePlay, (void *) this);
  mainMenuBar->add("Machine/Tape/Stop (Alt+O)",
                   (char *) 0, &menuCallback_Machine_TapeStop, (void *) this);
  mainMenuBar->add("Machine/Tape/Record",
                   (char *) 0, &menuCallback_Machine_TapeRecord, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To beginning of tape (Alt+R)",
                   (char *) 0, &menuCallback_Machine_TapeRewind, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To previous marker (Alt+,)",
                   (char *) 0, &menuCallback_Machine_TapePrvCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 10 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd10s, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 60 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd60s, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/To next marker (Alt+.)",
                   (char *) 0, &menuCallback_Machine_TapeNxtCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/By 10 seconds",
                   (char *) 0, &menuCallback_Machine_TapeFwd10s, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/By 60 seconds",
                   (char *) 0, &menuCallback_Machine_TapeFwd60s, (void *) this);
  mainMenuBar->add("Machine/Tape/Marker/Add",
                   (char *) 0, &menuCallback_Machine_AddCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Marker/Delete nearest",
                   (char *) 0, &menuCallback_Machine_DeleteCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Marker/Delete all",
                   (char *) 0, &menuCallback_Machine_DeleteCPs, (void *) this);
  mainMenuBar->add("Machine/Tape/Close",
                   (char *) 0, &menuCallback_Machine_TapeClose, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset (F11)",
                   (char *) 0, &menuCallback_Machine_Reset, (void *) this);
  mainMenuBar->add("Machine/Reset/Force reset (Ctrl+F11)",
                   (char *) 0, &menuCallback_Machine_ColdReset, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset clock frequencies",
                   (char *) 0, &menuCallback_Machine_ResetFreqs, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset machine configuration (Shift+F11)",
                   (char *) 0, &menuCallback_Machine_ResetAll, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Load config 1 (PageDown)",
                   (char *) 0, &menuCallback_Machine_QuickCfgL1, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Load config 2 (PageUp)",
                   (char *) 0, &menuCallback_Machine_QuickCfgL2, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Save config 1",
                   (char *) 0, &menuCallback_Machine_QuickCfgS1, (void *) this);
  mainMenuBar->add("Machine/Quick configuration/Save config 2",
                   (char *) 0, &menuCallback_Machine_QuickCfgS2, (void *) this);
  mainMenuBar->add("Machine/Toggle pause (F10)",
                   (char *) 0, &menuCallback_Machine_Pause, (void *) this);
  mainMenuBar->add("Machine/Configure... (Shift+F10)",
                   (char *) 0, &menuCallback_Machine_Configure, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 384x288",
                   (char *) 0, &menuCallback_Options_DpySize1, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 768x576",
                   (char *) 0, &menuCallback_Options_DpySize2, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 1152x864",
                   (char *) 0, &menuCallback_Options_DpySize3, (void *) this);
  mainMenuBar->add("Options/Display/Cycle display mode (F9)",
                   (char *) 0, &menuCallback_Options_DpyMode, (void *) this);
  mainMenuBar->add("Options/Display/Configure... (Shift+F9)",
                   (char *) 0, &menuCallback_Options_DpyConfig, (void *) this);
  mainMenuBar->add("Options/Sound/Increase volume",
                   (char *) 0, &menuCallback_Options_SndIncVol, (void *) this);
  mainMenuBar->add("Options/Sound/Decrease volume",
                   (char *) 0, &menuCallback_Options_SndDecVol, (void *) this);
  mainMenuBar->add("Options/Sound/Configure... (Alt+U)",
                   (char *) 0, &menuCallback_Options_SndConfig, (void *) this);
  if (typeid(vm) != typeid(ZX128::ZX128VM)) {
    mainMenuBar->add("Options/Disk/Remove floppy/Drive A", (char *) 0,
                     &menuCallback_Options_FloppyRmA, (void *) this);
    mainMenuBar->add("Options/Disk/Remove floppy/Drive B", (char *) 0,
                     &menuCallback_Options_FloppyRmB, (void *) this);
    mainMenuBar->add("Options/Disk/Remove floppy/Drive C", (char *) 0,
                     &menuCallback_Options_FloppyRmC, (void *) this);
    mainMenuBar->add("Options/Disk/Remove floppy/Drive D", (char *) 0,
                     &menuCallback_Options_FloppyRmD, (void *) this);
    mainMenuBar->add("Options/Disk/Remove floppy/All drives", (char *) 0,
                     &menuCallback_Options_FloppyRmv, (void *) this);
    mainMenuBar->add("Options/Disk/Replace floppy/Drive A (Alt+H)", (char *) 0,
                     &menuCallback_Options_FloppyRpA, (void *) this);
    mainMenuBar->add("Options/Disk/Replace floppy/Drive B", (char *) 0,
                     &menuCallback_Options_FloppyRpB, (void *) this);
    mainMenuBar->add("Options/Disk/Replace floppy/Drive C", (char *) 0,
                     &menuCallback_Options_FloppyRpC, (void *) this);
    mainMenuBar->add("Options/Disk/Replace floppy/Drive D", (char *) 0,
                     &menuCallback_Options_FloppyRpD, (void *) this);
    mainMenuBar->add("Options/Disk/Replace floppy/All drives", (char *) 0,
                     &menuCallback_Options_FloppyRpl, (void *) this);
    mainMenuBar->add("Options/Disk/Configure... (Alt+D)", (char *) 0,
                     &menuCallback_Options_FloppyCfg, (void *) this);
  }
  else {
    diskStatusDisplayGroup->hide();
  }
  mainMenuBar->add("Options/Process priority/Idle",
                   (char *) 0, &menuCallback_Options_PPriority, (void *) this);
  mainMenuBar->add("Options/Process priority/Below normal",
                   (char *) 0, &menuCallback_Options_PPriority, (void *) this);
  mainMenuBar->add("Options/Process priority/Normal",
                   (char *) 0, &menuCallback_Options_PPriority, (void *) this);
  mainMenuBar->add("Options/Process priority/Above normal",
                   (char *) 0, &menuCallback_Options_PPriority, (void *) this);
  mainMenuBar->add("Options/Process priority/High",
                   (char *) 0, &menuCallback_Options_PPriority, (void *) this);
  mainMenuBar->add("Options/Keyboard map (Alt+I)",
                   (char *) 0, &menuCallback_Options_KbdConfig, (void *) this);
  mainMenuBar->add("Options/Set working directory (Alt+F)",
                   (char *) 0, &menuCallback_Options_FileIODir, (void *) this);
  mainMenuBar->add("Debug/Start debugger (Alt+B)",
                   (char *) 0, &menuCallback_Debug_OpenDebugger, (void *) this);
  mainMenuBar->add("Help/About",
                   (char *) 0, &menuCallback_Help_About, (void *) this);
  // "File/Record video/Stop"
  getMenuItem(2).deactivate();
  // "Machine/Speed/No limit (Alt+W)"
  mainMenuBar->mode(getMenuItemIndex(3), FL_MENU_TOGGLE);
  // "Options/Process priority/Idle"
  // "Options/Process priority/Below normal"
  // "Options/Process priority/Normal"
  // "Options/Process priority/Above normal"
  // "Options/Process priority/High"
  for (int i = 4; i <= 8; i++)
    mainMenuBar->mode(getMenuItemIndex(i), FL_MENU_RADIO);
#ifndef WIN32
  // TODO: implement process priority setting on non-Windows platforms
  for (int i = 4; i <= 8; i++)
    getMenuItem(i).deactivate();
#endif
  updateMenu();
}

void Ep128EmuGUI::run()
{
  config.setErrorCallback(&errorMessageCallback, (void *) this);
  // set initial window size from saved configuration
  flDisplay->setFLTKEventCallback(&handleFLTKEvent, (void *) this);
  mainWindow->resizable((Fl_Widget *) 0);
  emulatorWindow->color(47, 47);
  resizeWindow(config.display.width, config.display.height);
  updateDisplay_windowTitle();
  // create menu bar
  createMenus();
  mainWindow->show();
  emulatorWindow->show();
  // load and apply GUI configuration
  try {
    Ep128Emu::File  f("gui_cfg.dat", true);
    try {
      guiConfig.registerChunkType(f);
      f.processAllChunks();
    }
    catch (std::exception& e) {
      errorMessage(e.what());
    }
  }
  catch (...) {
  }
  vmThread.lock(0x7FFFFFFF);
  vmThread.setUserData((void *) this);
  vmThread.setErrorCallback(&errorMessageCallback);
  vmThread.setProcessCallback(&pollJoystickInput);
  vm.setFileNameCallback(&fileNameCallback, (void *) this);
  vm.setBreakPointCallback(&Ep128EmuGUI_DebugWindow::breakPointCallback,
                           (void *) debugWindow);
  applyEmulatorConfiguration();
  vmThread.unlock();
  // run emulation
  vmThread.pause(false);
  do {
    updateDisplay();
  } while (mainWindow->shown() && !exitFlag);
  // close windows and stop emulation thread
  browseFileWindowShowFlag = false;
  windowToShow = (Fl_Window *) 0;
  if (errorMessageWindow->shown())
    errorMessageWindow->hide();
  debugWindowShowFlag = false;
  if (debugWindow->shown())
    debugWindow->hide();
  if (debugWindowOpenFlag) {
    debugWindowOpenFlag = false;
    unlockVMThread();
  }
  updateDisplay();
  Fl::unlock();
  vmThread.quit(true);
  Fl::lock();
  if (mainWindow->shown())
    mainWindow->hide();
  updateDisplay();
  if (errorFlag)
    errorMessage("exiting due to a fatal error");
  // if still recording a demo, stop now, and write file
  try {
    vm.stopDemo();
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
  closeDemoFile(false);
  // save GUI configuration
  try {
    Ep128Emu::File  f;
    guiConfig.saveState(f);
    f.writeFile("gui_cfg.dat", true);
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
}

void Ep128EmuGUI::resizeWindow(int w, int h)
{
  if ((displayMode & 2) == 0) {
    config.display.width = w;
    config.display.height = h;
    if ((displayMode & 1) == 0)
      h += (w >= 745 ? 30 : 60);
    mainWindow->resize(mainWindow->x(), mainWindow->y(), w, h);
    if ((displayMode & 1) == 0)
      mainWindow->size_range(384, 348, 1536, 1182);
    else
      mainWindow->size_range(384, 288, 1536, 1152);
  }
  if ((displayMode & 1) == 0)
    emulatorWindow->cursor(FL_CURSOR_DEFAULT);
  else
    emulatorWindow->cursor(FL_CURSOR_NONE);
}

int Ep128EmuGUI::handleFLTKEvent(void *userData, int event)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  switch (event) {
  case FL_FOCUS:
    return 1;
  case FL_UNFOCUS:
    gui_.functionKeyState = 0U;
    try {
      gui_.vmThread.resetKeyboard();
    }
    catch (std::exception& e) {
      gui_.errorMessage(e.what());
    }
    return 1;
  case FL_PUSH:
  case FL_DRAG:
    gui_.emulatorWindow->take_focus();
    return 1;
  case FL_RELEASE:
    return 1;
  case FL_KEYUP:
  case FL_KEYDOWN:
    {
      int   keyCode = Fl::event_key();
      bool  isKeyPress = (event == FL_KEYDOWN);
      if (!(keyCode >= (FL_F + 9) && keyCode <= (FL_F + 12))) {
        int   n = gui_.config.convertKeyCode(keyCode);
        if (n >= 0 && (gui_.functionKeyState == 0U || !isKeyPress)) {
          try {
#ifdef WIN32
            if (keyCode == FL_Shift_L || keyCode == FL_Shift_R) {
              // work around FLTK bug
              int   tmp = gui_.config.convertKeyCode(FL_Shift_L);
              if (tmp >= 0)
                gui_.vmThread.setKeyboardState(uint8_t(tmp),
                                               (GetKeyState(VK_LSHIFT) < 0));
              tmp = gui_.config.convertKeyCode(FL_Shift_R);
              if (tmp >= 0)
                gui_.vmThread.setKeyboardState(uint8_t(tmp),
                                               (GetKeyState(VK_RSHIFT) < 0));
            }
            else
#endif
            {
              gui_.vmThread.setKeyboardState(uint8_t(n), isKeyPress);
            }
          }
          catch (std::exception& e) {
            gui_.errorMessage(e.what());
          }
          if (gui_.functionKeyState == 0U || isKeyPress)
            return 1;
        }
      }
      int   n = -1;
      switch (keyCode) {
      case (FL_F + 9):
      case (FL_F + 10):
      case (FL_F + 11):
      case (FL_F + 12):
        n = keyCode - (FL_F + 9);
        if (Fl::event_shift())
          n += 4;
        else if (Fl::event_ctrl())
          n += 8;
        break;
      case FL_Page_Down:
        n = 12;
        break;
      case FL_Page_Up:
        n = 13;
        break;
      case FL_Alt_L:
        n = 14;
        break;
      case FL_Alt_R:
        n = 15;
        break;
      case (FL_KP + '0'):
      case (FL_KP + '1'):
      case (FL_KP + '3'):
      case (FL_KP + '4'):
        n = 16 + (keyCode - (FL_KP + '0'));
        break;
      case 0x2C:                // ','
        n = 20;
        break;
      case 0x2E:                // '.'
        n = 21;
        break;
      default:                  // 'A' to 'W'
        if (keyCode >= 0x61 && keyCode <= 0x77) {
          // 'G', 'J', 'M', 'N', and 'V' are not used
          static const unsigned char altKeyMap_[0x17] = {
            23, 24, 25, 26, 27, 28,  0, 29, 30,  0, 31, 32,
             0,  0, 33, 34, 35, 36, 37, 38, 39,  0, 40
          };
          n = int(altKeyMap_[keyCode - 0x61]) - 1;
        }
        break;
      }
      if (n >= 20) {
        if (isKeyPress && !Fl::event_alt())
          n = -1;
      }
      if (n >= 0) {
        uint32_t  bitMask = 1U << (unsigned char) (n < 12 ? (n & 3) : (n - 8));
        bool      wasPressed = bool(gui_.functionKeyState & bitMask);
        if (isKeyPress != wasPressed) {
          if (isKeyPress)
            gui_.functionKeyState |= bitMask;
          else
            gui_.functionKeyState &= (bitMask ^ uint32_t(0xFFFFFFFFU));
          if (isKeyPress) {
            switch (n) {
            case 0:                                     // F9:
              gui_.menuCallback_Options_DpyMode((Fl_Widget *) 0, userData);
              break;
            case 1:                                     // F10:
              gui_.menuCallback_Machine_Pause((Fl_Widget *) 0, userData);
              break;
            case 2:                                     // F11:
              gui_.menuCallback_Machine_Reset((Fl_Widget *) 0, userData);
              break;
            case 3:                                     // F12:
            case 24:                                    // Alt+C:
              gui_.menuCallback_File_Screenshot((Fl_Widget *) 0, userData);
              break;
            case 4:                                     // Shift+F9:
              gui_.menuCallback_Options_DpyConfig((Fl_Widget *) 0, userData);
              break;
            case 5:                                     // Shift+F10:
              gui_.menuCallback_Machine_Configure((Fl_Widget *) 0, userData);
              break;
            case 6:                                     // Shift+F11:
              gui_.menuCallback_Machine_ResetAll((Fl_Widget *) 0, userData);
              break;
            case 7:                                     // Shift+F12:
              gui_.menuCallback_File_Quit((Fl_Widget *) 0, userData);
              break;
            case 8:                                     // Ctrl+F9:
              gui_.menuCallback_File_QSSave((Fl_Widget *) 0, userData);
              break;
            case 9:                                     // Ctrl+F10:
              gui_.menuCallback_File_QSLoad((Fl_Widget *) 0, userData);
              break;
            case 10:                                    // Ctrl+F11:
              gui_.menuCallback_Machine_ColdReset((Fl_Widget *) 0, userData);
              break;
            case 12:                                    // PageDown:
              gui_.menuCallback_Machine_QuickCfgL1((Fl_Widget *) 0, userData);
              break;
            case 13:                                    // PageUp:
              gui_.menuCallback_Machine_QuickCfgL2((Fl_Widget *) 0, userData);
              break;
            case 16:                                    // KP_0:
            case 37:                                    // Alt+T:
              gui_.menuCallback_Machine_OpenTape((Fl_Widget *) 0, userData);
              break;
            case 17:                                    // KP_1:
            case 32:                                    // Alt+O:
              gui_.menuCallback_Machine_TapeStop((Fl_Widget *) 0, userData);
              break;
            case 18:                                    // KP_3:
            case 33:                                    // Alt+P:
              gui_.menuCallback_Machine_TapePlay((Fl_Widget *) 0, userData);
              break;
            case 19:                                    // KP_4:
            case 35:                                    // Alt+R:
              gui_.menuCallback_Machine_TapeRewind((Fl_Widget *) 0, userData);
              break;
            case 20:                                    // Alt+,:
              gui_.menuCallback_Machine_TapePrvCP((Fl_Widget *) 0, userData);
              break;
            case 21:                                    // Alt+.:
              gui_.menuCallback_Machine_TapeNxtCP((Fl_Widget *) 0, userData);
              break;
            case 23:                                    // Alt+B:
              gui_.menuCallback_Debug_OpenDebugger((Fl_Widget *) 0, userData);
              break;
            case 25:                                    // Alt+D:
              gui_.menuCallback_Options_FloppyCfg((Fl_Widget *) 0, userData);
              break;
            case 26:                                    // Alt+E:
              gui_.menuCallback_Machine_Speed_100((Fl_Widget *) 0, userData);
              break;
            case 27:                                    // Alt+F:
              gui_.menuCallback_Options_FileIODir((Fl_Widget *) 0, userData);
              break;
            case 28:                                    // Alt+H:
              gui_.menuCallback_Options_FloppyRpA((Fl_Widget *) 0, userData);
              break;
            case 29:                                    // Alt+I:
              gui_.menuCallback_Options_KbdConfig((Fl_Widget *) 0, userData);
              break;
            case 30:                                    // Alt+K:
              gui_.menuCallback_File_StopDemo((Fl_Widget *) 0, userData);
              break;
            case 31:                                    // Alt+L:
              gui_.menuCallback_File_LoadFile((Fl_Widget *) 0, userData);
              break;
            case 34:                                    // Alt+Q:
              gui_.menuCallback_File_LoadConfig((Fl_Widget *) 0, userData);
              break;
            case 36:                                    // Alt+S:
              gui_.menuCallback_File_SaveSnapshot((Fl_Widget *) 0, userData);
              break;
            case 38:                                    // Alt+U:
              gui_.menuCallback_Options_SndConfig((Fl_Widget *) 0, userData);
              break;
            case 39:                                    // Alt+W:
              gui_.menuCallback_Machine_FullSpeed((Fl_Widget *) 0, userData);
              break;
            }
          }
        }
      }
    }
    return 1;
  case FL_SHORTCUT:
    return 1;
  }
  return 0;
}

bool Ep128EmuGUI::lockVMThread(size_t t)
{
  if (vmThread.lock(t) != 0) {
    errorMessage("cannot lock virtual machine: "
                 "the emulation thread does not respond");
    return false;
  }
  return true;
}

void Ep128EmuGUI::unlockVMThread()
{
  vmThread.unlock();
}

bool Ep128EmuGUI::browseFile(std::string& fileName, std::string& dirName,
                             const char *pattern, int type, const char *title)
{
  bool    retval = false;
  bool    lockFlag = false;
  try {
#ifdef WIN32
    uintptr_t currentThreadID = uintptr_t(GetCurrentThreadId());
#else
    uintptr_t currentThreadID = uintptr_t(pthread_self());
#endif
    if (currentThreadID != mainThreadID) {
      Fl::lock();
      lockFlag = true;
    }
    browseFileWindow->type(type);
    browseFileWindow->title(title);
    browseFileWindow->filter(pattern);
    browseFileWindow->directory(dirName.c_str());
    if (type != Fl_Native_File_Chooser::BROWSE_DIRECTORY &&
        type != Fl_Native_File_Chooser::BROWSE_MULTI_DIRECTORY &&
        type != Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY) {
      std::string tmp;
      std::string tmp2;
      Ep128Emu::splitPath(fileName, tmp2, tmp);
#if !(defined(WIN32) || defined(__APPLE__))
      if (dirName.length() > 0) {
        tmp2 = dirName;
        if (dirName[dirName.length() - 1] != '/' &&
            dirName[dirName.length() - 1] != '\\') {
          tmp2 += '/';
        }
        tmp2 += tmp;
        tmp = tmp2;
      }
#endif
      browseFileWindow->preset_file(tmp.c_str());
    }
    else {
      browseFileWindow->preset_file((char *) 0);
    }
    fileName.clear();
    browseFileStatus = 1;
    if (currentThreadID != mainThreadID) {
      browseFileWindowShowFlag = true;
      do {
        Fl::unlock();
        lockFlag = false;
        updateDisplay();
        Fl::lock();
        lockFlag = true;
      } while (browseFileWindowShowFlag);
    }
    else {
      browseFileStatus = browseFileWindow->show();
    }
    if (browseFileStatus < 0) {
      const char  *s = browseFileWindow->errmsg();
      if (s == (char *) 0 || s[0] == '\0')
        s = "error selecting file";
      std::string tmp(s);
      if (currentThreadID != mainThreadID) {
        Fl::unlock();
        lockFlag = false;
      }
      errorMessage(tmp.c_str());
      if (currentThreadID != mainThreadID) {
        Fl::lock();
        lockFlag = true;
      }
    }
    else if (browseFileStatus == 0) {
      const char  *s = browseFileWindow->filename();
      if (s != (char *) 0) {
        fileName = s;
        Ep128Emu::stripString(fileName);
        std::string tmp;
        Ep128Emu::splitPath(fileName, dirName, tmp);
        retval = true;
      }
    }
    if (currentThreadID != mainThreadID) {
      Fl::unlock();
      lockFlag = false;
    }
  }
  catch (std::exception& e) {
    if (lockFlag)
      Fl::unlock();
    fileName.clear();
    errorMessage(e.what());
  }
  return retval;
}

void Ep128EmuGUI::applyEmulatorConfiguration(bool updateWindowFlag_)
{
  if (lockVMThread()) {
    try {
      bool    updateMenuFlag_ =
          config.soundSettingsChanged | config.vmProcessPriorityChanged;
      config.applySettings();
      vmThread.setSpeedPercentage(config.vm.speedPercentage == 100U &&
                                  config.sound.enabled ?
                                  0 : int(config.vm.speedPercentage));
      if (config.joystickSettingsChanged) {
        joystickInput.setConfiguration(config.joystick);
        config.joystickSettingsChanged = false;
      }
      if (updateWindowFlag_) {
        if (diskConfigWindow->shown())
          diskConfigWindow->updateWindow();
        if (displaySettingsWindow->shown())
          displaySettingsWindow->updateWindow();
        if (keyboardConfigWindow->shown())
          keyboardConfigWindow->updateWindow();
        if (soundSettingsWindow->shown())
          soundSettingsWindow->updateWindow();
        if (machineConfigWindow->shown())
          machineConfigWindow->updateWindow();
        if (debugWindow->shown())
          debugWindow->updateWindow();
      }
      if (updateMenuFlag_)
        updateMenu();
    }
    catch (...) {
      unlockVMThread();
      throw;
    }
    unlockVMThread();
  }
}

void Ep128EmuGUI::updateMenu()
{
  // "Machine/Speed/No limit (Alt+W)"
  if (config.vm.speedPercentage == 0U)
    getMenuItem(3).set();
  else
    getMenuItem(3).clear();
  // "File/Record audio/Stop"
  if (config.sound.file.length() > 0)
    getMenuItem(1).activate();
  else
    getMenuItem(1).deactivate();
  // "Options/Process priority"
  int     tmp = config.vm.processPriority + 6;
  tmp = (tmp > 4 ? (tmp < 8 ? tmp : 8) : 4);
  getMenuItem(tmp).setonly();
}

void Ep128EmuGUI::errorMessageCallback(void *userData, const char *msg)
{
  reinterpret_cast<Ep128EmuGUI *>(userData)->errorMessage(msg);
}

void Ep128EmuGUI::fileNameCallback(void *userData, std::string& fileName)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  try {
    std::string tmp(gui_.config.fileio.workingDirectory);
    gui_.browseFile(fileName, tmp, "All files\t*",
#ifdef WIN32
                    Fl_Native_File_Chooser::BROWSE_FILE,
#else
                    Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
#endif
                    "Open file");
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::fltkCheckCallback(void *userData)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  try {
    if (gui_.flDisplay->checkEvents())
      gui_.emulatorWindow->redraw();
  }
  catch (...) {
  }
}

size_t Ep128EmuGUI::rleCompressLine(unsigned char *outBuf,
                                    const unsigned char *inBuf, size_t width)
{
  unsigned char   rleLengths[1024];
  unsigned char   rleLength = 0;
  unsigned char   rleByte = 0x00;
  for (size_t i = width; i-- > 0; ) {
    if (inBuf[i] == rleByte) {
      if (rleLength < 255)
        rleLength++;
    }
    else {
      rleLength = 1;
      rleByte = inBuf[i];
    }
    rleLengths[i] = rleLength;
  }
  unsigned short  compressedSizes[256];
  unsigned char   literalLengths[1024];
  compressedSizes[width & 255] = 2;
  for (size_t i = width; i-- > 0; ) {
    size_t  k = rleLengths[i];
    size_t  l = 0;
    size_t  bestSize = size_t(compressedSizes[(i + k) & 255]) + 2;
    if (k < 2) {
      size_t  m = width - i;
      if (m > 255)
        m = 255;
      for (size_t j = 3; j <= m; j++) {
        size_t  tmp =
            size_t(compressedSizes[(i + j) & 255]) + ((j + 3) & (~(size_t(1))));
        if (tmp <= bestSize) {
          l = j;
          bestSize = tmp;
        }
        else if (tmp > (bestSize + 3)) {
          break;
        }
      }
    }
    compressedSizes[i & 255] = (unsigned short) bestSize;
    literalLengths[i] = (unsigned char) l;
  }
  size_t  j = 0;
  for (size_t i = 0; i < width; ) {
    unsigned char l = literalLengths[i];
    if (!l) {
      l = rleLengths[i];
      outBuf[j++] = l;
      outBuf[j++] = inBuf[i];
      i += size_t(l);
    }
    else {
      outBuf[j++] = 0x00;
      outBuf[j++] = l;
      do {
        outBuf[j++] = inBuf[i++];
      } while (--l);
      if (j & 1)
        outBuf[j++] = 0x00;
    }
  }
  outBuf[j++] = 0x00;                   // end of line
  outBuf[j++] = 0x00;
  return j;
}

void Ep128EmuGUI::screenshotCallback(void *userData,
                                     const unsigned char *buf, int w_, int h_)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  std::string   fName;
  std::FILE     *f = (std::FILE *) 0;
  try {
    fName = gui_.screenshotFileName;
    if (!gui_.browseFile(fName, gui_.screenshotDirectory, "BMP files\t*.bmp",
                         Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                         "Save screenshot"))
      return;
    Ep128Emu::addFileNameExtension(fName, "bmp");
    gui_.screenshotFileName = fName;
    f = std::fopen(fName.c_str(), "wb");
    if (!f)
      throw Ep128Emu::Exception("error opening screenshot file");
    unsigned char tmpBuf[1078];
    for (int i = 0; i < 54; i++)
      tmpBuf[i] = 0x00;
    for (int i = 0; i < 1024; i += 4) {         // palette:
      tmpBuf[i + 54] = buf[(i >> 2) * 3 + 2];   // B
      tmpBuf[i + 55] = buf[(i >> 2) * 3 + 1];   // G
      tmpBuf[i + 56] = buf[(i >> 2) * 3];       // R
      tmpBuf[i + 57] = 0x00;                    // unused
    }
    if (std::fwrite(&(tmpBuf[0]), sizeof(unsigned char), 1078, f) != 1078) {
      throw Ep128Emu::Exception("error writing screenshot file "
                                "- is the disk full ?");
    }
    size_t  compressedSize = 0;
    for (int yc = h_; yc-- > 0; ) {
      // RLE encode line
      size_t  nBytes = rleCompressLine(&(tmpBuf[0]),
                                       &(buf[(yc * w_) + 768]), size_t(w_));
      if (yc == 0) {
        tmpBuf[nBytes++] = 0x00;        // end of file
        tmpBuf[nBytes++] = 0x01;
      }
      compressedSize += nBytes;
      if (std::fwrite(&(tmpBuf[0]), sizeof(unsigned char), nBytes, f)
          != nBytes) {
        throw Ep128Emu::Exception("error writing screenshot file "
                                  "- is the disk full ?");
      }
    }
    if (std::fseek(f, 0L, SEEK_SET) < 0)
      throw Ep128Emu::Exception("error writing screenshot file header");
    for (int i = 0; i < 54; i++)
      tmpBuf[i] = 0x00;
    tmpBuf[0] = 0x42;                   // 'B'
    tmpBuf[1] = 0x4D;                   // 'M'
    tmpBuf[2] = (unsigned char) ((compressedSize + 1078) & 0xFF);
    tmpBuf[3] = (unsigned char) (((compressedSize + 1078) >> 8) & 0xFF);
    tmpBuf[4] = (unsigned char) ((compressedSize + 1078) >> 16);
    tmpBuf[10] = 0x36;                  // bitmap data offset (1078) LSB
    tmpBuf[11] = 0x04;                  // bitmap data offset (1078) MSB
    tmpBuf[14] = 0x28;                  // size of BITMAPINFOHEADER
    tmpBuf[18] = (unsigned char) (w_ & 0xFF);   // image width
    tmpBuf[19] = (unsigned char) (w_ >> 8);
    tmpBuf[22] = (unsigned char) (h_ & 0xFF);   // image height
    tmpBuf[23] = (unsigned char) (h_ >> 8);
    tmpBuf[26] = 0x01;                  // biPlanes
    tmpBuf[28] = 0x08;                  // biBitCount
    tmpBuf[30] = 0x01;                  // biCompression: BI_RLE8
    tmpBuf[34] = (unsigned char) (compressedSize & 0xFF);   // bitmap data size
    tmpBuf[35] = (unsigned char) ((compressedSize >> 8) & 0xFF);
    tmpBuf[36] = (unsigned char) (compressedSize >> 16);
    tmpBuf[38] = 0x13;                  // biXPelsPerMeter (72 dpi)
    tmpBuf[39] = 0x0B;
    tmpBuf[42] = 0x13;                  // biYPelsPerMeter (72 dpi)
    tmpBuf[43] = 0x0B;
    tmpBuf[47] = 0x01;                  // biClrUsed (256) MSB
    tmpBuf[51] = 0x01;                  // biClrImportant (256) MSB
    if (std::fwrite(&(tmpBuf[0]), sizeof(unsigned char), 54, f) != 54)
      throw Ep128Emu::Exception("error writing screenshot file header");
  }
  catch (std::exception& e) {
    if (f) {
      std::fclose(f);
      f = (std::FILE *) 0;
      if (fName.length() > 0)
        std::remove(fName.c_str());
    }
    gui_.errorMessage(e.what());
  }
  catch (...) {
    if (f) {
      std::fclose(f);
      f = (std::FILE *) 0;
      if (fName.length() > 0)
        std::remove(fName.c_str());
    }
    throw;
  }
  if (f)
    std::fclose(f);
}

void Ep128EmuGUI::pollJoystickInput(void *userData)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  while (true) {
    int     e = gui_.joystickInput.getEvent();
    if (!e)
      break;
    int     keyCode = (e > 0 ? e : (-e));
    bool    isKeyPress = (e > 0);
    // NOTE: since this function is called from the emulation thread, it
    // could be unsafe to access the configuration from here; however, the
    // emulation thread is locked while the keyboard map is updated (see
    // applyEmulatorConfiguration())
    int     n = gui_.config.convertKeyCode(keyCode);
    if (n >= 0)
      gui_.vm.setKeyboardState(uint8_t(n), isKeyPress);
  }
}

bool Ep128EmuGUI::closeDemoFile(bool stopDemo_)
{
  if (demoRecordFile) {
    if (stopDemo_) {
      if (lockVMThread()) {
        try {
          vm.stopDemo();
        }
        catch (std::exception& e) {
          unlockVMThread();
          delete demoRecordFile;
          demoRecordFile = (Ep128Emu::File *) 0;
          demoRecordFileName.clear();
          errorMessage(e.what());
          return true;
        }
        catch (...) {
          unlockVMThread();
          delete demoRecordFile;
          demoRecordFile = (Ep128Emu::File *) 0;
          demoRecordFileName.clear();
          throw;
        }
        unlockVMThread();
      }
      else
        return false;
    }
    try {
      demoRecordFile->writeFile(demoRecordFileName.c_str());
    }
    catch (std::exception& e) {
      errorMessage(e.what());
    }
    delete demoRecordFile;
    demoRecordFile = (Ep128Emu::File *) 0;
  }
  demoRecordFileName.clear();
  return true;
}

void Ep128EmuGUI::saveQuickConfig(int n)
{
  const char  *fName = (char *) 0;
  if (typeid(vm) == typeid(Ep128::Ep128VM))
    fName = (n == 1 ? "epvmcfg1.cfg" : "epvmcfg2.cfg");
  else if (typeid(vm) == typeid(ZX128::ZX128VM))
    fName = (n == 1 ? "zxvmcfg1.cfg" : "zxvmcfg2.cfg");
  else
    fName = (n == 1 ? "cpvmcfg1.cfg" : "cpvmcfg2.cfg");
  try {
    Ep128Emu::ConfigurationDB tmpCfg;
    tmpCfg.createKey("vm.cpuClockFrequency", config.vm.cpuClockFrequency);
    tmpCfg.createKey("vm.videoClockFrequency", config.vm.videoClockFrequency);
    tmpCfg.createKey("vm.soundClockFrequency", config.vm.soundClockFrequency);
    tmpCfg.createKey("vm.speedPercentage", config.vm.speedPercentage);
    tmpCfg.createKey("vm.enableMemoryTimingEmulation",
                     config.vm.enableMemoryTimingEmulation);
    tmpCfg.saveState(fName, true);
  }
  catch (std::exception& e) {
    errorMessage(e.what());
  }
}

// ----------------------------------------------------------------------------

void Ep128EmuGUI::menuCallback_File_LoadFile(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.loadFileDirectory, "All files\t*",
                        Fl_Native_File_Chooser::BROWSE_FILE,
                        "Load ep128emu binary file")) {
      if (gui_.lockVMThread()) {
        try {
          Ep128Emu::File  f(tmp.c_str());
          gui_.vm.registerChunkTypes(f);
          gui_.config.registerChunkType(f);
          f.processAllChunks();
          gui_.applyEmulatorConfiguration(true);
        }
        catch (...) {
          gui_.unlockVMThread();
          throw;
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_LoadConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.configDirectory,
                        "Configuration files\t*.cfg",
                        Fl_Native_File_Chooser::BROWSE_FILE,
                        "Load ASCII format configuration file")) {
      gui_.config.loadState(tmp.c_str());
      gui_.applyEmulatorConfiguration(true);
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_SaveConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.configDirectory,
                        "Configuration files\t*.cfg",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Save configuration as ASCII text file")) {
      Ep128Emu::addFileNameExtension(tmp, "cfg");
      gui_.config.saveState(tmp.c_str());
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_SaveMainCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    Ep128Emu::File  f;
    gui_.config.saveState(f);
    const char  *fName = "cpc_cfg.dat";
    if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
      fName = "ep128cfg.dat";
    else if (typeid(gui_.vm) == typeid(ZX128::ZX128VM))
      fName = "zx128cfg.dat";
    f.writeFile(fName, true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_RevertCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    const char  *fName = "cpc_cfg.dat";
    if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
      fName = "ep128cfg.dat";
    else if (typeid(gui_.vm) == typeid(ZX128::ZX128VM))
      fName = "zx128cfg.dat";
    Ep128Emu::File  f(fName, true);
    gui_.config.registerChunkType(f);
    f.processAllChunks();
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_QSFileName(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.snapshotDirectory, "Snapshot files\t*",
#ifdef WIN32
                        Fl_Native_File_Chooser::BROWSE_FILE,
#else
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
#endif
                        "Select quick snapshot file"))
      gui_.quickSnapshotFileName = tmp;
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_QSLoad(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        const char  *fName = gui_.quickSnapshotFileName.c_str();
        bool        useHomeDirectory = false;
        if (fName[0] == '\0') {
          if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
            fName = "qs_ep128.dat";
          else if (typeid(gui_.vm) == typeid(ZX128::ZX128VM))
            fName = "qs_zx128.dat";
          else
            fName = "qs_cpc.dat";
          useHomeDirectory = true;
        }
        Ep128Emu::File  f(fName, useHomeDirectory);
        gui_.vm.registerChunkTypes(f);
        f.processAllChunks();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_QSSave(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        const char  *fName = gui_.quickSnapshotFileName.c_str();
        bool        useHomeDirectory = false;
        if (fName[0] == '\0') {
          if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
            fName = "qs_ep128.dat";
          else if (typeid(gui_.vm) == typeid(ZX128::ZX128VM))
            fName = "qs_zx128.dat";
          else
            fName = "qs_cpc.dat";
          useHomeDirectory = true;
        }
        Ep128Emu::File  f;
        gui_.vm.saveState(f);
        f.writeFile(fName, useHomeDirectory);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_SaveSnapshot(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.snapshotDirectory, "Snapshot files\t*.ep128s",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Save snapshot")) {
      Ep128Emu::addFileNameExtension(tmp, "ep128s");
      gui_.loadFileDirectory = gui_.snapshotDirectory;
      if (gui_.lockVMThread()) {
        try {
          Ep128Emu::File  f;
          gui_.vm.saveState(f);
          f.writeFile(tmp.c_str());
        }
        catch (...) {
          gui_.unlockVMThread();
          throw;
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_RecordDemo(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.demoDirectory, "Demo files\t*.ep128d",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Record demo")) {
      Ep128Emu::addFileNameExtension(tmp, "ep128d");
      gui_.loadFileDirectory = gui_.demoDirectory;
      if (gui_.lockVMThread()) {
        if (gui_.closeDemoFile(true)) {
          try {
            gui_.demoRecordFile = new Ep128Emu::File();
            gui_.demoRecordFileName = tmp;
            gui_.vm.recordDemo(*(gui_.demoRecordFile));
          }
          catch (...) {
            gui_.unlockVMThread();
            if (gui_.demoRecordFile) {
              delete gui_.demoRecordFile;
              gui_.demoRecordFile = (Ep128Emu::File *) 0;
            }
            gui_.demoRecordFileName.clear();
            throw;
          }
        }
        gui_.unlockVMThread();
      }
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_StopDemo(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.stopDemo();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_RecordSound(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.soundFileDirectory, "Sound files\t*.wav",
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                        "Record sound output to WAV file")) {
      Ep128Emu::addFileNameExtension(tmp, "wav");
      gui_.config["sound.file"] = tmp;
      gui_.applyEmulatorConfiguration(true);
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_StopSndRecord(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["sound.file"] = std::string("");
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_RecordVideo(Fl_Widget *o, void *v)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (!gui_.browseFile(tmp, gui_.soundFileDirectory, "AVI files\t*.avi",
                         Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
                         "Record video output to AVI file")) {
      return;
    }
    if (tmp.length() < 1) {
      gui_.menuCallback_File_StopAVIRecord(o, v);
      return;
    }
    Ep128Emu::addFileNameExtension(tmp, "avi");
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.openVideoCapture(gui_.config.videoCapture.frameRate,
                                 gui_.config.videoCapture.yuvFormat,
                                 &Ep128EmuGUI::errorMessageCallback,
                                 &Ep128EmuGUI::fileNameCallback, v);
        gui_.getMenuItem(2).activate();         // "File/Record video/Stop"
        gui_.vm.setVideoCaptureFile(tmp);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.menuCallback_File_StopAVIRecord(o, v);
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_StopAVIRecord(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.closeVideoCapture();
        gui_.getMenuItem(2).deactivate();       // "File/Record video/Stop"
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_File_Screenshot(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.flDisplay->setScreenshotCallback(&screenshotCallback, v);
}

void Ep128EmuGUI::menuCallback_File_Quit(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.quit(false);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Pause(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.pause(!gui_.oldPauseFlag);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_FullSpeed(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["vm.speedPercentage"];
    if ((unsigned int) cv != 0U) {
      gui_.savedSpeedPercentage = (unsigned int) cv;
      cv = 0U;
    }
    else
      cv = gui_.savedSpeedPercentage;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Speed_10(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 10U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Speed_25(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 25U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Speed_50(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 50U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Speed_100(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 100U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Speed_200(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 200U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Speed_400(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["vm.speedPercentage"] = 400U;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_OpenTape(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.tapeImageDirectory,
                        "Tape files\t*.{tap,wav,aif,aiff,au,snd,tzx,cdt}",
#ifdef WIN32
                        Fl_Native_File_Chooser::BROWSE_FILE,
#else
                        Fl_Native_File_Chooser::BROWSE_SAVE_FILE,
#endif
                        "Select tape image file")) {
      Ep128EmuGUI::menuCallback_Machine_TapeStop(o, v);
      gui_.config["tape.imageFile"] = tmp;
      gui_.applyEmulatorConfiguration();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapePlay(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.tapePlay();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
    return;
  }
  gui_.tapeButtonState = 1;
}

void Ep128EmuGUI::menuCallback_Machine_TapeStop(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.tapeStop();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
    return;
  }
  gui_.tapeButtonState = 0;
}

void Ep128EmuGUI::menuCallback_Machine_TapeRecord(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.tapeRecord();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
    return;
  }
  gui_.tapeButtonState = 2;
}

void Ep128EmuGUI::menuCallback_Machine_TapeRewind(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.tapeSeek(0.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapePrvCP(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.tapeSeekToCuePoint(false, 30.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapeBwd10s(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 - 10.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapeBwd60s(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 - 60.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapeNxtCP(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
    gui_.vmThread.tapeSeekToCuePoint(true, 30.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapeFwd10s(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 + 10.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapeFwd60s(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.oldTapePosition >= 0L)
      gui_.vmThread.tapeSeek(double(gui_.oldTapePosition) / 10.0 + 60.0);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_AddCP(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.tapeAddCuePoint();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_DeleteCP(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.tapeDeleteNearestCuePoint();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_DeleteCPs(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.tapeDeleteAllCuePoints();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_TapeClose(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["tape.imageFile"] = std::string("");
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_Reset(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.reset(false);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_ColdReset(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.vmThread.reset(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_ResetFreqs(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config.vmConfigurationChanged = true;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_ResetAll(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        gui_.vm.stopDemo();
        gui_.config.vmConfigurationChanged = true;
        gui_.config.memoryConfigurationChanged = true;
        gui_.config.applySettings();
        gui_.vm.reset(true);
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_QuickCfgL1(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    const char  *fName = "cpvmcfg1.cfg";
    if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
      fName = "epvmcfg1.cfg";
    else if (typeid(gui_.vm) == typeid(ZX128::ZX128VM))
      fName = "zxvmcfg1.cfg";
    gui_.config.loadState(fName, true);
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_QuickCfgL2(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    const char  *fName = "cpvmcfg2.cfg";
    if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
      fName = "epvmcfg2.cfg";
    else if (typeid(gui_.vm) == typeid(ZX128::ZX128VM))
      fName = "zxvmcfg2.cfg";
    gui_.config.loadState(fName, true);
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Machine_QuickCfgS1(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.saveQuickConfig(1);
}

void Ep128EmuGUI::menuCallback_Machine_QuickCfgS2(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.saveQuickConfig(2);
}

void Ep128EmuGUI::menuCallback_Machine_Configure(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.machineConfigWindow->show();
}

void Ep128EmuGUI::menuCallback_Options_DpySize1(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.resizeWindow(384, 288);
}

void Ep128EmuGUI::menuCallback_Options_DpySize2(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.resizeWindow(768, 576);
}

void Ep128EmuGUI::menuCallback_Options_DpySize3(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.resizeWindow(1152, 864);
}

void Ep128EmuGUI::menuCallback_Options_DpyMode(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  switch (gui_.displayMode) {
  case 0:
    gui_.displayMode = 2;
    break;
  case 2:
    gui_.displayMode = 3;
    break;
  case 3:
    gui_.displayMode = 1;
    break;
  default:
    gui_.displayMode = 0;
    break;
  }
}

void Ep128EmuGUI::menuCallback_Options_DpyConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.displaySettingsWindow->show();
}

void Ep128EmuGUI::menuCallback_Options_SndIncVol(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["sound.volume"];
    cv = double(cv) * 1.2589;
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_SndDecVol(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["sound.volume"];
    cv = double(cv) * 0.7943;
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_SndConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.soundSettingsWindow->show();
}

void Ep128EmuGUI::menuCallback_Options_FloppyRmA(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["floppy.a.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRmB(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["floppy.b.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRmC(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["floppy.c.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRmD(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["floppy.d.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRmv(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["floppy.a.imageFile"] = "";
    gui_.config["floppy.b.imageFile"] = "";
    gui_.config["floppy.c.imageFile"] = "";
    gui_.config["floppy.d.imageFile"] = "";
    gui_.applyEmulatorConfiguration(true);
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRpA(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.a.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRpB(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.b.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRpC(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.c.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRpD(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
            gui_.config["floppy.d.imageFile"];
        std::string tmp = std::string(cv);
        cv = "";
        gui_.applyEmulatorConfiguration();
        cv = tmp;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyRpl(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (gui_.lockVMThread()) {
      try {
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cvA =
            gui_.config["floppy.a.imageFile"];
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cvB =
            gui_.config["floppy.b.imageFile"];
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cvC =
            gui_.config["floppy.c.imageFile"];
        Ep128Emu::ConfigurationDB::ConfigurationVariable& cvD =
            gui_.config["floppy.d.imageFile"];
        std::string tmpA = std::string(cvA);
        std::string tmpB = std::string(cvB);
        std::string tmpC = std::string(cvC);
        std::string tmpD = std::string(cvD);
        cvA = "";
        cvB = "";
        cvC = "";
        cvD = "";
        gui_.applyEmulatorConfiguration();
        cvA = tmpA;
        cvB = tmpB;
        cvC = tmpC;
        cvD = tmpD;
        gui_.applyEmulatorConfiguration();
      }
      catch (...) {
        gui_.unlockVMThread();
        throw;
      }
      gui_.unlockVMThread();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_FloppyCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  if (typeid(gui_.vm) != typeid(ZX128::ZX128VM))
    gui_.diskConfigWindow->show();
}

void Ep128EmuGUI::menuCallback_Options_PPriority(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  int     n = -2;
  for (int i = 4; i <= 8; i++) {
    if (gui_.getMenuItem(i).value() != 0)
      n = i - 6;
  }
  try {
    gui_.config["vm.processPriority"] = n;
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Options_KbdConfig(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.keyboardConfigWindow->show();
}

void Ep128EmuGUI::menuCallback_Options_FileIODir(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    Ep128Emu::ConfigurationDB::ConfigurationVariable& cv =
        gui_.config["fileio.workingDirectory"];
    std::string tmp;
    std::string tmp2 = std::string(cv);
    if (gui_.browseFile(tmp, tmp2, "*",
                        Fl_Native_File_Chooser::BROWSE_SAVE_DIRECTORY,
                        "Select working directory for the emulated machine")) {
      cv = tmp;
      gui_.applyEmulatorConfiguration();
    }
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Debug_OpenDebugger(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  if (!gui_.debugWindow->active()) {
    if (gui_.debugWindowShowFlag)
      return;
    if (!gui_.lockVMThread())
      return;
    gui_.debugWindowOpenFlag = true;
  }
  gui_.debugWindow->show();
}

void Ep128EmuGUI::menuCallback_Help_About(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.aboutWindow->show();
}

