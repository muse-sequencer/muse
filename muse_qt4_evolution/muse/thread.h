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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

//---------------------------------------------------------
//   Poll
//---------------------------------------------------------

struct Poll {
      int fd;
      int action;
      void (*handler)(void*,void*);
      void* param1;
      void* param2;

      Poll(int _fd, int _action, void(*_handler)(void*,void*), void* p, void* q) {
            fd      = _fd;
            action  = _action;
            handler = _handler;
            param1   = p;
            param2   = q;
            }
      };

typedef std::list<Poll> PollList;
typedef std::list<Poll>::iterator iPoll;


//---------------------------------------------------------
//   ThreadMsg
//---------------------------------------------------------

struct ThreadMsg {
      int id;
      mutable int serialNo;     // debug
      };

//---------------------------------------------------------
//   Thread
//---------------------------------------------------------

class Thread {
      const char* _name;
      volatile bool _running;
      int _pollWait;    // poll timeout in msec (-1 = infinite)
      int sendSerialNo; // debug

      pthread_t thread;

      int toThreadFdw;     // message to thread (app write)

      PollList plist;
      void* userPtr;

   protected:
      int realTimePriority;
      int fromThreadFdr;   // message from thread (seq read)
      int fromThreadFdw;   // message from thread (app write)
      int toThreadFdr;     // message to thread (seq read)
      struct pollfd* pfd;  // poll file descriptors
      int npfd;
      int maxpfd;
      virtual void processMsg(const ThreadMsg*) {}
      virtual void processMsg1(const void *) {}
      virtual void defaultTick() {}

   public:
      Thread(const char* name);
      virtual ~Thread();
      const char* name() const { return _name; }
      virtual void start(int priority, void* ptr=0);
      void stop(bool);
      void clearPollFd() {    plist.clear(); npfd = 0; }
      void addPollFd(int fd, int action, void (*handler)(void*,void*), void*, void*);
      void removePollFd(int fd, int action);
      void loop();
      void readMsg();
      void readMsg1(int size);
      bool sendMsg1(const void* m, int n);
      bool sendMsg(const ThreadMsg* m);
      bool isRunning() const { return _running; }
      void setPollWait(int val) { _pollWait = val; }
      virtual void threadStart(void*){ }  // called from loop
      virtual void threadStop() { }  // called from loop before leaving
      };

#endif

