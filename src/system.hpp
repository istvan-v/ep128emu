
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

#ifndef EP128EMU_SYSTEM_HPP
#define EP128EMU_SYSTEM_HPP

#include "ep128emu.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32_LEAN_AND_MEAN   1
#  include <windows.h>
#else
#  include <pthread.h>
#endif

namespace Ep128Emu {

  class ThreadLock {
   private:
    struct ThreadLock_ {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
      HANDLE          evt;
#else
      pthread_mutex_t m;
      pthread_cond_t  c;
      unsigned char   s;
#endif
      long            refCnt;
    };
    ThreadLock_ *st;
   public:
    ThreadLock(bool isSignaled = false);
    ThreadLock(const ThreadLock&);
    ~ThreadLock();
    ThreadLock& operator=(const ThreadLock&);
    void wait();
    // wait with a timeout of 't' (in milliseconds)
    // returns 'true' if the lock was signaled before the timeout,
    // and 'false' otherwise
    bool wait(size_t t);
    void notify();
  };

  class Thread {
   private:
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    HANDLE    thread_;
    static __stdcall unsigned int threadRoutine_(void *userData);
#else
    pthread_t thread_;
    static void * threadRoutine_(void *userData);
#endif
    ThreadLock  threadLock_;
    bool    isJoined_;
   protected:
    // thread routine (should be implemented by classes derived from Thread)
    virtual void run() = 0;
    // wait until start() is called by another thread
    inline void wait()
    {
      threadLock_.wait();
    }
    // wait until start() is called by another thread (return value = true),
    // or the timeout of 't' milliseconds is elapsed (return value = false)
    inline bool wait(size_t t)
    {
      return threadLock_.wait(t);
    }
   public:
    Thread();
    virtual ~Thread();
    // signal the child thread, allowing it to execute run() after the thread
    // object is created, or to return from wait()
    inline void start()
    {
      threadLock_.notify();
    }
    // wait until the child thread finishes (implies calling start() first,
    // and the destructor calls join())
    void join();
  };

  class Mutex {
   private:
    struct Mutex_ {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
      CRITICAL_SECTION  mutex_;
#else
      pthread_mutex_t   mutex_;
#endif
      long    refCnt_;
    };
    Mutex_  *m;
   public:
    Mutex();
    Mutex(const Mutex& m_);
    ~Mutex();
    Mutex& operator=(const Mutex& m_);
    void lock();
    void unlock();
  };

  class Timer {
   private:
    uint64_t  startTime;
    double    secondsPerTick;
    static uint64_t getRealTime_();
   public:
    Timer();
    ~Timer();
    double getRealTime();
    void reset();
    void reset(double t);
    static void wait(double t);
  };

  // remove leading and trailing whitespace from string
  void stripString(std::string& s);

  // convert string to upper case
  void stringToUpperCase(std::string& s);

  // convert string to lower case
  void stringToLowerCase(std::string& s);

  // split path into directory name and base name
  // note: the result of passing multiple references to the same string
  // is undefined
  void splitPath(const std::string& path_,
                 std::string& dirname_, std::string& basename_);

  // returns full path to ~/.ep128emu, creating the directory first
  // if it does not exist yet
  std::string getEp128EmuHomeDirectory();

}       // namespace Ep128Emu

#endif  // EP128EMU_SYSTEM_HPP

