/*
 * COPYRIGHT
 *
 * This file is part of Mustajuuri GPL modules. You may distribute it
 * with or without other Mustajuuri components.
 *
 * Author: Tommi Ilmonen, 2001.
 * Tommi.Ilmonen@hut.fi
 *
 * http://www.tml.hut.fi/~tilmonen/mustajuuri/

 * This app also has its own home page at (installation instruction
 * etc.): http://www.tml.hut.fi/~tilmonen/givertcap/

 * This file is licensed under the GNU Public License (GPL) version
 * 2. The GPL can also be found from the givertcap home page. Any
 * application may call civertcap (regardless of the license of the
 * calling application).

 * If you want a parallel license (for commercial reasons for example),
 * you should negotiate the matter with the author(s).
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#undef _POSIX_SOURCE
#include <sys/capability.h>
/* If the compilation fails on the preceding line, then you probably
   do not have the libcap installed.

*/

static void usage(const char *programName)
{
  fprintf(stderr,
 	  "usage: %s \n\n"
	  "  This program gives real-time application capabilities to the"
	  "  parent process\n\n"
	  "[Copyright (c) 2001 Tommi Ilmonen <Tommi.Ilmonen@hut.fi>]\n"
	  "Home page: http://www.tml.hut.fi/~tilmonen/givertcap/\n",
	  programName);
}

int main(int argc, char **argv)
{
  if(argc > 1) {
    usage(argv[0]);
    return 1;
  }

  pid_t parentPid = getppid();

  if(!parentPid)
    return 1;

  cap_t caps = cap_init();

#define nofCaps 3

  /* We need these capabilities:

     CAP_SYS_NICE      -> Real-time priority
     CAP_SYS_RESOURCE  -> RTC above 64 Hz
     CAP_IPC_LOCK      -> mlockall
  */

  cap_value_t capList[nofCaps] =
  { CAP_SYS_NICE, CAP_SYS_RESOURCE, CAP_IPC_LOCK} ;

  cap_clear(caps);
  cap_set_flag(caps, CAP_EFFECTIVE,   nofCaps, capList , CAP_SET);
  cap_set_flag(caps, CAP_INHERITABLE, nofCaps, capList , CAP_SET);
  cap_set_flag(caps, CAP_PERMITTED,   nofCaps, capList , CAP_SET);

  /* If your COMPILATION FAILS here then you probably are not running
     Linux. the function "capsetp" is not part of the POSIX capability
     standard, but a Linux-specific extension. */
  if (capsetp(parentPid, caps)) {
    perror("mjsucaps: capsetp");
    return 1;
  }

  ssize_t x;
//  printf("The process %d was give capabilities %s\n",
//	 (int) parentPid, cap_to_text(caps, &x));
  fflush(0);

  // Don't bother to free the memory...

  return 0;
}
