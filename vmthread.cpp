
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

#include "ep128emu.hpp"
#include "system.hpp"
#include "vm.hpp"
#include "vmthread.hpp"

#include <iostream>

static void defaultErrorCallback(void *userData_, const char *msg)
{
  (void) userData_;
  std::cerr << "ERROR: " << msg << std::endl;
}

static void dummyErrorCallback(void *userData_, const char *msg)
{
  (void) userData_;
  (void) msg;
}

namespace Ep128Emu {

  VMThread::VMThread(VirtualMachine& vm_, void *userData_)
    : vm(vm_),
      lockCnt(0UL),
      threadLock1(true),
      threadLock2(true),
      messageQueue((Message *) 0),
      lastMessage((Message *) 0),
      freeMessageStack((Message *) 0),
      messageCnt(0),
      exitFlag(false),
      joinFlag(false),
      errorFlag(false),
      pauseFlag(true),
      isRecordingDemo(false),
      isPlayingDemo(false),
      tapeReadOnly(true),
      tapePosition(-1.0),
      tapeLength(-1.0),
      tapeSampleRate(0L),
      tapeSampleSize(0),
      userData(userData_),
      errorCallback(&defaultErrorCallback)
  {
    for (int i = 0; i < 128; i++)
      keyboardState[i] = false;
    this->start();
  }

  VMThread::~VMThread()
  {
    this->quit(true);
  }

  void VMThread::cleanup()
  {
    mutex_.lock();
    exitFlag = true;
    pauseFlag = true;
    try {
      vm.stopDemo();
    }
    catch (...) {
      errorFlag = true;
    }
    isRecordingDemo = false;
    isPlayingDemo = false;
    try {
      vm.setTapeFileName(std::string(""));
    }
    catch (...) {
      errorFlag = true;
    }
    tapeReadOnly = true;
    tapePosition = -1.0;
    tapeLength = -1.0;
    tapeSampleRate = 0L;
    tapeSampleSize = 0;
    while (messageQueue) {
      Message *m = messageQueue;
      messageQueue = m->nextMessage;
      m->~Message();
      m->nextMessage = freeMessageStack;
      freeMessageStack = m;
    }
    lastMessage = (Message *) 0;
    messageCnt = 0;
    while (freeMessageStack) {
      Message *m = freeMessageStack;
      freeMessageStack = m->nextMessage;
      std::free(m);
    }
    mutex_.unlock();
  }

  bool VMThread::process()
  {
    // check and process any pending messages
    mutex_.lock();
    while (true) {
      if (exitFlag) {
        mutex_.unlock();
        this->cleanup();
        return false;
      }
      Message *m = messageQueue;
      if (!m)
        break;
      messageQueue = m->nextMessage;
      if (!messageQueue)
        lastMessage = (Message *) 0;
      messageCnt--;
      try {
        m->process();
      }
      catch (Exception& e) {
        pauseFlag = true;
        mutex_.unlock();
        errorCallback(userData, e.what());
        mutex_.lock();
      }
      catch (...) {
        errorFlag = true;
        mutex_.unlock();
        m->~Message();
        std::free(m);
        this->cleanup();
        return false;
      }
      m->~Message();
      m->nextMessage = freeMessageStack;
      freeMessageStack = m;
    }
    mutex_.unlock();
    // run emulation, or wait if paused
    if (!pauseFlag) {
      try {
        vm.run(2000);
      }
      catch (Exception& e) {
        mutex_.lock();
        pauseFlag = true;
        mutex_.unlock();
        errorCallback(userData, e.what());
      }
      catch (...) {
        mutex_.lock();
        errorFlag = true;
        mutex_.unlock();
        this->cleanup();
        return false;
      }
    }
    else
      Timer::wait(0.01);
    // update status information
    mutex_.lock();
    try {
      isRecordingDemo = vm.getIsRecordingDemo();
      isPlayingDemo = vm.getIsPlayingDemo();
      tapeReadOnly = vm.getIsTapeReadOnly();
      tapePosition = vm.getTapePosition();
      tapeLength = vm.getTapeLength();
      tapeSampleRate = vm.getTapeSampleRate();
      tapeSampleSize = vm.getTapeSampleSize();
    }
    catch (...) {
      errorFlag = true;
      mutex_.unlock();
      this->cleanup();
      return false;
    }
    mutex_.unlock();
    return true;
  }

  void VMThread::run()
  {
    while (true) {
      mutex_.lock();
      bool  lockFlag = (lockCnt != 0UL);
      if (lockFlag)
        threadLock2.notify();
      mutex_.unlock();
      if (lockFlag)
        threadLock1.wait();
      if (!this->process())
        break;
    }
    this->cleanup();
  }

  int VMThread::getStatus(bool& isPaused_,
                          bool& isRecordingDemo_, bool& isPlayingDemo_,
                          bool& tapeReadOnly_,
                          double& tapePosition_, double& tapeLength_,
                          long& tapeSampleRate_, int& tapeSampleSize_)
  {
    int     retval = 0;
    mutex_.lock();
    isPaused_ = pauseFlag;
    isRecordingDemo_ = isRecordingDemo;
    isPlayingDemo_ = isPlayingDemo;
    tapeReadOnly_ = tapeReadOnly;
    tapePosition_ = tapePosition;
    tapeLength_ = tapeLength;
    tapeSampleRate_ = tapeSampleRate;
    tapeSampleSize_ = tapeSampleSize;
    if (exitFlag)
      retval = (errorFlag ? -1 : 1);
    mutex_.unlock();
    return retval;
  }

  int VMThread::lock(size_t t)
  {
    mutex_.lock();
    if (exitFlag) {
      lockCnt = 0UL;
      threadLock1.notify();
      mutex_.unlock();
      return -1;
    }
    lockCnt++;
    if (lockCnt > 1UL) {
      mutex_.unlock();
      return 0;
    }
    threadLock1.wait(0);
    threadLock2.wait(0);
    mutex_.unlock();
    bool  tmp = threadLock2.wait(t);
    mutex_.lock();
    if (exitFlag || !tmp) {
      lockCnt--;
      int   retval = (exitFlag ? -1 : 1);
      threadLock1.notify();
      mutex_.unlock();
      return retval;
    }
    mutex_.unlock();
    return 0;
  }

  void VMThread::unlock()
  {
    mutex_.lock();
    if (lockCnt)
      lockCnt--;
    if (!lockCnt)
      threadLock1.notify();
    mutex_.unlock();
  }

  // --------------------------------------------------------------------------

  void VMThread::pause(bool n)
  {
    mutex_.lock();
    pauseFlag = n;
    mutex_.unlock();
  }

  void VMThread::quit(bool waitFlag_)
  {
    mutex_.lock();
    exitFlag = true;
    pauseFlag = true;
    lockCnt = 0UL;
    threadLock1.notify();
    if (joinFlag || !waitFlag_) {
      mutex_.unlock();
      return;
    }
    joinFlag = true;
    mutex_.unlock();
    this->join();
  }

  void VMThread::setUserData(void *userData_)
  {
    userData = userData_;
  }

  void VMThread::setErrorCallback(void (*func)(void *userData_,
                                               const char *msg_))
  {
    if (func)
      errorCallback = func;
    else
      errorCallback = &dummyErrorCallback;
  }

  void VMThread::reset(bool isColdReset_)
  {
    queueMessage(allocateMessage<Message_Reset, bool>(isColdReset_));
  }

  void VMThread::setAudioOutputVolume(double ampScale_)
  {
    queueMessage(allocateMessage<Message_SetVolume, double>(ampScale_));
  }

  void VMThread::setKeyboardState(uint8_t keyCode_, bool isPressed_)
  {
    queueMessage(allocateMessage<Message_KeyboardEvent, uint8_t, bool>(
                     keyCode_, isPressed_));
  }

  void VMThread::resetKeyboard()
  {
    queueMessage(allocateMessage<Message_ResetKeyboard>());
  }

  void VMThread::tapePlay()
  {
    queueMessage(allocateMessage<Message_TapePlay>());
  }

  void VMThread::tapeRecord()
  {
    queueMessage(allocateMessage<Message_TapeRecord>());
  }

  void VMThread::tapeStop()
  {
    queueMessage(allocateMessage<Message_TapeStop>());
  }

  void VMThread::tapeSeek(double seekTime_)
  {
    queueMessage(allocateMessage<Message_TapeSeek, int, double>(0, seekTime_));
  }

  void VMThread::tapeSeekToCuePoint(bool isForward, double t)
  {
    queueMessage(allocateMessage<Message_TapeSeek, int, double>((isForward ?
                                                                 1 : -1), t));
  }

  void VMThread::stopDemo()
  {
    queueMessage(allocateMessage<Message_StopDemo>());
  }

  VMThread::Message * VMThread::allocateMessage_()
  {
    mutex_.lock();
    if (freeMessageStack) {
      Message *m = freeMessageStack;
      freeMessageStack = m->nextMessage;
      mutex_.unlock();
      return m;
    }
    if (messageCnt >= 1024) {
      mutex_.unlock();
      errorCallback(userData, "message queue overflow");
      return (Message *) 0;
    }
    mutex_.unlock();
    Message *m = (Message *) std::malloc(sizeof(Message_Dummy));
    if (!m)
      errorCallback(userData, "cannot allocate memory for message");
    return m;
  }

  void VMThread::queueMessage(VMThread::Message *m)
  {
    if (!m)
      return;
    mutex_.lock();
    if (exitFlag) {
      mutex_.unlock();
      m->~Message();
      std::free(m);
      return;
    }
    m->nextMessage = (Message *) 0;
    if (lastMessage)
      lastMessage->nextMessage = m;
    else
      messageQueue = m;
    lastMessage = m;
    messageCnt++;
    mutex_.unlock();
  }

  // --------------------------------------------------------------------------

  VMThread::Message::~Message()
  {
  }

  VMThread::Message_Reset::~Message_Reset()
  {
  }

  void VMThread::Message_Reset::process()
  {
    vmThread.vm.reset(isColdReset);
  }

  VMThread::Message_SetVolume::~Message_SetVolume()
  {
  }

  void VMThread::Message_SetVolume::process()
  {
    vmThread.vm.setAudioOutputVolume(float(ampScale));
  }

  VMThread::Message_KeyboardEvent::~Message_KeyboardEvent()
  {
  }

  void VMThread::Message_KeyboardEvent::process()
  {
    if (vmThread.keyboardState[keyCode & 0x7F] != isPressed) {
      vmThread.keyboardState[keyCode & 0x7F] = isPressed;
      vmThread.vm.setKeyboardState(keyCode & 0x7F, isPressed);
    }
  }

  VMThread::Message_ResetKeyboard::~Message_ResetKeyboard()
  {
  }

  void VMThread::Message_ResetKeyboard::process()
  {
    for (uint8_t i = 0; i <= 127; i++) {
      if (vmThread.keyboardState[i]) {
        vmThread.keyboardState[i] = false;
        vmThread.vm.setKeyboardState(i, false);
      }
    }
  }

  VMThread::Message_TapePlay::~Message_TapePlay()
  {
  }

  void VMThread::Message_TapePlay::process()
  {
    vmThread.vm.tapePlay();
  }

  VMThread::Message_TapeRecord::~Message_TapeRecord()
  {
  }

  void VMThread::Message_TapeRecord::process()
  {
    vmThread.vm.tapeRecord();
  }

  VMThread::Message_TapeStop::~Message_TapeStop()
  {
  }

  void VMThread::Message_TapeStop::process()
  {
    vmThread.vm.tapeStop();
  }

  VMThread::Message_TapeSeek::~Message_TapeSeek()
  {
  }

  void VMThread::Message_TapeSeek::process()
  {
    if (seekDirection == 0)
      vmThread.vm.tapeSeek(seekTime);
    else if (seekDirection > 0)
      vmThread.vm.tapeSeekToCuePoint(true, seekTime);
    else
      vmThread.vm.tapeSeekToCuePoint(false, seekTime);
  }

  VMThread::Message_StopDemo::~Message_StopDemo()
  {
  }

  void VMThread::Message_StopDemo::process()
  {
    vmThread.vm.stopDemo();
  }

  VMThread::Message_Dummy::~Message_Dummy()
  {
  }

  void VMThread::Message_Dummy::process()
  {
    throw std::exception();
  }

}       // namespace Ep128Emu

