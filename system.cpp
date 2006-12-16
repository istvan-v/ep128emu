
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

#include "ep128emu.hpp"
#include "system.hpp"

#ifdef WIN32
#  undef WIN32
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
#  define WIN32 1
#endif

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN   1
#  include <direct.h>
#  include <windows.h>
#  include <process.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#  include <pthread.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace Ep128Emu {

  ThreadLock::ThreadLock(bool isSignaled)
  {
    st = new ThreadLock_;
    st->refCnt = 1L;
#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
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

#ifdef WIN32
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
#ifdef WIN32
    SetEvent(st->evt);
#else
    pthread_mutex_lock(&(st->m));
    st->s = 1;
    pthread_cond_signal(&(st->c));
    pthread_mutex_unlock(&(st->m));
#endif
  }

#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
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
#ifdef WIN32
    EnterCriticalSection(&(m->mutex_));
#else
    pthread_mutex_lock(&(m->mutex_));
#endif
  }

  void Mutex::unlock()
  {
#ifdef WIN32
    LeaveCriticalSection(&(m->mutex_));
#else
    pthread_mutex_unlock(&(m->mutex_));
#endif
  }

  // --------------------------------------------------------------------------

  void stripString(std::string& s)
  {
    const std::string&  t = s;
    size_t  i, j;
    for (i = 0; i < t.length(); i++) {
      if (!(t[i] == ' ' || t[i] == '\t' || t[i] == '\r' || t[i] == '\n'))
        break;
    }
    for (j = t.length(); j > i; j--) {
      size_t  k = j - 1;
      if (!(t[k] == ' ' || t[k] == '\t' || t[k] == '\r' || t[k] == '\n'))
        break;
    }
    size_t  l = (j - i);
    if (l == 0) {
      s = "";
      return;
    }
    if (i) {
      for (size_t k = 0; k < l; k++)
        s[k] = t[k + i];
    }
    if (l != t.length())
      s.resize(l, ' ');
  }

  void stringToUpperCase(std::string& s)
  {
    const std::string&  t = s;
    for (size_t i = 0; i < t.length(); i++) {
      if (t[i] >= 'a' && t[i] <= 'z')
        s[i] = (t[i] - 'a') + 'A';
    }
  }

  void stringToLowerCase(std::string& s)
  {
    const std::string&  t = s;
    for (size_t i = 0; i < t.length(); i++) {
      if (t[i] >= 'A' && t[i] <= 'Z')
        s[i] = (t[i] - 'A') + 'a';
    }
  }

  void splitPath(const std::string& path_,
                 std::string& dirname_, std::string& basename_)
  {
    dirname_ = "";
    basename_ = "";
    if (path_.length() == 0)
      return;
    size_t  i = path_.length();
    for ( ; i != 0; i--) {
      if (path_[i - 1] == '/' || path_[i - 1] == '\\')
        break;
#ifdef WIN32
      if (i == 2) {
        if (((path_[0] >= 'A' && path_[0] <= 'Z') ||
             (path_[0] >= 'a' && path_[0] <= 'z')) &&
            path_[1] == ':')
          break;
      }
#endif
    }
    size_t  j = 0;
    for ( ; j < i; j++)
      dirname_ += path_[j];
    for ( ; j < path_.length(); j++)
      basename_ += path_[j];
  }

  std::string getEp128EmuHomeDirectory()
  {
    std::string dirName;

    dirName = "";
#ifndef WIN32
    if (std::getenv("HOME") != (char*) 0)
      dirName = std::getenv("HOME");
    if ((int) dirName.size() == 0)
      dirName = ".";
    mkdir(dirName.c_str(), 0700);
    if (dirName[dirName.size() - 1] != '/')
      dirName += '/';
    dirName += ".ep128emu";
    mkdir(dirName.c_str(), 0700);
#else
    stripString(dirName, std::getenv("USERPROFILE"));
    if ((int) dirName.size() != 0) {
      struct _stat tmp;
      std::memset(&tmp, 0, sizeof(struct _stat));
      if (dirName[dirName.size() - 1] != '\\')
        dirName += '\\';
      dirName += "Application Data";
      if (_stat(dirName.c_str(), &tmp) != 0 ||
          !(tmp.st_mode & _S_IFDIR))
        dirName = "";
    }
    if ((int) dirName.size() == 0) {
      stripString(dirName, std::getenv("HOME"));
      if ((int) dirName.size() != 0) {
        struct _stat tmp;
        std::memset(&tmp, 0, sizeof(struct _stat));
        if (_stat(dirName.c_str(), &tmp) != 0 ||
            !(tmp.st_mode & _S_IFDIR))
          dirName = "";
      }
    }
    if ((int) dirName.size() == 0) {
      char  buf[512];
      int   len;
      len = (int) GetModuleFileName((HMODULE) 0, &(buf[0]), (DWORD) 512);
      if (len >= 512)
        len = 0;
      while (len > 0) {
        len--;
        if (buf[len] == '\\') {
          buf[len] = (char) 0;
          break;
        }
      }
      if (len > 0)
        dirName = &(buf[0]);
      else
        dirName = ".";
    }
    if (dirName[dirName.size() - 1] != '\\')
      dirName += '\\';
    dirName += ".ep128emu";
    _mkdir(dirName.c_str());
#endif
    return dirName;
  }

}       // namespace Ep128Emu

