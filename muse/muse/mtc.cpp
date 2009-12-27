//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mtc.cpp,v 1.1.1.1 2003/10/27 18:51:48 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#include "mtc.h"
#include <stdio.h>

extern int mtcType;

//---------------------------------------------------------
//   MTC::time
//    converts MTC Time to seconds according to
//    global mtcType
//---------------------------------------------------------

double MTC::time() const
      {
      double time = _h * 3600 + _m * 60 + _s;
      double ft = 0.0;
      switch (mtcType) {
            case 0:     // 24 frames sec
                  ft = 1.0/24.0;
                  break;
            case 1:     // 25
                  ft = 0.04;
                  break;
            case 2:     // 30 drop frame        TODO
            case 3:     // 30 non drop frame
                  ft = 1.0/30.0;
                  break;
            }
      return time + ft *_f + 0.01 * ft * _sf;
      }

//---------------------------------------------------------
//   MTC
//---------------------------------------------------------

MTC::MTC(double t)
      {
      _h  = int(t/3600);
      t -= _h * 3600;
      _m  = int(t/60);
      t -= _m * 60;
      _s  = int(t);
      t -= _s;
      double ft = 1.0/24.0;
      switch (mtcType) {
            case 0:     // 24 frames sec
                  ft = 1.0/24.0;
                  break;
            case 1:     // 25
                  ft = 0.04;
                  break;
            case 2:     // 30 drop frame
            case 3:     // 30 non drop frame
                  ft = 1.0/30.0;
                  break;
            }
      double frames = t / ft;
      _f  = int(frames);
      frames -= _f;
      _sf = int(frames * 100);
      }

//---------------------------------------------------------
//   incQuarter
//    increment MTC time one quarter frame time
//---------------------------------------------------------

void MTC::incQuarter()
      {
      int frames = 24;
      switch (mtcType) {
            case 0:
                  frames = 24;
                  break;
            case 1:
                  frames = 25;
                  break;
            case 2:
            case 3:
                  frames = 30;
                  break;
            }
      _sf += 25;
      if (_sf >= 100) {
            ++_f;
            _sf -= 100;
            }
      if (_f == frames) {
            ++_s;
            _f = 0;
            }
      if (_s == 60) {
            ++_m;
            _s = 0;
            }
      if (_m == 60) {
            ++_h;
            _m = 0;
            }
      if (_h == 24) {
            _h = 0;
            }
      }

//---------------------------------------------------------
//   print
//---------------------------------------------------------

void MTC::print() const
      {
      printf("%02d:%02d:%02d:%02d:%02d", _h, _m, _s, _f, _sf);
      }


