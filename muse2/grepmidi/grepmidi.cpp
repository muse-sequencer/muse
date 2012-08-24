//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: grepmidi.cpp,v 1.1.1.1.2.1 2009/03/09 02:05:17 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#ifdef __i486__
#define XCHG_LONG(x) \
     ({ int __value; \
        asm ("bswap %1; movl %1,%0" : "=g" (__value) : "r" (x)); \
       __value; })
#else
#define XCHG_LONG(x) ((((x)&0xFF)<<24) | \
		      (((x)&0xFF00)<<8) | \
		      (((x)&0xFF0000)>>8) | \
		      (((x)>>24)&0xFF))
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BE_SHORT(x) XCHG_SHORT(x)
#define BE_LONG(x) XCHG_LONG(x)
#else
#define BE_SHORT(x) x
#define BE_LONG(x) x
#endif


int cpos;
bool printName = false;
const char* curName;

//---------------------------------------------------------
//   readLong
//---------------------------------------------------------

int readLong(FILE* f)
      {
      int format;
      if (fread(&format, 4, 1, f) != 1) {
            printf("read long failed\n");
            exit(1);
            }
      cpos += 4;
      return BE_LONG(format);
      }

//---------------------------------------------------------
//   readShort
//---------------------------------------------------------

int readShort(FILE* f)
      {
      short format;
      if (fread(&format, 2, 1, f) != 1) {
            printf("read short failed\n");
            exit(1);
            }
      cpos += 2;
      return BE_SHORT(format);
      }

/*---------------------------------------------------------
 *    getvl
 *    Read variable-length number (7 bits per byte, MSB first)
 *---------------------------------------------------------*/

int getvl(FILE* f)
      {
      int l = 0;
      for (int i = 0;i < 8; i++) {
            int c = getc(f);
            ++cpos;
            if (c == EOF)
                  break;
            c &= 0xff;
            l += (c & 0x7f);
            if (!(c & 0x80))
                  return l;
            l <<= 7;
            }
      printf("Variable Len too long\n");
      return -1;
      }

//---------------------------------------------------------
//   skip
//---------------------------------------------------------

void skip(FILE* f, int n)
      {
      while (n--) {
            ++cpos;
            if (getc(f) == EOF) {
                  printf("skip %d failed\n", n);
                  exit(1);
                  }
            }
      }

//---------------------------------------------------------
//   grepTrack
//---------------------------------------------------------

int grepTrack(FILE* f, int trackno)
      {
//      printf("TRACK %d\n", trackno);
      int mtype, mlen;
      char* buffer;

      char tmp[4];
      fread(tmp, 4, 1, f);
      if (memcmp(tmp, "MTrk", 4))
            return -5;
      int len = readLong(f);
      int endpos = cpos + len;
      int runstate = -1;

      for (;;) {
/*            int nclick = */ getvl(f);
            int me     = getc(f);
            ++cpos;
            int a = -1;
            if ((me & 0x80) == 0) {
                  if (runstate == -1)
                        return -6;
                  a  = me;
                  me = runstate;
                  }
            switch (me & 0xf0) {
                  case 0x80:
                  case 0x90:
                  case 0xa0:
                  case 0xb0:
                  case 0xe0:
                        getc(f);
                        ++cpos;
                  case 0xc0:
                  case 0xd0:
                        if (a == -1) {
                              a = getc(f);
                              ++cpos;
                              }
                        runstate = me;
                        break;
                  case 0xf0:
                        switch(me & 0xf) {
                              case 0:   // SYSEX
                                    mlen = getvl(f);
                                    skip(f, mlen);
                                    break;
                              case 1:
                              case 2:
                              case 3:
                              case 4:
                              case 5:
                              case 6:
                                    if (printName)
                                          printf("%s: ", curName);
                                    printf("unknown Message\n");
                                    break;
                              case 7:     // EOX
                              case 8:     // timing clock
                              case 9:     // undefined
                              case 0xa:   // start
                              case 0xb:   // continue
                              case 0xc:   // stop
                              case 0xd:   // undefined
                              case 0xe:   // active sensing
                                    if (printName)
                                          printf("%s: ", curName);
                                    printf("RT Message??\n");
                                    break;
                              case 0xf:   // META
                                    mtype = getc(f);
                                    ++cpos;
                                    mlen  = getvl(f);
                                    buffer = new char[mlen+1];
                                    if (mlen) {
                                          if (fread(buffer, mlen, 1, f) != 1) {
                                                if (printName)
                                                      printf("%s: ", curName);
                                                printf("---meta %d too short (%d)\n", mtype, mlen);
                                                exit(1);
                                                }
                                          cpos += mlen;
                                          }
                                    switch(mtype) {
                                          case 0x2f:
                                                delete buffer;
                                                goto end;
                                          case 1:
                                          case 2:
                                          case 3:
                                          case 4:
                                          case 5:
                                          case 6:  // Marker
                                          case 7:
                                          case 8:
                                          case 9:
                                          case 10:
                                          case 11:
                                          case 12:
                                          case 13:
                                          case 14:
                                          case 15:
                                                buffer[mlen] = 0;
                                                if (printName)
                                                      printf("%s: ", curName);
                                                printf("%02d Meta %0d: <%s>\n", trackno, mtype, buffer);
                                          default:
                                                break;
                                          }
                                    delete buffer;
                                    break;
                              }
                        break;
                  }
            }
end:
      if (cpos != endpos) {
            if (printName)
                  printf("%s: ", curName);
            printf("   %d zu kurz\n", cpos - endpos);
            }
      if (endpos-cpos > 0)
            skip(f, endpos-cpos);
      return 0;
      }

//---------------------------------------------------------
//   grep
//---------------------------------------------------------

int grep(FILE* f)
      {
      char tmp[4];

      fread(tmp, 4, 1, f);
      if (memcmp(tmp, "MThd", 4) != 0)
            return -2;
      int len = readLong(f);
      if (len < 6)
            return -3;
      cpos += 8;
      int format     = readShort(f);
      int ntracks    = readShort(f);
      readShort(f);     // division
      if (len > 6)
            skip(f, len-6); /* skip the excess */
      int rv = 0;
      switch (format) {
            case 0: rv = grepTrack(f, 0); break;
            case 1:
                  for (int i = 0; i < ntracks; i++) {
                        rv = grepTrack(f, i);
                        if (rv != 0)
                              return rv;
                        }
                  break;
            default:
                  return -4;
            }

      return 0;
      }

//---------------------------------------------------------
//   grep
//---------------------------------------------------------

int grepMidi(const char* name)
      {
      curName = name;
      const char*p = strrchr(name, '.');
      FILE* f;
      if (p && strcmp(p, ".gz") == 0) {
            char buffer[512];
            sprintf(buffer, "gunzip < %s", name);
            f = popen(buffer, "r");
            }
      else {
            p = 0;
            f = fopen(name, "r");
            }
      if (f == 0)
            return -1;
      cpos = 0;
      int rc = grep(f);
      if (p)
            pclose(f);
      else
            fclose(f);
      return rc;
      }

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

void usage(const char* fname, const char* txt)
      {
      fprintf(stderr, "%s: %s\n", fname, txt);
//      fprintf(stderr, "usage:\n");
      }

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
      {
      int c;
      while ((c = getopt(argc, argv, "f")) != EOF) {
            switch (c) {
                  case 'f': printName = true; break;
                  default:  usage(argv[0], "bad argument"); return -1;
                  }
            }
      argc -= optind;
      ++argc;
      const char* p = 0;
      for (int i = 1; i < argc; ++i) {
            switch (grepMidi(argv[i])) {
                  case 0:     break;
                  case -1:    p = "not found"; break;
                  case -2:    p = "no 'MThd': not a midi file"; break;
                  case -3:    p = "file too short"; break;
                  case -4:    p = "bad file type"; break;
                  case -5:    p = "no 'MTrk': not a midi file"; break;
                  case -6:    p = "no running state"; break;
                  default:
                        printf("was??\n");
                        return -1;
                  }
            }
      if (p)
            printf("Error: <%s>\n", p);
      return 0;
      }
