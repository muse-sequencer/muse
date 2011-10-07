//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: siglabel.cpp,v 1.1.1.1 2003/10/27 18:54:28 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
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

#include "siglabel.h"
#include <stdio.h>

#define TIMER1    400
#define TIMER2    200
#define TIMEC     7
#define TIMER3    100
#define TIMEC2    20
#define TIMER4    50

#include "globals.h"
#include "gconfig.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>

namespace MusEGui {

//---------------------------------------------------------
//   SigLabel
//    edit Signature Values  (4/4)
//---------------------------------------------------------

SigLabel::SigLabel(int zz, int nn, QWidget* parent) : QLabel(parent)
      {
      z = n = 0;  
      setFocusPolicy(Qt::NoFocus);
      setAlignment(Qt::AlignCenter);
      setValue(zz, nn);
      }

SigLabel::SigLabel(const AL::TimeSignature& sig, QWidget* parent) : QLabel(parent)
      {
      z = n = 0;
      setFocusPolicy(Qt::NoFocus);
      setAlignment(Qt::AlignCenter);
      setValue(sig.z, sig.n);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void SigLabel::mousePressEvent(QMouseEvent* event)
      {
      int button = event->button();
      bool zaehler = event->x() < width() /2;

      int zz = z, nn = n;
      switch (button) {
            case Qt::LeftButton:
                  if (!MusEGlobal::config.leftMouseButtonCanDecrease)
                    return;
                  // else fall through
            case Qt::MidButton:
                  incValue(zaehler, false, zz, nn);
                  break;
            case Qt::RightButton:
                  incValue(zaehler, true, zz, nn);
                  break;
            default:
                  break;
            }
      if ((zz != z) || (nn != n)) {
            setValue(zz, nn);
            emit valueChanged(AL::TimeSignature(zz, nn));
            }
      }

//---------------------------------------------------------
//   incValue
//---------------------------------------------------------

void SigLabel::incValue(bool zaehler, bool up, int& zz, int& nn)
      {
      if (!up) {
            if (zaehler) {
                  --zz;
                  if (zz < 1)
                        zz = 1;
                  }
            else {
                  switch (nn) {
                        case 1:    break;
                        case 2:    nn = 1; break;
                        case 4:    nn = 2; break;
                        case 8:    nn = 4; break;
                        case 16:   nn = 8; break;
                        case 32:   nn = 16; break;
                        case 64:   nn = 32; break;
                        case 128:  nn = 64; break;
                        }
                  }
            }
      else {
            if (zaehler) {
                  ++zz;
                  if (zz > 16)
                        zz = 16;
                  }
            else {
                  switch (nn) {
                        case 1:     nn = 2; break;
                        case 2:     nn = 4; break;
                        case 4:     nn = 8; break;
                        case 8:     nn = 16; break;
                        case 16:    nn = 32; break;
                        case 32:    nn = 64; break;
                        case 64:    nn = 128; break;
                        case 128:   break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void SigLabel::wheelEvent(QWheelEvent* event)
      {
      bool zaehler = event->x() < width() /2;
      int delta = event->delta();
      int zz = z, nn = n;

      bool inc = delta >= 0;
      incValue(zaehler, inc, zz, nn);
      if ((zz != z) || (nn != n)) {
            setValue(zz, nn);
            emit valueChanged(AL::TimeSignature(zz, nn));
            }
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void SigLabel::setValue(int a, int b)
      {
      if (a == z && b == n)
            return;
      z = a;
      n = b;
      QString sa;
      sa.setNum(a);

      QString sb;
      sb.setNum(b);

      QString s = sa + QString("/") + sb;
      setText(s);
      }

//---------------------------------------------------------
//   setFrame
//---------------------------------------------------------

void SigLabel::setFrame(bool flag)
      {
      setFrameStyle(flag ? Panel | Sunken : NoFrame);
      setLineWidth(2);
      }

} // namespace MusEGui
