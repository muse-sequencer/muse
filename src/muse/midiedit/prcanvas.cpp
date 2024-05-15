//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: prcanvas.cpp,v 1.20.2.19 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
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

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QList>
#include <QPair>
#include <QToolTip>
#include <QUuid>

#include <stdio.h>
#include "muse_math.h"
// REMOVE Tim. wave. Added.
#include "muse_time.h"

#include "prcanvas.h"
#include "globals.h"
#include "cmd.h"
#include "song.h"
#include "audio.h"
#include "functions.h"
#include "gconfig.h"
#include "helper.h"
#include "app.h"
#include "type_defs.h"

// Forwards from header:
#include <QDragMoveEvent>
#include <QPainter>
#include <QWidget>
#include "part.h"
#include "event.h"
#include "midieditor.h"
#include "citem.h"
#include "scrollscale.h"
#include "steprec.h"

#define CHORD_TIMEOUT 75

namespace MusEGui {

// Static. This is how thick our event borders are.
int PianoCanvas::eventBorderWidth = 1;

//---------------------------------------------------------
//   NEvent
//---------------------------------------------------------

NEvent::NEvent(const MusECore::Event& e, MusECore::Part* p, int y) : EItem(e, p)
      {
      y = y - KH/4;
      unsigned tick = e.tick() + p->tick();
      setPos(QPoint(tick, y));
      setBBox(QRect(tick, y, e.lenTick(), KH/2));
      // Give the moving point an initial value.
      setMp(pos());
      }



void PianoCanvas::setLastEdited(MusECore::Event& e)
{
    if (lastEditedEvent==nullptr)
        lastEditedEvent = new MusECore::Event();
    *lastEditedEvent = e.clone();
}

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

CItem* PianoCanvas::addItem(MusECore::Part* part, const MusECore::Event& event)
      {
//       if (signed(event.tick())<0) {
//             printf("ERROR: trying to add event before current part!\n");
//             return nullptr;
//       }

      NEvent* ev = new NEvent(event, part, pitch2y(event.pitch()));
      items.add(ev);

      return ev;
      }

//---------------------------------------------------------
//   PianoCanvas
//---------------------------------------------------------

PianoCanvas::PianoCanvas(MidiEditor* pr, QWidget* parent, int sx, int sy)
   : EventCanvas(pr, parent, sx, sy)
      {
      setObjectName("Pianoroll");
      colorMode = MidiEventColorMode::blueEvents;
      for (int i=0;i<128;i++) noteHeldDown[i]=false;
      supportsResizeToTheLeft = true;
      supportsMultipleResize = true;
      setStatusTip(tr("Pianoroll canvas: Use Pencil tool to draw and edit MIDI events, Pointer tool to select and edit. Press F1 for help."));

      steprec=new MusECore::StepRec(noteHeldDown);

      songChanged(SC_TRACK_INSERTED);
      connect(MusEGlobal::song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));
      }

PianoCanvas::~PianoCanvas()
{
  delete steprec;
}
//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int PianoCanvas::pitch2y(int pitch) const
      {
      int tt[] = {
            5, 13, 19, 26, 34, 44, 52, 58, 65, 71, 78, 85
            };
      int y = (75 * KH) - (tt[pitch%12] + (7 * KH) * (pitch/12));
      if (y < 0)
            y = 0;
      return y;
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int PianoCanvas::y2pitch(int y) const
      {
    if (y < KH)
        return 127;
    const int total = (10 * 7 + 5) * KH;       // 75 Ganztonschritte
      y = total - y;
      if (y < 0)
          return 0;
      int oct = (y / (7 * KH)) * 12;
      char kt[] = {
            0, 0, 0, 0, 0,  0,   0, 0, 0, 0,          // 5
            1, 1, 1,      1,   1, 1, 1,               // 13
            2, 2,         2,   2, 2, 2,               // 19
            3, 3, 3,      3,   3, 3, 3,               // 26
            4, 4, 4, 4,   4,   4, 4, 4, 4,            // 34
            5, 5, 5, 5,   5,   5, 5, 5, 5, 5,         // 43
            6, 6, 6,      6,   6, 6, 6,               // 52
            7, 7,         7,   7, 7, 7,               // 58
            8, 8, 8,      8,   8, 8, 8,               // 65
            9, 9,         9,   9, 9, 9,               // 71
            10, 10, 10,  10,   10, 10, 10,            // 78
            11, 11, 11, 11, 11,   11, 11, 11, 11, 11  // 87
            };
      return kt[y % 91] + oct;
      }


// REMOVE Tim. citem. Changed. The original code...
// //---------------------------------------------------------
// //   drawEvent
// //    draws a note
// //---------------------------------------------------------
//
// void PianoCanvas::drawItem(QPainter& p, const MusEGui::CItem* item,
//    const QRect& rect)
//       {
//       QRect r = item->bbox();
//       if(!virt())
//         r.moveCenter(map(item->pos()));
//
//       //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
//       QRect rr = map(rect);                      // Use our own map instead.
//       QRect mer = map(r);
//
//       QRect mr = rr & mer;
//       if(mr.isNull())
//         return;
//
//       p.setPen(Qt::black);
//       struct Triple {
//             int r, g, b;
//             };
//
//       static Triple myColors [12] = {  // ddskrjp
//             { 0xff, 0x3d, 0x39 },
//             { 0x39, 0xff, 0x39 },
//             { 0x39, 0x3d, 0xff },
//             { 0xff, 0xff, 0x39 },
//             { 0xff, 0x3d, 0xff },
//             { 0x39, 0xff, 0xff },
//             { 0xff, 0x7e, 0x7a },
//             { 0x7a, 0x7e, 0xff },
//             { 0x7a, 0xff, 0x7a },
//             { 0xff, 0x7e, 0xbf },
//             { 0x7a, 0xbf, 0xff },
//             { 0xff, 0xbf, 0x7a }
//             };
//
//       QColor color;
//       NEvent* nevent   = (NEvent*) item;
//       MusECore::Event event = nevent->event();
//       if (nevent->part() != curPart){
//             if(item->isMoving())
//               color = Qt::gray;
//             else if(item->isSelected())
//               color = Qt::black;
//             else
//               color = Qt::lightGray;
//       }
//       else {
//             if (item->isMoving()) {
//                     color = Qt::gray;
//                 }
//             else if (item->isSelected()) {
//                   color = Qt::black;
//                   }
//             else {
//                   color.setRgb(0, 0, 255);
//                   switch(colorMode) {
//                         case 0:
//                               break;
//                         case 1:     // pitch
//                               {
//                               Triple* c = &myColors[event.pitch() % 12];
//                               color.setRgb(c->r, c->g, c->b);
//                               }
//                               break;
//                         case 2:     // velocity
//                               {
//                               int velo = event.velo();
//                               if (velo < 64)
//                                     color.setRgb(velo*4, 0, 0xff);
//                               else
//                                     color.setRgb(0xff, 0, (127-velo) * 4);
//                               }
//                               break;
//                         }
//                   }
//             }
//
//       bool wmtxen = p.worldMatrixEnabled();
//       p.setWorldMatrixEnabled(false);
//       int mx = mr.x();
//       int my = mr.y();
//       int mw = mr.width();
//       int mh = mr.height();
//       int mex = mer.x();
//       int mey = mer.y();
//       int mew = mer.width();
//       int meh = mer.height();
//       color.setAlpha(MusEGlobal::config.globalAlphaBlend);
//       QBrush brush(color);
//       p.fillRect(mr, brush);
//
//       if(mex >= mx && mex <= mx + mw)
//         p.drawLine(mex, my, mex, my + mh - 1);                       // The left edge
//       if(mex + mew >= mx && mex + mew <= mx + mw)
//         p.drawLine(mex + mew, my, mex + mew, my + mh - 1);           // The right edge
//       if(mey >= my && mey <= my + mh)
//         p.drawLine(mx, mey, mx + mw - 1, mey);                       // The top edge
//       if(mey + meh >= my && mey + meh <= my + mh)
//         p.drawLine(mx, mey + meh - 1, mx + mw - 1, mey + meh - 1);   // The bottom edge
//
//       // print note name on the drawn notes
//       if (MusEGlobal::config.showNoteNamesInPianoRoll) {
//         QFont f(MusEGlobal::config.fonts[1]);
//
//         f.setPointSize(f.pointSize() * 0.85);
//         p.setFont(f);
//
//         if (color.lightnessF() > 0.6f) {
//
//           p.setPen(Qt::black);
//
//         } else {
//
//           p.setPen(Qt::white);
//
//         }
//         QString noteStr = MusECore::pitch2string(event.pitch());
//
//         p.drawText(mer,Qt::AlignHCenter|Qt::AlignCenter, noteStr.toUpper());
//       }
//
//
//       p.setWorldMatrixEnabled(wmtxen);
//       }


#if 1

// Unmapped version works OK. But let's use the mapped version below for now...
// The "mapped version" causes drawing artefacts, at least with HiDPI.
// This one is OK, and probably faster too (kybos)
//---------------------------------------------------------
//   drawEvent
//    draws a note
//---------------------------------------------------------

void PianoCanvas::drawItem(QPainter& p, const MusEGui::CItem* item,
   const QRect& mr, const QRegion&)
      {
      MusECore::MidiPart* part = (MusECore::MidiPart*)(item->part());

// REMOVE Tim. wave. Added. Diagnostics.
      fprintf(stderr, "PianoCanvas::drawItem part:%p part->track():%p\n", part, part ? part->track() : nullptr);

      if(!part /*|| !part->track()*/)
        return;

      const MusECore::Pos::TType partTType = part->type();
      const MusECore::Pos::TType canvasTType = MusECore::Pos::TICKS;

      QRect ur = mapDev(mr).adjusted(0, 0, 0, 1);

// REMOVE Tim. wave. Added. Moved from below.
      NEvent* nevent   = (NEvent*) item;
      const MusECore::Event event = nevent->event();
      if(event.empty())
        return;
      const MusECore::Pos::TType eTType = event.pos().type();

      // This is the update comparison rectangle. This would normally be the same as the item's bounding rectangle
      //  but in this case we have a one-pixel wide border. To accommodate for our border, expand the right edge right
      //  by one, and the bottom edge down by one. This way we catch the full
      //  necessary drawing rectangle when checking the requested update rectangle.
      // Note that this is units of ticks.
// REMOVE Tim. wave. Changed.
//       QRect ubbr = item->bbox().adjusted(0, 0, 0, -1);
//       if(!virt())
//         ubbr.moveCenter(map(item->pos()));
      const int pPos = item->tmpPartPos();
      const unsigned pPosEType = MusECore::Pos::convert(pPos, partTType, eTType);
      const unsigned absEPos = pPosEType + item->tmpPos();
      const unsigned eLen = item->tmpLen();
      const unsigned absEEnd = absEPos + eLen;
      const unsigned absEPosCTType = MusECore::Pos::convert(absEPos, eTType, canvasTType);
      const unsigned absEEndCTType = MusECore::Pos::convert(absEEnd, eTType, canvasTType);
      const QRect ubbr = QRect(absEPosCTType, item->bbox().y(), absEEndCTType - absEPosCTType, item->bbox().height()).adjusted(0, 0, 0, -1);


      const QRect mbbr = map(ubbr); // Use our own map instead.
      QRect ubr = ur & ubbr;

      // REMOVE Tim. wave. Added. Diagnostics.
      fprintf(stderr, "pPos:%d eLen:%d absEPos:%d absEEnd:%d absEPosCTType:%d absEEndCTType:%d\n",
              pPos, eLen, absEPos, absEEnd, absEPosCTType, absEEndCTType);
      fprintf(stderr, "bbox x:%d y:%d w:%d h:%d\n", item->bbox().x(), item->bbox().y(), item->bbox().width(), item->bbox().height());
      fprintf(stderr, "mr x:%d y:%d w:%d h:%d\n", mr.x(), mr.y(), mr.width(), mr.height());
      fprintf(stderr, "ur x:%d y:%d w:%d h:%d\n", ur.x(), ur.y(), ur.width(), ur.height());
      fprintf(stderr, "ubbr x:%d y:%d w:%d h:%d\n", ubbr.x(), ubbr.y(), ubbr.width(), ubbr.height());
      fprintf(stderr, "ubr x:%d y:%d w:%d h:%d\n", ubr.x(), ubr.y(), ubr.width(), ubr.height());

      const int ux = ur.x();
      const int uy = ur.y();
      const int uw = ur.width();
      const int uh = ur.height();
      const int ux_2 = ux + uw;
      const int uy_2 = uy + uh;

      const int ubbx = ubbr.x();
      const int ubby = ubbr.y();
      const int ubbw = ubbr.width();
      const int ubbh = ubbr.height();
      const int ubbx_2 = ubbx + ubbw;
      const int ubby_2 = ubby + ubbh;

      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);

      struct Triple {
            int r, g, b;
            };

      static Triple myColors [12] = {  // ddskrjp
            { 0xff, 0x3d, 0x39 },
            { 0x39, 0xff, 0x39 },
            { 0x39, 0x3d, 0xff },
            { 0xff, 0xff, 0x39 },
            { 0xff, 0x3d, 0xff },
            { 0x39, 0xff, 0xff },
            { 0xff, 0x7e, 0x7a },
            { 0x7a, 0x7e, 0xff },
            { 0x7a, 0xff, 0x7a },
            { 0xff, 0x7e, 0xbf },
            { 0x7a, 0xbf, 0xff },
            { 0xff, 0xbf, 0x7a }
            };

      QColor color;
//       NEvent* nevent   = (NEvent*) item;
//       MusECore::Event event = nevent->event();
      if (nevent->part() != curPart){
          if(item->isMoving())
              color = Qt::gray;
          else if(item->isSelected())
              color = Qt::black;
          else
              color = Qt::lightGray;
      }
      else {
          if (item->isMoving()) {
              color = Qt::gray;
          }
          else if (item->isSelected()) {
              color = MusEGlobal::config.midiItemSelectedColor;
          }
          else {
              color = MusEGlobal::config.midiItemColor;
              switch(colorMode) {
              case MidiEventColorMode::blueEvents:
                  break;
              case MidiEventColorMode::pitchColorEvents:
                  {
                  Triple* c = &myColors[event.pitch() % 12];
                  color.setRgb(c->r, c->g, c->b);
                  }
                  break;
              case MidiEventColorMode::velocityColorEvents:
                  {
                  int velo = event.velo();
                  if (velo < 64)
                      color.setRgb(velo*4, 0, 0xff);
                  else
                      color.setRgb(0xff, 0, (127-velo) * 4);
                  break;
                  }
              default:
                  break;
              }
          }
      }

      color.setAlpha(MusEGlobal::config.globalAlphaBlend);
      QBrush brush(color);
      if(!ubr.isEmpty())
        p.fillRect(ubr, brush);

      if(ubbx >= ux && ubbx < ux_2)
        p.drawLine(ubbx, ubby, ubbx, ubby_2);                       // The left edge

      if(ubbx_2 >= ux && ubbx_2 <= ux_2)
        p.drawLine(ubbx_2, ubby, ubbx_2, ubby_2);           // The right edge

      if(ubby >= uy && ubby < uy_2)
        p.drawLine(ubbx, ubby, ubbx_2, ubby);                       // The top edge

      if(ubby_2 >= uy && ubby_2 <= uy_2)
        p.drawLine(ubbx, ubby_2, ubbx_2, ubby_2);   // The bottom edge

      // print note name on the drawn notes
      if (!ubr.isEmpty() && MusEGlobal::config.showNoteNamesInPianoRoll) {
        QFont f(MusEGlobal::config.fonts[1]);

        f.setPointSize(f.pointSize() * 0.85);
        p.setFont(f);

        if (color.lightnessF() > 0.6f) {
          pen.setColor(Qt::black);
          p.setPen(pen);

        } else {
          pen.setColor(Qt::white);
          p.setPen(pen);
        }
        QString noteStr = MusECore::pitch2string(event.pitch());

        const bool wmtxen = p.worldMatrixEnabled();
        p.setWorldMatrixEnabled(false);
        p.drawText(mbbr,Qt::AlignHCenter|Qt::AlignCenter, noteStr.toUpper());
        p.setWorldMatrixEnabled(wmtxen);
      }

      }

#endif


#if 0
// Mapped version also works OK. (Except the text is shifted up one pixel from current release or master).
//---------------------------------------------------------
//   drawEvent
//    draws a note
//---------------------------------------------------------

void PianoCanvas::drawItem(QPainter& p, const MusEGui::CItem* item,
   const QRect& mr, const QRegion&)
      {
      QRect ur = mapDev(mr).adjusted(0, 0, 0, 1);

      QRect ubbr = item->bbox();
      // This is the update comparison rectangle. This would normally be the same as the item's bounding rectangle
      //  but in this case we have a one-pixel wide border. To accommodate for our border, expand the right edge right
      //  by one, and the bottom edge down by one. This way we catch the full
      //  necessary drawing rectangle when checking the requested update rectangle.
      // Note that this is units of ticks.
      if(!virt())
        ubbr.moveCenter(map(item->pos()));

      //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
      const QRect mbbr = map(ubbr).adjusted(0, 0, 0, -1);

      const QRect mbr = mr & mbbr;

      const bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      const int ux = ur.x();
      const int uy = ur.y();
      const int uw = ur.width();
      const int uh = ur.height();
      const int ux_2 = ux + uw;
      const int uy_2 = uy + uh;

      const int ubbx = ubbr.x();
      const int ubby = ubbr.y();
      const int ubbw = ubbr.width();
      const int ubbh = ubbr.height();
      const int ubbx_2 = ubbx + ubbw;
      const int ubby_2 = ubby + ubbh;
      const int mbbx = mapx(ubbx);
      const int mbby = mapy(ubby);
      const int mbbx_2 = mapx(ubbx_2);
      const int mbby_2 = mapy(ubby_2);

      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);

      struct Triple {
            int r, g, b;
            };

      static Triple myColors [12] = {  // ddskrjp
            { 0xff, 0x3d, 0x39 },
            { 0x39, 0xff, 0x39 },
            { 0x39, 0x3d, 0xff },
            { 0xff, 0xff, 0x39 },
            { 0xff, 0x3d, 0xff },
            { 0x39, 0xff, 0xff },
            { 0xff, 0x7e, 0x7a },
            { 0x7a, 0x7e, 0xff },
            { 0x7a, 0xff, 0x7a },
            { 0xff, 0x7e, 0xbf },
            { 0x7a, 0xbf, 0xff },
            { 0xff, 0xbf, 0x7a }
            };

      QColor color;
      NEvent* nevent   = (NEvent*) item;
      MusECore::Event event = nevent->event();
      if (nevent->part() != curPart){
            if(item->isMoving())
              color = Qt::gray;
            else if(item->isSelected())
              color = Qt::black;
            else
              color = Qt::lightGray;
      }
      else {
            if (item->isMoving()) {
                    color = Qt::gray;
                }
            else if (item->isSelected()) {
                  color = Qt::black;
                  }
            else {
                  color.setRgb(0, 0, 255);
                  switch(colorMode) {
                        case 0:
                              break;
                        case 1:     // pitch
                              {
                              Triple* c = &myColors[event.pitch() % 12];
                              color.setRgb(c->r, c->g, c->b);
                              }
                              break;
                        case 2:     // velocity
                              {
                              int velo = event.velo();
                              if (velo < 64)
                                    color.setRgb(velo*4, 0, 0xff);
                              else
                                    color.setRgb(0xff, 0, (127-velo) * 4);
                              }
                              break;
                        }
                  }
            }

      color.setAlpha(MusEGlobal::config.globalAlphaBlend);
      QBrush brush(color);
      if(!mbr.isEmpty())
        p.fillRect(mbr, brush);

      if(ubbx >= ux && ubbx < ux_2)
        p.drawLine(mbbx, mbby, mbbx, mbby_2 - 1);                       // The left edge

      if(ubbx_2 >= ux && ubbx_2 <= ux_2)
        p.drawLine(mbbx_2, mbby, mbbx_2, mbby_2 - 1);           // The right edge

      if(ubby >= uy && ubby < uy_2)
        p.drawLine(mbbx, mbby, mbbx_2, mbby);                       // The top edge

      if(ubby_2 >= uy && ubby_2 <= uy_2)
        p.drawLine(mbbx, mbby_2 - 1, mbbx_2, mbby_2 - 1);   // The bottom edge

      // print note name on the drawn notes
      if (!mbr.isEmpty() && MusEGlobal::config.showNoteNamesInPianoRoll) {
        QFont f(MusEGlobal::config.fonts[1]);

        f.setPointSize(f.pointSize() * 0.85);
        p.setFont(f);

        if (color.lightnessF() > 0.6f) {
          pen.setColor(Qt::black);
          p.setPen(pen);

        } else {
          pen.setColor(Qt::white);
          p.setPen(pen);
        }
        QString noteStr = MusECore::pitch2string(event.pitch());

        p.drawText(mbbr,Qt::AlignHCenter|Qt::AlignVCenter, noteStr.toUpper());
      }

      p.setWorldMatrixEnabled(wmtxen);
      }

#endif


// This version uses the ViewRect and ViewCoordinate classes for a more 'agnostic' technique.
#if 0

//---------------------------------------------------------
//   drawEvent
//    draws a note
//---------------------------------------------------------

void PianoCanvas::drawItem(QPainter& p, const CItem* item,
   const QRect& mr, const QRegion&)
      {
      QRect ubbr = item->bbox();
      if(!virt())
        ubbr.moveCenter(map(item->pos()));

      //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
      const ViewRect vr(mr, true);
      const ViewRect vbbr(ubbr, false);

      // This is the update comparison rectangle. This would normally be the same as the item's bounding rectangle
      //  but in this case we have a one-pixel wide border. To accommodate for our border, expand the right edge right
      //  by one, and the bottom edge down by one. This way we catch the full
      //  necessary drawing rectangle when checking the requested update rectangle.
      // Note that this is units of ticks.
      ViewRect vbbr_exp(item->bbox(), false);
//      adjustRect(vbbr_exp,
//                 ViewWCoordinate(0, true),
//                 ViewHCoordinate(0, true),
//                 ViewWCoordinate(0, true),
//                 // Normally we would use the y + h for our border, but here we need to
//                 //  use the bottom because of the uneven black/white key 6 - 7 - 6 height progressions.
//                 ViewHCoordinate(-1, true));

      const ViewRect vbr = intersected(vr, vbbr);
      const QRect mbr = asQRectMapped(vbr);
      const QRect mbbr_exp = asQRectMapped(vbbr_exp);

      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);

      struct Triple {
            int r, g, b;
            };

      static Triple myColors [12] = {  // ddskrjp
            { 0xff, 0x3d, 0x39 },
            { 0x39, 0xff, 0x39 },
            { 0x39, 0x3d, 0xff },
            { 0xff, 0xff, 0x39 },
            { 0xff, 0x3d, 0xff },
            { 0x39, 0xff, 0xff },
            { 0xff, 0x7e, 0x7a },
            { 0x7a, 0x7e, 0xff },
            { 0x7a, 0xff, 0x7a },
            { 0xff, 0x7e, 0xbf },
            { 0x7a, 0xbf, 0xff },
            { 0xff, 0xbf, 0x7a }
            };

      QColor color;
      NEvent* nevent   = (NEvent*) item;
      MusECore::Event event = nevent->event();
      if (nevent->part() != curPart){
            if(item->isMoving())
              color = Qt::gray;
            else if(item->isSelected())
              color = Qt::black;
            else
              color = Qt::lightGray;
      }
      else {
            if (item->isMoving()) {
                    color = Qt::gray;
                }
            else if (item->isSelected()) {
                  color = MusEGlobal::config.midiItemSelectedColor;
//                  color = Qt::black;
                  }
            else {
                  color = MusEGlobal::config.midiItemColor;
                  switch(colorMode) {
                        case 0:
                              break;
                        case 1:     // pitch
                              {
                              Triple* c = &myColors[event.pitch() % 12];
                              color.setRgb(c->r, c->g, c->b);
                              }
                              break;
                        case 2:     // velocity
                              {
                              int velo = event.velo();
                              if (velo < 64)
                                    color.setRgb(velo*4, 0, 0xff);
                              else
                                    color.setRgb(0xff, 0, (127-velo) * 4);
                              }
                              break;
                        }
                  }
            }

      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      const ViewWCoordinate vw1m(1, true);
      const ViewHCoordinate vh1m(1, true);

      color.setAlpha(MusEGlobal::config.globalAlphaBlend);
      QBrush brush(color);
      p.fillRect(mbr, brush);
      p.drawRect(mbr);

      // print note name on the drawn notes
      if (MusEGlobal::config.showNoteNamesInPianoRoll) {
        QFont f(MusEGlobal::config.fonts[1]);

        f.setPointSize(f.pointSize() * 0.85);
        p.setFont(f);

        if (color.lightnessF() > 0.6f) {

          pen.setColor(Qt::black);
          p.setPen(pen);

        } else {

          pen.setColor(Qt::white);
          p.setPen(pen);

        }
        QString noteStr = MusECore::pitch2string(event.pitch());

        p.drawText(mbbr_exp,Qt::AlignHCenter|Qt::AlignVCenter, noteStr.toUpper());
      }


      p.setWorldMatrixEnabled(wmtxen);
      }

#endif

//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------

void PianoCanvas::drawTopItem(QPainter& , const QRect&, const QRegion&)
{}

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void PianoCanvas::drawMoving(QPainter& p, const CItem* item, const QRect& mr, const QRegion&)
    {
      const QRect ur = mapDev(mr);
      QRect ur_item = QRect(item->mp().x(), item->mp().y() - item->height()/2, item->width(), item->height());
      ur_item = ur_item.intersected(ur);
      if(!ur_item.isValid())
        return;
      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);
      p.setBrush(Qt::NoBrush);
      p.drawRect(ur_item);
    }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void PianoCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if ((_tool != MusEGui::PointerTool) && (event->button() != Qt::LeftButton)) {
            mousePress(event);
            return;
            }
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

MusECore::Undo PianoCanvas::moveCanvasItems(CItemMap& items, int dp, int dx, DragType dtype, bool rasterize)
{
  if(editor->parts()->empty())
    return MusECore::Undo(); //return empty list

  MusECore::PartsToChangeMap parts2change;
  MusECore::Undo operations;

  for(MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
  {
    MusECore::Part* part = ip->second;
    if(!part)
      continue;

    int npartoffset = 0;
    for(iCItem ici = items.begin(); ici != items.end(); ++ici)
    {
      CItem* ci = ici->second;
      ci->setMoving(false);

      if(ci->part() != part)
        continue;

      int x = ci->pos().x() + dx;
      int y = pitch2y(y2pitch(ci->pos().y()) + dp);
      QPoint newpos = QPoint(x, y);
      if(rasterize)
        newpos = raster(newpos);

      // Test moving the item...
      NEvent* nevent = (NEvent*) ci;
      MusECore::Event event    = nevent->event();
      x              = newpos.x();
      if(x < 0)
        x = 0;
      int ntick = (rasterize ? editor->rasterVal(x) : x) - part->tick();
      if(ntick < 0)
        ntick = 0;
      int diff = ntick + event.lenTick() - part->lenTick();

      // If moving the item would require a new part size...
      if(diff > npartoffset)
        npartoffset = diff;
    }

    if(npartoffset > 0)
    {
      MusECore::iPartToChange ip2c = parts2change.find(part);
      if(ip2c == parts2change.end())
      {
        MusECore::PartToChange p2c = {0, npartoffset};
        parts2change.insert(std::pair<MusECore::Part*, MusECore::PartToChange> (part, p2c));
      }
      else
        ip2c->second.xdiff = npartoffset;
    }
  }

  bool forbidden=false;
  for(MusECore::iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
  {
    MusECore::Part* opart = ip2c->first;
    if (opart->hasHiddenEvents() & MusECore::Part::RightEventsHidden)
    {
			forbidden=true;
			break;
		}
  }    

	
	if (!forbidden)
	{
		std::vector< CItem* > doneList;
		typedef std::vector< CItem* >::iterator iDoneList;
		
		for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
		{
		  CItem* ci = ici->second;
			
      const QPoint oldpos(ci->pos());
			const int x = ci->pos().x();
			const int y = ci->pos().y();
			int nx = x + dx;
			int ny = pitch2y(y2pitch(y) + dp);
			QPoint newpos = QPoint(nx, ny);
			if(rasterize)
			  newpos = raster(newpos);
			selectItem(ci, true);
			
			iDoneList idl;
			for(idl = doneList.begin(); idl != doneList.end(); ++idl)
				// This compares EventBase pointers to see if they're the same...
				if((*idl)->event() == ci->event())
					break;
				
			// Do not process if the event has already been processed (meaning it's an event in a clone part)...
			if (idl == doneList.end())
			{
				moveItem(operations, ci, newpos, dtype, rasterize); // always returns true. if not, change is necessary here!
				doneList.push_back(ci);
			}
			ci->move(newpos);
						
			itemReleased(ci, oldpos);

			if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
						selectItem(ci, false);
		}  

		itemsReleased();

    for(MusECore::iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
    {
      MusECore::Part* opart = ip2c->first;
      int diff = ip2c->second.xdiff;

      schedule_resize_all_same_len_clone_parts(opart, opart->lenTick() + diff, operations);
    }

    return operations;
  }
  else
  {
        return MusECore::Undo(); //return empty list
    }
}

//---------------------------------------------------------
//   moveItem
//    called after moving an object
//---------------------------------------------------------

bool PianoCanvas::moveItem(MusECore::Undo& operations, CItem* item, const QPoint& pos, DragType dtype, bool rasterize)
      {
      // Events within an event list cannot be cloned, only copied or moved! Each must have a unique ID.
      if (dtype == MOVE_CLONE)
        return false;

      NEvent* nevent = (NEvent*) item;
      MusECore::Event event    = nevent->event();
      int npitch     = y2pitch(pos.y());
      MusECore::Event newEvent = (dtype == MOVE_COPY) ? event.duplicate() : event.clone();
      newEvent.setSelected(true);
      int x          = pos.x();
      if (x < 0)
            x = 0;
      MusECore::Part* part = nevent->part();

      newEvent.setPitch(npitch);
      int ntick = (rasterize ? editor->rasterVal(x) : x) - part->tick();
      if (ntick < 0)
            ntick = 0;
      newEvent.setTick(ntick);
      newEvent.setLenTick(event.lenTick());

      // don't check, whether the new event is within the part
      // at this place. with operation groups, the part isn't
      // resized yet. (flo93)

      if (dtype == MOVE_COPY)
      {
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SelectEvent, event, part, false, event.selected()));
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, newEvent, part, false, false));
      }
      else
            operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));

      return true;
}

//---------------------------------------------------------
//   newItem(p, state)
//---------------------------------------------------------

CItem* PianoCanvas::newItem(const QPoint& p, int state)
      {
      int pitch = y2pitch(p.y());
      int tick = p.x();
      if(tick < 0)
        tick = 0;
      if(!(state & Qt::ShiftModifier))
        tick  = editor->rasterVal1(tick);
      int len   = p.x() - tick;
      int eventVelocity = curVelo;

      // if option is enabled and there was a prior event we clone velocity and length from that event
      if (MusEGlobal::config.useLastEditedEvent && lastEditedEvent != nullptr)
      {
          // curVelo = lastEditedEvent->velo(); currently not used, last changed velocity overrides it, haven't quite figured out how.
          len = lastEditedEvent->lenTick();
      }

      tick     -= curPart->tick();
      if (tick < 0)
            return nullptr;
      MusECore::Event e =  MusECore::Event(MusECore::Note);
      e.setTick(tick);
      e.setPitch(pitch);
      e.setVelo(eventVelocity);
      e.setLenTick(len);

      NEvent *newEvent = new NEvent(e, curPart, pitch2y(pitch));
      if(_playEvents)
              startPlayEvent(e.pitch(), e.velo());
      return newEvent;
      }

void PianoCanvas::newItem(CItem* item, bool noSnap)
      {
      NEvent* nevent = (NEvent*) item;
      MusECore::Event event    = nevent->event();
      MusECore::Part* part = nevent->part();
      int ptick = part->tick();
      int x = item->x();
      if (x<ptick)
            x=ptick;
      if(!noSnap)
        x = editor->rasterVal1(x); //round down
      if (x<ptick)
            x=ptick;
      int w = item->width();
      event.setTick(x - ptick);
      if (!noSnap)
            w = editor->rasterVal(w);
      if (w == 0)
            w = editor->rasterStep(ptick);
      event.setLenTick(w);

      event.setPitch(y2pitch(item->y()));
      event.setSelected(true);

      MusECore::Undo operations;
      int diff = event.endTick()-part->lenTick();

      if (! ((diff > 0) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
      {
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent,event, part, false, false));

        if (diff > 0)// part must be extended?
        {
              schedule_resize_all_same_len_clone_parts(part, event.endTick(), operations);
              printf("newItem: extending\n");
        }

        MusEGlobal::song->applyOperationGroup(operations);
        setLastEdited(event);
      }
      else // forbid action by not applying it
          //this forces an update of the itemlist, which is necessary
          //to remove "forbidden" events from the list again
          //otherwise, if a moving operation was forbidden,
          //the canvas would still show the movement
          songChanged(SC_EVENT_INSERTED);
      }

// REMOVE Tim. wave. Added.
//---------------------------------------------------------
//   adjustItemSize
//---------------------------------------------------------

//void PianoCanvas::adjustItemTempValues(CItem* item, int pos, bool noSnap, bool ctrl, bool /*alt*/)
void PianoCanvas::adjustItemSize(CItem* item, int pos, bool left, bool noSnap, bool ctrl, bool /*alt*/, QRegion * region)
      {
//     Q_UNUSED(item)
    Q_UNUSED(ctrl)

//     if(resizeDirection != MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT &&
//       resizeDirection != MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//       return;

//     MusECore::Undo operations;
    MusECore::MuseCount_t newPEnd = 0;

//     NEvent* nevent = (NEvent*) item;

//     MusECore::Part* part;
    const MusECore::Part* itemPart = item->part();

    const MusECore::Pos::TType itemPartTType = itemPart->type();

    const MusECore::MuseCount_t itemPPos = MUSE_TIME_UINT_TO_INT64 itemPart->posValue();
    const MusECore::MuseCount_t tmpItemPLen = item->tmpPartLen();
    const MusECore::MuseCount_t tmpItemPEnd = itemPPos + tmpItemPLen;

    //const MusECore::Pos::TType originalPartTType = resizeTType;
    const MusECore::Pos::TType canvasTType = MusECore::Pos::TICKS;

    if(!noSnap)
      // Ignore the return y, which may be altered, we only want the x.
      pos = raster(QPoint(pos, item->y())).x();

//     for (auto &it: items)
//     {
//         if (!it.second->isSelected())
//             continue;

//         const MusECore::Part* part = it.second->part();
        //const MusECore::Part* part = item->part();
        //const MusECore::Pos::TType itemPartTType = itemPart->type();

//         // REMOVE Tim. wave. Added. Diagnostics.
//         fprintf(stderr, "pre nevent bbox x:%d y:%d w:%d h:%d\n",
//                 it.second->bbox().x(), it.second->bbox().y(), it.second->bbox().width(), it.second->bbox().height());
//         fprintf(stderr, "pre nevent pos x:%d y:%d\n", it.second->pos().x(), it.second->pos().y());

// // REMOVE Tim. wave. Changed.
// //         QPoint topLeft = QPoint(qMax((unsigned)it.second->x(), part->tick()), it.second->y());
//         const QPoint topLeft = QPoint(editor->rasterVal(qMax((unsigned)it.second->x(), part->tick())), it.second->y());
// //         fprintf(stderr, "pre-raster topLeft x:%d y:%d\n", topLeft.x(), topLeft.y());
// // REMOVE Tim. wave. Changed. Diagnostics.
// //         it.second->setTopLeft(raster(topLeft));
// //         topLeft = raster(topLeft);
//         fprintf(stderr, "post-raster topLeft x:%d y:%d\n", topLeft.x(), topLeft.y());
//         it.second->setTopLeft(topLeft);

//         NEvent* nevent = (NEvent*) it.second;
//         NEvent* nevent = (NEvent*) item;

//         // REMOVE Tim. wave. Added. Diagnostics.
//         fprintf(stderr, "item bbox x:%d y:%d w:%d h:%d\n", item->bbox().x(), item->bbox().y(), item->bbox().width(), item->bbox().height());
//         fprintf(stderr, "nevent pos x:%d y:%d\n", nevent->pos().x(), nevent->pos().y());
//         fprintf(stderr, "nevent bbox x:%d y:%d w:%d h:%d\n", nevent->bbox().x(), nevent->bbox().y(), nevent->bbox().width(), nevent->bbox().height());
//         fprintf(stderr, "mr x:%d y:%d w:%d h:%d\n", mr.x(), mr.y(), mr.width(), mr.height());
//         fprintf(stderr, "ur x:%d y:%d w:%d h:%d\n", ur.x(), ur.y(), ur.width(), ur.height());
//         fprintf(stderr, "ubbr x:%d y:%d w:%d h:%d\n", ubbr.x(), ubbr.y(), ubbr.width(), ubbr.height());
//         fprintf(stderr, "ubr x:%d y:%d w:%d h:%d\n", ubr.x(), ubr.y(), ubr.width(), ubr.height());

        const MusECore::MuseCount_t ePos = item->tmpPos();
        const MusECore::MuseCount_t eLen = item->tmpLen();

        const MusECore::Event event = item->event();
        const MusECore::Pos::TType ePosTType   = event.pos().type();
        const MusECore::MuseCount_t PPosETType = MUSE_TIME_UINT_TO_INT64 itemPart->posValue(ePosTType);
        const MusECore::MuseCount_t pPosCTType = MUSE_TIME_UINT_TO_INT64 itemPart->posValue(canvasTType);
        //const MusECore::MuseCount_t PPosEType =
        //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(nevent->tmpPartPos(), part->type(), ePosType);
        //const MusECore::MuseCount_t PTick =
        //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(nevent->tmpPartPos(), part->type(), canvasTType);

        const MusECore::MuseCount_t absEPos       = PPosETType + ePos;
        const MusECore::MuseCount_t absEPosCTType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEPos, ePosTType, canvasTType);
        const MusECore::MuseCount_t absEEnd       = absEPos + eLen;
        const MusECore::MuseCount_t absEEndCTType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEEnd, ePosTType, canvasTType);

        MusECore::MuseCount_t newAbsEPos         = absEPos;
        MusECore::MuseCount_t newAbsEPosCTType   = absEPosCTType;
        MusECore::MuseCount_t newAbsEEnd         = absEEnd;
        MusECore::MuseCount_t newAbsEEndCTType   = absEEndCTType;

        MusECore::MuseCount_t newEPos;
        MusECore::MuseCount_t newELen;

//         newAbsETick = nevent->x();
//         newAbsEEndTick = newAbsETick + nevent->width();

        if (left)
        {
          newAbsEPosCTType = pos;

          if(noSnap)
          {
            // Leave at least one pixel width (at current mag) so that the user can still hover and adjust it.
            if(newAbsEEndCTType - newAbsEPosCTType < rmapxDev(1))
              newAbsEPosCTType = newAbsEEndCTType - rmapxDev(1);
            newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosCTType, canvasTType, ePosTType);
          }
          else
          {
            MusECore::MuseCount_t newAbsEPosTick =
              MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosCTType, canvasTType, MusECore::Pos::TICKS);

            const MusECore::MuseCount_t newAbsEEndTick =
              MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, MusECore::Pos::TICKS);

            const MusECore::MuseCount_t rastStep = editor->rasterStep(newAbsEPosTick);

//             if(newAbsEEndCTType - newAbsEPosCTType < rastStep)
//               newAbsEPosCTType = newAbsEEndCTType - rastStep;
//             newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosCTType, canvasTType, ePosTType);

            if(newAbsEEndTick - newAbsEPosTick < rastStep)
              newAbsEPosTick = newAbsEEndTick - rastStep;

            newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosTick, MusECore::Pos::TICKS, ePosTType);
          }
          // Limit the left edge of the event to the left edge of the part.
          if(newAbsEPos < PPosETType)
            newAbsEPos = PPosETType;
        }
        else
        {
          newAbsEEndCTType = pos;

          if(noSnap)
          {
            // Leave at least one pixel width (at currect mag) so that the user can still hover and adjust it.
            if(newAbsEEndCTType - newAbsEPosCTType < rmapxDev(1))
              newAbsEEndCTType = newAbsEPosCTType + rmapxDev(1);
            newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndCTType, canvasTType, ePosTType);
          }
          else
          {
            const MusECore::MuseCount_t newAbsEPosTick =
              MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, ePosTType, MusECore::Pos::TICKS);

            MusECore::MuseCount_t newAbsEEndTick =
              MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndCTType, canvasTType, MusECore::Pos::TICKS);

            const MusECore::MuseCount_t rastStep = editor->rasterStep(newAbsEPosTick);

//             if(newAbsEEndCTType - newAbsEPosCTType < rastStep)
//               newAbsEEndCTType = newAbsEPosCTType + rastStep;
//             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndCTType, canvasTType, ePosTType);

            if(newAbsEEndTick - newAbsEPosTick < rastStep)
              newAbsEEndTick = newAbsEPosTick + rastStep;

            newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, ePosTType);
          }
        }



//         if (noSnap)
        {
            //newELen = nevent->width();
//             newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosCTType, canvasTType, ePosTType);
//             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndCTType, canvasTType, ePosTType);
        }
//         else
//         {
// // //             MusECore::MuseCount_t absEPos = event.tick() + part->tick();
// //             const MusECore::MuseCount_t absEPosTicks =
// //               MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEPos, event.pos().type(), canvasTType);
// //
// // //             newELen = editor->rasterVal(absEPos + nevent->width()) - absEPos;
// //             //newELen = editor->rasterVal(absEPos + nevent->width()) - absEPos;
// //             //newELen = editor->rasterVal(absEPosTicks + nevent->width()) - absEPosTicks;
// //             const MusECore::MuseCount_t newAbsEPosTicks = editor->rasterVal(absEPosTicks);
// //             const MusECore::MuseCount_t newAbsEPos =
// //               MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosTicks, canvasTType, event.pos().type());
// //             newELen = editor->rasterVal(absEPosTicks + nevent->width());
// //             if (newELen <= 0)
// //                 newELen = editor->raster();
//
//
//             // TODO: PianoCanvas items are currently in TICKS. If TICKS/FRAMES timeline is ever added, convert this.
//             if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//             {
//               newAbsETick = MUSE_TIME_UINT_TO_INT64 editor->rasterVal(newAbsETick);
//               newAbsEPos  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, canvasTType, ePosType);
//               newAbsEEnd  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, canvasTType, ePosType);
//             }
//             else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//             {
//               newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, canvasTType, ePosType);
//               newAbsEEndTick = MUSE_TIME_UINT_TO_INT64 editor->rasterVal(newAbsEEndTick);
//               newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, canvasTType, ePosType);
//             }
//
//         }

        newEPos = newAbsEPos - PPosETType;
        if(newEPos < 0)
          newEPos = 0;

//         // The Canvas limits the distance to 1 rather than 0, so let's catch that.
//         if(newAbsEEnd - newAbsEPos <= 1)
//         {
//           if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//           {
//             MusECore::MuseCount_t newAbsETick = newAbsEEndTick - editor->raster();
//             if(newAbsETick < PTick)
//             {
//               newAbsETick = PTick;
//               const MusECore::MuseCount_t newAbsEEndTick = newAbsETick + editor->raster();
//               newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, canvasTType, ePosType);
//             }
//             newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, canvasTType, ePosType);
//             if(newAbsEPos < PPosEType)
//               newAbsEPos = PPosEType;
//             newEPos = newAbsEPos - PPosEType;
//           }
//           else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//           {
//             const MusECore::MuseCount_t newAbsEEndTick = newAbsETick + editor->raster();
//             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, canvasTType, ePosType);
//           }
//         }

        newELen = newAbsEEnd - newAbsEPos;

//         if(newELen < rmapxDev(1))
//           newELen = rmapxDev(1);

        const QRect curr_ud_rect(item->bbox());

//         int diff = event.tick() + newELen - part->lenTick();

        //const MusECore::MuseCount_t diff = event.tick() + newELen - part->lenTick();
        //const MusECore::MuseCount_t diff = newAbsEEnd - part->endValue(ePosType);
        const MusECore::MuseCount_t newAbsEEndPType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, itemPartTType);
        //const MusECore::MuseCount_t diff = newAbsEEndPType - part->endValue();
        //const bool doExtendPart = newAbsEEnd > part->endValue(ePosType);
        //const bool doExtendPart = newAbsEEndPType > part->endValue();

        // While adjusting visually, only extend the part if it is the same as the given item's part.
        //const bool doExtendPart = /*(part == itemPart) &&*/ newAbsEEnd > PEndETType;
        const bool doExtendPart = /*(part == itemPart) &&*/ (newAbsEEndPType > tmpItemPEnd);

//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
// //             int x = qMax(0, nevent->x());
//             int x = qMax(0, ePos);
//             int ntick = qMax(0u, x - part->tick());
//             newEvent.setTick(ntick);
//         }

//         if (! ((diff > 0) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
        if (! (doExtendPart && (itemPart->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
        {
//             const MusECore::MuseCount_t newPartLen =
// //             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, itemPart->type()) - itemPart->posValue();
//               MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, itemPartTType) - itemPart->posValue();

            if (doExtendPart) // part must be extended?
//                 max_diff_len = qMax(event.tick() + newELen, max_diff_len);
//                 newPEnd = qMax(newAbsEEndPType, newPEnd);
                newPEnd = newAbsEEndPType;

            // Update the canvas item's bounding box and position.
            // NOTE: Some accuracy may be lost AT BOTH BORDERS if converting the
            //        dimensions from frames to ticks here. Currently that
            //        is the case with wave events on a tick-only based canvas.
//             const MusECore::MuseCount_t newPosValCType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//               newEPos, ePosTType, canvasTType);
//             const MusECore::MuseCount_t newLenValCType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//               newEPos + newELen, ePosTType, canvasTType) - newPosValCType;

            const MusECore::MuseCount_t newPosValCType = newAbsEPosCTType - pPosCTType;
            const MusECore::MuseCount_t newLenValCType = newAbsEEndCTType - newAbsEPosCTType;

            item->setPos(QPoint(newPosValCType, item->y()));
            item->setBBox(QRect(newPosValCType, item->y(), newLenValCType, item->height()));
            //
            // If we want to use more accurate temporary position and length values,
            //  which are in the same time type as the item.
            item->setTmpPos(newEPos);
            item->setTmpLen(newELen);
        }
//     }

//     if (max_diff_len > 0) {
    if (newPEnd > 0) {
//         schedule_resize_all_same_len_clone_parts(part, max_diff_len, operations);
//         printf("resizeItem: extending\n");
//         const MusECore::MuseCount_t newPartLen =
//           MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - part->posValue();
        //item->setTmpPartLen(newAbsPEnd - part->posValue());
        item->setTmpPartLen(newPEnd - itemPPos);
    }

    if(region)
    {

//       const QRect curr_ud_rect(item->bbox());

      // Take the part position into account.
      const QRect new_ud_rect((item->bbox() | curr_ud_rect).adjusted(pPosCTType, 0, pPosCTType, 0));
      // Our border box is wider than the item box. Take that into account when updating.
      const QRect new_ud_rect_m((map(new_ud_rect) & rect()).adjusted(
        0, 0, eventBorderWidth, eventBorderWidth));

      // REMOVE Tim. wave. Added. TESTING. Refine later.
//       const QRect new_ud_rect_m(map(rect()));
//       const QRect new_ud_rect_m(rect());

// REMOVE Tim. wave. Added. Diagnostics.
          fprintf(stderr, "PartCanvas::adjustItemSize curr_ud_rect x:%d y:%d w:%d h:%d\n",
                  curr_ud_rect.x(), curr_ud_rect.y(), curr_ud_rect.width(), curr_ud_rect.height());
          fprintf(stderr, "                            new_ud_rect x:%d y:%d w:%d h:%d\n",
                  new_ud_rect.x(), new_ud_rect.y(), new_ud_rect.width(), new_ud_rect.height());
          fprintf(stderr, "PianoCanvas::adjustItemSize new_ud_rect_m x:%d y:%d w:%d h:%d\n",
                  new_ud_rect_m.x(), new_ud_rect_m.y(), new_ud_rect_m.width(), new_ud_rect_m.height());
          fprintf(stderr, "                                     rect x:%d y:%d w:%d h:%d\n",
                  rect().x(), rect().y(), rect().width(), rect().height());
          fprintf(stderr, "                                  newPosVal:%ld newLenVal:%ld\n",
                  newEPos, newELen);

      *region += new_ud_rect_m;
//       *region += new_ud_rect;
    }

//     if(operations.empty())
//       //this forces an update of the itemlist, which is necessary
//       //to remove "forbidden" events from the list again
//       //otherwise, if a moving operation was forbidden,
//       //the canvas would still show the movement
//       songChanged(SC_EVENT_MODIFIED);
//     else
//       //else forbid action by not performing it
//       MusEGlobal::song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

// REMOVE Tim. wave. Changed.
// void PianoCanvas::resizeItem(CItem* item, bool noSnap, bool rasterize)         // experimental changes to try dynamically extending parts
// {
//     Q_UNUSED(item)
//     Q_UNUSED(rasterize)
//
//     MusECore::Undo operations;
//     unsigned max_diff_len = 0;
//     MusECore::Part* part;
//
//     for (auto &it: items) {
//         if (!it.second->isSelected())
//             continue;
//
//         part = it.second->part();
//
//         QPoint topLeft = QPoint(qMax((unsigned)it.second->x(), part->tick()), it.second->y());
//         it.second->setTopLeft(raster(topLeft));
//
//         NEvent* nevent = (NEvent*) it.second;
//         MusECore::Event event    = nevent->event();
//         MusECore::Event newEvent = event.clone();
//         int len;
//
//         if (noSnap)
//             len = nevent->width();
//         else {
//             unsigned tick = event.tick() + part->tick();
//             len = editor->rasterVal(tick + nevent->width()) - tick;
//             if (len <= 0)
//                 len = editor->raster();
//         }
//
//         int diff = event.tick() + len - part->lenTick();
//
//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
//             int x = qMax(0, nevent->x());
//             int ntick = qMax(0u, x - part->tick());
//             newEvent.setTick(ntick);
//         }
//
//         if (! ((diff > 0) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
//         {
//             newEvent.setLenTick(len);
//             operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, nevent->part(), false, false));
//
//             if (diff > 0) // part must be extended?
//                 max_diff_len = qMax(event.tick() + len, max_diff_len);
//         }
//
//         setLastEdited(newEvent);
//     }
//
//     if (max_diff_len > 0) {
//         schedule_resize_all_same_len_clone_parts(part, max_diff_len, operations);
//         printf("resizeItem: extending\n");
//     }
//
//     if(operations.empty())
//       //this forces an update of the itemlist, which is necessary
//       //to remove "forbidden" events from the list again
//       //otherwise, if a moving operation was forbidden,
//       //the canvas would still show the movement
//       songChanged(SC_EVENT_MODIFIED);
//     else
//       //else forbid action by not performing it
//       MusEGlobal::song->applyOperationGroup(operations);
// }

void PianoCanvas::resizeItem(CItem* item, bool /*noSnap*/, bool /*rasterize*/)         // experimental changes to try dynamically extending parts
{
//     Q_UNUSED(item)
//     Q_UNUSED(rasterize)

    MusECore::Undo operations;
//     unsigned max_diff_len = 0;
    MusECore::MuseCount_t newPEnd = 0;
//     MusECore::Part* part;

    for (auto &it: items) {
        if (!it.second->isSelected())
            continue;

        MusECore::Part* part = it.second->part();

//         QPoint topLeft = QPoint(qMax((unsigned)it.second->x(), part->tick()), it.second->y());
//         it.second->setTopLeft(raster(topLeft));

        NEvent* nevent = (NEvent*) it.second;
        MusECore::Event event    = nevent->event();


        const MusECore::Pos::TType ePosType   = event.pos().type();
        const MusECore::MuseCount_t PPosEType =
          MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(nevent->tmpPartPos(), part->type(), ePosType);
        //const MusECore::MuseCount_t PEndEType =
        //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(nevent->tmpPartPos() + nevent->tmpPartLen(), part->type(), ePosType);

        MusECore::MuseCount_t newEPos    = nevent->tmpPos();
        MusECore::MuseCount_t newELen    = nevent->tmpLen();

        MusECore::MuseCount_t newAbsEPos = PPosEType + newEPos;
        MusECore::MuseCount_t newAbsEEnd = newAbsEPos + newELen;

        const MusECore::MuseCount_t newAbsEEndPType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type());
        const bool doExtendPart = newAbsEEndPType > nevent->tmpPartPos() + nevent->tmpPartLen();

        if (! (doExtendPart && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
        {
            if (doExtendPart) // part must be extended?
                newPEnd = qMax(newAbsEEndPType, newPEnd);

            // Anything changed?
            const bool posChanged = event.posValue() != newEPos;
            if(posChanged || event.lenValue() != newELen)
            {
              operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEventProperties,
                part,
                event,
                newEPos,
                newELen,
                0,
                // May be redundant, but force it to do the port cached controllers, per event.
                // 'schedule_resize_all_same_len_clone_parts' above is currently hard-wired to
                //  always handle the port cached controllers, BUT it may exclude some parts.
                // Only if the event's position changed - Length and SPos have no meaning for controllers.
                posChanged,
                // Include all clone port cached controllers as well.
                posChanged
              ));
            }
       }


//         MusECore::Event newEvent = event.clone();
//         int len;
//
//         if (noSnap)
//             len = nevent->width();
//         else {
//             unsigned tick = event.tick() + part->tick();
//             len = editor->rasterVal(tick + nevent->width()) - tick;
//             if (len <= 0)
//                 len = editor->raster();
//         }
//
//         int diff = event.tick() + len - part->lenTick();
//
//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
//             int x = qMax(0, nevent->x());
//             int ntick = qMax(0u, x - part->tick());
//             newEvent.setTick(ntick);
//         }
//
//         if (! ((diff > 0) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
//         {
//             newEvent.setLenTick(len);
//             operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, nevent->part(), false, false));
//
//             if (diff > 0) // part must be extended?
//                 max_diff_len = qMax(event.tick() + len, max_diff_len);
//         }
//
//         setLastEdited(newEvent);
    }

    if (newPEnd > 0) {
//         schedule_resize_all_same_len_clone_parts(part, newAbsPEnd - item->tmpPartPos(), operations);
        fprintf(stderr, "resizeItem: extending\n");
        schedule_resize_all_same_len_clone_parts(item->part(), newPEnd - item->tmpPartPos(), operations);
    }

    if(operations.empty())
      //this forces an update of the itemlist, which is necessary
      //to remove "forbidden" events from the list again
      //otherwise, if a moving operation was forbidden,
      //the canvas would still show the movement
      songChanged(SC_EVENT_MODIFIED);
    else
      //else forbid action by not performing it
      MusEGlobal::song->applyOperationGroup(operations);
}

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PianoCanvas::deleteItem(CItem* item)
      {
      NEvent* nevent = (NEvent*) item;
      if (nevent->part() == curPart) {
            MusECore::Event ev = nevent->event();
            // Indicate do undo, and do not do port controller values and clone parts.
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                            ev, curPart, false, false));
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   pianoCmd
//---------------------------------------------------------

void PianoCanvas::pianoCmd(int cmd)
      {
      switch(cmd) {
            case CMD_LEFT:
                  {
                  int spos = pos[0];
                  if(spos > 0)
                  {
                    spos -= 1;     // Nudge by -1, then snap down with raster1.
                    spos = MusEGlobal::sigmap.raster1(spos, editor->rasterStep(pos[0]));
                  }
                  if(spos < 0)
                    spos = 0;
                  MusECore::Pos p(spos,true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
                  }
                  break;
            case CMD_RIGHT:
                  {
                  int spos = MusEGlobal::sigmap.raster2(pos[0] + 1, editor->rasterStep(pos[0]));    // Nudge by +1, then snap up with raster2.
                  MusECore::Pos p(spos,true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
                  }
                  break;
            case CMD_LEFT_NOSNAP:
                  {
                  int spos = pos[0] - editor->rasterStep(pos[0]);
                  if (spos < 0)
                        spos = 0;
                  MusECore::Pos p(spos,true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true); //CDW
                  }
                  break;
            case CMD_RIGHT_NOSNAP:
                  {
                  MusECore::Pos p(pos[0] + editor->rasterStep(pos[0]), true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true); //CDW
                  }
                  break;
            case CMD_PUSH:
                  {
                  if (pos[0] < start() || pos[0] >= end())
                        break;
                  MusECore::MidiPart* part = (MusECore::MidiPart*)curPart;

                  if (part == 0)
                        break;

                  MusECore::Undo operations;

                  std::list <MusECore::Event> elist;
                  for (MusECore::ciEvent e = part->events().lower_bound(pos[0] - part->tick()); e != part->events().end(); ++e)
                        elist.push_back((MusECore::Event)e->second);
                  for (std::list<MusECore::Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        MusECore::Event event = *i;
                        MusECore::Event newEvent = event.clone();
                        newEvent.setTick(event.tick() + editor->raster());// - part->tick()); DELETETHIS
                        // Do not do port controller values and clone parts.
                        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
                        }
                  MusEGlobal::song->applyOperationGroup(operations);

                  MusECore::Pos p(editor->rasterVal(pos[0] + editor->rasterStep(pos[0])), true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, false, true);
                  }
                  return;
            case CMD_PULL:
                  if (pos[0] < start() || pos[0] >= end())
                        break;
                  {
                  MusECore::MidiPart* part = (MusECore::MidiPart*)curPart;
                  if (part == 0)
                        break;

                  MusECore::Undo operations;
                  std::list<MusECore::Event> elist;
                  for (MusECore::ciEvent e = part->events().lower_bound(pos[0]); e != part->events().end(); ++e)
                        elist.push_back((MusECore::Event)e->second);
                  for (std::list<MusECore::Event>::iterator i = elist.begin(); i != elist.end(); ++i) {
                        MusECore::Event event = *i;
                        MusECore::Event newEvent = event.clone();
                        newEvent.setTick(event.tick() - editor->raster() - part->tick());
                        // Do not do port controller values and clone parts.
                        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
                        }
                  MusEGlobal::song->applyOperationGroup(operations);
                  MusECore::Pos p(editor->rasterVal(pos[0] - editor->rasterStep(pos[0])), true);
                  MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, false, true);
                  }
                  break;
            }
      }

//---------------------------------------------------------
//   pianoPressed
//---------------------------------------------------------

void PianoCanvas::pianoPressed(int pitch, int velocity, bool shift)
      {
      // REMOVE Tim. Noteoff. Added. Zero note on vel is not allowed now.
      if(velocity > 127)
        velocity = 127;
      else if(velocity <= 0)
        velocity = 1;
      
      // Stop all notes.
      if (!shift)
          stopPlayEvents();

      // play note:
      if(_playEvents)
      {
        startPlayEvent(pitch, velocity);
      }
      
      if (_steprec && curPart) // && pos[0] >= start_tick && pos[0] < end_tick [removed by flo93: this is handled in steprec->record]
          steprec->record(curPart,pitch,editor->raster(),editor->raster(),velocity,MusEGlobal::globalKeyState&Qt::ControlModifier,shift, -1 /* anything which is != rcSteprecNote */);
      }

//---------------------------------------------------------
//   pianoReleased
//---------------------------------------------------------

void PianoCanvas::pianoReleased(int /*pitch*/, bool)
{
    // release key:
    if(_playEvents)
        stopPlayEvents();
}

void PianoCanvas::pianoShiftReleased()
{
    if (_playEvents)
        stopPlayEvents();

    if (_steprec && curPart) {
        steprec->moveon(editor->raster());
    }
}


// NOTE Keep this for now in case we can get it to work...
#if 0

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PianoCanvas::drawCanvas(QPainter& p, const QRect& mr, const QRegion& mrg)
      {

      const ViewRect vr(mr, true);
      const ViewXCoordinate& vx = vr._x;
      const ViewYCoordinate& vy = vr._y;
      const ViewWCoordinate& vw = vr._width;
      const ViewHCoordinate& vh = vr._height;
      const ViewYCoordinate vy_2 = mathYCoordinates(vy, vh, MathAdd);

      const ViewHCoordinate vkh(KH, false);
      const ViewYCoordinate vy0(0, false);
      const ViewYCoordinate vy3(3, false);
      const ViewHCoordinate vh1(1, false);
      const ViewHCoordinate vh3(3, false);
      const ViewHCoordinate vh6(6, false);
      const ViewHCoordinate vh7(7, false);
//       const ViewHCoordinate vh_m1(-1, false);
//       const ViewHCoordinate vh_75(75, false);
      const ViewYCoordinate vy75(75, false);

      int mx = mr.x();
//       int y = mr.y();
      int mw = mr.width();

//       int h = mr.height();

      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);

      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      //---------------------------------------------------
      //  horizontal lines
      //---------------------------------------------------

//       int yy  = ((y-1) / KH) * KH + KH;
      ViewYCoordinate vyy = mathYCoordinates(vy, vh1, MathSubtract);
// For testing...
      fprintf(stderr, "PianoCanvas::drawCanvas: A vkh:%d map:%d vyy:%d map:%d\n", vkh._value, vkh.isMapped(), vyy._value, vyy.isMapped());
      mathRefYCoordinates(vyy, vkh, MathDivide);
      fprintf(stderr, "PianoCanvas::drawCanvas: B vyy:%d map:%d\n", vyy._value, vyy.isMapped());
      mathRefYCoordinates(vyy, vkh, MathMultiply);
      fprintf(stderr, "PianoCanvas::drawCanvas: C vyy:%d map:%d\n", vyy._value, vyy.isMapped());
      mathRefYCoordinates(vyy, vkh, MathAdd);
      fprintf(stderr, "PianoCanvas::drawCanvas: D vyy:%d map:%d\n", vyy._value, vyy.isMapped());

//       int key = 75 - (yy / KH);
      const ViewYCoordinate vkeyy = mathYCoordinates(vyy, vkh, MathDivide);
      const ViewHCoordinate vkeyh(vkeyy._value, vkeyy.isMapped());
      ViewYCoordinate vkey = mathYCoordinates(vy75, vkeyh, MathSubtract);

//       for (; yy < y + h; yy += KH) {
//             switch (key % 7) {
//                   case 0:
//                   case 3:
// For testing...
//                         fprintf(stderr, "PianoCanvas::drawCanvas: Drawing horizontal line at x:%d yy:%d x + w:%d yy:%d\n", x, yy, x + w, yy);
//
//                         p.drawLine(x, yy, x + w, yy);
//                         break;
//                   default:
// For testing...
//                         fprintf(stderr, "PianoCanvas::drawCanvas: Filling rectangle at x:%d yy - 3:%d w:%d h:%d\n", x, yy - 3, w, 6);
//
//                         p.fillRect(x, yy-3, w, 6, MusEGlobal::config.midiCanvasBg.darker(110));
//                         break;
//                   }
//             --key;

      const int myy = asIntMapped(vyy);

// For testing...
      fprintf(stderr, "PianoCanvas::drawCanvas: x:%d vy:%d map:%d vyy:%d map:%d mx + mw:%d vy_2:%d map:%d\n",
              mx,
              vy._value, vy.isMapped(),
              vyy._value, vyy.isMapped(),
              mx + mw,
              vy_2._value, vy_2.isMapped());

      for (; compareYCoordinates(vyy, vy_2, CompareLess); mathRefYCoordinates(vyy, vkh, MathAdd)) {

            ViewYCoordinate vkey_md7 = mathYCoordinates(vkey, vh7, MathModulo);

// For testing...
            fprintf(stderr, "...Comparing y coordinates vkey_md7:%d vy0:%d vy3:%d\n", vkey_md7._value, vy0._value, vy3._value);

            if(compareYCoordinates(vkey_md7, vy0, CompareEqual) ||
               compareYCoordinates(vkey_md7, vy3, CompareEqual))
            {
// For testing...
              fprintf(stderr, "...Drawing horizontal line at x:%d myy:%d x + w:%d myy:%d\n", mx, myy, mx + mw, myy);

              p.drawLine(mx, myy, mx + mw, myy);
            }
            else
            {
              const ViewRect vfr(vx, mathYCoordinates(vyy, vh3, MathSubtract), vw, vh6);
// For testing...
              fprintf(stderr, "...Filling rectangle at x:%d myy - 3:%d w:%d h:%d\n", mx, myy - 3, mw, 6);

              p.fillRect(asQRectMapped(vfr), MusEGlobal::config.midiCanvasBg.darker(110));
            }

            mathRefYCoordinates(vkey, vh1, MathSubtract);
            }

      p.setWorldMatrixEnabled(wmtxen);

      //---------------------------------------------------
      // vertical lines
      //---------------------------------------------------

      drawTickRaster(p, mr, mrg, editor->raster(), false, false, false,
                         MusEGlobal::config.midiCanvasBarColor,
                         MusEGlobal::config.midiCanvasBeatColor);

      }

#endif

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PianoCanvas::drawCanvas(QPainter& p, const QRect& mr, const QRegion& rg)
      {
      const int pianoHeight = 91 * 10 + KH * 5 + 1;
      QRect ur = mapDev(mr);
      if (ur.height() > pianoHeight)
      ur.setHeight(pianoHeight);
      // FIXME: For some reason need the expansion otherwise drawing
      //        artifacts (incomplete drawing). Can't figure out why.
      ur.adjust(0, -4, 0, 4);

      int ux = ur.x();
      if(ux < 0)
        ux = 0;
      const int uy = ur.y();
      const int uw = ur.width();
      const int uh = ur.height();
      const int ux_2 = ux + uw;
      const int uy_2 = uy + uh;

      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(MusEGlobal::config.midiDividerColor);
//      pen.setColor(Qt::black);
      p.setPen(pen);

      //---------------------------------------------------
      //  horizontal lines
      //---------------------------------------------------

      int uyy  = ((uy-1) / KH) * KH + KH;
      int key = 75 - (uyy / KH);

// For testing...
//       fprintf(stderr, "PianoCanvas::drawCanvas: x:%d y:%d yy:%d x + w:%d y + h:%d\n", x, y, yy, x + w, y + h);

      for (; uyy < uy_2; uyy += KH) {
            switch (key % 7) {
                  case 0:
                  case 3:
// For testing...
//                         fprintf(stderr, "...Drawing horizontal line at x:%d yy:%d x + w:%d yy:%d\n", x, yy, x + w, yy);

                        if (MusEGlobal::config.canvasShowGrid || MusEGlobal::config.canvasShowGridHorizontalAlways)
                          p.drawLine(ux, uyy, ux_2, uyy);
                        break;
                  default:
// For testing...
//                         fprintf(stderr, "...Filling rectangle at x:%d yy - 3:%d w:%d h:%d\n", x, yy - 3, w, 6);

                        p.fillRect(ux, uyy-3, uw, 6, MusEGlobal::config.midiCanvasBg.darker(110));
                        break;
                  }
            --key;
            }

      if (MusEGlobal::config.canvasShowGrid)
      {
        //---------------------------------------------------
        // vertical lines
        //---------------------------------------------------

        drawTickRaster(p, mr, rg, editor->raster(), false, false, false,
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasFineColor,
                      MusEGlobal::config.midiCanvasBarColor);
      }
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoCanvas::cmd(int cmd)
{
    switch (cmd) {
    case CMD_SELECT_ALL:     // select all
        for (iCItem k = items.begin(); k != items.end(); ++k) {
            if (!k->second->isSelected())
                selectItem(k->second, true);
        }
        break;
    case CMD_SELECT_NONE:     // select none
        deselectAll();
        break;
    case CMD_SELECT_INVERT:     // invert selection
        for (iCItem k = items.begin(); k != items.end(); ++k) {
            selectItem(k->second, !k->second->isSelected());
        }
        break;
    case CMD_SELECT_ILOOP:     // select inside loop
        for (iCItem k = items.begin(); k != items.end(); ++k) {
            NEvent* nevent = (NEvent*)(k->second);
            MusECore::Part* part     = nevent->part();
            MusECore::Event event    = nevent->event();
            unsigned tick  = event.tick() + part->tick();
            if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                selectItem(k->second, false);
            else
                selectItem(k->second, true);
        }
        break;
    case CMD_SELECT_OLOOP:     // select outside loop
        for (iCItem k = items.begin(); k != items.end(); ++k) {
            NEvent* nevent = (NEvent*)(k->second);
            MusECore::Part* part     = nevent->part();
            MusECore::Event event    = nevent->event();
            unsigned tick  = event.tick() + part->tick();
            if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                selectItem(k->second, true);
            else
                selectItem(k->second, false);
        }
        break;
    case CMD_RANGE_TO_SELECTION:
        setRangeToSelection();
        break;
    case CMD_SELECT_PREV_PART:     // select previous part
    {
        MusECore::Part* pt = editor->curCanvasPart();
        MusECore::Part* newpt = pt;
        MusECore::PartList* pl = editor->parts();
        for(MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip)
            if(ip->second == pt)
            {
                if(ip == pl->begin())
                    ip = pl->end();
                --ip;
                newpt = ip->second;
                break;
            }
        if(newpt != pt)
            editor->setCurCanvasPart(newpt);
    }
        break;
    case CMD_SELECT_NEXT_PART:     // select next part
    {
        MusECore::Part* pt = editor->curCanvasPart();
        MusECore::Part* newpt = pt;
        MusECore::PartList* pl = editor->parts();
        for(MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip)
            if(ip->second == pt)
            {
                ++ip;
                if(ip == pl->end())
                    ip = pl->begin();
                newpt = ip->second;
                break;
            }
        if(newpt != pt)
            editor->setCurCanvasPart(newpt);
    }
        break;

    case CMD_FIXED_LEN:
    case CMD_CRESCENDO:
    case CMD_TRANSPOSE:
    case CMD_THIN_OUT:
    case CMD_ERASE_EVENT:
    case CMD_NOTE_SHIFT:
    case CMD_MOVE_CLOCK:
    case CMD_COPY_MEASURE:
    case CMD_ERASE_MEASURE:
    case CMD_DELETE_MEASURE:
    case CMD_CREATE_MEASURE:
        break;
    default:
        //                  printf("unknown ecanvas cmd %d\n", cmd);
        break;
    }
    itemSelectionsChanged();
    redraw();
}

//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------
void PianoCanvas::midiNote(int pitch, int velo)
      {
      if (MusEGlobal::debugMsg) printf("PianoCanvas::midiNote: pitch=%i, velo=%i\n", pitch, velo);

      if (velo)
        noteHeldDown[pitch]=true;
      else
        noteHeldDown[pitch]=false;

      if (MusEGlobal::heavyDebugMsg)
      {
        printf("  held down notes are: ");
        for (int i=0;i<128;i++)
          if (noteHeldDown[i])
            printf("%i ",i);
        printf("\n");
      }

      if (_midiin && _steprec && curPart
         && !MusEGlobal::audio->isPlaying() && velo && pos[0] >= start_tick
         /* && pos[0] < end_tick [removed by flo93: this is handled in steprec->record] */
         && !(MusEGlobal::globalKeyState & Qt::AltModifier)) {
                     steprec->record(curPart,pitch,editor->raster(),editor->raster(),velo,MusEGlobal::globalKeyState&Qt::ControlModifier,MusEGlobal::globalKeyState&Qt::ShiftModifier);
         }
      }


//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PianoCanvas::startDrag(CItem* /* item*/, DragType t)
{
   QMimeData* md = selected_events_to_mime(partlist_to_set(editor->parts()), 1);

   if (md)
   {
      // "Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object.
      //  The QDrag must be constructed on the heap with a parent QWidget to ensure that Qt can
      //  clean up after the drag and drop operation has been completed. "
      QDrag* drag = new QDrag(this);
      drag->setMimeData(md);

      if (t == MOVE_COPY || t == MOVE_CLONE)
         drag->exec(Qt::CopyAction);
      else
         drag->exec(Qt::MoveAction);
   }
}

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void PianoCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      //event->accept(Q3TextDrag::canDecode(event));
      event->acceptProposedAction();  // TODO CHECK Tim.
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void PianoCanvas::dragMoveEvent(QDragMoveEvent*)
      {
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void PianoCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
      }

//---------------------------------------------------------
//   itemPressed
//---------------------------------------------------------

void PianoCanvas::itemPressed(const CItem* item)
      {
      if (!_playEvents)
            return;
      MusECore::Event e = ((NEvent*)item)->event();
      startPlayEvent(e.pitch(), e.velo());
      }

//---------------------------------------------------------
//   itemReleased
//---------------------------------------------------------

void PianoCanvas::itemReleased(const CItem* item, const QPoint&)
      {
      if(!track())
      {
        // Stop all playing notes:
        stopPlayEvents();
        return;
      }

      const int opitch = y2pitch(item->mp().y());
      const int port = track()->outPort();
      const int channel = track()->outChannel();
      
      // Stop any playing note:
      stopStuckNote(port, channel, opitch);
      }

//---------------------------------------------------------
//   itemMoving
//---------------------------------------------------------

void PianoCanvas::itemMoving(const CItem* item, const QPoint& newMP)
{
      if(!track())
      {
        // Stop all playing notes:
        stopPlayEvents();
        return;
      }

      const int opitch = y2pitch(item->mp().y());
      const int npitch = y2pitch(newMP.y());
      if(opitch == npitch)
        return;

      const int port = track()->outPort();
      const int channel = track()->outChannel();
      
      // Stop any playing note:
      stopStuckNote(port, channel, opitch);
}

//---------------------------------------------------------
//   itemMoved
//---------------------------------------------------------

void PianoCanvas::itemMoved(const CItem* item, const QPoint& oldMP)
      {
      const int opitch = y2pitch(oldMP.y());
      const int npitch = y2pitch(item->mp().y());
      if(opitch == npitch)
        return;

      if(_playEvents)
      {
        if((_playEventsMode == PlayEventsSingleNote && item == curItem) ||
           (_playEventsMode == PlayEventsChords && curItem && curItem->mp().x() == item->mp().x()))
        {
            const MusECore::Event e = ((NEvent*)item)->event();
            // play note:
            startPlayEvent(npitch, e.velo());
        }
      }
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void PianoCanvas::curPartChanged()
      {
      EventCanvas::curPartChanged();
      editor->setWindowTitle(getCaption());
      }

//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void PianoCanvas::modifySelected(MusEGui::NoteInfo::ValType type, int val, bool delta_mode)
{
    QList< QPair<QUuid,MusECore::Event> > already_done;
    MusECore::Undo operations;
    unsigned int playedEventTick = UINT_MAX;

    for (const auto& i : qAsConst(items)) {
        if (!(i.second->isSelected()))
            continue;
        NEvent* e   = (NEvent*)(i.second);
        MusECore::Event event = e->event();
        if (event.type() != MusECore::Note)
            continue;

        MusECore::MidiPart* part = (MusECore::MidiPart*)(e->part());

        if (already_done.contains(QPair<QUuid,MusECore::Event>(part->clonemaster_uuid(), event)))
            continue;

        MusECore::Event newEvent = event.clone();

        switch (type) {
            case MusEGui::NoteInfo::VAL_TIME:
                {
                    int newTime = val;
                    if(delta_mode)
                        newTime += event.tick();
                    else
                        newTime -= part->tick();
                    if (newTime < 0)
                        newTime = 0;
                    newEvent.setTick(newTime);

                    if (_playEvents) {
                        if (playedEventTick == UINT_MAX) {
                            playedEventTick = newEvent.tick();
                            startPlayEvent(newEvent.pitch(), newEvent.velo());
                        } else if (_playEventsMode == PlayEventsChords) {
                            if (playedEventTick == newEvent.tick())
                                startPlayEvent(newEvent.pitch(), newEvent.velo());
                        }
                    }
                }
                break;
            case MusEGui::NoteInfo::VAL_LEN:
                {
                    int len = val;
                    if(delta_mode)
                        len += event.lenTick();
                    if (len < 1)
                        len = 1;
                    newEvent.setLenTick(len);
                }
                break;
            case MusEGui::NoteInfo::VAL_VELON:
                {
                    int velo = val;
                    if(delta_mode)
                        velo += event.velo();
                    if (velo > 127)
                        velo = 127;
                    else if (velo < 0)
                        // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
                        //                               velo = 0;
                        velo = 1;
                    newEvent.setVelo(velo);
                }
                break;
            case MusEGui::NoteInfo::VAL_VELOFF:
                {
                    int velo = val;
                    if(delta_mode)
                        velo += event.veloOff();
                    if (velo > 127)
                        velo = 127;
                    else if (velo < 0)
                        velo = 0;
                    newEvent.setVeloOff(velo);
                }
                break;
            case MusEGui::NoteInfo::VAL_PITCH:
                {
                    int pitch = val;
                    if(delta_mode)
                        pitch += event.pitch();
                    if (pitch > 127)
                        pitch = 127;
                    else if (pitch < 0)
                        pitch = 0;
                    newEvent.setPitch(pitch);

                    if (_playEvents) {
                        if (playedEventTick == UINT_MAX) {
                            playedEventTick = newEvent.tick();
                            startPlayEvent(pitch, newEvent.velo());
                        } else if (_playEventsMode == PlayEventsChords) {
                            if (playedEventTick == newEvent.tick())
                                startPlayEvent(pitch, newEvent.velo());
                        }
                    }
                }
                break;
        }

        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, part, false, false));
        already_done.append(QPair<QUuid,MusECore::Event>(part->clonemaster_uuid(), event));
    }
    MusEGlobal::song->applyOperationGroup(operations);
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void PianoCanvas::resizeEvent(QResizeEvent* ev)
      {
      if (ev->size().width() != ev->oldSize().width())
            emit newWidth(ev->size().width());
      EventCanvas::resizeEvent(ev);
      }

//---------------------------------------------------------
//   mouseMove (override)
//---------------------------------------------------------

void PianoCanvas::mouseMove(QMouseEvent* event) {

    EventCanvas::mouseMove(event);

    if (MusEGlobal::config.showNoteTooltips)
        showNoteTooltip(event);

    if (MusEGlobal::config.showStatusBar)
        showStatusTip(event);
}

//---------------------------------------------------------
//   genItemPopup (override)
//---------------------------------------------------------
QMenu* PianoCanvas::genItemPopup(MusEGui::CItem* item) {
    // no context menu available, use for single item selection
    deselectAll();
    item->setSelected(true);
    return nullptr;
}

//---------------------------------------------------------
//   setColorMode
//---------------------------------------------------------

void PianoCanvas::setColorMode(MidiEventColorMode mode)
      {
      colorMode = mode;
      redraw();
      }
      
void PianoCanvas::showNoteTooltip(QMouseEvent* event) {

    static CItem* hoverItem = nullptr;

    if (_tool & (MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool)) {

        QString str;
        CItem* item = findCurrentItem(event->pos());
        if (item) {
            if (hoverItem == item)
                return;

            hoverItem = item;

            int pitch = item->event().pitch();
            MusECore::Pos start(item->event().tick() + item->part()->tick());

            int bar, beat, tick, hour, min, sec, msec;

            start.mbt(&bar, &beat, &tick);
            QString str_bar = QString("%1.%2.%3")
                    .arg(bar + 1,  4, 10, QLatin1Char('0'))
                    .arg(beat + 1, 2, 10, QLatin1Char('0'))
                    .arg(tick,     3, 10, QLatin1Char('0'));

            start.msmu(&hour, &min, &sec, &msec, nullptr);
            QString str_time = QString("%1:%2:%3.%4")
                    .arg(hour,  2, 10, QLatin1Char('0'))
                    .arg(min,   2, 10, QLatin1Char('0'))
                    .arg(sec,   2, 10, QLatin1Char('0'))
                    .arg(msec,  3, 10, QLatin1Char('0'));

            str = tr("Note: ") + MusECore::pitch2string(pitch) + " (" + QString::number(pitch) + ")\n"
                    + tr("Velocity: ") + QString::number(item->event().velo()) + "\n"
                    + tr("Start (bar): ") +  str_bar + "\n"
                    + tr("Start (time): ") + str_time + "\n"
                    + tr("Length (ticks): ") + QString::number(item->event().lenTick());

        } else {
            hoverItem = nullptr;
            int pitch = y2pitch(event->pos().y());
            str = MusECore::pitch2string(pitch) + " (" + QString::number(pitch) + ")";
        }

        QToolTip::showText(QPoint(event->globalX(), event->globalY() + 20), str);
    }
}

void PianoCanvas::showStatusTip(QMouseEvent* event) {

    static CItem* hoverItem = nullptr;
    static Tool localTool;

    CItem* item = findCurrentItem(event->pos());
    if (item) {
        if (hoverItem == item && localTool == _tool)
            return;

        hoverItem = item;
        localTool = _tool;

        QString s;
        if (_tool & (MusEGui::PointerTool ))
            s = tr("LMB: Select/Move | CTRL+LMB: Multi select/Move&copy | SHIFT+LMB: Select pitch | MMB: Delete | CTRL+RMB: Trim length");
        else if (_tool & (MusEGui::PencilTool))
            s = tr("LMB: Resize | CTRL+LMB: Multi select | CTRL+SHIFT+LMB: Multi pitch select | MMB: Delete | RMB: Select exclusive | CTRL+RMB: Trim length");
        else if (_tool & (MusEGui::RubberTool))
            s = tr("LMB: Delete | RMB: Select exclusive | CTRL+RMB: Trim length");

        if (!s.isEmpty())
            MusEGlobal::muse->setStatusBarText(s);
    } else {
        if (hoverItem != nullptr) {
            MusEGlobal::muse->clearStatusBarText();
            hoverItem = nullptr;
        }
    }
}


} // namespace MusEGui
