//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: thread.h,v 1.1.1.1.2.2 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>
#include <list>

struct pollfd;

namespace MusECore {

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
      };

//---------------------------------------------------------
//   Thread
//---------------------------------------------------------

class Thread {
      const char* _name;
      volatile bool _running;
      int _pollWait;    // poll timeout in msec (-1 = infinite)

      pthread_t thread;

      int toThreadFdw;     // message to thread (app write)

      PollList plist;
//      pthread_mutex_t lock; DELETETHIS 2
//      pthread_cond_t ready;
      void* userPtr;

   protected:
      int _realTimePriority;
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

} // namespace MusECore

#endif

