//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include <poll.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "thread.h"
#include "globals.h"
#include "errno.h"

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

      realTimePriority = prio;
      if (realTimePriority) {
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
            rt_param.sched_priority = realTimePriority;
            if (pthread_attr_setschedparam (attributes, &rt_param)) {
                  printf("Cannot set scheduling priority %d for RT thread (%s)\n",
                     realTimePriority, strerror(errno));
                  }
            }

      int rv;
      if (rv = pthread_create(&thread, attributes, ::loop, this)) {
            fprintf(stderr, "creating thread <%s> failed: %s\n",
               _name, strerror(rv));
            thread = 0;
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
      if (pthread_join(thread, 0)) {
            // perror("Failed to join sequencer thread");
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
      realTimePriority = 0;
      pfd              = 0;
      npfd             = 0;
      maxpfd           = 0;
      _running         = false;
      _pollWait        = -1;
      thread           = 0;
      sendSerialNo     = 0x00a5a500;

      // create message channels
      int filedes[2];         // 0 - reading   1 - writing
      if (pipe(filedes) == -1) {
            perror("thread:creating pipe4");
            exit(-1);
            }
      toThreadFdr = filedes[0];
      toThreadFdw = filedes[1];
      if (fcntl(toThreadFdw, F_SETFL, O_NONBLOCK) == -1)
            perror("set pipe nonblocking\n");

      if (pipe(filedes) == -1) {
            perror("thread: creating pipe5");
            exit(-1);
            }
      fromThreadFdr = filedes[0];
      fromThreadFdw = filedes[1];
      if (fcntl(fromThreadFdw, F_SETFL, O_NONBLOCK) == -1)
            perror("set pipe nonblocking\n");
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
      if (!debugMode) {
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

      int policy;
      /*if ((policy = sched_getscheduler (0)) < 0) {
            printf("Thread: Cannot get current client scheduler: %s\n", strerror(errno));
            }
*/
      if (debugMsg)
            printf("Thread <%s, id %p> has %s priority %d\n",
               _name, pthread_self(), policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_OTHER",
                realTimePriority);

      _running = true;

      threadStart(userPtr);
      while (_running) {
            if (debugMode)          // DEBUG
                  _pollWait = 10;   // ms

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

extern const char* seqMsgList[];

bool Thread::sendMsg(const ThreadMsg* m)
      {
// fprintf(stderr, "<%s> self: %p sendMsg %d  id %s running %d\n",
//   _name,
//   pthread_self(), sendSerialNo, seqMsgList[m->id], _running);

      m->serialNo = sendSerialNo;
      if (_running) {
            int rv = write(toThreadFdw, &m, sizeof(ThreadMsg*));
            if (rv != sizeof(ThreadMsg*)) {
                  perror("Thread::sendMessage(): write pipe failed");
                  return true;
                  }

            // wait for thread to finish operation
            int c;
            rv = read(fromThreadFdr, &c, sizeof(c));
            if (rv != sizeof(c)) {
                  perror("Thread::sendMessage(): read pipe failed");
                  return true;
                  }
            if (c != sendSerialNo)
                  fprintf(stderr, "%p Thread:sendMsg() serial No mismatch %d - %d\n",
                     this, c, sendSerialNo);
            }
      else {
            // if thread is not running (during initialization)
            // process commands directly:
            processMsg(m);
            }
      ++sendSerialNo;
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
            perror("Thread::readMsg(): read pipe failed");
            exit(-1);
            }
// fprintf(stderr, "%p read msg %d %s\n", this, p->serialNo, seqMsgList[p->id]);

      processMsg(p);
      int c = p->serialNo;
      int rv = write(fromThreadFdw, &c, sizeof(c));
      if (rv != sizeof(c))
            perror("Thread::readMsg(): write pipe failed");
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
            fprintf(stderr, "Thread::readMessage1(): read pipe failed, get %d, expected %d: %s\n",
               n, size, strerror(errno));
            exit(-1);
            }
      processMsg1(buffer);
      }

