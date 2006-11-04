
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2006 Istvan Varga <istvanv@users.sourceforge.net>
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

#include "ep128.hpp"
#include "system.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  include <windows.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#  include <pthread.h>
#endif

namespace Ep128Emu {

  ThreadLock::ThreadLock(bool isSignaled)
  {
    st = new ThreadLock_;
    st->refCnt = 1L;
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    st->evt = CreateEvent(NULL, FALSE, (isSignaled ? TRUE : FALSE), NULL);
    if (st->evt == (HANDLE) 0) {
      delete st;
      throw std::bad_alloc();
    }
#else
    if (pthread_mutex_init(&(st->m), NULL) != 0) {
      delete st;
      throw std::bad_alloc();
    }
    if (pthread_cond_init(&(st->c), NULL) != 0) {
      pthread_mutex_destroy(&(st->m));
      delete st;
      throw std::bad_alloc();
    }
    st->s = (isSignaled ? 1 : 0);
#endif
  }

  ThreadLock::ThreadLock(const ThreadLock& oldInstance)
  {
    st = oldInstance.st;
    st->refCnt++;
  }

  ThreadLock::~ThreadLock()
  {
    if (--(st->refCnt) > 0L)
      return;
    this->notify();
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    CloseHandle(st->evt);
#else
    pthread_cond_destroy(&(st->c));
    pthread_mutex_destroy(&(st->m));
    delete st;
#endif
  }

  void ThreadLock::operator=(const ThreadLock& oldInstance)
  {
    st = oldInstance.st;
    st->refCnt++;
  }

  void ThreadLock::wait()
  {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    WaitForSingleObject(st->evt, INFINITE);
#else
    pthread_mutex_lock(&(st->m));
    while (!st->s)
      pthread_cond_wait(&(st->c), &(st->m));
    st->s = 0;
    pthread_mutex_unlock(&(st->m));
#endif
  }

  bool ThreadLock::wait(size_t t)
  {
    bool    retval = true;

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    WaitForSingleObject(st->evt, DWORD(t));
#else
    pthread_mutex_lock(&(st->m));
    if (!st->s) {
      if (!t)
        retval = false;
      else {
        struct timeval  tv;
        struct timespec ts;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + (t / 1000);
        ts.tv_nsec = (tv.tv_usec + ((t % 1000) * 1000)) * 1000;
        if (ts.tv_nsec >= 1000000000) {
          ts.tv_nsec -= 1000000000;
          ts.tv_sec++;
        }
        do {
          retval = !(pthread_cond_timedwait(&(st->c), &(st->m), &ts));
        } while (retval && !st->s);
      }
    }
    st->s = 0;
    pthread_mutex_unlock(&(st->m));
    return retval;
#endif
  }

  void ThreadLock::notify()
  {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    SetEvent(st->evt);
#else
    pthread_mutex_lock(&(st->m));
    st->s = 1;
    pthread_cond_signal(&(st->c));
    pthread_mutex_unlock(&(st->m));
#endif
  }

}       // namespace Ep128Emu

