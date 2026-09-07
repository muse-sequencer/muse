//=========================================================
//  MusE
//  Linux Music Editor
//
//  Windows portability shim for the internal pipe-based messaging
//  used between the GUI thread and the audio/sequencer threads
//  (see thread.cpp, audio.cpp, seqmsg.cpp).
//
//  On POSIX this is a thin wrapper around pipe()/read()/write()/
//  fcntl(O_NONBLOCK).
//
//  On Windows, anonymous pipes created with _pipe() cannot be used
//  here:
//   - QSocketNotifier only supports sockets on Windows, not pipes
//     (see app.cpp, which watches Audio::getFromThreadFdr() this way).
//   - The poll() emulation in poll_win.c waits on non-socket handles
//     with WaitForMultipleObjects(), but anonymous pipes do not
//     support the overlapped-I/O signaling semantics that relies on,
//     so readiness is not reported reliably.
//   - fcntl(O_NONBLOCK) does not work on Windows pipe descriptors.
//
//  A loopback TCP socket pair behaves like a bidirectional pipe for
//  our purposes, and is already handled correctly by both
//  QSocketNotifier and poll_win.c's socket path (select()-based).
//=========================================================

#ifndef __MUSE_PLATFORM_PIPE_H__
#define __MUSE_PLATFORM_PIPE_H__

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <cstdio>

namespace MusECore {

// Creates a connected pair of loopback TCP sockets, stored as plain
// ints using the same convention as poll_win.c (which casts a
// pollfd::fd back to SOCKET). Returns 0 on success, -1 on failure,
// mirroring the POSIX pipe() signature/semantics as closely as
// practical.
//
// NOTE: this truncates a SOCKET (UINT_PTR) to int. poll_win.c already
// relies on the same assumption (it casts pollfd::fd to SOCKET), so
// this does not add a new constraint, but it is worth flagging: it
// is only safe because a process realistically never holds anywhere
// near INT_MAX Winsock handles at once.
inline int win_socketpair(int fds[2])
      {
      SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (listener == INVALID_SOCKET)
            return -1;

      sockaddr_in addr;
      std::memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      addr.sin_port = 0; // ask the OS for a free ephemeral port

      if (bind(listener, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR
          || listen(listener, 1) == SOCKET_ERROR) {
            closesocket(listener);
            return -1;
            }

      int addrlen = sizeof(addr);
      if (getsockname(listener, (sockaddr*)&addr, &addrlen) == SOCKET_ERROR) {
            closesocket(listener);
            return -1;
            }

      SOCKET writer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (writer == INVALID_SOCKET) {
            closesocket(listener);
            return -1;
            }

      // Loopback connect completes essentially immediately, so a
      // blocking connect()+accept() pair (rather than async/overlapped)
      // is fine here: this only runs a couple of times at startup.
      if (connect(writer, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(listener);
            closesocket(writer);
            return -1;
            }

      SOCKET reader = accept(listener, nullptr, nullptr);
      closesocket(listener);
      if (reader == INVALID_SOCKET) {
            closesocket(writer);
            return -1;
            }

      int nodelay = 1;
      setsockopt(reader, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));
      setsockopt(writer, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

      fds[0] = (int)reader; // read end
      fds[1] = (int)writer; // write end
      return 0;
      }

} // namespace MusECore

// NOTE: every call below is ::-qualified. Several classes that use
// these macros (e.g. MusECore::Song) declare their own member
// functions named read()/write(), which would otherwise hide the
// global/POSIX ones from unqualified name lookup inside their methods
// and silently resolve to the wrong overload (a compile error, but a
// confusing one).
#define muse_pipe(fds)              MusECore::win_socketpair(fds)
#define muse_pipe_read(fd, buf, n)  ::recv((fd), (char*)(buf), (int)(n), 0)
#define muse_pipe_write(fd, buf, n) ::send((fd), (const char*)(buf), (int)(n), 0)
#define muse_pipe_set_nonblock(fd)  do { u_long _muse_nb = 1; ioctlsocket((fd), FIONBIO, &_muse_nb); } while (0)

#else // !_WIN32

#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

// See the NOTE above the Windows macros: these are ::-qualified for
// the same reason (e.g. MusECore::Song declares its own read()).
#define muse_pipe(fds)              ::pipe(fds)
#define muse_pipe_read(fd, buf, n)  ::read((fd), (buf), (n))
#define muse_pipe_write(fd, buf, n) ::write((fd), (buf), (n))
#define muse_pipe_set_nonblock(fd)  do { int _muse_rv = ::fcntl((fd), F_SETFL, O_NONBLOCK); \
                                          if (_muse_rv == -1) perror("set pipe O_NONBLOCK"); } while (0)

#endif // _WIN32

#endif // __MUSE_PLATFORM_PIPE_H__
