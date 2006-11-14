
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
#  define WIN32_LEAN_AND_MEAN   1
#  include <windows.h>
#  include <process.h>
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

  ThreadLock& ThreadLock::operator=(const ThreadLock& oldInstance)
  {
    if (this != &oldInstance) {
      st = oldInstance.st;
      st->refCnt++;
    }
    return (*this);
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
    retval = !(WaitForSingleObject(st->evt, DWORD(t)));
    return retval;
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

#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
  __stdcall unsigned int Thread::threadRoutine_(void *userData)
  {
    Thread  *p = reinterpret_cast<Thread *>(userData);
    p->threadLock_.wait();
    p->run();
    return 0U;
  }
#else
  void * Thread::threadRoutine_(void *userData)
  {
    Thread  *p = reinterpret_cast<Thread *>(userData);
    p->threadLock_.wait();
    p->run();
    return (void *) 0;
  }
#endif

  Thread::Thread()
    : threadLock_(false),
      isJoined_(false)
  {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    thread_ = (HANDLE) _beginthreadex(NULL, 0U,
                                      &Thread::threadRoutine_, this, 0U, NULL);
    if (!thread_)
      throw std::bad_alloc();
#else
    if (pthread_create(&thread_, (pthread_attr_t *) 0,
                       &Thread::threadRoutine_, this) != 0)
      throw std::bad_alloc();
#endif
  }

  Thread::~Thread()
  {
    this->join();
  }

  void Thread::join()
  {
    if (!isJoined_) {
      this->start();
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
      WaitForSingleObject(thread_, INFINITE);
      CloseHandle(thread_);
#else
      void  *dummy;
      pthread_join(thread_, &dummy);
#endif
      isJoined_ = true;
    }
  }

  Mutex::Mutex()
  {
    m = new Mutex_;
    m->refCnt_ = 1L;
    try {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
      InitializeCriticalSection(&(m->mutex_));
#else
      pthread_mutexattr_t   attr;
      if (pthread_mutexattr_init(&attr) != 0)
        throw std::bad_alloc();
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      int   err = pthread_mutex_init(&(m->mutex_), &attr);
      pthread_mutexattr_destroy(&attr);
      if (err)
        throw std::bad_alloc();
#endif
    }
    catch (...) {
      delete m;
      throw;
    }
  }

  Mutex::Mutex(const Mutex& m_)
  {
    m = m_.m;
    m->refCnt_++;
  }

  Mutex::~Mutex()
  {
    if (--(m->refCnt_) <= 0) {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
      DeleteCriticalSection(&(m->mutex_));
#else
      pthread_mutex_destroy(&(m->mutex_));
#endif
      delete m;
    }
  }

  Mutex& Mutex::operator=(const Mutex& m_)
  {
    if (this != &m_) {
      m = m_.m;
      m->refCnt_++;
    }
    return (*this);
  }

  void Mutex::lock()
  {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    EnterCriticalSection(&(m->mutex_));
#else
    pthread_mutex_lock(&(m->mutex_));
#endif
  }

  void Mutex::unlock()
  {
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    LeaveCriticalSection(&(m->mutex_));
#else
    pthread_mutex_unlock(&(m->mutex_));
#endif
  }

}       // namespace Ep128Emu

