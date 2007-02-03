
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
  flDisplay = (Ep128Emu::FLTKDisplay *) 0;
  glDisplay = (Ep128Emu::OpenGLDisplay *) 0;
  emulatorWindow = dynamic_cast<Fl_Window *>(&display);
  if (typeid(display) == typeid(Ep128Emu::FLTKDisplay))
    flDisplay = dynamic_cast<Ep128Emu::FLTKDisplay *>(&display);
  if (typeid(display) == typeid(Ep128Emu::OpenGLDisplay))
    glDisplay = dynamic_cast<Ep128Emu::OpenGLDisplay *>(&display);
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
  for (int i = 0; i < 12; i++)
    functionKeyState[i] = false;
  tapeButtonState = 0;
  oldTapeButtonState = -1;
  demoRecordFileName = "";
  demoRecordFile = (Ep128Emu::File *) 0;
  quickSnapshotFileName = "";
  updateDisplayEntered = false;
  singleThreadedMode = false;
  snapshotDirectory = "";
  demoDirectory = "";
  soundFileDirectory = "";
  configDirectory = "";
  loadFileDirectory = "";
  tapeImageDirectory = "";
  diskImageDirectory = "";
  romImageDirectory = "";
  prgFileDirectory = "";
  guiConfig.createKey("gui.singleThreadedMode", singleThreadedMode);
  guiConfig.createKey("gui.snapshotDirectory", snapshotDirectory);
  guiConfig.createKey("gui.demoDirectory", demoDirectory);
  guiConfig.createKey("gui.soundFileDirectory", soundFileDirectory);
  guiConfig.createKey("gui.configDirectory", configDirectory);
  guiConfig.createKey("gui.loadFileDirectory", loadFileDirectory);
  guiConfig.createKey("gui.tapeImageDirectory", tapeImageDirectory);
  guiConfig.createKey("gui.diskImageDirectory", diskImageDirectory);
  guiConfig.createKey("gui.romImageDirectory", romImageDirectory);
  guiConfig.createKey("gui.prgFileDirectory", prgFileDirectory);
  browseFileWindow = new Fl_File_Chooser("", "*", Fl_File_Chooser::SINGLE, "");
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
  if (updateDisplayEntered) {
    // if re-entering this function:
    try {
      if (flDisplay) {
        if (flDisplay->checkEvents())
          flDisplay->redraw();
      }
      else if (glDisplay) {
        if (glDisplay->checkEvents())
          glDisplay->redraw();
      }
      Fl::wait(t);
    }
    catch (...) {
    }
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
    newWindowWidth = mainWindow->w();
    newWindowHeight = mainWindow->h();
    oldDisplayMode = displayMode;
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
  }
  if (isPaused_ != oldPauseFlag) {
    oldPauseFlag = isPaused_;
    if (isPaused_)
      mainWindow->label("ep128emu 2.0 alpha (paused)");
    else
      mainWindow->label("ep128emu 2.0 alpha");
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
  if (flDisplay) {
    if (flDisplay->checkEvents())
      flDisplay->redraw();
  }
  else if (glDisplay) {
    if (glDisplay->checkEvents())
      glDisplay->redraw();
  }
  Fl::wait(t);
  updateDisplayEntered = false;
}

void Ep128EmuGUI::errorMessage(const char *msg)
{
  Fl::lock();
  while (errorMessageWindow->shown()) {
    Fl::unlock();
    Ep128Emu::Timer::wait(0.01);
  }
  if (msg)
    errorMessageText->label(msg);
  else
    errorMessageText->label("");
  errorMessageWindow->set_modal();
  errorMessageWindow->show();
  Fl::unlock();
  while (true) {
    try {
      updateDisplay();
    }
    catch (...) {
    }
    Fl::lock();
    if (!errorMessageWindow->shown()) {
      errorMessageText->label("");
      Fl::unlock();
      break;
    }
    Fl::unlock();
  }
}

void Ep128EmuGUI::run()
{
  config.setErrorCallback(&errorMessageCallback, (void *) this);
  vmThread.setUserData((void *) this);
  vmThread.setErrorCallback(&errorMessageCallback);
  // set initial window size from saved configuration
  resizeWindow(config.display.width, config.display.height);
  // create menu bar
  mainMenuBar->add("File/Set working directory",
                   (char *) 0, &menuCallback_File_SetFileIODir, (void *) this);
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
  mainMenuBar->add("File/Stop demo (Ctrl+F11)",
                   (char *) 0, &menuCallback_File_StopDemo, (void *) this);
  mainMenuBar->add("File/Record sound file",
                   (char *) 0, &menuCallback_File_RecordSound, (void *) this);
  mainMenuBar->add("File/Stop sound recording",
                   (char *) 0, &menuCallback_File_StopSndRecord, (void *) this);
  if (typeid(vm) == typeid(Plus4::Plus4VM)) {
    mainMenuBar->add("File/Load program",
                     (char *) 0, &menuCallback_File_LoadPRG, (void *) this);
    mainMenuBar->add("File/Save program",
                     (char *) 0, &menuCallback_File_SavePRG, (void *) this);
  }
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
  mainMenuBar->add("Machine/Tape/Record (Shift + F11)",
                   (char *) 0, &menuCallback_Machine_TapeRecord, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To beginning of tape",
                   (char *) 0, &menuCallback_Machine_TapeRewind, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/To previous marker",
                   (char *) 0, &menuCallback_Machine_TapePrvCP, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 10 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd10s, (void *) this);
  mainMenuBar->add("Machine/Tape/Rewind/By 60 seconds",
                   (char *) 0, &menuCallback_Machine_TapeBwd60s, (void *) this);
  mainMenuBar->add("Machine/Tape/Fast forward/To next marker (F11)",
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
  mainMenuBar->add("Machine/Reset/Reset (F12)",
                   (char *) 0, &menuCallback_Machine_Reset, (void *) this);
  mainMenuBar->add("Machine/Reset/Force reset (Shift+F12)",
                   (char *) 0, &menuCallback_Machine_ColdReset, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset clock frequencies",
                   (char *) 0, &menuCallback_Machine_ResetFreqs, (void *) this);
  mainMenuBar->add("Machine/Reset/Reset machine configuration (Ctrl+F12)",
                   (char *) 0, &menuCallback_Machine_ResetAll, (void *) this);
  mainMenuBar->add("Options/Display/Cycle display mode (F9)",
                   (char *) 0, &menuCallback_Options_DpyMode, (void *) this);
  mainMenuBar->add("Debug/Start debugger",
                   (char *) 0, &menuCallback_Debug_OpenDebugger, (void *) this);
  mainMenuBar->add("Help/About",
                   (char *) 0, &menuCallback_Help_About, (void *) this);
  mainWindow->show();
  emulatorWindow->show();
  emulatorWindow->cursor(FL_CURSOR_NONE);
  // load and apply GUI configuration
  {
    bool  savedSingleThreadedMode = singleThreadedMode;
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
    bool  newSingleThreadedMode = singleThreadedMode;
    singleThreadedMode = savedSingleThreadedMode;
    setSingleThreadedMode(newSingleThreadedMode);
  }
  // run emulation
  vmThread.pause(false);
  do {
    if (singleThreadedMode) {
      vmThread.process();
      updateDisplay(0.0);
    }
    else
      updateDisplay();
  } while (mainWindow->shown() && !exitFlag);
  setSingleThreadedMode(false);
  vmThread.quit(true);
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
}

int Ep128EmuGUI::handleFLTKEvent(int event)
{
  if (event == FL_FOCUS)
    return 1;
  if (event == FL_UNFOCUS) {
    for (int i = 0; i < 12; i++)
      functionKeyState[i] = false;
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
    if (keyCode >= (FL_F + 9) && keyCode <= (FL_F + 12)) {
      int   n = keyCode - (FL_F + 9);
      if (Fl::event_shift() != 0)
        n += 4;
      if (Fl::event_ctrl() != 0)
        n += 8;
      if (n >= 0 && n < 12) {
        if (functionKeyState[n] != isKeyPress) {
          functionKeyState[n] = isKeyPress;
          if (isKeyPress) {
            switch (n) {
            case 0:                                     // F9:
              menuCallback_Options_DpyMode((Fl_Widget *) 0, (void *) this);
              break;
            case 1:                                     // F10:
              menuCallback_Machine_Pause((Fl_Widget *) 0, (void *) this);
              break;
            case 2:                                     // F11:
              menuCallback_Machine_TapeNxtCP((Fl_Widget *) 0, (void *) this);
              break;
            case 3:                                     // F12:
              menuCallback_Machine_Reset((Fl_Widget *) 0, (void *) this);
              break;
            case 4:                                     // Shift + F9:
              menuCallback_Machine_TapePlay((Fl_Widget *) 0, (void *) this);
              break;
            case 5:                                     // Shift + F10:
              menuCallback_Machine_TapeStop((Fl_Widget *) 0, (void *) this);
              break;
            case 6:                                     // Shift + F11:
              menuCallback_Machine_TapeRecord((Fl_Widget *) 0, (void *) this);
              break;
            case 7:                                     // Shift + F12:
              menuCallback_Machine_ColdReset((Fl_Widget *) 0, (void *) this);
              break;
            case 8:                                     // Control + F9:
              menuCallback_File_QSSave((Fl_Widget *) 0, (void *) this);
              break;
            case 9:                                     // Control + F10:
              menuCallback_File_QSLoad((Fl_Widget *) 0, (void *) this);
              break;
            case 10:                                    // Control + F11:
              menuCallback_File_StopDemo((Fl_Widget *) 0, (void *) this);
              break;
            case 11:                                    // Control + F12:
              menuCallback_Machine_ResetAll((Fl_Widget *) 0, (void *) this);
              break;
            }
          }
        }
      }
    }
    else {
      try {
        vmThread.setKeyboardState(uint8_t(config.convertKeyCode(keyCode)),
                                  isKeyPress);
      }
      catch (std::exception& e) {
        errorMessage(e.what());
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

bool Ep128EmuGUI::setSingleThreadedMode(bool isEnabled)
{
  if (isEnabled != singleThreadedMode) {
    if (isEnabled) {
      if (lockVMThread())
        singleThreadedMode = true;
      else
        return false;
    }
    else {
      unlockVMThread();
      singleThreadedMode = false;
    }
  }
  return true;
}

bool Ep128EmuGUI::browseFile(std::string& fileName, std::string& dirName,
                             const char *pattern, int type, const char *title)
{
  bool    retval = false;
  try {
    fileName.clear();
#ifdef WIN32
    uintptr_t currentThreadID = uintptr_t(GetCurrentThreadId());
#else
    uintptr_t currentThreadID = uintptr_t(pthread_self());
#endif
    if (currentThreadID != mainThreadID) {
      Fl::lock();
      while (browseFileWindow->shown()) {
        Fl::unlock();
        Ep128Emu::Timer::wait(0.01);
      }
      browseFileWindow->directory(dirName.c_str());
      browseFileWindow->filter(pattern);
      browseFileWindow->type(type);
      browseFileWindow->label(title);
      browseFileWindow->show();
      if (type == Fl_File_Chooser::CREATE)
        browseFileWindow->value(dirName.c_str());
      Fl::unlock();
      while (true) {
        updateDisplay();
        Fl::lock();
        if (!browseFileWindow->shown()) {
          try {
            const char  *s = browseFileWindow->value();
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
        Fl::unlock();
      }
    }
    else {
      Fl_File_Chooser *w =
          new Fl_File_Chooser(dirName.c_str(), pattern, type, title);
      try {
        w->show();
        if (type == Fl_File_Chooser::CREATE)
          w->value(dirName.c_str());
        while (true) {
          updateDisplay();
          if (!w->shown()) {
            const char  *s = w->value();
            if (s != (char *) 0) {
              fileName = s;
              Ep128Emu::stripString(fileName);
              std::string tmp;
              Ep128Emu::splitPath(fileName, dirName, tmp);
              retval = true;
            }
            break;
          }
        }
      }
      catch (...) {
        if (w->shown())
          w->hide();
        delete w;
        throw;
      }
      delete w;
    }
  }
  catch (std::exception& e) {
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

// ----------------------------------------------------------------------------

void Ep128EmuGUI::menuCallback_File_SetFileIODir(Fl_Widget *o, void *v)
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
          else if (typeid(gui_.vm) == typeid(Plus4::Plus4VM)) {
            fName = "qs_plus4.dat";
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
          else if (typeid(gui_.vm) == typeid(Plus4::Plus4VM)) {
            fName = "qs_plus4.dat";
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

void Ep128EmuGUI::menuCallback_File_LoadPRG(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.prgFileDirectory, "PRG files (*.prg)",
                        Fl_File_Chooser::SINGLE, "Load program file")) {
      if (gui_.lockVMThread()) {
        try {
          if (typeid(gui_.vm) == typeid(Plus4::Plus4VM)) {
            Plus4::Plus4VM  *p4vm = dynamic_cast<Plus4::Plus4VM *>(&(gui_.vm));
            p4vm->loadProgram(tmp.c_str());
          }
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

void Ep128EmuGUI::menuCallback_File_SavePRG(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    std::string tmp;
    if (gui_.browseFile(tmp, gui_.prgFileDirectory, "PRG files (*.prg)",
                        Fl_File_Chooser::CREATE, "Save program file")) {
      if (gui_.lockVMThread()) {
        try {
          if (typeid(gui_.vm) == typeid(Plus4::Plus4VM)) {
            Plus4::Plus4VM  *p4vm = dynamic_cast<Plus4::Plus4VM *>(&(gui_.vm));
            p4vm->saveProgram(tmp.c_str());
          }
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
    if (gui_.browseFile(tmp, gui_.tapeImageDirectory, "Tape files (*.tap)",
                        Fl_File_Chooser::SINGLE, "Select tape image file")) {
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

void Ep128EmuGUI::menuCallback_Debug_OpenDebugger(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    throw Ep128Emu::Exception("FIXME: this function is not implemented yet");
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

void Ep128EmuGUI::menuCallback_Help_About(Fl_Widget *o, void *v)
{
  (void) o;
  Ep128EmuGUI&  gui_ = *(reinterpret_cast<Ep128EmuGUI *>(v));
  try {
    throw Ep128Emu::Exception("FIXME: this function is not implemented yet");
  }
  catch (std::exception& e) {
    gui_.errorMessage(e.what());
  }
}

