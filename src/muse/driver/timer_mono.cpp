//=========================================================
//  MusE
//  Linux Music Editor
//
//  Monotonic timer using timerfd(CLOCK_MONOTONIC)
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
//
//  MonoTimer - MusE MIDI sequencer timer backend (Linux).
//
//  Timer method: Linux timerfd(2) with CLOCK_MONOTONIC. Periodic ticks are
//  delivered on a file descriptor (poll/read), immune to wall-clock changes
//  (NTP, daylight saving, manual time adjustment).
//
//  Note: this is a high-resolution timer (jitter typically less than 1 us),
//    that usually works better than the RTC timer for modern systems, 
//    but may not be available on very old systems.
//
//  Optional: set_real_time_prio() requests SCHED_FIFO priority 84 for the
//  calling thread, which can reduce scheduling jitter during MIDI timing.
//  Requires appropriate realtime privileges (limits.conf / realtime group).
//
//  Reported resolution: 4096 Hz (~244 us interval); actual tick rate is set
//  via setTimerFreq() / MusE config rtcTicks (typically 1024-32768 Hz).
//
//=========================================================

#include "timer_mono.h"

#ifndef _WIN32

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <sched.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <time.h>

namespace MusECore {

static const int kMonoTimerRtpPriority = 84;
// 4096 Hz ~= 244 us interval; well within timerfd(CLOCK_MONOTONIC) capability
static const unsigned long kMonoTimerResolution = 4096;
static const long int kNanoPerSec = 1000L * 1000000L;

static void monoTimerMsg(const char* msg)
{
  fprintf(stderr, "MonoTimer: %s\n", msg);
}

static void monoTimerHint(const char* msg, const char* hint)
{
  fprintf(stderr, "MonoTimer: %s\n", msg);
  if (hint)
    fprintf(stderr, "  Hint: %s\n", hint);
}

static void monoTimerSysError(const char* msg, const char* hint = nullptr)
{
  fprintf(stderr, "MonoTimer: %s: %s\n", msg, strerror(errno));
  if (hint)
    fprintf(stderr, "  Hint: %s\n", hint);
}

static void monoTimerCloseFd(int& fd)
{
  if (fd == -1)
    return;
  if (close(fd) != 0)
    monoTimerSysError("could not close timer file descriptor");
  fd = -1;
}

static bool monoTimerFreqToInterval(unsigned long freq, long int& sec, long int& nsec)
{
  if (freq == 0)
    return false;

  sec = 1 / freq;
  nsec = (kNanoPerSec / freq) % kNanoPerSec;

  if (sec == 0 && nsec == 0) {
    monoTimerHint("requested timer frequency is too high for timerfd",
                  "Try a lower value (MusE normally uses 1024 to 4096.");
    return false;
  }
  return true;
}

MonoTimer::MonoTimer()
{
  timerFd = -1;
  timerResolution = kMonoTimerResolution;
}

MonoTimer::~MonoTimer()
{
  monoTimerCloseFd(timerFd);
}

bool MonoTimer::set_real_time_prio()
{
  struct sched_param param;
  memset(&param, 0, sizeof(param));
  param.sched_priority = kMonoTimerRtpPriority;
  if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
    monoTimerSysError("could not enable real-time scheduling (SCHED_FIFO)",
                      "Run as root, add your user to a realtime group, or raise "
                      "rtprio/memlock limits in /etc/security/limits.conf.");
    fprintf(stderr, "  Requested priority: %d\n", kMonoTimerRtpPriority);
    return false;
  }
  return true;
}

signed int MonoTimer::initTimer(unsigned long desiredFrequency)
{
  if (TIMER_DEBUG)
    printf("MonoTimer::initTimer(%lu Hz)\n", desiredFrequency);

  if (timerFd != -1) {
    monoTimerHint("initTimer() called while timer is already active",
                  "Stop and delete the existing timer before creating a new one.");
    return -1;
  }

  if (desiredFrequency == 0) {
    monoTimerHint("invalid desired timer frequency 0",
                  "Check MusE Settings -> Performance -> Sequencer timer resolution.");
    return -1;
  }

  timerFd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (timerFd == -1) {
    monoTimerSysError("could not create monotonic timer (timerfd_create)",
                      "Requires Linux with timerfd support (kernel 2.6.25 or newer).");
    return -1;
  }


  // Runtime check: clock_getres(CLOCK_MONOTONIC) returns 1ns with HIGH_RES_TIMERS,
  // or ~1/HZ (e.g. 3333333ns at HZ=300) without it. Warn if degraded.
  {
      struct timespec res;
      if (clock_getres(CLOCK_MONOTONIC, &res) == 0) {
          const long res_ns = res.tv_sec * kNanoPerSec + res.tv_nsec;
          if (res_ns > 1000L) { // worse than 1us → hrtimer not active
              fprintf(stderr,
                  "MonoTimer: WARNING: CLOCK_MONOTONIC resolution is %ld ns "
                  "(expected 1 ns with CONFIG_HIGH_RES_TIMERS=y).\n"
                  "  Timer intervals will be rounded to ~%ld us. "
                  "MIDI timing may suffer at high tick rates.\n"
                  "  Hint: check CONFIG_HIGH_RES_TIMERS in your kernel config.\n",
                  res_ns, res_ns / 1000L);
          }
      }
  }

  if (!setTimerFreq(desiredFrequency)) {
    monoTimerHint("could not configure timer frequency during initialization",
                  "Try another sequencer timer resolution in MusE Settings, "
                  "or fall back to another timer backend.");
    monoTimerCloseFd(timerFd);
    return -1;
  }

  if (!startTimer()) {
    monoTimerMsg("timer self-test failed: could not start timer.");
    monoTimerCloseFd(timerFd);
    return -1;
  }
  if (!stopTimer()) {
    monoTimerMsg("timer self-test failed: could not stop timer.");
    monoTimerCloseFd(timerFd);
    return -1;
  }

  if (TIMER_DEBUG)
    printf("MonoTimer: initialized at %lu Hz (fd=%d)\n", getTimerFreq(), timerFd);

  return timerFd;
}

unsigned long MonoTimer::setTimerResolution(unsigned long resolution)
{
  if (TIMER_DEBUG)
    printf("MonoTimer::setTimerResolution(%lu)\n", resolution);

  if (resolution == 0) {
    monoTimerMsg("invalid timer resolution 0.");
    return 0;
  }

  // timerfd supports 4096 Hz and other MusE rtc tick rates as intervals
  timerResolution = kMonoTimerResolution;
  return timerResolution;
}

unsigned long MonoTimer::setTimerFreq(unsigned long freq)
{
  if (timerFd == -1) {
    monoTimerMsg("setTimerFreq() called before timer was initialized.");
    return 0;
  }

  if (freq == 0) {
    monoTimerHint("invalid timer frequency 0",
                  "MusE timer rates are usually 1024, 2048, 4096, 8192, 16384, or 32768 Hz.");
    return 0;
  }

  if (TIMER_DEBUG)
    printf("MonoTimer::setTimerFreq(%lu Hz)\n", freq);

  struct itimerspec new_value;
  long int sec = 0;
  long int nsec = 0;
  if (!monoTimerFreqToInterval(freq, sec, nsec))
    return 0;

  new_value.it_value.tv_sec = 0;
  new_value.it_value.tv_nsec = 0;
  new_value.it_interval.tv_sec = sec;
  new_value.it_interval.tv_nsec = nsec;

  if (timerfd_settime(timerFd, 0, &new_value, nullptr) == -1) {
    fprintf(stderr,
            "MonoTimer: could not set timer interval for %lu Hz (interval %ld s + %ld ns): %s\n",
            freq, sec, nsec, strerror(errno));
    fprintf(stderr,
            "  Hint: Choose a timer resolution from MusE Settings -> Performance.\n");
    return 0;
  }

  const unsigned long actual = getTimerFreq();
  if (actual != 0 && actual + 1 < freq) {
    fprintf(stderr,
            "MonoTimer: requested %lu Hz, timer reports %lu Hz (rounding due to nanosecond resolution).\n",
            freq, actual);
  }

  return actual;
}

unsigned long MonoTimer::getTimerResolution()
{
  return timerResolution;
}

unsigned long MonoTimer::getTimerFreq()
{
  if (timerFd == -1)
    return 0;

  struct itimerspec curr_value;
  if (timerfd_gettime(timerFd, &curr_value) == -1) {
    monoTimerSysError("could not read current timer settings");
    return 0;
  }

  if (curr_value.it_interval.tv_sec == 0 && curr_value.it_interval.tv_nsec == 0)
    return 0;

  const long int nsec = kNanoPerSec * curr_value.it_interval.tv_sec +
                        curr_value.it_interval.tv_nsec;
  if (nsec <= 0)
    return 0;

  return kNanoPerSec / nsec;
}

bool MonoTimer::startTimer()
{
  if (TIMER_DEBUG)
    printf("MonoTimer::startTimer()\n");

  if (timerFd == -1) {
    monoTimerMsg("startTimer() called before timer was initialized.");
    return false;
  }

  struct itimerspec curr_value;
  if (timerfd_gettime(timerFd, &curr_value) == -1) {
    monoTimerSysError("could not read timer interval before start");
    return false;
  }

  if (curr_value.it_interval.tv_sec == 0 && curr_value.it_interval.tv_nsec == 0) {
    monoTimerHint("timer interval is not configured",
                  "Call setTimerFreq() or initTimer() before startTimer().");
    return false;
  }

  curr_value.it_value.tv_sec = curr_value.it_interval.tv_sec;
  curr_value.it_value.tv_nsec = curr_value.it_interval.tv_nsec;
  if (timerfd_settime(timerFd, 0, &curr_value, nullptr) == -1) {
    monoTimerSysError("could not start timer");
    return false;
  }
  return true;
}

bool MonoTimer::stopTimer()
{
  if (TIMER_DEBUG)
    printf("MonoTimer::stopTimer()\n");

  if (timerFd == -1) {
    monoTimerMsg("stopTimer() called before timer was initialized.");
    return false;
  }

  struct itimerspec curr_value;
  if (timerfd_gettime(timerFd, &curr_value) == -1) {
    monoTimerSysError("could not read timer state before stop");
    return false;
  }

  curr_value.it_value.tv_sec = 0;
  curr_value.it_value.tv_nsec = 0;
  if (timerfd_settime(timerFd, 0, &curr_value, nullptr) == -1) {
    monoTimerSysError("could not stop timer");
    return false;
  }
  return true;
}

unsigned long MonoTimer::getTimerTicks(bool printTicks)
{
  if (TIMER_DEBUG)
    printf("MonoTimer::getTimerTicks()\n");

  if (timerFd == -1) {
    monoTimerMsg("getTimerTicks() called before timer was initialized.");
    return 0;
  }

  uint64_t nn = 0;
  const ssize_t nread = read(timerFd, &nn, sizeof(nn));
  if (nread == sizeof(nn)) {
    if (printTicks)
      fprintf(stderr, "MonoTimer: ticks = %lu\n", (unsigned long)nn);
    return nn;
  }

  if (nread == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      if (TIMER_DEBUG)
        monoTimerMsg("no timer tick available yet.");
      return 0;
    }
    monoTimerSysError("could not read timer ticks");
    return 0;
  }

  if (nread == 0) {
    monoTimerMsg("unexpected end-of-file while reading timer ticks.");
    return 0;
  }

  monoTimerHint("short read from timer device",
                "The timer may have been closed or is in an inconsistent state.");
  return 0;
}

} // namespace MusECore

#endif // _WIN32
