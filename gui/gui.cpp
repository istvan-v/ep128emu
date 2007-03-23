
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2007 Istvan Varga <istvanv@users.sourceforge.net>
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

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

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
  oldPauseFlag = true;
  oldTapeSampleRate = -1L;
  oldTapeSampleSize = -1;
  oldTapeReadOnlyFlag = false;
  oldTapePosition = -2L;
  functionKeyState = 0U;
  tapeButtonState = 0;
  oldTapeButtonState = -1;
  demoRecordFileName = "";
  demoRecordFile = (Ep128Emu::File *) 0;
  quickSnapshotFileName = "";
  updateDisplayEntered = false;
  browseFileWindowShowFlag = false;
  debugWindowShowFlag = false;
  debugWindowOpenFlag = false;
  browseFileWindow = (Fl_File_Chooser *) 0;
  windowToShow = (Fl_Window *) 0;
  diskConfigWindow = (Ep128EmuGUI_DiskConfigWindow *) 0;
  displaySettingsWindow = (Ep128EmuGUI_DisplayConfigWindow *) 0;
  soundSettingsWindow = (Ep128EmuGUI_SoundConfigWindow *) 0;
  machineConfigWindow = (Ep128EmuGUI_MachineConfigWindow *) 0;
  debugWindow = (Ep128EmuGUI_DebugWindow *) 0;
  aboutWindow = (Ep128EmuGUI_AboutWindow *) 0;
  std::string defaultDir_(".");
  snapshotDirectory = defaultDir_;
  demoDirectory = defaultDir_;
  soundFileDirectory = defaultDir_;
  configDirectory = defaultDir_;
  loadFileDirectory = defaultDir_;
  tapeImageDirectory = defaultDir_;
  diskImageDirectory = defaultDir_;
  romImageDirectory = defaultDir_;
  prgFileDirectory = defaultDir_;
  debuggerDirectory = defaultDir_;
  screenshotDirectory = defaultDir_;
  guiConfig.createKey("gui.snapshotDirectory", snapshotDirectory);
  guiConfig.createKey("gui.demoDirectory", demoDirectory);
  guiConfig.createKey("gui.soundFileDirectory", soundFileDirectory);
  guiConfig.createKey("gui.configDirectory", configDirectory);
  guiConfig.createKey("gui.loadFileDirectory", loadFileDirectory);
  guiConfig.createKey("gui.tapeImageDirectory", tapeImageDirectory);
  guiConfig.createKey("gui.diskImageDirectory", diskImageDirectory);
  guiConfig.createKey("gui.romImageDirectory", romImageDirectory);
  guiConfig.createKey("gui.prgFileDirectory", prgFileDirectory);
  guiConfig.createKey("gui.debuggerDirectory", debuggerDirectory);
  guiConfig.createKey("gui.screenshotDirectory", screenshotDirectory);
  browseFileWindow = new Fl_File_Chooser("", "*", Fl_File_Chooser::SINGLE, "");
  Fl::add_check(&fltkCheckCallback, (void *) this);
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
      browseFileWindow->show();
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
  if (updateDisplayEntered) {
    // if re-entering this function:
    Fl::wait(t);
    return;
  }
  updateDisplayEntered = true;
  int     newWindowWidth = mainWindow->w();
  int     newWindowHeight = mainWindow->h();
  bool    isPaused_ = false;
  bool    isRecordingDemo_ = false;
  bool    isPlayingDemo_ = false;
  bool    tapeReadOnly_ = false;
  double  tapePosition_ = 0.0;
  double  tapeLength_ = 0.0;
  long    tapeSampleRate_ = 0L;
  int     tapeSampleSize_ = 0;
  int   vmThreadStatus = vmThread.getStatus(isPaused_, isRecordingDemo_,
                                            isPlayingDemo_, tapeReadOnly_,
                                            tapePosition_, tapeLength_,
                                            tapeSampleRate_, tapeSampleSize_);
  if (vmThreadStatus != 0) {
    exitFlag = true;
    if (vmThreadStatus < 0)
      errorFlag = true;
  }
  if (displayMode != oldDisplayMode) {
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
    newWindowWidth = mainWindow->w();
    newWindowHeight = mainWindow->h();
    if ((displayMode & 1) == 0) {
      if (newWindowWidth >= 700)
        statusDisplayGroup->resize(newWindowWidth - 360, 0, 360, 30);
      else
        statusDisplayGroup->resize(newWindowWidth - 360, newWindowHeight - 30,
                                   360, 30);
      statusDisplayGroup->show();
      mainMenuBar->resize(0, 0, 340, 30);
      mainMenuBar->show();
    }
    else {
      statusDisplayGroup->hide();
      mainMenuBar->hide();
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
    statusDisplayGroup->redraw();
  }
  else if (newWindowWidth != oldWindowWidth ||
           newWindowHeight != oldWindowHeight) {
    if ((displayMode & 1) == 0) {
      int   h = newWindowHeight - (newWindowWidth >= 700 ? 30 : 60);
      emulatorWindowGroup->resize(0, 30, newWindowWidth, h);
      emulatorWindow->resize(0, 30, newWindowWidth, h);
      if ((displayMode & 2) == 0) {
        config.display.width = newWindowWidth;
        config.display.height = h;
      }
      if (newWindowWidth >= 700)
        statusDisplayGroup->resize(newWindowWidth - 360, 0, 360, 30);
      else
        statusDisplayGroup->resize(newWindowWidth - 360, newWindowHeight - 30,
                                   360, 30);
      mainMenuBar->resize(0, 0, 340, 30);
    }
    else {
      emulatorWindowGroup->resize(0, 0, newWindowWidth, newWindowHeight);
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
    statusDisplayGroup->redraw();
  }
  if (isPaused_ != oldPauseFlag) {
    oldPauseFlag = isPaused_;
    if (isPaused_)
      mainWindow->label("ep128emu 2.0 beta (paused)");
    else
      mainWindow->label("ep128emu 2.0 beta");
  }
  int   newDemoStatus = (isRecordingDemo_ ? 2 : (isPlayingDemo_ ? 1 : 0));
  if (newDemoStatus != oldDemoStatus) {
    if (newDemoStatus == 0) {
      if (oldDemoStatus == 2)
        closeDemoFile(false);
      demoStatusDisplay1->hide();
      demoStatusDisplay2->hide();
    }
    else {
      demoStatusDisplay1->show();
      demoStatusDisplay2->show();
      if (newDemoStatus == 1) {
        demoStatusDisplay2->textcolor(247);
        demoStatusDisplay2->value("P");
      }
      else {
        demoStatusDisplay2->textcolor(FL_RED);
        demoStatusDisplay2->value("R");
      }
    }
    oldDemoStatus = newDemoStatus;
    mainWindow->redraw();
  }
  if (tapeSampleRate_ != oldTapeSampleRate ||
      tapeSampleSize_ != oldTapeSampleSize ||
      tapeReadOnly_ != oldTapeReadOnlyFlag) {
    oldTapeSampleRate = tapeSampleRate_;
    oldTapeSampleSize = tapeSampleSize_;
    oldTapeReadOnlyFlag = tapeReadOnly_;
    char  tmpBuf[256];
    if (tapeSampleRate_ < 1L || tapeSampleSize_ < 1)
      std::sprintf(&(tmpBuf[0]), "Tape: none");
    else {
      std::sprintf(&(tmpBuf[0]), "Tape: %ldHz %dbit %s",
                   tapeSampleRate_, tapeSampleSize_,
                   (tapeReadOnly_ ? "RO" : "RW"));
    }
    tapeInfoDisplay->value(&(tmpBuf[0]));
    mainWindow->redraw();
  }
  if (tapeButtonState != oldTapeButtonState) {
    oldTapeButtonState = tapeButtonState;
    switch (tapeButtonState) {
    case 1:
      tapeStatusDisplay->textcolor(247);
      tapeStatusDisplay->value(" P");
      break;
    case 2:
      tapeStatusDisplay->textcolor(FL_RED);
      tapeStatusDisplay->value(" R");
      break;
    default:
      tapeStatusDisplay->textcolor(247);
      tapeStatusDisplay->value("");
      break;
    }
    tapeStatusDisplayGroup->redraw();
  }
  long  newTapePosition = long(tapePosition_ * 10.0 + 0.25);
  newTapePosition = (newTapePosition >= 0L ? newTapePosition : -1L);
  if (newTapePosition != oldTapePosition) {
    oldTapePosition = newTapePosition;
    char  tmpBuf[256];
    if (newTapePosition >= 0L) {
      int   h, m, s, ds;
      ds = int(newTapePosition % 10L);
      s = int((newTapePosition / 10L) % 60L);
      m = int((newTapePosition / 600L) % 60L);
      h = int((newTapePosition / 36000L) % 100L);
      std::sprintf(&(tmpBuf[0]), "%2d:%02d:%02d.%d", h, m, s, ds);
    }
    else
      std::sprintf(&(tmpBuf[0]), " -:--:--.-");
    tapePositionDisplay->value(&(tmpBuf[0]));
    tapeStatusDisplayGroup->redraw();
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
    errorMessageText->label(msg);
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

void Ep128EmuGUI::run()
{
  config.setErrorCallback(&errorMessageCallback, (void *) this);
  // set initial window size from saved configuration
  emulatorWindow->color(36, 36);
  resizeWindow(config.display.width, config.display.height);
  // create menu bar
  mainMenuBar->add("File/Load file",
                   (char *) 0, &menuCallback_File_LoadFile, (void *) this);
  mainMenuBar->add("File/Load configuration file",
                   (char *) 0, &menuCallback_File_LoadConfig, (void *) this);
  mainMenuBar->add("File/Save configuration file",
                   (char *) 0, &menuCallback_File_SaveConfig, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Set file name",
                   (char *) 0, &menuCallback_File_QSFileName, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Load (Ctrl+F10)",
                   (char *) 0, &menuCallback_File_QSLoad, (void *) this);
  mainMenuBar->add("File/Quick snapshot/Save (Ctrl+F9)",
                   (char *) 0, &menuCallback_File_QSSave, (void *) this);
  mainMenuBar->add("File/Save snapshot",
                   (char *) 0, &menuCallback_File_SaveSnapshot, (void *) this);
  mainMenuBar->add("File/Record demo",
                   (char *) 0, &menuCallback_File_RecordDemo, (void *) this);
  mainMenuBar->add("File/Stop demo (Ctrl+F12)",
                   (char *) 0, &menuCallback_File_StopDemo, (void *) this);
  mainMenuBar->add("File/Record sound file",
                   (char *) 0, &menuCallback_File_RecordSound, (void *) this);
  mainMenuBar->add("File/Stop sound recording",
                   (char *) 0, &menuCallback_File_StopSndRecord, (void *) this);
  mainMenuBar->add("File/Save screenshot",
                   (char *) 0, &menuCallback_File_Screenshot, (void *) this);
  mainMenuBar->add("File/Quit",
                   (char *) 0, &menuCallback_File_Quit, (void *) this);
  mainMenuBar->add("Machine/Toggle pause (F10)",
                   (char *) 0, &menuCallback_Machine_Pause, (void *) this);
  mainMenuBar->add("Machine/Toggle full speed",
                   (char *) 0, &menuCallback_Machine_FullSpeed, (void *) this);
  mainMenuBar->add("Machine/Tape/Select image file",
                   (char *) 0, &menuCallback_Machine_OpenTape, (void *) this);
  mainMenuBar->add("Machine/Tape/Play (Shift + F9)",
                   (char *) 0, &menuCallback_Machine_TapePlay, (void *) this);
  mainMenuBar->add("Machine/Tape/Stop (Shift + F10)",
                   (char *) 0, &menuCallback_Machine_TapeStop, (void *) this);
  mainMenuBar->add("Machine/Tape/Record (Shift + F12)",
                   (char *) 0, &menuCallback_Machine_TapeRecord, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To beginning of tape",
                   (char *) 0, &menuCallback_Machine_TapeRewind, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To previous marker",
                   (char *) 0, &menuCallback_Machine_TapePrvCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 10 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd10s, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 60 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd60s, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/To next marker (F12)",
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
  mainMenuBar->add("Machine/Configure...",
                   (char *) 0, &menuCallback_Machine_Configure, (void *) this);
  mainMenuBar->add("Options/Display/Cycle display mode (F9)",
                   (char *) 0, &menuCallback_Options_DpyMode, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 384x288",
                   (char *) 0, &menuCallback_Options_DpySize1, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 768x576",
                   (char *) 0, &menuCallback_Options_DpySize2, (void *) this);
  mainMenuBar->add("Options/Display/Set size to 1152x864",
                   (char *) 0, &menuCallback_Options_DpySize3, (void *) this);
  mainMenuBar->add("Options/Display/Configure...",
                   (char *) 0, &menuCallback_Options_DpyConfig, (void *) this);
  mainMenuBar->add("Options/Sound/Increase volume",
                   (char *) 0, &menuCallback_Options_SndIncVol, (void *) this);
  mainMenuBar->add("Options/Sound/Decrease volume",
                   (char *) 0, &menuCallback_Options_SndDecVol, (void *) this);
  mainMenuBar->add("Options/Sound/Configure...",
                   (char *) 0, &menuCallback_Options_SndConfig, (void *) this);
  mainMenuBar->add("Options/Floppy/Configure...",
                   (char *) 0, &menuCallback_Options_FloppyCfg, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk A",
                   (char *) 0, &menuCallback_Options_FloppyRmA, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk B",
                   (char *) 0, &menuCallback_Options_FloppyRmB, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk C",
                   (char *) 0, &menuCallback_Options_FloppyRmC, (void *) this);
  mainMenuBar->add("Options/Floppy/Remove disk D",
                   (char *) 0, &menuCallback_Options_FloppyRmD, (void *) this);
  mainMenuBar->add("Options/Set working directory",
                   (char *) 0, &menuCallback_Options_FileIODir, (void *) this);
  mainMenuBar->add("Debug/Start debugger",
                   (char *) 0, &menuCallback_Debug_OpenDebugger, (void *) this);
  mainMenuBar->add("Help/About",
                   (char *) 0, &menuCallback_Help_About, (void *) this);
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
  vm.setFileNameCallback(&fileNameCallback, (void *) this);
  vm.setBreakPointCallback(&breakPointCallback, (void *) this);
  vmThread.unlock();
  // run emulation
  vmThread.pause(false);
  do {
    updateDisplay();
  } while (mainWindow->shown() && !exitFlag);
  // close windows and stop emulation thread
  browseFileWindowShowFlag = false;
  if (browseFileWindow->shown())
    browseFileWindow->hide();
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
      h += (w >= 700 ? 30 : 60);
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

int Ep128EmuGUI::handleFLTKEvent(int event)
{
  if (event == FL_FOCUS)
    return 1;
  if (event == FL_UNFOCUS) {
    functionKeyState = 0U;
    try {
      vmThread.resetKeyboard();
    }
    catch (std::exception& e) {
      errorMessage(e.what());
    }
    return 1;
  }
  if (event == FL_KEYUP || event == FL_KEYDOWN) {
    int   keyCode = Fl::event_key();
    bool  isKeyPress = (event == FL_KEYDOWN);
    if (!(keyCode >= (FL_F + 9) && keyCode <= (FL_F + 12))) {
      int   n = config.convertKeyCode(keyCode);
      if (n >= 0 && (functionKeyState == 0U || !isKeyPress)) {
        try {
          vmThread.setKeyboardState(uint8_t(n), isKeyPress);
        }
        catch (std::exception& e) {
          errorMessage(e.what());
        }
        if (functionKeyState == 0U || isKeyPress)
          return 1;
      }
    }
    int   n = -1;
    switch (keyCode) {
    case FL_Alt_L:
      n = 28;
      break;
    case FL_Alt_R:
      n = 29;
      break;
    case (FL_F + 5):
    case (FL_F + 6):
    case (FL_F + 7):
    case (FL_F + 8):
    case (FL_F + 9):
    case (FL_F + 10):
    case (FL_F + 11):
    case (FL_F + 12):
      n = keyCode - (FL_F + 5);
      if (Fl::event_shift())
        n += 8;
      else if (Fl::event_ctrl())
        n += 16;
      break;
    case 0x64:
      if (Fl::event_alt() || !isKeyPress)
        n = 24;
      break;
    case 0x6D:
      if (Fl::event_alt() || !isKeyPress)
        n = 25;
      break;
    case 0x77:
      if (Fl::event_alt() || !isKeyPress)
        n = 26;
      break;
    case FL_Pause:
      n = 27;
      break;
    case FL_Page_Down:
      n = 30;
      break;
    case FL_Page_Up:
      n = 31;
      break;
    }
    if (n >= 0) {
      uint32_t  bitMask = 1U << n;
      bool      wasPressed = !!(functionKeyState & bitMask);
      if (isKeyPress != wasPressed) {
        if (isKeyPress)
          functionKeyState |= bitMask;
        else
          functionKeyState &= (bitMask ^ uint32_t(0xFFFFFFFFU));
        if (isKeyPress) {
          switch (n) {
          case 0:                                       // F5:
            menuCallback_Machine_TapePlay((Fl_Widget *) 0, (void *) this);
            break;
          case 1:                                       // F6:
            menuCallback_Machine_OpenTape((Fl_Widget *) 0, (void *) this);
            break;
          case 2:                                       // F7:
            menuCallback_File_LoadFile((Fl_Widget *) 0, (void *) this);
            break;
          case 4:                                       // F9:
            menuCallback_Options_DpyMode((Fl_Widget *) 0, (void *) this);
            break;
          case 5:                                       // F10:
            menuCallback_Machine_Pause((Fl_Widget *) 0, (void *) this);
            break;
          case 6:                                       // F11:
            menuCallback_Machine_Reset((Fl_Widget *) 0, (void *) this);
            break;
          case 7:                                       // F12:
            menuCallback_Machine_TapeNxtCP((Fl_Widget *) 0, (void *) this);
            break;
          case 8:                                       // Shift + F5:
            menuCallback_Machine_TapeStop((Fl_Widget *) 0, (void *) this);
            break;
          case 9:                                       // Shift + F6:
            menuCallback_Machine_TapeRecord((Fl_Widget *) 0, (void *) this);
            break;
          case 10:                                      // Shift + F7:
            menuCallback_File_SaveSnapshot((Fl_Widget *) 0, (void *) this);
            break;
          case 12:                                      // Shift + F9:
            menuCallback_Machine_TapePlay((Fl_Widget *) 0, (void *) this);
            break;
          case 13:                                      // Shift + F10:
            menuCallback_Machine_TapeStop((Fl_Widget *) 0, (void *) this);
            break;
          case 14:                                      // Shift + F11:
            menuCallback_Machine_ResetAll((Fl_Widget *) 0, (void *) this);
            break;
          case 15:                                      // Shift + F12:
            menuCallback_Machine_TapeRecord((Fl_Widget *) 0, (void *) this);
            break;
          case 20:                                      // Ctrl + F9:
            menuCallback_File_QSSave((Fl_Widget *) 0, (void *) this);
            break;
          case 21:                                      // Ctrl + F10:
            menuCallback_File_QSLoad((Fl_Widget *) 0, (void *) this);
            break;
          case 22:                                      // Ctrl + F11:
            menuCallback_Machine_ColdReset((Fl_Widget *) 0, (void *) this);
            break;
          case 23:                                      // Ctrl + F12:
            menuCallback_File_StopDemo((Fl_Widget *) 0, (void *) this);
            break;
          case 24:                                      // Alt + D:
            menuCallback_Options_FloppyCfg((Fl_Widget *) 0, (void *) this);
            break;
          case 25:                                      // Alt + M:
            menuCallback_Debug_OpenDebugger((Fl_Widget *) 0, (void *) this);
            break;
          case 26:                                      // Alt + W:
            menuCallback_Machine_FullSpeed((Fl_Widget *) 0, (void *) this);
            break;
          case 27:                                      // Pause:
            menuCallback_Machine_Pause((Fl_Widget *) 0, (void *) this);
            break;
          case 30:                                      // PageDown:
            menuCallback_Machine_QuickCfgL1((Fl_Widget *) 0, (void *) this);
            break;
          case 31:                                      // PageUp:
            menuCallback_Machine_QuickCfgL2((Fl_Widget *) 0, (void *) this);
            break;
          }
        }
      }
    }
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
  Fl_File_Chooser *w = (Fl_File_Chooser *) 0;
  try {
    fileName.clear();
#ifdef WIN32
    uintptr_t currentThreadID = uintptr_t(GetCurrentThreadId());
#else
    uintptr_t currentThreadID = uintptr_t(pthread_self());
#endif
    if (currentThreadID != mainThreadID) {
      Fl::lock();
      w = browseFileWindow;
      while (w->shown()) {
        Fl::unlock();
        updateDisplay();
        Fl::lock();
      }
      w->directory(dirName.c_str());
      w->filter(pattern);
      w->type(type);
      w->label(title);
      browseFileWindowShowFlag = true;
      while (true) {
        bool  tmp = browseFileWindowShowFlag;
        Fl::unlock();
        if (!tmp)
          break;
        updateDisplay();
        Fl::lock();
      }
    }
    else {
      w = new Fl_File_Chooser(dirName.c_str(), pattern, type, title);
      w->show();
    }
    Fl::lock();
    while (true) {
      Fl::unlock();
      updateDisplay();
      Fl::lock();
      if (!w->shown()) {
        try {
          const char  *s = w->value();
          if (s != (char *) 0) {
            fileName = s;
            Ep128Emu::stripString(fileName);
            std::string tmp;
            Ep128Emu::splitPath(fileName, dirName, tmp);
            retval = true;
          }
        }
        catch (...) {
          Fl::unlock();
          throw;
        }
        Fl::unlock();
        break;
      }
    }
    if (currentThreadID == mainThreadID)
      delete w;
  }
  catch (std::exception& e) {
    if (w != (Fl_File_Chooser *) 0 && w != browseFileWindow) {
      if (w->shown())
        w->hide();
      delete w;
    }
    errorMessage(e.what());
  }
  return retval;
}

void Ep128EmuGUI::applyEmulatorConfiguration()
{
  if (lockVMThread()) {
    try {
      config.applySettings();
    }
    catch (...) {
      unlockVMThread();
      throw;
    }
    unlockVMThread();
  }
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
    gui_.browseFile(fileName, tmp, "All files (*)",
                    Fl_File_Chooser::CREATE, "Open file");
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

void Ep128EmuGUI::breakPointCallback(void *userData,
                                     bool isIO, bool isWrite,
                                     uint16_t addr, uint8_t value)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  Fl::lock();
  if (gui_.exitFlag || !gui_.mainWindow->shown()) {
    Fl::unlock();
    return;
  }
  gui_.debugWindow->breakPoint(isIO, isWrite, addr, value);
  if (!gui_.debugWindow->shown()) {
    gui_.debugWindowShowFlag = true;
    Fl::awake();
  }
  while (gui_.debugWindowShowFlag) {
    Fl::unlock();
    gui_.updateDisplay();
    Fl::lock();
  }
  while (true) {
    bool  tmp = gui_.debugWindow->shown();
    Fl::unlock();
    if (!tmp)
      break;
    gui_.updateDisplay();
    Fl::lock();
  }
}

void Ep128EmuGUI::screenshotCallback(void *userData,
                                     const unsigned char *buf, int w_, int h_)
{
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(userData));
  std::string   fName;
  std::FILE     *f = (std::FILE *) 0;
  try {
    if (!gui_.browseFile(fName, gui_.screenshotDirectory, "TGA files (*.tga)",
                         Fl_File_Chooser::CREATE, "Save screenshot"))
      return;
    f = std::fopen(fName.c_str(), "wb");
    if (!f)
      throw Ep128Emu::Exception("error opening screenshot file");
    unsigned char tmpBuf[1032];
    tmpBuf[0] = 0;                      // ID length
    tmpBuf[1] = 1;                      // use colormap
    tmpBuf[2] = 9;                      // image type (colormap/RLE)
    tmpBuf[3] = 0;                      // color map specification
    tmpBuf[4] = 0;                      // first entry index = 0
    tmpBuf[5] = 0;
    tmpBuf[6] = 1;                      // length = 256
    tmpBuf[7] = 24;                     // colormap entry size
    tmpBuf[8] = 0;                      // X origin
    tmpBuf[9] = 0;
    tmpBuf[10] = 0;                     // Y origin
    tmpBuf[11] = 0;
    tmpBuf[12] = (unsigned char) (w_ & 0xFF);   // image width
    tmpBuf[13] = (unsigned char) (w_ >> 8);
    tmpBuf[14] = (unsigned char) (h_ & 0xFF);   // image height
    tmpBuf[15] = (unsigned char) (h_ >> 8);
    tmpBuf[16] = 8;                     // bits per pixel
    tmpBuf[17] = 0x20;                  // image descriptor (origin: top/left)
    for (int i = 0; i < 768; i += 3) {
      tmpBuf[i + 18] = buf[i + 2];
      tmpBuf[i + 19] = buf[i + 1];
      tmpBuf[i + 20] = buf[i];
    }
    if (std::fwrite(&(tmpBuf[0]), sizeof(unsigned char), 786, f) != 786)
      throw Ep128Emu::Exception("error writing screenshot file "
                                "- is the disk full ?");
    for (int yc = 0; yc < h_; yc++) {
      const unsigned char *bufp = &(buf[yc * w_ + 768]);
      // RLE encode line
      unsigned char   *p = &(tmpBuf[0]);
      int     xc = 0;
      while (xc < w_) {
        if (xc == (w_ - 1)) {
          *(p++) = 0x00;
          *(p++) = bufp[xc];
          xc++;
        }
        else if (bufp[xc] == bufp[xc + 1]) {
          int     tmp = xc + 2;
          while (tmp < w_ && (tmp - xc) < 128) {
            if (bufp[tmp] != bufp[xc])
              break;
            tmp++;
          }
          *(p++) = (unsigned char) (((tmp - xc) - 1) | 0x80);
          *(p++) = bufp[xc];
          xc = tmp;
        }
        else {
          int     tmp = xc + 2;
          while (tmp < w_ && (tmp - xc) < 128) {
            if (bufp[tmp] == bufp[tmp - 1]) {
              tmp--;
              break;
            }
            tmp++;
          }
          *(p++) = (unsigned char) ((tmp - xc) - 1);
          while (xc < tmp) {
            *(p++) = bufp[xc];
            xc++;
          }
        }
      }
      size_t  nBytes = size_t(p - &(tmpBuf[0]));
      if (std::fwrite(&(tmpBuf[0]), sizeof(unsigned char), nBytes, f) != nBytes)
        throw Ep128Emu::Exception("error writing screenshot file "
                                  "- is the disk full ?");
    }
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
  if (typeid(vm) == typeid(Ep128::Ep128VM)) {
    if (n == 1)
      fName = "epvmcfg1.cfg";
    else
      fName = "epvmcfg2.cfg";
  }
  if (!fName)
    return;
  try {
    Ep128Emu::ConfigurationDB tmpCfg;
    tmpCfg.createKey("vm.cpuClockFrequency", config.vm.cpuClockFrequency);
    tmpCfg.createKey("vm.videoClockFrequency", config.vm.videoClockFrequency);
    tmpCfg.createKey("vm.soundClockFrequency", config.vm.soundClockFrequency);
    tmpCfg.createKey("vm.videoMemoryLatency", config.vm.videoMemoryLatency);
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
    if (gui_.browseFile(tmp, gui_.loadFileDirectory, "All files (*)",
                        Fl_File_Chooser::SINGLE, "Load ep128emu binary file")) {
      if (gui_.lockVMThread()) {
        try {
          Ep128Emu::File  f(tmp.c_str());
          gui_.vm.registerChunkTypes(f);
          gui_.config.registerChunkType(f);
          f.processAllChunks();
          gui_.applyEmulatorConfiguration();
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
                        "Configuration files (*.cfg)",
                        Fl_File_Chooser::SINGLE,
                        "Load ASCII format configuration file")) {
      gui_.config.loadState(tmp.c_str());
      gui_.applyEmulatorConfiguration();
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
                        "Configuration files (*.cfg)",
                        Fl_File_Chooser::CREATE,
                        "Save configuration as ASCII text file")) {
      gui_.config.saveState(tmp.c_str());
    }
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
    if (gui_.browseFile(tmp, gui_.snapshotDirectory, "Snapshot files (*)",
                        Fl_File_Chooser::CREATE, "Select quick snapshot file"))
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
          if (typeid(gui_.vm) == typeid(Ep128::Ep128VM)) {
            fName = "qs_ep128.dat";
            useHomeDirectory = true;
          }
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
          if (typeid(gui_.vm) == typeid(Ep128::Ep128VM)) {
            fName = "qs_ep128.dat";
            useHomeDirectory = true;
          }
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
    if (gui_.browseFile(tmp, gui_.snapshotDirectory, "Snapshot files (*)",
                        Fl_File_Chooser::CREATE, "Save snapshot")) {
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
    if (gui_.browseFile(tmp, gui_.demoDirectory, "Demo files (*)",
                        Fl_File_Chooser::CREATE, "Record demo")) {
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
    if (gui_.browseFile(tmp, gui_.soundFileDirectory, "Sound files (*.wav)",
                        Fl_File_Chooser::CREATE,
                        "Record sound output to WAV file")) {
      gui_.config["sound.file"] = tmp;
      gui_.applyEmulatorConfiguration();
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
    gui_.applyEmulatorConfiguration();
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
    gui_.config["sound.enabled"] = !gui_.config.sound.enabled;
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
    if (gui_.browseFile(tmp, gui_.tapeImageDirectory, "Tape files (*)",
                        Fl_File_Chooser::CREATE, "Select tape image file")) {
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

void Ep128EmuGUI::menuCallback_Machine_EnableSID(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  if (gui_.lockVMThread()) {
    try {
      gui_.vm.writeMemory(0xFD40, 0x00, true);
    }
    catch (...) {
    }
    gui_.unlockVMThread();
  }
}

void Ep128EmuGUI::menuCallback_Machine_QuickCfgL1(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
      gui_.config.loadState("epvmcfg1.cfg", true);
    gui_.applyEmulatorConfiguration();
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
    if (typeid(gui_.vm) == typeid(Ep128::Ep128VM))
      gui_.config.loadState("epvmcfg2.cfg", true);
    gui_.applyEmulatorConfiguration();
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
    cv = double(cv) * 1.1892;
    gui_.applyEmulatorConfiguration();
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
    cv = double(cv) * 0.8409;
    gui_.applyEmulatorConfiguration();
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

void Ep128EmuGUI::menuCallback_Options_FloppyCfg(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.diskConfigWindow->show();
}

void Ep128EmuGUI::menuCallback_Options_FloppyRmA(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    gui_.config["floppy.a.imageFile"] = "";
    gui_.applyEmulatorConfiguration();
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
    gui_.applyEmulatorConfiguration();
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
    gui_.applyEmulatorConfiguration();
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
    gui_.applyEmulatorConfiguration();
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
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
    if (gui_.browseFile(tmp, tmp2, "*", Fl_File_Chooser::DIRECTORY,
                        "Select working directory for emulated machine")) {
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
  if (!gui_.debugWindow->shown()) {
    if (!gui_.debugWindowShowFlag) {
      if (gui_.lockVMThread()) {
        gui_.debugWindowOpenFlag = true;
        gui_.debugWindow->show();
      }
    }
  }
}

void Ep128EmuGUI::menuCallback_Help_About(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  gui_.aboutWindow->show();
}

