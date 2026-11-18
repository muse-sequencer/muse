//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: grepmidi.cpp,v 1.1.1.1.2.1 2009/03/09 02:05:17 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//
//  Qt conversion by Tim.
//  (C) Copyright 2025 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QtGlobal>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QByteArray>

#include "libs/file/file.h"

#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#ifdef Q_PROCESSOR_X86
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

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
#define BE_SHORT(x) XCHG_SHORT(x)
#define BE_LONG(x) XCHG_LONG(x)
#else
#define BE_SHORT(x) x
#define BE_LONG(x) x
#endif


int cpos;
bool printName = false;
QString curName;

//---------------------------------------------------------
//   readLong
//---------------------------------------------------------

int readLong(QIODevice *f, int *res = nullptr)
      {
      int format;
      qint64 rv = f->read((char*)&format, 4);
      if (rv != 4) {
            printf("read long failed\n");
            if(res)
              *res = -7;
            return 0;
            }
      cpos += 4;
      if(res)
        *res = 0;
      return BE_LONG(format);
      }

//---------------------------------------------------------
//   readShort
//---------------------------------------------------------

int readShort(QIODevice *f, int *res = nullptr)
      {
      short format;
      qint64 rv = f->read((char*)&format, 2);
      if (rv != 2) {
            printf("read short failed\n");
            if(res)
              *res = -8;
            return 0;
            }
      cpos += 2;
      if(res)
        *res = 0;
      return BE_SHORT(format);
      }

/*---------------------------------------------------------
 *    getvl
 *    Read variable-length number (7 bits per byte, MSB first)
 *---------------------------------------------------------*/

int getvl(QIODevice *f)
      {
      int l = 0;
      for (int i = 0;i < 8; i++) {
            char c;
            const bool rv = f->getChar(&c);
            ++cpos;
            if (!rv)
                  break;
            l += (c & 0x7f);
            if (!(c & 0x80))
                  return l;
            l <<= 7;
            }
      printf("Variable Len too long\n");
      return -9;
      }

//---------------------------------------------------------
//   skip
//---------------------------------------------------------

bool skip(QIODevice *f, qint64 n)
      {
        ++cpos;
        if (f->skip(n) != n) {
              printf("skip %lld failed\n", n);
              return false;
              }
        return true;
      }

//---------------------------------------------------------
//   grepTrack
//---------------------------------------------------------

int grepTrack(QIODevice *f, int trackno)
      {
//      printf("TRACK %d\n", trackno);
      int mtype, mlen;
      char* buffer;

      QByteArray ba = f->read(4);
      if (ba != "MTrk")
            return -5;
      int res;
      int len = readLong(f, &res);
      if(res != 0)
        return res;
      int endpos = cpos + len;
      int runstate = -1;

      for (;;) {
            res = getvl(f);
            if(res < 0)
              return res;
            int me;
            char c;
            f->getChar(&c);
            me = c;
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
                        f->getChar(&c);
                        ++cpos;
                  // NOTE: Error suppressor for new gcc 7 'fallthrough' level 3 and 4:
                  // FALLTHROUGH
                  case 0xc0:
                  case 0xd0:
                        if (a == -1) {
                              f->getChar(&c);
                              a = c;
                              ++cpos;
                              }
                        runstate = me;
                        break;
                  case 0xf0:
                        switch(me & 0xf) {
                              case 0:   // SYSEX
                                    mlen = getvl(f);
                                    if(mlen < 0)
                                      return mlen;
                                    if(!skip(f, mlen))
                                      return -10;
                                    break;
                              case 1:
                              case 2:
                              case 3:
                              case 4:
                              case 5:
                              case 6:
                                    if (printName)
                                          printf("%s: ", curName.toLocal8Bit().constData());
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
                                          printf("%s: ", curName.toLocal8Bit().constData());
                                    printf("RT Message??\n");
                                    break;
                              case 0xf:   // META
                                    f->getChar(&c);
                                    mtype = c;
                                    ++cpos;
                                    mlen  = getvl(f);
                                    if(mlen < 0)
                                      return mlen;
                                    buffer = new char[mlen+1];
                                    if (mlen) {
                                          qint64 rv = f->read(buffer, mlen);
                                          if (rv != mlen) {
                                                if (printName)
                                                      printf("%s: ", curName.toLocal8Bit().constData());
                                                printf("---meta %d too short (%d)\n", mtype, mlen);
                                                return -11;
                                                }
                                          cpos += mlen;
                                          }
                                    switch(mtype) {
                                          case 0x2f:
                                                delete[] buffer;
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
                                                      printf("%s: ", curName.toLocal8Bit().constData());
                                                printf("%02d Meta %0d: <%s>\n", trackno, mtype, buffer);
                                          default:
                                                break;
                                          }
                                    delete[] buffer;
                                    break;
                              }
                        break;
                  }
            }
end:
      if (cpos != endpos) {
            if (printName)
                  printf("%s: ", curName.toLocal8Bit().constData());
            printf("   %d zu kurz\n", cpos - endpos);
            }
      if (endpos-cpos > 0)
            skip(f, endpos-cpos);
      return 0;
      }

//---------------------------------------------------------
//   grep
//---------------------------------------------------------

int grep(QIODevice *f)
      {
      QByteArray ba = f->read(4);
      if (ba != "MThd")
            return -2;
      int res;
      int len = readLong(f, &res);
      if(res < 0)
        return res;
      if (len < 6)
            return -3;
      cpos += 8;
      int format     = readShort(f, &res);
      if(res < 0)
        return res;
      int ntracks    = readShort(f, &res);
      if(res < 0)
        return res;
      readShort(f, &res);     // division
      if(res < 0)
        return res;
      if (len > 6)
            if(!skip(f, len-6)) /* skip the excess */
              return -10;
      int rv = 0;
      switch (format) {
            case 0:
                  rv = grepTrack(f, 0);
                  if (rv != 0)
                        return rv;
                  break;
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

int grepMidi(const QString &name)
      {
      curName = name;

      MusEFile::File f(name, QString());
      if(!f.open(QIODevice::ReadOnly))
            return -1;

      cpos = 0;
      int rc = grep(f.iodevice());
      f.close();
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
      QCoreApplication app(argc, argv);
      QCoreApplication::setApplicationName("grepmidi");
      QCoreApplication::setApplicationVersion("1.0");

      QCommandLineParser parser;
      parser.setApplicationDescription("Print a summary of the contents of midi files.");
      parser.addHelpOption();
      parser.addVersionOption();
      parser.addPositionalArgument("files", QCoreApplication::translate("main", "MIDI files to examine.", "[files...]"));


      QCommandLineOption printFilenameOption("f", QCoreApplication::translate("main", "Print filename along with messages."));
      parser.addOption(printFilenameOption);

      parser.process(app);

      printName = parser.isSet(printFilenameOption);

      const QStringList args = parser.positionalArguments();
      const int numargs = args.size();

      const char* p = 0;
      for (int i = 0; i < numargs; ++i)
      {
            switch (grepMidi(args.at(i)))
            {
                  case 0:     break;
                  case -1:    p = "not found"; break;
                  case -2:    p = "no 'MThd': not a midi file"; break;
                  case -3:    p = "file too short"; break;
                  case -4:    p = "bad file type"; break;
                  case -5:    p = "no 'MTrk': not a midi file"; break;
                  case -6:    p = "no running state"; break;

                  case -7:
                  case -8:
                  case -9:
                  case -10:
                  case -11:
                        return -1;
                  break;

                  default:
                        printf("unknown error\n");
                        return -1;
            }
      }

      if (p)
      {
            printf("Error in %s: <%s>\n", curName.toLocal8Bit().constData(), p);
            return -1;
      }
      return 0;
      }
