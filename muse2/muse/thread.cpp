//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: thread.cpp,v 1.4.2.5 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <fcntl.h>

#include "globals.h"
#include "errno.h"

namespace MusECore {

//---------------------------------------------------------
//   Thread
//---------------------------------------------------------

Thread::~Thread()
      {
      }

//---------------------------------------------------------
//   serverloop
//---------------------------------------------------------

static void* loop(void* mops)
      {
      Thread* t = (Thread*) mops;
      t->loop();
      return 0;
      }

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void Thread::start(int prio, void* ptr)
      {
      userPtr = ptr;
      pthread_attr_t* attributes = 0;
      _realTimePriority = prio;
    
      /* DELETETHIS 14
      attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
      pthread_attr_init(attributes);
      */

//      pthread_mutexattr_t mutexattr;
//      pthread_mutexattr_init(&mutexattr);
//      pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_TIMED_NP);
//      pthread_mutex_init(&lock, &mutexattr);
//      pthread_cond_init(&ready, 0);

//      pthread_mutex_lock(&lock);


      //if (_realTimePriority) {
      if (MusEGlobal::realTimeScheduling && _realTimePriority > 0) {                        // p4.0.16
            attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
            pthread_attr_init(attributes);

            if (pthread_attr_setschedpolicy(attributes, SCHED_FIFO)) {
                  printf("cannot set FIFO scheduling class for RT thread\n");
                  }
            if (pthread_attr_setscope (attributes, PTHREAD_SCOPE_SYSTEM)) {
                  printf("Cannot set scheduling scope for RT thread\n");
                  }
            if (pthread_attr_setinheritsched(attributes, PTHREAD_EXPLICIT_SCHED)) {
                  printf("Cannot set setinheritsched for RT thread\n");
                  }

            struct sched_param rt_param;
            memset(&rt_param, 0, sizeof(rt_param));
            rt_param.sched_priority = _realTimePriority;
            if (pthread_attr_setschedparam (attributes, &rt_param)) {
                  printf("Cannot set scheduling priority %d for RT thread (%s)\n",
                     _realTimePriority, strerror(errno));
                  }
            }


      /* DELETETHIS 8
      if (pthread_create(&thread, attributes, MusECore::loop, this))
            perror("creating thread failed:");
//      else
//      {
//           pthread_cond_wait(&ready, &lock);
//      }
//      pthread_mutex_unlock(&lock);
      */


      int rv = pthread_create(&thread, attributes, MusECore::loop, this); 
      if(rv)
      {
        // p4.0.16: MusEGlobal::realTimeScheduling is unreliable. It is true even in some clearly non-RT cases.
        // I cannot seem to find a reliable answer to the question of "are we RT or not".
        // MusE was failing with a stock kernel because of PTHREAD_EXPLICIT_SCHED.
        // So we'll just have to try again without attributes.
        if (MusEGlobal::realTimeScheduling && _realTimePriority > 0) 
          rv = pthread_create(&thread, NULL, MusECore::loop, this); 
      }

      if(rv)
          fprintf(stderr, "creating thread <%s> failed: %s\n", _name, strerror(rv));

      if (attributes)                      // p4.0.16
      {
        pthread_attr_destroy(attributes);
        free(attributes);
      }
      
      }

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void Thread::stop(bool force)
      {
      if (thread == 0)
            return;
      if (force) {
            pthread_cancel(thread);
            threadStop();
            }
      _running = false;
      if (thread) {
          if (pthread_join(thread, 0)) {
                // perror("Failed to join sequencer thread"); DELETETHIS and the if around?
                }
          }
      }
//---------------------------------------------------------
//   Thread
//    prio = 0    no realtime scheduling
//---------------------------------------------------------

Thread::Thread(const char* s)
      {
      userPtr          = 0;
      _name            = s;
      _realTimePriority = 0;
      
      pfd              = 0;
      npfd             = 0;
      maxpfd           = 0;
      _running         = false;
      _pollWait        = -1;
      thread           = 0;

      // create message channels
      int filedes[2];         // 0 - reading   1 - writing
      if (pipe(filedes) == -1) {
            perror("thread:creating pipe");
            exit(-1);
            }
      toThreadFdr = filedes[0];
      toThreadFdw = filedes[1];

      if (pipe(filedes) == -1) {
            perror("thread: creating pipe");
            exit(-1);
            }
      fromThreadFdr = filedes[0];
      fromThreadFdw = filedes[1];

//      pthread_mutexattr_t mutexattr; DELETETHIS 5
//      pthread_mutexattr_init(&mutexattr);
//      pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_TIMED_NP);
//      pthread_mutex_init(&lock, &mutexattr);
//      pthread_cond_init(&ready, 0);
      }

//---------------------------------------------------------
//   addPollFd
//---------------------------------------------------------

void Thread::addPollFd(int fd, int action, void (*handler)(void*,void*), void* p, void* q)
      {
      if (fd == -1)
            return;
      for (iPoll i = plist.begin(); i != plist.end(); ++i) {
            if ((i->fd == fd) && (i->action == action))
                  return;
            }

      plist.push_back(Poll(fd, action, handler, p, q));

      if (npfd == maxpfd) {
            int n = (maxpfd == 0) ? 4 : maxpfd * 2;
            //TODO: delete old pfd
            pfd   = new struct pollfd[n];
            maxpfd = n;
            }
      ++npfd;
      int idx = 0;
      for (iPoll i = plist.begin(); i != plist.end(); ++i, ++idx) {
            pfd[idx].fd     = i->fd;
            pfd[idx].events = i->action;
            }
      }

//---------------------------------------------------------
//   removePollFd
//---------------------------------------------------------

void Thread::removePollFd(int fd, int action)
      {
      for (iPoll i = plist.begin(); i != plist.end(); ++i) {
            if (i->fd == fd && i->action == action) {
                  plist.erase(i);
                  --npfd;
                  break;
                  }
            }
      int idx = 0;
      for (iPoll i = plist.begin(); i != plist.end(); ++i, ++idx) {
            pfd[idx].fd     = i->fd;
            pfd[idx].events = i->action;
            }
      }

//---------------------------------------------------------
//   loop
//---------------------------------------------------------

void Thread::loop()
      {
      if (!MusEGlobal::debugMode) {
            if (mlockall(MCL_CURRENT | MCL_FUTURE))
                  perror("WARNING: Cannot lock memory:");
            }
      
#ifdef __APPLE__
#define BIG_ENOUGH_STACK (1024*256*1)
#else
#define BIG_ENOUGH_STACK (1024*1024*1)
#endif
      char buf[BIG_ENOUGH_STACK];
      for (int i = 0; i < BIG_ENOUGH_STACK; i++)
            buf[i] = i;
#undef BIG_ENOUGH_STACK

      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
      pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);

      int policy = buf[0]; // Initialize using buf[0] to keep the compiler from complaining about unused buf.
      policy = 0;          // Now set the true desired inital value.
      if ((policy = sched_getscheduler (0)) < 0) {
            printf("Thread: Cannot get current client scheduler: %s\n", strerror(errno));
            }

      if (MusEGlobal::debugMsg)
            printf("Thread <%s, id %p> has %s priority %d\n",
               _name, (void *)pthread_self(), policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_OTHER",
                policy == SCHED_FIFO ? _realTimePriority : 0);


//      pthread_mutex_lock(&lock); DELETETHIS and below
      _running = true;
//      pthread_cond_signal(&ready);
//      pthread_mutex_unlock(&lock);

      threadStart(userPtr);

      while (_running) {
            if (MusEGlobal::debugMode)          // DEBUG
                  _pollWait = 10;   // ms
            else
                  _pollWait = -1;

            int n = poll(pfd, npfd, _pollWait);
            if (n < 0) {
                  if (errno == EINTR)
                        continue;
                  fprintf(stderr, "poll failed: %s\n", strerror(errno));
                  exit(-1);
                  }
            if (n == 0) {       // timeout
                  defaultTick();
                  continue;
                  }

            struct pollfd* p = &pfd[0];
            int i = 0;
            for (iPoll ip = plist.begin(); ip != plist.end(); ++ip, ++p, ++i) {
                  if (ip->action & p->revents) {
                        (ip->handler)(ip->param1, ip->param2);
                        break;
                        }
                  }
            }
      threadStop();
      }

//---------------------------------------------------------
//   send
//    send request from gui to thread
//    wait until request is processed
//---------------------------------------------------------

bool Thread::sendMsg(const ThreadMsg* m)
{
      if (_running) 
      {
            int rv = write(toThreadFdw, &m, sizeof(ThreadMsg*));
            if (rv != sizeof(ThreadMsg*)) {
                  perror("Thread::sendMessage(): write pipe failed");
                  return true;
                  }

           // wait for sequencer to finish operation
            char c;
            rv = read(fromThreadFdr, &c, 1);
            if (rv != 1) 
            {
                  perror("Thread::sendMessage(): read pipe failed");
                  return true;
            }
            //int c; DELETETHIS 6
            //rv = read(fromThreadFdr, &c, sizeof(c));
            //if (rv != sizeof(c)) {
            //      perror("Thread::sendMessage(): read pipe failed");
            //      return true;
            //      }
      }
      else 
      {
            // if thread is not running (during initialization)
            // process commands directly:
            processMsg(m);
      }
      return false;
}

//---------------------------------------------------------
//   send
//    send request from gui to thread
//    do __not__ wait until request is processed
//---------------------------------------------------------

bool Thread::sendMsg1(const void* m, int n)
      {
      int rv = write(toThreadFdw, m, n);
      if (rv != n) {
            perror("Thread::sendMessage1(): write pipe failed");
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   readMsg
//---------------------------------------------------------

void Thread::readMsg()
      {
      ThreadMsg* p;
      if (read(toThreadFdr, &p, sizeof(p)) != sizeof(p)) {
            perror("Thread::readMessage(): read pipe failed");
            exit(-1);
            }
      processMsg(p);
      char c = 'x';
      int rv = write(fromThreadFdw, &c, 1);
      if (rv != 1)
            perror("Thread::readMessage(): write pipe failed");
      //int c = p->serialNo; DELETETHIS 4
      //int rv = write(fromThreadFdw, &c, sizeof(c));
      //if (rv != sizeof(c))
      //      perror("Thread::readMsg(): write pipe failed");
      }

//---------------------------------------------------------
//   readMsg
//    sequencer reads one gui message
//---------------------------------------------------------

void Thread::readMsg1(int size)
      {
      char buffer[size];
      int n = read(toThreadFdr, buffer, size);
      if (n != size) {
            fprintf(stderr, "Thread::readMsg1(): read pipe failed, get %d, expected %d: %s\n",
               n, size, strerror(errno));
            exit(-1);
            }
      processMsg1(buffer);
      }

} // namespace MusECore
