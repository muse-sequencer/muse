//=========================================================
//  MusE
//  Linux Music Editor
//    wavecanvas.cpp
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  Based on WaveView.cpp and PianoCanvas.cpp
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//   and others.
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
#include <QDragLeaveEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPair>
#include <QMessageBox>
#include <QDir>
#include <QLine>
#include <QVector>
#include <QProcess>
#include <QColor>
#include <QPen>
#include <QUuid>
#include <QBrush>
#include <QLinearGradient>

#include <set>

#include <limits.h>
#include <stdio.h>
#include "muse_math.h"
#include <set>

#include "app.h"
#include "icons.h"
#include "xml.h"
#include "wavecanvas.h"
#include "event.h"
#include "globals.h"
#include "cmd.h"
#include "song.h"
#include "audio.h"
#include "functions.h"
#include "gconfig.h"
#include "shortcuts.h"
#include "editgain.h"
#include "wave.h"
#include "waveedit.h"
#include "fastlog.h"
#include "utils.h"
#include "tools.h"
#include "copy_on_write.h"
#include "helper.h"
#include "sig.h"
#include "wave_helper.h"
#include "config.h"

#include "menutitleitem.h"
#include "audio_converter_settings.h"
#include "audio_convert/audio_converter_plugin.h"
#include "audio_convert/audio_converter_settings_group.h"
#include "sndfile.h"
#include "operations.h"

// Forwards from header:
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPainter>
#include "part.h"
#include "track.h"

#define ABS(x) (abs(x))
#define FABS(x) (fabs(x))

// For debugging output: Uncomment the fprintf section.
#define ERROR_WAVECANVAS(dev, format, args...)  fprintf(dev, format, ##args)
#define INFO_WAVECANVAS(dev, format, args...) // fprintf(dev, format, ##args)
#define DEBUG_WAVECANVAS(dev, format, args...) // fprintf(dev, format, ##args)


namespace MusEGui {

// Static. This is how thick our event borders are.
int WaveCanvas::eventBorderWidth = 1;

const int WaveCanvas::_stretchAutomationPointDetectDist = 4;
const int WaveCanvas::_stretchAutomationPointWidthUnsel = 2;
const int WaveCanvas::_stretchAutomationPointWidthSel = 3;

//---------------------------------------------------------
//   WEvent
//---------------------------------------------------------

WEvent::WEvent(const MusECore::Event& e, MusECore::Part* p, int height) : EItem(e, p)
      {
      unsigned frame = e.frame() + p->frame();
      setPos(QPoint(frame, 0));
      unsigned len = e.lenFrame();
      if(e.frame() + e.lenFrame() >= p->lenFrame())
        len = p->lenFrame() - e.frame();
      setBBox(QRect(frame, 0, len, height));    
      // Give the moving point an initial value.
      setMp(pos());
// REMOVE Tim. wave. Added.
      // (Done by PItem or EItem.)
      //setTmpPartPos(p->posValue());
      //setTmpPartLen(p->lenValue());
      //setTmpOffset(0);
      //setTmpPos(e.posValue());
      //setTmpLen(e.lenValue());
      //setTmpWaveSPos(e.spos());
// REMOVE Tim. wave. Added.
//       initItemTempValues();
      }

// REMOVE Tim. wave. Added.
// void WEvent::horizResize(int newPos, bool left)
// {
//   EItem::horizResize(newPos, left);
//
// }

// REMOVE Tim. wave. Added.
StretchSelectedItem::StretchSelectedItem(MusECore::StretchListItem::StretchEventType type,
                      MusECore::SndFileR sndFile)
  {
    _type = type;
    _sndFile = sndFile;
  }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

CItem* WaveCanvas::addItem(MusECore::Part* part, const MusECore::Event& event)
      {
      WEvent* ev = new WEvent(event, part, height());  
      items.add(ev);
      return ev;
      }

//---------------------------------------------------------
//   WaveCanvas
//---------------------------------------------------------

WaveCanvas::WaveCanvas(MidiEditor* pr, QWidget* parent, int sx, int sy)
   : EventCanvas(pr, parent, sx, 1)
      {
      setObjectName("WaveCanvas");

      setStatusTip(tr("Wave canvas: Use Pencil tool to edit wave events, Pointer tool to select and edit. Press F1 for help."));

      colorMode = 0;
      button = 0;
      supportsResizeToTheLeft = true;
      
      editor = pr;
      setVirt(true);
      
      setBg(QColor());
      
      pos[0] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->cpos());
      pos[1] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->lpos());
      pos[2] = MusEGlobal::tempomap.tick2frame(MusEGlobal::song->rpos());
      yScale = sy;
      mode = NORMAL;
      selectionStart = 0;
      selectionStop  = 0;
      lastGainvalue = 100;

      songChanged(SC_TRACK_INSERTED);
      }

WaveCanvas::~WaveCanvas()
{
  //delete steprec;
}

//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void WaveCanvas::updateItems()
{
  bool curItemNeedsRestore=false;
  MusECore::Event storedEvent;
  QUuid partSn;
  if (curItem)
  {
    curItemNeedsRestore=true;
    storedEvent=curItem->event();
    partSn=curItem->part()->uuid();
  }
  curItem=nullptr;

  items.clearDelete();
  startSample  = INT_MAX;
  endSample    = 0;
  curPart = 0;
  for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) {
        MusECore::WavePart* part = (MusECore::WavePart*)(p->second);
        if (part->uuid() == curPartId)
              curPart = part;
        unsigned ssample = part->frame();
        unsigned len = part->lenFrame();
        unsigned esample = ssample + len;
        if (ssample < startSample)
              startSample = ssample;
        if (esample > endSample)
              endSample = esample;

        for (MusECore::ciEvent i = part->events().begin(); i != part->events().end(); ++i) {
              const MusECore::Event& e = i->second;
              // Do not add events which are past the end of the part.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
              if((int)e.frame() + (int)e.lenFrame() < 0)
                continue;
              if((int)e.frame() >= (int)len)
                break;
#else
              if(e.frame() > len)
                break;
#endif

              if (e.type() == MusECore::Wave) {
                    CItem* temp = addItem(part, e);

                    if(temp)
                      temp->setSelected(e.selected());

                    if (temp && curItemNeedsRestore && e==storedEvent && part->uuid()==partSn)
                    {
                        if (curItem!=nullptr)
                          printf("THIS SHOULD NEVER HAPPEN: curItemNeedsRestore=true, event fits, but there was already a fitting event!?\n");

                        curItem=temp;
                        }
                    }
              }
        }
}

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void WaveCanvas::songChanged(MusECore::SongChangedStruct_t flags)
      {
      if (flags & ~(SC_SELECTION | SC_PART_SELECTION | SC_TRACK_SELECTION)) {
            // TODO FIXME: don't we actually only want SC_PART_*, and maybe SC_TRACK_DELETED?
            //             (same in waveview.cpp)
            updateItems();
            }

      MusECore::Event event;
      MusECore::WavePart* part   = 0;
      int x            = 0;
      CItem*   nevent  = 0;

      int n  = 0;       // count selections
      for (iCItem k = items.begin(); k != items.end(); ++k) {
            if (k->second->event().selected()) {
                  ++n;
                  if (!nevent) {
                        nevent   =  k->second;
                        }
                  }
            }
            
      if (flags & SC_AUDIO_STRETCH) {
        for(iStretchSelectedItem is = _stretchAutomation._stretchSelectedList.begin(); 
            is != _stretchAutomation._stretchSelectedList.end(); )
        {
          MusECore::MuseFrame_t frame   = is->first;
          StretchSelectedItem& ssi  = is->second;
          MusECore::StretchList* sl = ssi._sndFile.stretchList();
          if(!sl)
            continue;
          
          MusEGui::ciCItem i;
          for(i = items.begin(); i != items.end(); ++i)
          {
            WEvent* we = static_cast<WEvent*>(i->second);
            MusECore::Event e = we->event();
            if(MusECore::StretchList* e_sl = e.sndFile().stretchList())
            {
              if(e_sl == sl && e_sl->find(frame) != e_sl->end())
                break;
            }
          }
          
          if(i == items.end())
          {
            iStretchSelectedItem is_save = is;
            _stretchAutomation._stretchSelectedList.erase(is);
            is = is_save;
          }
          else
            ++is;
        }
      }
            
      
      if (flags & SC_CLIP_MODIFIED) {
            redraw(); // Boring, but the only thing possible to do
            }
      if (flags & SC_TEMPO) {
            setPos(0, MusEGlobal::song->cpos(), false);
            setPos(1, MusEGlobal::song->lpos(), false);
            setPos(2, MusEGlobal::song->rpos(), false);
            }
      
      if (n >= 1)    
      {
            x     = nevent->x();
            event = nevent->event();
            part  = (MusECore::WavePart*)nevent->part();
            if (_setCurPartIfOnlyOneEventIsSelected && n == 1 && curPart != part) {
                  curPart = part;
                  curPartId = curPart->uuid();
                  curPartChanged();
                  }
      }
      
      if(flags & (SC_SELECTION))
      {
        // Prevent race condition: Ignore if the change was ultimately sent by the canvas itself.
        if(flags._sender != this)
          updateItemSelections();
      }
      
      bool f1 = static_cast<bool>(flags & (SC_EVENT_INSERTED | SC_EVENT_MODIFIED | SC_EVENT_REMOVED | 
                         SC_PART_INSERTED | SC_PART_MODIFIED | SC_PART_REMOVED |
                         SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                         SC_SIG | SC_TEMPO | SC_KEY | SC_MASTER | SC_CONFIG | SC_DRUMMAP)); 
      bool f2 = static_cast<bool>(flags & SC_SELECTION);
      if(f1 || f2)   // Try to avoid all unnecessary emissions.
        emit selectionChanged(x, event, part, !f1);
      
      if (curPart == 0)
            curPart = (MusECore::WavePart*)(editor->parts()->begin()->second);
      redraw();
      }

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------

void WaveCanvas::selectAtTick(unsigned int tick)
      {
      selectAtFrame(MusEGlobal::tempomap.tick2frame(tick));
      }

//---------------------------------------------------------
//   selectAtFrame
//---------------------------------------------------------

void WaveCanvas::selectAtFrame(unsigned int frame)
      {
      //Select event nearest frame, if none selected and there are any
      if (!items.empty() && selectionSize() == 0) {
            iCItem i = items.begin();
            CItem* nearest = i->second;

            while (i != items.end()) {
                CItem* cur=i->second;                
                unsigned int curf=abs(cur->x() + (int)cur->part()->frame() - (int)frame);
                unsigned int nearf=abs(nearest->x() + (int)nearest->part()->frame() - (int)frame);

                if (curf < nearf) {
                    nearest=cur;
                    }

                i++;
                }

            if (!nearest->isSelected()) {
                  selectItem(nearest, true);
                  songChanged(SC_SELECTION);
                  }
            }
      }

////---------------------------------------------------------
////   getCaption
////---------------------------------------------------------

//QString WaveCanvas::getCaption() const
//      {
//      int bar1, bar2, xx;
//      unsigned x;
//      MusEGlobal::sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
//      MusEGlobal::sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);

//      return QString("Part <") + curPart->name()
//         + QString("> %1-%2").arg(bar1+1).arg(bar2+1);
//      }

//---------------------------------------------------------
//   track
//---------------------------------------------------------

MusECore::WaveTrack* WaveCanvas::track() const
      {
      return ((MusECore::WavePart*)curPart)->track();
      }


//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void WaveCanvas::keyPress(QKeyEvent* event)
      {
      int key = event->key();
      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
            key+= Qt::CTRL;

      if (key == shortcuts[SHRT_DELETE].key)
      {
        switch (_tool)
        {
          case StretchTool:
          case SamplerateTool:
          {
            // REMOVE Tim. wave. Added.
            event->accept();

            MusECore::PendingOperationList operations;
            StretchSelectedList_t& ssl = _stretchAutomation._stretchSelectedList;
            for(iStretchSelectedItem isi = ssl.begin(); isi != ssl.end(); ++isi)
            {
              StretchSelectedItem& ssi = isi->second;
//               ssi._sndFile.delAtStretchListOperation(ssi._type, isi->first, operations);
              MusEGlobal::song->delAtStretchListOperation(ssi._sndFile, ssi._type, isi->first, operations);
            }
            ssl.clear();
            MusEGlobal::audio->msgExecutePendingOperations(operations, true);
          }
          break;

          default:
          break;
        }

        return;
      }

      // TODO: New WaveCanvas: Convert these to frames, and remove unneeded functions.
            
      // Select items by key (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
              rciCItem i;

              if (items.empty())
                  return;
              for (i = items.rbegin(); i != items.rend(); ++i) 
                if (i->second->isSelected()) 
                  break;

              if(i == items.rend())
                i = items.rbegin();
              
              if(i != items.rbegin())
                --i;
              if(i->second)
              {
                if (key != shortcuts[SHRT_SEL_RIGHT_ADD].key)
                      deselectAll();
                CItem* sel = i->second;
                sel->setSelected(true);
                redraw();
                if (sel->x() + sel->width() > mapxDev(width())) 
                {  
                  int mx = rmapx(sel->x());  
                  int newx = mx + rmapx(sel->width()) - width();
                  // Leave a bit of room for the specially-drawn drum notes. But good for piano too.
                  emit horizontalScroll( (newx > mx ? mx - 10: newx + 10) - rmapx(xorg) );
                }  
              }
            }
      //Select items by key: (PianoRoll & DrumEditor)
      else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key) {
              ciCItem i;
              if (items.empty())
                  return;
              for (i = items.begin(); i != items.end(); ++i)
                if (i->second->isSelected()) 
                  break;

              if(i == items.end())
                i = items.begin();
              
              if(i != items.begin())
                --i;
              if(i->second)
              {
                if (key != shortcuts[SHRT_SEL_LEFT_ADD].key)
                      deselectAll();
                CItem* sel = i->second;
                sel->setSelected(true);
                redraw();
                if (sel->x() <= mapxDev(0)) 
                  emit horizontalScroll(rmapx(sel->x() - xorg) - 10);  // Leave a bit of room.
              }
            }
      //else if (key == shortcuts[SHRT_INC_PITCH].key) {
      //      modifySelected(NoteInfo::VAL_PITCH, 1);
      //      }
      //else if (key == shortcuts[SHRT_DEC_PITCH].key) {
      //      modifySelected(NoteInfo::VAL_PITCH, -1);
      //      }
      else if (key == shortcuts[SHRT_INC_POS].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_TIME, editor->raster());
            }
      else if (key == shortcuts[SHRT_DEC_POS].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_TIME, 0 - editor->raster());
            }

      else if (key == shortcuts[SHRT_INCREASE_LEN].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_LEN, editor->raster());
            }
      else if (key == shortcuts[SHRT_DECREASE_LEN].key) {
            // TODO: Check boundaries
            modifySelected(NoteInfo::VAL_LEN, 0 - editor->raster());
            }

      else
            event->ignore();
      }

//---------------------------------------------------------
//   keyRelease
//---------------------------------------------------------

void WaveCanvas::keyRelease(QKeyEvent* event)
{
      const int key = event->key();
      
      // We do not want auto-repeat events.
      // It does press and release repeatedly. Wait till the last release comes.
      if(!event->isAutoRepeat())
      {
        // Select part to the right
        if(key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key ||
        // Select part to the left
          key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key)
        {
          itemSelectionsChanged();
        }
        return;
      }
      
  EventCanvas::keyRelease(event);
}


//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void WaveCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
      val = MusEGlobal::tempomap.tick2frame(val);
      if (pos[idx] == val)
            return;
      int opos = mapx(pos[idx]);
      int npos = mapx(val);

      if (adjustScrollbar && idx == 0) {
            switch (MusEGlobal::song->follow()) {
                  case  MusECore::Song::NO:
                        break;
                  case MusECore::Song::JUMP:
                        if (npos >= width()) {
                              int ppos =  val - xorg - rmapxDev(width()/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - xorg - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
            case MusECore::Song::CONTINUOUS:
                        if (npos > (width()*5)/8) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()*5/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()*3)/8) {
                              int ppos =  pos[idx] - xorg - rmapxDev(width()*3/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  }
            }

      int x;
      int w = 1;
      if (opos > npos) {
            w += opos - npos;
            x = npos;
            }
      else {
            w += npos - opos;
            x = opos;
            }
      pos[idx] = val;
      redraw(QRect(x-1, 0, w+2, height()));    // From Canvas::draw (is otherwise identical). Fix for corruption. (TEST: New WaveCanvas: Still true?)
      }

//---------------------------------------------------------
//   setYScale
//---------------------------------------------------------

void WaveCanvas::setYScale(int val)
      {
      yScale = val;
      redraw();
      }

//---------------------------------------------------------
//   drawMarkers
//---------------------------------------------------------

void WaveCanvas::drawMarkers(QPainter& p, const QRect& mr, const QRegion&)
{
      const int mx = mr.x();
      const int my = mr.y();
      const int mw = mr.width();
      const int mh = mr.height();
      const int my_2 = my + mh;
      
      const ViewXCoordinate vx(mx, true);
      const ViewWCoordinate vw(mw, true);
      const ViewXCoordinate vx_2(mx + mw, true);
      
      QPen pen;
      pen.setCosmetic(true);
      
      MusECore::MarkerList* marker = MusEGlobal::song->marker();
      pen.setColor(MusEGlobal::config.markerColor);
      p.setPen(pen);
      for (MusECore::iMarker m = marker->begin(); m != marker->end(); ++m) {
            const ViewXCoordinate xp(MusEGlobal::tempomap.tick2frame(m->second.tick()), false);
            if (isXInRange(xp, vx, vx_2)) {
                  const int mxp = asMapped(xp)._value;
                  p.drawLine(mxp, my, mxp, my_2);
                  }
            }
}

//---------------------------------------------------------
//   drawWaveParts
//---------------------------------------------------------

// REMOVE Tim. wave. Changed.
// void WaveCanvas::drawParts(QPainter& p, bool do_cur_part, const QRect& mr, const QRegion&)
// {
//       bool wmtxen = p.worldMatrixEnabled();
//       p.setWorldMatrixEnabled(false);
//
//       if(do_cur_part)
//       {
//         // Draw current part:
//         if(curPart)
//         {
// // REMOVE Tim. wave. Changed.
// //               QRect mwpr  = map(QRect(curPart->frame(), 0, curPart->lenFrame(), height()));
//               QRect mwpr;
//               // If we are resizing borders (and the current item's part is the same as the current part)
//               //  then use the temp values in the canvas item, to give a 'live' dynamic update.
//               if(drag == DRAG_RESIZE && curItem && curItem->part() == curPart)
//                 mwpr = map(QRect(curItem->tmpPartPos(), 0, curItem->tmpPartLen(), height()));
//               else
//                 mwpr = map(QRect(curPart->frame(), 0, curPart->lenFrame(), height()));
//
//               QRect mpbgr = mr & mwpr;
//               if(!mpbgr.isNull())
//               {
//                 QColor c;
//                 switch(colorMode)
//                 {
//                   default:
//                   case 0:
//                     if (curPart->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
//                         c = curPart->track()->color();
//                     else
//                         c = MusEGlobal::config.partColors[curPart->colorIndex()];
//                     break;
//                   case 1:
//                     c = Qt::lightGray;
//                     break;
//                 }
//                 c.setAlpha(MusEGlobal::config.globalAlphaBlend);
//                 QBrush part_bg_brush(MusECore::gGradientFromQColor(c, mwpr.topLeft(), mwpr.bottomLeft()));
//                 p.fillRect(mpbgr, part_bg_brush);
//               }
//         }
//       }
//       else
//       {
//         // Draw non-current parts:
//         for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
//         {
//               MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
//               if(wp == curPart)
//                 continue;
//
//               QRect mwpr  = map(QRect(wp->frame(), 0, wp->lenFrame(), height()));
//               QRect mpbgr = mr & mwpr;
//               if(!mpbgr.isNull())
//               {
//                 QColor c(MusEGlobal::config.waveNonselectedPart);
//                 c.setAlpha(MusEGlobal::config.globalAlphaBlend);
//                 QBrush part_bg_brush(MusECore::gGradientFromQColor(c, mwpr.topLeft(), mwpr.bottomLeft()));
//                 p.fillRect(mpbgr, part_bg_brush);
//               }
//         }
//       }
//
//       p.setWorldMatrixEnabled(wmtxen);
// }

void WaveCanvas::drawParts(QPainter& p, bool do_cur_part, const QRect& mr, const QRegion&)
{
      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      const MusECore::Pos::TType canvasTType = MusECore::Pos::FRAMES;
      if(do_cur_part)
      {
        // Draw current part:
        if(curPart)
        {
              const MusECore::Pos::TType curPartTType = curPart->type();
              const MusECore::MuseCount_t partPosCType = MUSE_TIME_UINT_TO_INT64 curPart->posValue(curPartTType);
              const MusECore::MuseCount_t partEndCType =
                MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(curPart->posValue() + curPart->lenValue(), curPartTType, canvasTType);
              QRect mwpr;
              // If we are resizing borders (and the current item's part is the same as the current part)
              //  and the item end is greater than part end, then allow the part to expand to the item end,
              //  to give a 'live' dynamic update.
              if(drag == DRAG_RESIZE && curItem && curItem->part() == curPart)
              {
                const MusECore::MuseCount_t absItemEnd = partPosCType + curItem->x() + curItem->width();
                const MusECore::MuseCount_t finEnd = absItemEnd > partEndCType ? absItemEnd : partEndCType;
                mwpr = map(QRect(partPosCType, 0, finEnd - partPosCType, height()));
              }
              else
                mwpr = map(QRect(partPosCType, 0, partEndCType - partPosCType, height()));

              QRect mpbgr = mr & mwpr;
              if(!mpbgr.isNull())
              {
                QColor c;
                switch(colorMode)
                {
                  default:
                  case 0:
                    if (curPart->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
                        c = curPart->track()->color();
                    else
                        c = MusEGlobal::config.partColors[curPart->colorIndex()];
                    break;
                  case 1:
                    c = Qt::lightGray;
                    break;
                }
                c.setAlpha(MusEGlobal::config.globalAlphaBlend);
                QBrush part_bg_brush(MusECore::gGradientFromQColor(c, mwpr.topLeft(), mwpr.bottomLeft()));
                p.fillRect(mpbgr, part_bg_brush);
              }
        }
      }
      else
      {
        // Draw non-current parts:
        for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
        {
              MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
              if(wp == curPart)
                continue;

//               QRect mwpr  = map(QRect(wp->frame(), 0, wp->lenFrame(), height()));
              QRect mwpr  = map(QRect(MUSE_TIME_UINT_TO_INT64 wp->posValue(canvasTType), 0,
                MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wp->posValue() + wp->lenValue(), wp->type(), canvasTType), height()));
              QRect mpbgr = mr & mwpr;
              if(!mpbgr.isNull())
              {
                QColor c(MusEGlobal::config.waveNonselectedPart);
                c.setAlpha(MusEGlobal::config.globalAlphaBlend);
                QBrush part_bg_brush(MusECore::gGradientFromQColor(c, mwpr.topLeft(), mwpr.bottomLeft()));
                p.fillRect(mpbgr, part_bg_brush);
              }
        }
      }

      p.setWorldMatrixEnabled(wmtxen);
}

// TODO: Overridden because we're in units of frames. 
//       After BBT/frame mode is added to Canvas, remove this override and let Canvas do it.
//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint WaveCanvas::raster(const QPoint& p) const
      {
      int x = p.x();
      if (x < 0)
            x = 0;
      // Normally frame to tick methods round down. But here we need it to 'snap'
      //  the frame from either side of a tick to the tick. So round to nearest.
      x = MusEGlobal::tempomap.tick2frame(editor->rasterVal(MusEGlobal::tempomap.frame2tick(x, 0, MusECore::LargeIntRoundNearest)));
      int pitch = y2pitch(p.y());
      int y = pitch2y(pitch);
      return QPoint(x, y);
      }

#define WHEEL_STEPSIZE 50
#define WHEEL_DELTA   120
#define WHEEL_ANGLE_DELTA   15

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------
void WaveCanvas::wheelEvent(QWheelEvent* ev)
{
  int keyState = ev->modifiers();

  bool shift      = keyState & Qt::ShiftModifier;
  bool ctrl       = keyState & Qt::ControlModifier;

  const QPoint pixelDelta = ev->pixelDelta();
  const QPoint angleDegrees = ev->angleDelta() / 8;
  int delta = 0;
  if(!pixelDelta.isNull())
      delta = pixelDelta.y();
  else if(!angleDegrees.isNull())
      delta = angleDegrees.y() / WHEEL_ANGLE_DELTA;
  else
    return;

  if (shift) { // scroll horizontally
      int d       = -delta / WHEEL_DELTA;
      int xpixelscale = 5*MusECore::fast_log10(rmapxDev(1));
      if (xpixelscale <= 0)
            xpixelscale = 1;
      int scrollstep = WHEEL_STEPSIZE * ( d );
      scrollstep = scrollstep / 10;
      int newXpos = xpos + xpixelscale * scrollstep;
      if (newXpos < 0)
            newXpos = 0;
      emit horizontalScroll((unsigned)newXpos);
  } else if (ctrl) {  // zoom horizontally
#if QT_VERSION >= 0x050e00
      emit horizontalZoom(delta>0, ev->globalPosition().toPoint());
#else
      emit horizontalZoom(delta>0, ev->globalPos());
#endif
  } else { // scroll horizontally
      emit mouseWheelMoved(delta / 10);
  }
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

bool WaveCanvas::mousePress(QMouseEvent* event)
      {
//     if (event->modifiers() & Qt::ControlModifier) {
//             return true;
//             }
      const bool ctl = event->modifiers() & Qt::ControlModifier;
      button = event->button();
      QPoint pt = event->pos();
      unsigned x = event->x();

      switch (_tool)
      {
            default:
                  break;
            case RangeTool:
                  if (ctl)
                          return true;
                  switch (button) 
                  {
                        case Qt::LeftButton:
                              if (mode == NORMAL)
                              {
                                    // redraw and reset:
                                    if (selectionStart != selectionStop) 
                                    {
                                          selectionStart = selectionStop = 0;
                                          redraw();
                                    }
                                    mode = DRAG;
                                    dragstartx = x;
                                    selectionStart = selectionStop = x;
                                    drag = DRAG_LASSO_START;
                                    Canvas::start = pt;
                                    cancelMouseOps();
                                    return false;
                              }
                              break;

                        case Qt::MiddleButton:
                        case Qt::RightButton:
                        default:
                              break;
                  }

            break;
            
            case StretchTool:
            case SamplerateTool:
            {
              if(button != Qt::LeftButton)
                return true;
              
              StretchSelectedList_t& ssl = _stretchAutomation._stretchSelectedList;
              //if(!ctl)
              //{
              //  ssl.clear();
              //  update();
              //}
              
              // TODO Look properly through the whole list instead of just current one.
              //if(!curItem)
              //  break;
              //WEvent* wevent = static_cast<WEvent*>(curItem);
              CItem* item = items.find(pt);
              if(!item)
                break;
              WEvent* wevent = static_cast<WEvent*>(item);

              const MusECore::Event event = wevent->event();
              if(event.type() != MusECore::Wave)
                break;
              
              MusECore::SndFileR sf = event.sndFile();
              if(sf.isNull())
                break;
              
              MusECore::StretchList* sl = sf.stretchList();
              if(!sl)
                break;

             const double sf_sr_ratio  = sf->sampleRateRatio();

             MusECore::StretchListItem::StretchEventType type;
              if(_tool == StretchTool)
                type = MusECore::StretchListItem::StretchEvent;
              else //if(_tool == SamplerateTool)
                type = MusECore::StretchListItem::SamplerateEvent;
              
              MusECore::iStretchListItem isli_hit_test = stretchListHitTest(type, pt, wevent);
              if(isli_hit_test == sl->end())
              {
                if(!ctl)
                {
                  ssl.clear();
                  update();
                }

                double newframe = sl->unSquish(sf_sr_ratio * double(x - wevent->x()));

                MusECore::PendingOperationList operations;
                MusEGlobal::song->addAtStretchListOperation(sf, type, newframe, sl->ratioAt(type, newframe), operations);
                MusEGlobal::audio->msgExecutePendingOperations(operations, true);
                ssl.insert(StretchSelectedItemInsertPair_t(newframe, StretchSelectedItem(type, sf)));
                _stretchAutomation._startMovePoint = pt;
                _stretchAutomation._controllerState = stretchStartMove;
                QWidget::setCursor(Qt::SizeHorCursor);
                break;
              }

              iStretchSelectedItemPair res = ssl.equal_range(isli_hit_test->first);
              iStretchSelectedItem isi;
              for(isi = res.first; isi != res.second; ++isi)
                if(isi->second._sndFile.stretchList() == sl && isi->second._type)
                  break;

              if(isi != res.second)
              {
                if(ctl)
                {
                  ssl.erase(isi);
                  //setCursor();
                  update();
                }
                else
                {
                  _stretchAutomation._startMovePoint = pt;
                  _stretchAutomation._controllerState = stretchStartMove;
                  QWidget::setCursor(Qt::SizeHorCursor);
                }
              }
              else
              {
                if(!ctl)
                  ssl.clear();
                ssl.insert(std::pair<MusECore::MuseFrame_t, StretchSelectedItem>(isli_hit_test->first, 
                                                                                 StretchSelectedItem(type, sf)));
                _stretchAutomation._startMovePoint = pt;
                _stretchAutomation._controllerState = stretchStartMove;
                QWidget::setCursor(Qt::SizeHorCursor);
                update();
              }
            }
            break;
            
      }

  return true;
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void WaveCanvas::mouseRelease(QMouseEvent* ev)
{
  QPoint pt = ev->pos();
  const bool ctl = ev->modifiers() & Qt::ControlModifier;

  switch(_tool)
  {
    case StretchTool:
    case SamplerateTool:
    {
      if(button != Qt::LeftButton)
      {
        _stretchAutomation._controllerState = stretchDoNothing;
        setStretchAutomationCursor(pt);
        return;
      }

      StretchSelectedList_t& ssl = _stretchAutomation._stretchSelectedList;
      switch(_stretchAutomation._controllerState)
      {
        case stretchMovingController:
        case stretchAddNewController:
          //setCursor();
          break;

        case stretchDoNothing:
        case stretchStartMove:
          if(!ctl)
          {
            ssl.clear();
            update();
          }

          // TODO Look properly through the whole list instead of just current one.
          //if(!curItem)
          //  break;
          //WEvent* wevent = static_cast<WEvent*>(curItem);
          CItem* item = items.find(pt);
          if(!item)
            break;
          WEvent* wevent = static_cast<WEvent*>(item);

          const MusECore::Event event = wevent->event();
          if(event.type() != MusECore::Wave)
            break;

          const MusECore::SndFileR sf = event.sndFile();
          if(sf.isNull())
            break;

          MusECore::StretchList* sl = sf.stretchList();
          if(!sl)
            break;

          MusECore::StretchListItem::StretchEventType type;
          if(_tool == StretchTool)
            type = MusECore::StretchListItem::StretchEvent;
          else // if(_tool == SamplerateTool)
            type = MusECore::StretchListItem::SamplerateEvent;

          MusECore::iStretchListItem isli_hit_test = stretchListHitTest(type, pt, wevent);
          if(isli_hit_test == sl->end())
            break;

          iStretchSelectedItemPair res = ssl.equal_range(isli_hit_test->first);
          iStretchSelectedItem isi;
          for(isi = res.first; isi != res.second; ++isi)
            if(isi->second._sndFile.stretchList() == sl && isi->second._type)
              break;

          if(isi == res.second)
          {
            ssl.insert(std::pair<MusECore::MuseFrame_t, StretchSelectedItem>(isli_hit_test->first,
                                                                              StretchSelectedItem(type, sf)));
            update();
          }

          break;
      }


    }
    break;

    default:
      break;
  }

  _stretchAutomation._controllerState = stretchDoNothing;
      
  button = Qt::NoButton;
  if(mode == DRAG)
    mode = NORMAL;

  setStretchAutomationCursor(pt);
}

//---------------------------------------------------------
//   viewMousevent
//---------------------------------------------------------

void WaveCanvas::mouseMove(QMouseEvent* event)
      {
      QPoint pt = event->pos();
      int x = pt.x();
      if (x < 0)
            x = 0;
      
      // Special for wave canvas: Normally in the base class EventCanvas we would rasterize the tick before emitting,
      //  but since these units are in frames we handle displaying the position specially in the WaveEditor.
      // One reason is that we may need the precise frame, such as for the editing 'functions' and the stretching below.
      // Snapping those things, even to the minimum 1 tick ('off'), might be undesirable when we're editing at wave level.
      emit timeChanged(x);

      switch(_tool)
      {
            case StretchTool:
            case SamplerateTool:
            {
              event->accept();
              //bool slowMotion = event->modifiers() & Qt::ShiftModifier;
              //processStretchAutomationMovements(event->pos(), slowMotion);

              //if(button != Qt::LeftButton)
              //{
              //  _stretchAutomation._controllerState = stretchDoNothing;
              //  setCursor();
              //  return;
              //}
              
              switch(_stretchAutomation._controllerState)
              {
                case stretchDoNothing:
                case stretchAddNewController:
                  setStretchAutomationCursor(pt);
                break;
                
                case stretchStartMove:
                  _stretchAutomation._controllerState = stretchMovingController;
                // NOTE: Error suppressor for new gcc 7 'fallthrough' level 3 and 4:
                // FALLTHROUGH
                case stretchMovingController:
                {
                  if(button != Qt::LeftButton)
                  {
                    _stretchAutomation._controllerState = stretchDoNothing;
                    //setCursor();
                    break;;
                  }
                  QPoint delta_pt = QPoint(pt.x() - _stretchAutomation._startMovePoint.x(),
                                        pt.y() - _stretchAutomation._startMovePoint.y());
                  if(delta_pt.x() == 0)
                    break;
                  double prevNewVal = 0, thisNewVal = 0;
                  double prevFrame, thisFrame, nextFrame, delta_fr, next_delta_fr;
                  MusECore::PendingOperationList operations;
                  StretchSelectedList_t& ssl = _stretchAutomation._stretchSelectedList;
                  for(ciStretchSelectedItem iss = ssl.begin(); iss != ssl.end(); ++iss)
                  {
                    const StretchSelectedItem& ssi = iss->second;
                    MusECore::SndFileR sf = ssi._sndFile;
                    if(sf.isNull())
                      continue;
                    MusECore::StretchList* sl = sf.stretchList();
                    if(!sl)
                      continue;
                    
                    MusECore::iStretchListItem isli_typed = sl->findEvent(ssi._type, iss->first);
                    if(isli_typed == sl->end())
                      continue;

                    const double wave_sr_ratio = sf.sampleRateRatio();

                    thisFrame = double(isli_typed->first);
                    
                    const MusECore::StretchListItem& sli_typed = isli_typed->second;
                    
                    MusECore::iStretchListItem prev_isli_typed = sl->previousEvent(ssi._type, isli_typed);
                    if(prev_isli_typed == sl->end())
                      continue;
                    
                    prevFrame = double(prev_isli_typed->first);
                    
                    const MusECore::StretchListItem& prev_sli_typed = prev_isli_typed->second;
                    
                    MusECore::iStretchListItem next_isli_typed = sl->nextEvent(ssi._type, isli_typed);
                    if(next_isli_typed == sl->end())
                      nextFrame = double(sf.samples());
                    else
                      nextFrame = double(next_isli_typed->first);
                    
                    next_delta_fr = nextFrame - thisFrame;
                    // FIXME: Comparing double with zero.
                    if(next_delta_fr <= 0)
                      continue;
                    
                    delta_fr = thisFrame - prevFrame;
                    // FIXME: Comparing double with zero.
                    if(delta_fr <= 0)
                      continue;
                    
                    const double minStretchRatio = sf.minStretchRatio();
                    const double maxStretchRatio = sf.maxStretchRatio();
                    const double minSamplerateRatio = sf.minSamplerateRatio();
                    const double maxSamplerateRatio = sf.maxSamplerateRatio();

                    switch(ssi._type)
                    {
                      case MusECore::StretchListItem::StretchEvent:
                      {
                        const double left_prev_smpx = prev_sli_typed._samplerateSquishedFrame;
                        const double left_this_smpx = sl->squish(thisFrame, MusECore::StretchListItem::SamplerateEvent);
                        const double left_dsmpx = left_this_smpx - left_prev_smpx;
                        const double left_effective_sr = left_dsmpx / delta_fr;
                        
                        const double right_this_smpx = sli_typed._samplerateSquishedFrame;
                        const double right_next_smpx = sl->squish(nextFrame, MusECore::StretchListItem::SamplerateEvent);
                        const double right_dsmpx = right_next_smpx - right_this_smpx;
                        const double right_effective_sr = right_dsmpx / next_delta_fr;

                        INFO_WAVECANVAS(stderr, "WaveCanvas::mouseMove StretchEvent delta_pt.x:%d\n", delta_pt.x());

                        double left_min_stretch_delta_x =
                          (minStretchRatio - prev_sli_typed._stretchRatio) * left_effective_sr * delta_fr;
                        if(left_min_stretch_delta_x > 0.0)
                          left_min_stretch_delta_x = 0.0;
                        if((double)delta_pt.x() < left_min_stretch_delta_x)
                          delta_pt.setX(left_min_stretch_delta_x);

                        double right_min_stretch_delta_x =
                          (minStretchRatio - sli_typed._stretchRatio) * right_effective_sr * next_delta_fr;
                        if(right_min_stretch_delta_x > 0.0)
                          right_min_stretch_delta_x = 0.0;
                        if(-(double)delta_pt.x() < right_min_stretch_delta_x)
                          delta_pt.setX(right_min_stretch_delta_x);

                        INFO_WAVECANVAS(stderr, "  left_min_stretch_delta_x:%f right_min_stretch_delta_x:%f\n",
                                        left_min_stretch_delta_x, right_min_stretch_delta_x);

                        if(maxStretchRatio > 0.0)
                        {
                          double left_max_stretch_delta_x =
                                (maxStretchRatio - prev_sli_typed._stretchRatio) * left_effective_sr * delta_fr;
                        if(left_max_stretch_delta_x < 0.0)
                            left_max_stretch_delta_x = 0.0;
                          if((double)delta_pt.x() > left_max_stretch_delta_x)
                            delta_pt.setX(left_max_stretch_delta_x);

                          double right_max_stretch_delta_x =
                                (maxStretchRatio - sli_typed._stretchRatio) * right_effective_sr * next_delta_fr;
                          if(right_max_stretch_delta_x < 0.0)
                            right_max_stretch_delta_x = 0.0;
                          if((double)delta_pt.x() > right_max_stretch_delta_x)
                            delta_pt.setX(right_max_stretch_delta_x);
                        }

                        const double left_effective_dx = wave_sr_ratio * (double)delta_pt.x() / left_effective_sr;
                        prevNewVal = prev_sli_typed._stretchRatio + (left_effective_dx / delta_fr);

                        const double right_effective_dx = wave_sr_ratio * (double)delta_pt.x() / right_effective_sr;
                        thisNewVal = sli_typed._stretchRatio - (right_effective_dx / next_delta_fr);
                      }
                      break;
                      case MusECore::StretchListItem::SamplerateEvent:
                      {
                        const double left_prev_strx = prev_sli_typed._stretchSquishedFrame;
                        const double left_this_strx = sl->squish(thisFrame, MusECore::StretchListItem::StretchEvent);
                        const double left_dstrx = left_this_strx - left_prev_strx;
                        const double left_effective_str = left_dstrx / delta_fr;

                        const double right_this_strx = sli_typed._stretchSquishedFrame;
                        const double right_next_strx = sl->squish(nextFrame, MusECore::StretchListItem::StretchEvent);
                        const double right_dstrx = right_next_strx - right_this_strx;
                        const double right_effective_str = right_dstrx / next_delta_fr;

                        INFO_WAVECANVAS(stderr, "WaveCanvas::mouseMove SamplerateEvent delta_pt.x:%d\n", delta_pt.x());

                        double left_min_samplerate_delta_x =
                          (minSamplerateRatio - prev_sli_typed._samplerateRatio) * left_effective_str * delta_fr;
                        if(left_min_samplerate_delta_x > 0.0)
                          left_min_samplerate_delta_x = 0.0;
                        if((double)delta_pt.x() < left_min_samplerate_delta_x)
                          delta_pt.setX(left_min_samplerate_delta_x);

                        double right_min_samplerate_delta_x =
                          (minSamplerateRatio - sli_typed._samplerateRatio) * right_effective_str * next_delta_fr;
                        if(right_min_samplerate_delta_x > 0.0)
                          right_min_samplerate_delta_x = 0.0;
                        if(-(double)delta_pt.x() < right_min_samplerate_delta_x)
                          delta_pt.setX(right_min_samplerate_delta_x);

                        INFO_WAVECANVAS(stderr, "  left_min_samplerate_delta_x:%f right_min_samplerate_delta_x:%f\n",
                                        left_min_samplerate_delta_x, right_min_samplerate_delta_x);

                        if(maxSamplerateRatio > 0.0)
                        {
                          double left_max_samplerate_delta_x =
                            (maxSamplerateRatio - prev_sli_typed._samplerateRatio) * left_effective_str * delta_fr;
                          if(left_max_samplerate_delta_x < 0.0)
                            left_max_samplerate_delta_x = 0.0;
                          if((double)delta_pt.x() > left_max_samplerate_delta_x)
                            delta_pt.setX(left_max_samplerate_delta_x);

                          double right_max_samplerate_delta_x =
                            (maxSamplerateRatio - sli_typed._samplerateRatio) * right_effective_str * next_delta_fr;
                          if(right_max_samplerate_delta_x < 0.0)
                            right_max_samplerate_delta_x = 0.0;
                          if((double)delta_pt.x() > right_max_samplerate_delta_x)
                            delta_pt.setX(right_max_samplerate_delta_x);
                        }

                        const double left_effective_dx = wave_sr_ratio * (double)delta_pt.x() / left_effective_str;
                        prevNewVal =
                          1.0 / ((1.0 / prev_sli_typed._samplerateRatio) + (left_effective_dx / delta_fr));

                        const double right_effective_dx = wave_sr_ratio * (double)delta_pt.x() / right_effective_str;
                        thisNewVal =
                          1.0 / ((1.0 / sli_typed._samplerateRatio) - (right_effective_dx / next_delta_fr));
                      }
                      break;
                      case MusECore::StretchListItem::PitchEvent:
                        prevNewVal = prev_sli_typed._pitchRatio; // TODO
                      break;
                    }
                    
                    MusEGlobal::song->modifyAtStretchListOperation(sf, ssi._type, prevFrame, prevNewVal, operations);
                    MusEGlobal::song->modifyAtStretchListOperation(sf, ssi._type, thisFrame, thisNewVal, operations);
                  }
                  MusEGlobal::audio->msgExecutePendingOperations(operations, true);
                  _stretchAutomation._startMovePoint = pt;
                } 
                break;
              }
            }
            break;
            
            
            default:
            {
              event->ignore();
              
              switch (button) 
              {
                case Qt::LeftButton:
                      if (mode == DRAG) 
                      {
                            int mx      = mapx(x);
                            int mstart  = mapx(selectionStart);
                            int mstop   = mapx(selectionStop);
                            //int mdstart = mapx(dragstartx);
                            QRect r(0, 0, 0, height());
                            
                            if (x < dragstartx) {
                                  if(x < selectionStart)
                                  {
                                    r.setLeft(mx);
                                    r.setWidth((selectionStop >= dragstartx ? mstop : mstart) - mx);
                                  }
                                  else
                                  {
                                    r.setLeft(mstart);
                                    r.setWidth(mx - mstart);
                                  }
                                  selectionStart = x;
                                  selectionStop = dragstartx;
                                  }
                            else {
                                  if(x >= selectionStop)
                                  {
                                    r.setLeft(selectionStart < dragstartx ? mstart : mstop);
                                    r.setWidth(mx - (selectionStart < dragstartx ? mstart : mstop));
                                  }
                                  else
                                  {
                                    r.setLeft(mx);
                                    r.setWidth(mstop - mx);
                                  }
                                  selectionStart = dragstartx;
                                  selectionStop = x;
                                  }
                            update(r);
                      }
                      break;
                case Qt::MiddleButton:
                      break;
                case Qt::RightButton:
                      break;
                default:
                      return;
              }              
              
          }
          break;
      }
      }
      
//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int WaveCanvas::pitch2y(int) const
      {
      return 0;
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int WaveCanvas::y2pitch(int) const
      {
      return 0;
      }

//---------------------------------------------------------
//   drawItem
//    draws a wave
//---------------------------------------------------------

// REMOVE Tim. wave. Changed.
// void WaveCanvas::drawItem(QPainter& p, const CItem* item, const QRect& mr, const QRegion&)
// {
//       MusECore::WavePart* wp = (MusECore::WavePart*)(item->part());
//       if(!wp || !wp->track())
//         return;
//
//       MusECore::Event event  = item->event();
//       if(event.empty())
//         return;
//
//       //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
//       const QRect ur = mapDev(mr);               // Use our own map instead.
//       const int ux = ur.x();
//       const int uw = ur.width();
//       const int ux_2 = ux + uw;
// // REMOVE Tim. wave. Changed.
// //       QRect uwpr  = QRect(wp->frame(), 0, wp->lenFrame(), height());
//       //const QRect uwpr = drag == DRAG_RESIZE ?
//       //    QRect(item->tmpPartPos(), 0, item->tmpPartLen(), height()) :
//       //    QRect(wp->frame(), 0, wp->lenFrame(), height());
//       const QRect uwpr = QRect(item->tmpPartPos(), 0, item->tmpPartLen(), height());
//       const QRect ubbr = item->bbox();
//       const QRect ubbr_exp = item->bbox().adjusted(0, 0, rmapxDev(1), 0);
//       const QRect mbbr = map(ubbr);
//       const int ubbx = ubbr.x();
//       const int ubbx_2 = ubbr.x() + ubbr.width();
//       const int mbbx = mbbr.x();
//       const int mbbx_2 = mapx(ubbr.x() + ubbr.width());
//       const QRect ubr = ur & ubbr;
//       const QRect ubr_exp = ur & ubbr_exp;
//       const int uby_exp = ubr_exp.y();
//       const int uby_2exp = ubr_exp.y() + ubr_exp.height();
//       const int mby_exp = mapy(uby_exp);
//       const int mby_2exp = mapy(uby_2exp);
//       const QRect ubrwp = ubr & uwpr;
//       const QRect mbrwp = map(ubrwp);
//
//       QPen pen;
//       pen.setCosmetic(true);
//       const QColor left_ch_color(0, 170, 255);
//       const QColor right_ch_color(Qt::red);
//
//       int x1 = mapx(ubrwp.x());
//       int x2 = mapx(ubrwp.x() + ubrwp.width());
//       if (x1 < 0)
//             x1 = 0;
//       if (x2 > width())
//             x2 = width();
//       int hh = height();
//       int y1 = mapy(ubrwp.y());
//       int y2 = mapy(ubrwp.y() + ubrwp.height());
//
//       int xScale = xmag;
//       if (xScale < 0)
//             xScale = -xScale;
//
// // REMOVE Tim. wave. Changed.
// //       int px = wp->frame();
//       //const int px = drag == DRAG_RESIZE ? item->tmpPartPos() : wp->frame();
//       const int px = item->tmpPartPos();
//       const unsigned absEPos = px + item->tmpPos();
//
//       bool wmtxen = p.worldMatrixEnabled();
//       p.setWorldMatrixEnabled(false);
//
//       int sx, ex;
//
//       // Changed. Possible BUG ? Why the half nudge?
//       // sx = event.frame() + px + xScale/2;
// // REMOVE Tim. wave. Changed.
// //       sx = event.frame() + px;
// //       ex = sx + event.lenFrame();
//       sx = ubbx;
//       ex = ubbx_2;
//       sx = sx / xScale - xpos - xorg;
//       ex = ex / xScale - xpos - xorg;
//
//       if(sx >= x2 || ex < x1)
//         return;
//
//       if (sx < x1)
//             sx = x1;
//       if (ex > x2)
//             ex = x2;
//
// // REMOVE Tim. wave. Changed.
// //       const int ev_spos = event.spos();
//       //int ev_spos = event.spos();
//       //if(drag == DRAG_COPY)
//       //{
//       //  ev_spos += ubbx - (event.frame() + px);
//       //  if(ev_spos < 0)
//       //    ev_spos = 0;
//       //}
//       //const int ev_spos = drag == DRAG_RESIZE ? item->tmpWaveSPos() : event.spos();
//       const int ev_spos = item->tmpWaveSPos();
//
// // REMOVE Tim. wave. Changed.
// //       int pos = (xpos + xorg + sx) * xScale - event.frame() - px;
//       //int pos = (xpos + xorg + sx) * xScale - ubbx;
//       //int pos = drag == DRAG_RESIZE ? item->tmpPos() : (xpos + xorg + sx) * xScale - event.frame() - px;
//       //int pos = (xpos + xorg + sx) * xScale - (drag == DRAG_RESIZE ? item->tmpPos() : event.frame()) - px;
//       //int pos = (xpos + xorg + sx) * xScale - item->tmpPos() - px;
//       int pos = (xpos + xorg + sx) * xScale - absEPos;
//
// //       fprintf(stderr, "\nWaveCanvas::drawItem:\nmr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               mr.x(), mr.y(), mr.width(), mr.height());
// //       fprintf(stderr, "\nur:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               ur.x(), ur.y(), ur.width(), ur.height());
// //       fprintf(stderr, "\nubbr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               ubbr.x(), ubbr.y(), ubbr.width(), ubbr.height());
// //       fprintf(stderr, "\nmbbr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               mbbr.x(), mbbr.y(), mbbr.width(), mbbr.height());
//
//       QBrush brush;
//       if (item->isMoving())
//       {
//             QColor c(Qt::gray);
//             c.setAlpha(MusEGlobal::config.globalAlphaBlend);
//             QLinearGradient gradient(ubbr.topLeft(), ubbr.bottomLeft());
//             gradient.setColorAt(0, c);
//             gradient.setColorAt(1, c.darker());
//             brush = QBrush(gradient);
//             p.fillRect(sx, y1, ex - sx + 1, y2, brush);
//       }
//       else
//       if (item->isSelected())
//       {
//           QColor c(Qt::black);
//           c.setAlpha(MusEGlobal::config.globalAlphaBlend);
//           QLinearGradient gradient(ubbr.topLeft(), ubbr.bottomLeft());
//           // Use a colour only about 20% lighter than black, rather than the 50% we use in MusECore::gGradientFromQColor
//           //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
//           gradient.setColorAt(0, QColor(51, 51, 51, MusEGlobal::config.globalAlphaBlend));
//           gradient.setColorAt(1, c);
//           brush = QBrush(gradient);
//           p.fillRect(sx, y1, ex - sx + 1, y2, brush);
//       }
//
//       int ev_channels = 0;
//       int wav_sx = 0;
//       int wav_ex = 0;
//       int wsx = 0;
//       int wex = 0;
//       bool wave_visible = false;
//
//       MusECore::SndFileR f = event.sndFile();
//       if(!f.isNull())
//       {
//         ev_channels = f.channels();
//         if (ev_channels > 0) {
//
//           int h   = hh / (ev_channels * 2);
//           int cc  = hh % (ev_channels * 2) ? 0 : 1;
//
// // REMOVE Tim. wave. Changed.
// //           unsigned peoffset = px + event.frame() - ev_spos;
//           //const unsigned absEPos = px + (drag == DRAG_RESIZE ? item->tmpPos() : event.frame());
//           //const unsigned eLen = drag == DRAG_RESIZE ? item->tmpLen() : event.lenFrame();
//           //const unsigned absEPos = px + item->tmpPos();
//           const unsigned eLen = item->tmpLen();
//           const unsigned peoffset = absEPos - ev_spos;
//
//           const sf_count_t smps = f.samples();
//
//           if(-ev_spos < smps && ev_spos <= smps)
//           {
//             wave_visible = true;
//             wav_sx = -ev_spos;
//             wav_ex = smps - ev_spos;
//             if(wav_sx < 0)
//               wav_sx = 0;
// // REMOVE Tim. wave. Changed.
// //             wav_sx += event.frame() + wp->frame();
//             wav_sx += absEPos;
//
//             wav_ex = f.unConvertPosition(wav_ex);
// // REMOVE Tim. wave. Changed.
// //             if(wav_ex >= (int)event.lenFrame())
//             if(wav_ex >= (int)eLen)
//             {
// // REMOVE Tim. wave. Changed.
// //               wav_ex = event.lenFrame();
//               wav_ex = eLen;
//               if(wav_ex > 0)
//                 --wav_ex;
//             }
// // REMOVE Tim. wave. Changed.
// //             wav_ex += event.frame() + wp->frame();
//             wav_ex += absEPos;
//
//             wav_sx = wav_sx / xScale - xpos - xorg;
//             wav_ex = wav_ex / xScale - xpos - xorg;
//             wsx = wav_sx < x1 ? x1 : wav_sx;
//             wex = wav_ex > x2 ? x2 : wav_ex;
//           }
//
//   //         fprintf(stderr, "WaveCanvas::drawItem: rect x:%d w:%d rr x:%d w:%d mr x:%d w:%d pos:%d sx:%d ex:%d\n",
//   //                 rect.x(), rect.width(),
//   //                 rr.x(), rr.width(),
//   //                 mr.x(), mr.width(),
//   //                 pos,
//   //                 sx, ex);
//
//           for (int i = sx; i < ex; i++) {
//                 int y = h;
//                 MusECore::SampleV sa[f.channels()];
//                 if((ev_spos + f.convertPosition(pos)) > smps)
//                   break;
//                 // Seek the file only once, not with every read!
//                 if(i == sx)
//                 {
//                   if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, ev_spos) == -1)
//                     break;
//                 }
//                 f.readConverted(sa, xScale, pos, ev_spos);
//
//                 pos += xScale;
//                 if (pos < 0)
//                       continue;
//
//                 int selectionStartPos = selectionStart - peoffset; // Offset transformed to event coords
//                 int selectionStopPos  = selectionStop  - peoffset;
//
//                 for (int k = 0; k < ev_channels; ++k) {
//                       int kk = k % f.channels();
//                       int peak = (sa[kk].peak * (h - 1)) / yScale;
//                       int rms  = (sa[kk].rms  * (h - 1)) / yScale;
//                       if (peak > h)
//                             peak = h;
//                       if (rms > h)
//                             rms = h;
//                       QColor peak_color = MusEGlobal::config.wavePeakColor;
//                       QColor rms_color  = MusEGlobal::config.waveRmsColor;
//
//                       if ((ev_spos + pos) > selectionStartPos && (ev_spos + pos) <= selectionStopPos) {
//                             peak_color = MusEGlobal::config.wavePeakColorSelected;
//                             rms_color  = MusEGlobal::config.waveRmsColorSelected;
//                             QLine l_inv = clipQLine(i, y - h + cc, i, y + h - cc, mbrwp);
//                             if(!l_inv.isNull())
//                             {
//                               // Draw inverted
//                               pen.setColor(QColor(Qt::black));
//                               p.setPen(pen);
//                               p.drawLine(l_inv);
//                             }
//                           }
//
//                       QLine l_peak = clipQLine(i, y - peak - cc, i, y + peak, mbrwp);
//                       if(!l_peak.isNull())
//                       {
//                         pen.setColor(peak_color);
//                         p.setPen(pen);
//                         p.drawLine(l_peak);
//                       }
//
//                       QLine l_rms = clipQLine(i, y - rms - cc, i, y + rms, mbrwp);
//                       if(!l_rms.isNull())
//                       {
//                         pen.setColor(rms_color);
//                         p.setPen(pen);
//                         p.drawLine(l_rms);
//                       }
//
//                       y += 2 * h;
//                     }
//                 }
//
//             // Only if there's something to draw.
//             if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
//             {
//               const int hn = hh / ev_channels;
//               const int hhn = hn / 2;
//               for (int i = 0; i < ev_channels; ++i) {
//                     const int h2     = hn * i;
//                     const int center = hhn + h2;
//                     if(center >= y1 && center < y2)
//                     {
//                       pen.setColor(QColor(i & 1 ? right_ch_color : left_ch_color));
//                       p.setPen(pen);
//                       p.drawLine(wsx, center, wex, center);
//                     }
//                   }
//             }
//           }
//         }
//
//         const int h2 = hh / 2;
//         if(h2 >= y1 && h2 < y2)
//         {
//           pen.setColor(QColor(Qt::black));
//           p.setPen(pen);
//           // Draw the complete line only if there are an even number of channels (space for the line in the middle).
//           // Ensure a complete line is drawn even if there is no sound file or channels.
//           if((ev_channels & 1) == 0)
//             p.drawLine(sx, h2, ex, h2);
//           else
//           {
//             // Draw only the required two segments of the line.
//             if(wave_visible)
//             {
//               if(sx < wsx)
//                 p.drawLine(sx, h2, wsx - 1, h2);
//               if(wex < ex)
//                 p.drawLine(wex + 1, h2, ex, h2);
//             }
//           }
//         }
//
//         // REMOVE Tim. wave. Added.
//         {
//           // Draw remaining 'hidden events' decorations with 'jagged' edges...
//           QColor partColor;
//           if (wp->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
//               partColor = wp->track()->color();
//           else
//               partColor = MusEGlobal::config.partColors[wp->colorIndex()];
//           int color_brightness;
//           if (MusECore::isColorBright(partColor) && !item->isSelected())
//               color_brightness=96; //0;    // too light: use dark color
//           else
//               color_brightness=180; //255;   // too dark: use lighter color
//           QColor c(color_brightness,color_brightness,color_brightness, MusEGlobal::config.globalAlphaBlend);
//           const int gradS = qBound(0, MusEGlobal::config.partGradientStrength, 200);
//           p.setBrush(MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS));
//
//           const int h = mbbr.height();
//           const double s = double(h) / 4.0;
//
//           const int xs_0 = mbbr.x();
//           const int xe_0 = xs_0 + mbbr.width();
//           int xs_1 = xs_0 + 1;
//           if(xs_1 > xe_0)
//             xs_1 = xe_0;
//           int xs_j = xs_0 + 8;
//           if(xs_j > xe_0)
//             xs_j = xe_0;
//
//           int xe_1 = xe_0 - 1;
//           if(xe_1 < xs_0)
//             xe_1 = xs_0;
//           int xe_j = xe_0 - 8;
//           if(xe_j < xs_0)
//             xe_j = xs_0;
//
//           const int ys_0 = mbbr.y();
//
//           const int y0 = ys_0;
//           const int y2 = y0 + lrint(s * 2.0);
//           const int y4 = y0 + h;
//
//           QPoint points[8];
//           int pts;
//
//           //if(het & MusECore::Part::RightEventsHidden)
//           {
//             pts = 0;
//             points[pts++] = QPoint(xe_0, y0);
//             points[pts++] = QPoint(xe_0, y4);
//             points[pts++] = QPoint(xe_j, y2);
//
//             p.drawConvexPolygon(points, pts);
//           }
//
//   //         if(het & MusECore::Part::LeftEventsHidden)
//           if(ev_spos > 0)
//           {
//             pts = 0;
//             points[pts++] = QPoint(xs_0, y0);
//             points[pts++] = QPoint(xs_j, y2);
//             points[pts++] = QPoint(xs_0, y4);
//
//             p.drawConvexPolygon(points, pts);
//           }
//
//         }
//       //
//       // Draw custom dashed borders around the wave event
//       //
//
//       QColor color(item->isSelected() ? Qt::white : Qt::black);
//       QPen penH(color);
//       QPen penV(color);
//       penH.setCosmetic(true);
//       penV.setCosmetic(true);
//       QVector<qreal> customDashPattern;
//       customDashPattern << 4.0 << 6.0;
//       penH.setDashPattern(customDashPattern);
//       penV.setDashPattern(customDashPattern);
//       penV.setDashOffset(2.0);
//       // FIXME: Some shifting still going on. Values likely not quite right here.
//       //int xdiff = sx - r.x();
//       int xdiff = sx - mbbx;
//       if(xdiff > 0)
//       {
//         int doff = xdiff % 10;
//         penH.setDashOffset(doff);
//       }
//       // Tested OK. Each segment drawn only when necessary.
//       if(y1 <= 0)
//       {
//         p.setPen(penH);
//         p.drawLine(sx, 0, ex, 0);
//       }
//       if(y2 >= hh - 1)
//       {
//         p.setPen(penH);
//         p.drawLine(sx, hh - 1, ex, hh - 1);
//       }
//
//       //fprintf(stderr, "...Checking left edge: ubbx:%d ux:%d ux_2:%d\n", ubbx, ux, ux_2);
//       if(ubbx >= ux && ubbx < ux_2)
//       {
//         //fprintf(stderr, "...Drawing left edge at mbbx:%d mby_exp:%d mby_2exp:%d\n", mbbx, mby_exp, mby_2exp);
//
//         p.setPen(penV);
//         p.drawLine(mbbx, mby_exp, mbbx, mby_2exp);
//       }
//
//
//       //fprintf(stderr, "...Checking right edge: ubbx_2:%d ux:%d ux_2:%d\n", ubbx_2, ux, ux_2);
//       if(ubbx_2 >= ux && ubbx_2 < ux_2)
//       {
//         //fprintf(stderr, "...Drawing right edge at mbbx_2:%d mby_exp:%d mby_2exp:%d\n", mbbx_2, mby_exp, mby_2exp);
//
//         p.setPen(penV);
//         p.drawLine(mbbx_2, mby_exp, mbbx_2, mby_2exp);
//       }
//
//       // Done. Restore and return.
//       p.setWorldMatrixEnabled(wmtxen);
// }

// void WaveCanvas::drawItem(QPainter& p, const CItem* item, const QRect& mr, const QRegion&)
// {
//       MusECore::WavePart* wp = (MusECore::WavePart*)(item->part());
//       if(!wp || !wp->track())
//         return;
//
//       const MusECore::Event event  = item->event();
//       if(event.empty())
//         return;
//
// // REMOVE Tim. wave. Added.
//       const MusECore::Pos::TType eType = event.pos().type();
//
//       //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
//       const QRect ur = mapDev(mr);               // Use our own map instead.
//       const int ux = ur.x();
//       const int uw = ur.width();
//       const int ux_2 = ux + uw;
// // REMOVE Tim. wave. Changed.
// //       QRect uwpr  = QRect(wp->frame(), 0, wp->lenFrame(), height());
//       //const QRect uwpr = drag == DRAG_RESIZE ?
//       //    QRect(item->tmpPartPos(), 0, item->tmpPartLen(), height()) :
//       //    QRect(wp->frame(), 0, wp->lenFrame(), height());
//       const QRect uwpr = QRect(item->tmpPartPos(), 0, item->tmpPartLen(), height());
// //       const QRect ubbr = item->bbox();
// //       const QRect ubbr_exp = item->bbox().adjusted(0, 0, rmapxDev(1), 0);
//
//       const int px = item->tmpPartPos();
//       const unsigned pPosEType = MusECore::Pos::convert(px, wp->type(), eType);
//       const unsigned absEPos = pPosEType + item->tmpPos();
//       const unsigned eLen = item->tmpLen();
//       const unsigned absEEnd = absEPos + eLen;
//       const unsigned absEPosFrames = MusECore::Pos::convert(absEPos, eType, MusECore::Pos::FRAMES);
//       const unsigned absEEndFrames = MusECore::Pos::convert(absEEnd, eType, MusECore::Pos::FRAMES);
//
//       const QRect ubbr = QRect(absEPosFrames, item->bbox().y(), absEEndFrames - absEPosFrames, item->bbox().height());
//       const QRect ubbr_exp = ubbr.adjusted(0, 0, rmapxDev(1), 0);
//
//
//       const QRect mbbr = map(ubbr);
//       const int ubbx = ubbr.x();
//       const int ubbx_2 = ubbr.x() + ubbr.width();
//       const int mbbx = mbbr.x();
//       const int mbbx_2 = mapx(ubbr.x() + ubbr.width());
//       const QRect ubr = ur & ubbr;
//       const QRect ubr_exp = ur & ubbr_exp;
//       const int uby_exp = ubr_exp.y();
//       const int uby_2exp = ubr_exp.y() + ubr_exp.height();
//       const int mby_exp = mapy(uby_exp);
//       const int mby_2exp = mapy(uby_2exp);
//       const QRect ubrwp = ubr & uwpr;
//       const QRect mbrwp = map(ubrwp);
//
//       QPen pen;
//       pen.setCosmetic(true);
//       const QColor left_ch_color(0, 170, 255);
//       const QColor right_ch_color(Qt::red);
//
//       int x1 = mapx(ubrwp.x());
//       int x2 = mapx(ubrwp.x() + ubrwp.width());
//       if (x1 < 0)
//             x1 = 0;
//       if (x2 > width())
//             x2 = width();
//       int hh = height();
//       int y1 = mapy(ubrwp.y());
//       int y2 = mapy(ubrwp.y() + ubrwp.height());
//
//       int xScale = xmag;
//       if (xScale < 0)
//             xScale = -xScale;
//
// // REMOVE Tim. wave. Changed.
// //       int px = wp->frame();
//       //const int px = drag == DRAG_RESIZE ? item->tmpPartPos() : wp->frame();
// //       const int px = item->tmpPartPos();
// //       const unsigned pPosEType = MusECore::Pos::convert(px, wp->type(), eType);
// //       const unsigned absEPos = pPosEType + item->tmpPos();
// //       const unsigned eLen = item->tmpLen();
// //       const unsigned absEEnd = absEPos + eLen;
// //
// //       const unsigned absEPosFrames = MusECore::Pos::convert(absEPos, eType, MusECore::Pos::FRAMES);
// //       const unsigned absEEndFrames = MusECore::Pos::convert(absEEnd, eType, MusECore::Pos::FRAMES);
//
//       bool wmtxen = p.worldMatrixEnabled();
//       p.setWorldMatrixEnabled(false);
//
//       int sx, ex;
//
//       // Changed. Possible BUG ? Why the half nudge?
//       // sx = event.frame() + px + xScale/2;
// // REMOVE Tim. wave. Changed.
// //       sx = event.frame() + px;
// //       ex = sx + event.lenFrame();
//       sx = ubbx;
//       ex = ubbx_2;
//       sx = sx / xScale - xpos - xorg;
//       ex = ex / xScale - xpos - xorg;
//
//       if(sx >= x2 || ex < x1)
//         return;
//
//       if (sx < x1)
//             sx = x1;
//       if (ex > x2)
//             ex = x2;
//
// // REMOVE Tim. wave. Changed.
// //       const int ev_spos = event.spos();
//       //int ev_spos = event.spos();
//       //if(drag == DRAG_COPY)
//       //{
//       //  ev_spos += ubbx - (event.frame() + px);
//       //  if(ev_spos < 0)
//       //    ev_spos = 0;
//       //}
//       //const int ev_spos = drag == DRAG_RESIZE ? item->tmpWaveSPos() : event.spos();
//       const int ev_spos = item->tmpWaveSPos();
//
// // REMOVE Tim. wave. Changed.
// //       int pos = (xpos + xorg + sx) * xScale - event.frame() - px;
//       //int pos = (xpos + xorg + sx) * xScale - ubbx;
//       //int pos = drag == DRAG_RESIZE ? item->tmpPos() : (xpos + xorg + sx) * xScale - event.frame() - px;
//       //int pos = (xpos + xorg + sx) * xScale - (drag == DRAG_RESIZE ? item->tmpPos() : event.frame()) - px;
//       //int pos = (xpos + xorg + sx) * xScale - item->tmpPos() - px;
//       int pos = (xpos + xorg + sx) * xScale - absEPos;
//
// //       fprintf(stderr, "\nWaveCanvas::drawItem:\nmr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               mr.x(), mr.y(), mr.width(), mr.height());
// //       fprintf(stderr, "\nur:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               ur.x(), ur.y(), ur.width(), ur.height());
// //       fprintf(stderr, "\nubbr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               ubbr.x(), ubbr.y(), ubbr.width(), ubbr.height());
// //       fprintf(stderr, "\nmbbr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
// //               mbbr.x(), mbbr.y(), mbbr.width(), mbbr.height());
//
//       QBrush brush;
//       if (item->isMoving())
//       {
//             QColor c(Qt::gray);
//             c.setAlpha(MusEGlobal::config.globalAlphaBlend);
//             QLinearGradient gradient(ubbr.topLeft(), ubbr.bottomLeft());
//             gradient.setColorAt(0, c);
//             gradient.setColorAt(1, c.darker());
//             brush = QBrush(gradient);
//             p.fillRect(sx, y1, ex - sx + 1, y2, brush);
//       }
//       else
//       if (item->isSelected())
//       {
//           QColor c(Qt::black);
//           c.setAlpha(MusEGlobal::config.globalAlphaBlend);
//           QLinearGradient gradient(ubbr.topLeft(), ubbr.bottomLeft());
//           // Use a colour only about 20% lighter than black, rather than the 50% we use in MusECore::gGradientFromQColor
//           //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
//           gradient.setColorAt(0, QColor(51, 51, 51, MusEGlobal::config.globalAlphaBlend));
//           gradient.setColorAt(1, c);
//           brush = QBrush(gradient);
//           p.fillRect(sx, y1, ex - sx + 1, y2, brush);
//       }
//
//       int ev_channels = 0;
//       int wav_sx = 0;
//       int wav_ex = 0;
//       int wsx = 0;
//       int wex = 0;
//       bool wave_visible = false;
//
//       MusECore::SndFileR f = event.sndFile();
//       if(!f.isNull())
//       {
//         ev_channels = f.channels();
//         if (ev_channels > 0) {
//
//           int h   = hh / (ev_channels * 2);
//           int cc  = hh % (ev_channels * 2) ? 0 : 1;
//
// // REMOVE Tim. wave. Changed.
// //           unsigned peoffset = px + event.frame() - ev_spos;
//           //const unsigned absEPos = px + (drag == DRAG_RESIZE ? item->tmpPos() : event.frame());
//           //const unsigned eLen = drag == DRAG_RESIZE ? item->tmpLen() : event.lenFrame();
//           //const unsigned absEPos = px + item->tmpPos();
//           const unsigned peoffset = absEPos - ev_spos;
//
//           const sf_count_t smps = f.samples();
//           //const sf_count_t smpsConverted = f.samplesConverted();
//
//           if(-ev_spos < smps && ev_spos <= smps)
//           {
//             wave_visible = true;
//             wav_sx = -ev_spos;
//             wav_ex = smps - ev_spos;
//             if(wav_sx < 0)
//               wav_sx = 0;
// // REMOVE Tim. wave. Changed.
// //             wav_sx += event.frame() + wp->frame();
//             wav_sx += absEPos;
//
//             wav_ex = f.unConvertPosition(wav_ex);
// // REMOVE Tim. wave. Changed.
// //             if(wav_ex >= (int)event.lenFrame())
//             if(wav_ex >= (int)eLen)
//             {
// // REMOVE Tim. wave. Changed.
// //               wav_ex = event.lenFrame();
//               wav_ex = eLen;
//               if(wav_ex > 0)
//                 --wav_ex;
//             }
// // REMOVE Tim. wave. Changed.
// //             wav_ex += event.frame() + wp->frame();
//             wav_ex += absEPos;
//
//             wav_sx = wav_sx / xScale - xpos - xorg;
//             wav_ex = wav_ex / xScale - xpos - xorg;
//             wsx = wav_sx < x1 ? x1 : wav_sx;
//             wex = wav_ex > x2 ? x2 : wav_ex;
//           }
//
//   //         fprintf(stderr, "WaveCanvas::drawItem: rect x:%d w:%d rr x:%d w:%d mr x:%d w:%d pos:%d sx:%d ex:%d\n",
//   //                 rect.x(), rect.width(),
//   //                 rr.x(), rr.width(),
//   //                 mr.x(), mr.width(),
//   //                 pos,
//   //                 sx, ex);
//
//           for (int i = sx; i < ex; i++) {
//                 int y = h;
//                 MusECore::SampleV sa[f.channels()];
//                 if((ev_spos + f.convertPosition(pos)) > smps)
//                   break;
//                 // Seek the file only once, not with every read!
//                 if(i == sx)
//                 {
//                   if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, ev_spos) == -1)
//                     break;
//                 }
//                 f.readConverted(sa, xScale, pos, ev_spos);
//
//                 pos += xScale;
//                 if (pos < 0)
//                       continue;
//
//                 int selectionStartPos = selectionStart - peoffset; // Offset transformed to event coords
//                 int selectionStopPos  = selectionStop  - peoffset;
//
//                 for (int k = 0; k < ev_channels; ++k) {
//                       int kk = k % f.channels();
//                       int peak = (sa[kk].peak * (h - 1)) / yScale;
//                       int rms  = (sa[kk].rms  * (h - 1)) / yScale;
//                       if (peak > h)
//                             peak = h;
//                       if (rms > h)
//                             rms = h;
//                       QColor peak_color = MusEGlobal::config.wavePeakColor;
//                       QColor rms_color  = MusEGlobal::config.waveRmsColor;
//
//                       if ((ev_spos + pos) > selectionStartPos && (ev_spos + pos) <= selectionStopPos) {
//                             peak_color = MusEGlobal::config.wavePeakColorSelected;
//                             rms_color  = MusEGlobal::config.waveRmsColorSelected;
//                             QLine l_inv = clipQLine(i, y - h + cc, i, y + h - cc, mbrwp);
//                             if(!l_inv.isNull())
//                             {
//                               // Draw inverted
//                               pen.setColor(QColor(Qt::black));
//                               p.setPen(pen);
//                               p.drawLine(l_inv);
//                             }
//                           }
//
//                       QLine l_peak = clipQLine(i, y - peak - cc, i, y + peak, mbrwp);
//                       if(!l_peak.isNull())
//                       {
//                         pen.setColor(peak_color);
//                         p.setPen(pen);
//                         p.drawLine(l_peak);
//                       }
//
//                       QLine l_rms = clipQLine(i, y - rms - cc, i, y + rms, mbrwp);
//                       if(!l_rms.isNull())
//                       {
//                         pen.setColor(rms_color);
//                         p.setPen(pen);
//                         p.drawLine(l_rms);
//                       }
//
//                       y += 2 * h;
//                     }
//                 }
//
//             // Only if there's something to draw.
//             if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
//             {
//               const int hn = hh / ev_channels;
//               const int hhn = hn / 2;
//               for (int i = 0; i < ev_channels; ++i) {
//                     const int h2     = hn * i;
//                     const int center = hhn + h2;
//                     if(center >= y1 && center < y2)
//                     {
//                       pen.setColor(QColor(i & 1 ? right_ch_color : left_ch_color));
//                       p.setPen(pen);
//                       p.drawLine(wsx, center, wex, center);
//                     }
//                   }
//             }
//
//             // REMOVE Tim. wave. Added.
//             {
//               p.setPen(Qt::NoPen);
//               // Draw remaining 'hidden events' decorations with 'jagged' edges...
//               QColor partColor;
//               if (wp->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
//                   partColor = wp->track()->color();
//               else
//                   partColor = MusEGlobal::config.partColors[wp->colorIndex()];
//               int color_brightness;
//               if (MusECore::isColorBright(partColor) && !item->isSelected())
//                   color_brightness=96; //0;    // too light: use dark color
//               else
//                   color_brightness=180; //255;   // too dark: use lighter color
//               QColor c(color_brightness,color_brightness,color_brightness, MusEGlobal::config.globalAlphaBlend);
//               const int gradS = qBound(0, MusEGlobal::config.partGradientStrength, 200);
//               p.setBrush(MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS));
//
//               const int h = mbbr.height();
//               const double s = double(h) / 4.0;
//
//               const int xs_0 = mbbr.x();
//               const int xe_0 = xs_0 + mbbr.width();
//               int xs_1 = xs_0 + 1;
//               if(xs_1 > xe_0)
//                 xs_1 = xe_0;
//               int xs_j = xs_0 + 8;
//               if(xs_j > xe_0)
//                 xs_j = xe_0;
//
//               int xe_1 = xe_0 - 1;
//               if(xe_1 < xs_0)
//                 xe_1 = xs_0;
//               int xe_j = xe_0 - 8;
//               if(xe_j < xs_0)
//                 xe_j = xs_0;
//
//               const int ys_0 = mbbr.y();
//
//               const int y0 = ys_0;
//               const int y2 = y0 + lrint(s * 2.0);
//               const int y4 = y0 + h;
//
//               QPoint points[8];
//               int pts;
//
//               //if(het & MusECore::Part::RightEventsHidden)
//               //if(-ev_spos >= smps || (smps > ev_spos &&
//               if(smps > ev_spos &&
//                 absEPosFrames + f.unConvertPosition(smps - ev_spos) > absEEndFrames)
//               {
//                 pts = 0;
//                 points[pts++] = QPoint(xe_0, y0);
//                 points[pts++] = QPoint(xe_0, y4);
//                 points[pts++] = QPoint(xe_j, y2);
//
//                 p.drawConvexPolygon(points, pts);
//               }
//
//       //         if(het & MusECore::Part::LeftEventsHidden)
//               if(ev_spos > 0)
//               {
//                 pts = 0;
//                 points[pts++] = QPoint(xs_0, y0);
//                 points[pts++] = QPoint(xs_j, y2);
//                 points[pts++] = QPoint(xs_0, y4);
//
//                 p.drawConvexPolygon(points, pts);
//               }
//             }
//
//           }
//         }
//
//         const int h2 = hh / 2;
//         if(h2 >= y1 && h2 < y2)
//         {
//           pen.setColor(QColor(Qt::black));
//           p.setPen(pen);
//           // Draw the complete line only if there are an even number of channels (space for the line in the middle).
//           // Ensure a complete line is drawn even if there is no sound file or channels.
//           if((ev_channels & 1) == 0)
//             p.drawLine(sx, h2, ex, h2);
//           else
//           {
//             // Draw only the required two segments of the line.
//             if(wave_visible)
//             {
//               if(sx < wsx)
//                 p.drawLine(sx, h2, wsx - 1, h2);
//               if(wex < ex)
//                 p.drawLine(wex + 1, h2, ex, h2);
//             }
//           }
//         }
//
//       //
//       // Draw custom dashed borders around the wave event
//       //
//
//       QColor color(item->isSelected() ? Qt::white : Qt::black);
//       QPen penH(color);
//       QPen penV(color);
//       penH.setCosmetic(true);
//       penV.setCosmetic(true);
//       QVector<qreal> customDashPattern;
//       customDashPattern << 4.0 << 6.0;
//       penH.setDashPattern(customDashPattern);
//       penV.setDashPattern(customDashPattern);
//       penV.setDashOffset(2.0);
//       // FIXME: Some shifting still going on. Values likely not quite right here.
//       //int xdiff = sx - r.x();
//       int xdiff = sx - mbbx;
//       if(xdiff > 0)
//       {
//         int doff = xdiff % 10;
//         penH.setDashOffset(doff);
//       }
//       // Tested OK. Each segment drawn only when necessary.
//       if(y1 <= 0)
//       {
//         p.setPen(penH);
//         p.drawLine(sx, 0, ex, 0);
//       }
//       if(y2 >= hh - 1)
//       {
//         p.setPen(penH);
//         p.drawLine(sx, hh - 1, ex, hh - 1);
//       }
//
//       //fprintf(stderr, "...Checking left edge: ubbx:%d ux:%d ux_2:%d\n", ubbx, ux, ux_2);
//       if(ubbx >= ux && ubbx < ux_2)
//       {
//         //fprintf(stderr, "...Drawing left edge at mbbx:%d mby_exp:%d mby_2exp:%d\n", mbbx, mby_exp, mby_2exp);
//
//         p.setPen(penV);
//         p.drawLine(mbbx, mby_exp, mbbx, mby_2exp);
//       }
//
//
//       //fprintf(stderr, "...Checking right edge: ubbx_2:%d ux:%d ux_2:%d\n", ubbx_2, ux, ux_2);
//       if(ubbx_2 >= ux && ubbx_2 < ux_2)
//       {
//         //fprintf(stderr, "...Drawing right edge at mbbx_2:%d mby_exp:%d mby_2exp:%d\n", mbbx_2, mby_exp, mby_2exp);
//
//         p.setPen(penV);
//         p.drawLine(mbbx_2, mby_exp, mbbx_2, mby_2exp);
//       }
//
//       // Done. Restore and return.
//       p.setWorldMatrixEnabled(wmtxen);
// }

void WaveCanvas::drawItem(QPainter& p, const CItem* item, const QRect& mr, const QRegion&)
{
      MusECore::WavePart* wp = (MusECore::WavePart*)(item->part());

// REMOVE Tim. wave. Added. Diagnostics.
      fprintf(stderr, "WaveCanvas::drawItem part:%p part->track():%p\n", wp, wp ? wp->track() : nullptr);

      if(!wp || !wp->track())
        return;

      const MusECore::Event event  = item->event();
      if(event.empty())
        return;

// REMOVE Tim. wave. Added.
//       const MusECore::Pos::TType eType = event.pos().type();
      const MusECore::Pos::TType partTType = wp->type();
      const MusECore::Pos::TType canvasTType = MusECore::Pos::FRAMES;

      //QRect rr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
      const QRect ur = mapDev(mr);               // Use our own map instead.
      const int ux = ur.x();
      const int uw = ur.width();
      const int ux_2 = ux + uw;
// REMOVE Tim. wave. Changed.
//       QRect uwpr  = QRect(wp->frame(), 0, wp->lenFrame(), height());
      //const QRect uwpr = drag == DRAG_RESIZE ?
      //    QRect(item->tmpPartPos(), 0, item->tmpPartLen(), height()) :
      //    QRect(wp->frame(), 0, wp->lenFrame(), height());
//       const QRect uwpr = QRect(item->tmpPartPos(), 0, item->tmpPartLen(), height());
      //const QRect ubbr = item->bbox();
      //const QRect ubbr_exp = item->bbox().adjusted(0, 0, rmapxDev(1), 0);


      QRect uwpr;
      const MusECore::MuseCount_t partPosCType = MUSE_TIME_UINT_TO_INT64 wp->posValue(canvasTType);
      const MusECore::MuseCount_t partEndCType =
        MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wp->posValue() + wp->lenValue(), partTType, canvasTType);
      if(drag == DRAG_RESIZE && item == curItem && wp == curPart)
      {
        // If we are resizing borders (and the current item's part is the same as the current part)
        //  and the item end is greater than part end, then allow the part to expand to the item end,
        //  to give a 'live' dynamic update.
        //if(drag == DRAG_RESIZE && curItem && curItem == item && curItem->part() == curPart)
        {
          const MusECore::MuseCount_t absItemEnd = partPosCType + item->x() + item->width();
          const MusECore::MuseCount_t finEnd = absItemEnd > partEndCType ? absItemEnd : partEndCType;
          uwpr = QRect(partPosCType, 0, finEnd - partPosCType, height());
        }
      }
      else
        uwpr = QRect(partPosCType, 0, partEndCType - partPosCType, height());





//       const int px = item->tmpPartPos();
//       const int px = partPosCType;
//       const unsigned pPosEType = MusECore::Pos::convert(px, wp->type(), eType);
//       const MusECore::MuseCount_t pPosEType = MUSE_TIME_UINT_TO_INT64 wp->posValue(eType);
//       const unsigned absEPos = pPosEType + item->tmpPos();
//       const unsigned absEPosCType = partPosCType + item->x();
//       const unsigned eLen = item->tmpLen();
      const MusECore::MuseCount_t absEPosCType = partPosCType + item->x();
//       const MusECore::MuseCount_t eLen = item->tmpLen();
//       const unsigned absEEnd = absEPos + eLen;
//       const unsigned absEEndCType = partPosCType + item->x() + item->width();
      const MusECore::MuseCount_t absEEndCType = partPosCType + item->x() + item->width();
//       const unsigned absEPosFrames = MusECore::Pos::convert(absEPos, eType, MusECore::Pos::FRAMES);
//       const unsigned absEEndFrames = MusECore::Pos::convert(absEEnd, eType, MusECore::Pos::FRAMES);
//       const unsigned absEPosFrames = MusECore::Pos::convert(absEPosCType, canvasTType, MusECore::Pos::FRAMES);
//       const unsigned absEEndFrames = MusECore::Pos::convert(absEEndCType, canvasTType, MusECore::Pos::FRAMES);
//       const unsigned eLenFrames = absEEndFrames - absEPosFrames;
      const MusECore::MuseCount_t absEPosFrames = MusECore::Pos::convert(absEPosCType, canvasTType, MusECore::Pos::FRAMES);
      const MusECore::MuseCount_t absEEndFrames = MusECore::Pos::convert(absEEndCType, canvasTType, MusECore::Pos::FRAMES);
      const MusECore::MuseCount_t eLenFrames = absEEndFrames - absEPosFrames;

//       const QRect ubbr = QRect(absEPosFrames, item->bbox().y(), absEEndFrames - absEPosFrames, item->bbox().height());
      const QRect ubbr = QRect(absEPosCType, item->y(), absEEndCType - absEPosCType, item->height());
      const QRect ubbr_exp = ubbr.adjusted(0, 0, rmapxDev(1), 0);


      const QRect mbbr = map(ubbr);
      const int ubbx = ubbr.x();
      const int ubbx_2 = ubbr.x() + ubbr.width();
      const int mbbx = mbbr.x();
      const int mbbx_2 = mapx(ubbr.x() + ubbr.width());
      const QRect ubr = ur & ubbr;
      const QRect ubr_exp = ur & ubbr_exp;
      const int uby_exp = ubr_exp.y();
      const int uby_2exp = ubr_exp.y() + ubr_exp.height();
      const int mby_exp = mapy(uby_exp);
      const int mby_2exp = mapy(uby_2exp);
      const QRect ubrwp = ubr & uwpr;
      const QRect mbrwp = map(ubrwp);

      QPen pen;
      pen.setCosmetic(true);
      const QColor left_ch_color(0, 170, 255);
      const QColor right_ch_color(Qt::red);

      int x1 = mapx(ubrwp.x());
      int x2 = mapx(ubrwp.x() + ubrwp.width());
      if (x1 < 0)
            x1 = 0;
      if (x2 > width())
            x2 = width();
      int hh = height();
      int y1 = mapy(ubrwp.y());
      int y2 = mapy(ubrwp.y() + ubrwp.height());

      int xScale = xmag;
      if (xScale < 0)
            xScale = -xScale;

// REMOVE Tim. wave. Changed.
//       int px = wp->frame();
      //const int px = drag == DRAG_RESIZE ? item->tmpPartPos() : wp->frame();
//       const int px = item->tmpPartPos();
//       const unsigned pPosEType = MusECore::Pos::convert(px, wp->type(), eType);
//       const unsigned absEPos = pPosEType + item->tmpPos();
//       const unsigned eLen = item->tmpLen();
//       const unsigned absEEnd = absEPos + eLen;
//
//       const unsigned absEPosFrames = MusECore::Pos::convert(absEPos, eType, MusECore::Pos::FRAMES);
//       const unsigned absEEndFrames = MusECore::Pos::convert(absEEnd, eType, MusECore::Pos::FRAMES);

      bool wmtxen = p.worldMatrixEnabled();
      p.setWorldMatrixEnabled(false);

      int sx, ex;

      // Changed. Possible BUG ? Why the half nudge?
      // sx = event.frame() + px + xScale/2;
// REMOVE Tim. wave. Changed.
//       sx = event.frame() + px;
//       ex = sx + event.lenFrame();
      sx = ubbx;
      ex = ubbx_2;
      sx = sx / xScale - xpos - xorg;
      ex = ex / xScale - xpos - xorg;

      if(sx >= x2 || ex < x1)
        return;

      if (sx < x1)
            sx = x1;
      if (ex > x2)
            ex = x2;

// REMOVE Tim. wave. Changed.
//       const int ev_spos = event.spos();
      //int ev_spos = event.spos();
      //if(drag == DRAG_COPY)
      //{
      //  ev_spos += ubbx - (event.frame() + px);
      //  if(ev_spos < 0)
      //    ev_spos = 0;
      //}
      //const int ev_spos = drag == DRAG_RESIZE ? item->tmpWaveSPos() : event.spos();
      const int ev_spos = item->tmpWaveSPos();

// REMOVE Tim. wave. Changed.
//       int pos = (xpos + xorg + sx) * xScale - event.frame() - px;
      //int pos = (xpos + xorg + sx) * xScale - ubbx;
      //int pos = drag == DRAG_RESIZE ? item->tmpPos() : (xpos + xorg + sx) * xScale - event.frame() - px;
      //int pos = (xpos + xorg + sx) * xScale - (drag == DRAG_RESIZE ? item->tmpPos() : event.frame()) - px;
      //int pos = (xpos + xorg + sx) * xScale - item->tmpPos() - px;
//       int pos = (xpos + xorg + sx) * xScale - absEPos;
      int pos = (xpos + xorg + sx) * xScale - absEPosCType;

//       fprintf(stderr, "\nWaveCanvas::drawItem:\nmr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
//               mr.x(), mr.y(), mr.width(), mr.height());
//       fprintf(stderr, "\nur:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
//               ur.x(), ur.y(), ur.width(), ur.height());
//       fprintf(stderr, "\nubbr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
//               ubbr.x(), ubbr.y(), ubbr.width(), ubbr.height());
//       fprintf(stderr, "\nmbbr:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
//               mbbr.x(), mbbr.y(), mbbr.width(), mbbr.height());

      QBrush brush;
      if (item->isMoving())
      {
            QColor c(Qt::gray);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            QLinearGradient gradient(ubbr.topLeft(), ubbr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
            p.fillRect(sx, y1, ex - sx + 1, y2, brush);
      }
      else
      if (item->isSelected())
      {
          QColor c(Qt::black);
          c.setAlpha(MusEGlobal::config.globalAlphaBlend);
          QLinearGradient gradient(ubbr.topLeft(), ubbr.bottomLeft());
          // Use a colour only about 20% lighter than black, rather than the 50% we use in MusECore::gGradientFromQColor
          //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
          gradient.setColorAt(0, QColor(51, 51, 51, MusEGlobal::config.globalAlphaBlend));
          gradient.setColorAt(1, c);
          brush = QBrush(gradient);
          p.fillRect(sx, y1, ex - sx + 1, y2, brush);
      }

      int ev_channels = 0;
      int wav_sx = 0;
      int wav_ex = 0;
      int wsx = 0;
      int wex = 0;
      bool wave_visible = false;

      MusECore::SndFileR f = event.sndFile();
      if(!f.isNull())
      {
        ev_channels = f.channels();
        if (ev_channels > 0) {

          int h   = hh / (ev_channels * 2);
          int cc  = hh % (ev_channels * 2) ? 0 : 1;

// REMOVE Tim. wave. Changed.
//           unsigned peoffset = px + event.frame() - ev_spos;
          //const unsigned absEPos = px + (drag == DRAG_RESIZE ? item->tmpPos() : event.frame());
          //const unsigned eLen = drag == DRAG_RESIZE ? item->tmpLen() : event.lenFrame();
          //const unsigned absEPos = px + item->tmpPos();
//           const unsigned peoffset = absEPos - ev_spos;
          const unsigned peoffset = absEPosFrames - ev_spos;

          const sf_count_t smps = f.samples();
          //const sf_count_t smpsConverted = f.samplesConverted();

          if(-ev_spos < smps && ev_spos <= smps)
          {
            wave_visible = true;
            wav_sx = -ev_spos;
            wav_ex = smps - ev_spos;
            if(wav_sx < 0)
              wav_sx = 0;
// REMOVE Tim. wave. Changed.
//             wav_sx += event.frame() + wp->frame();
//             wav_sx += absEPos;
            wav_sx += absEPosFrames;

            wav_ex = f.unConvertPosition(wav_ex);
// REMOVE Tim. wave. Changed.
//             if(wav_ex >= (int)event.lenFrame())
//             if(wav_ex >= (int)eLen)
            if(wav_ex >= eLenFrames)
            {
// REMOVE Tim. wave. Changed.
//               wav_ex = event.lenFrame();
//               wav_ex = eLen;
              wav_ex = eLenFrames;
              if(wav_ex > 0)
                --wav_ex;
            }
// REMOVE Tim. wave. Changed.
//             wav_ex += event.frame() + wp->frame();
//             wav_ex += absEPos;
            wav_ex += absEPosFrames;

            wav_sx = wav_sx / xScale - xpos - xorg;
            wav_ex = wav_ex / xScale - xpos - xorg;
            wsx = wav_sx < x1 ? x1 : wav_sx;
            wex = wav_ex > x2 ? x2 : wav_ex;
          }

  //         fprintf(stderr, "WaveCanvas::drawItem: rect x:%d w:%d rr x:%d w:%d mr x:%d w:%d pos:%d sx:%d ex:%d\n",
  //                 rect.x(), rect.width(),
  //                 rr.x(), rr.width(),
  //                 mr.x(), mr.width(),
  //                 pos,
  //                 sx, ex);

          for (int i = sx; i < ex; i++) {
                int y = h;
                MusECore::SampleV sa[f.channels()];
                if((ev_spos + f.convertPosition(pos)) > smps)
                  break;
                // Seek the file only once, not with every read!
                if(i == sx)
                {
                  if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, ev_spos) == -1)
                    break;
                }
                f.readConverted(sa, xScale, pos, ev_spos);

                pos += xScale;
                if (pos < 0)
                      continue;

                int selectionStartPos = selectionStart - peoffset; // Offset transformed to event coords
                int selectionStopPos  = selectionStop  - peoffset;

                for (int k = 0; k < ev_channels; ++k) {
                      int kk = k % f.channels();
                      int peak = (sa[kk].peak * (h - 1)) / yScale;
                      int rms  = (sa[kk].rms  * (h - 1)) / yScale;
                      if (peak > h)
                            peak = h;
                      if (rms > h)
                            rms = h;
                      QColor peak_color = MusEGlobal::config.wavePeakColor;
                      QColor rms_color  = MusEGlobal::config.waveRmsColor;

                      if ((ev_spos + pos) > selectionStartPos && (ev_spos + pos) <= selectionStopPos) {
                            peak_color = MusEGlobal::config.wavePeakColorSelected;
                            rms_color  = MusEGlobal::config.waveRmsColorSelected;
                            QLine l_inv = clipQLine(i, y - h + cc, i, y + h - cc, mbrwp);
                            if(!l_inv.isNull())
                            {
                              // Draw inverted
                              pen.setColor(QColor(Qt::black));
                              p.setPen(pen);
                              p.drawLine(l_inv);
                            }
                          }

                      QLine l_peak = clipQLine(i, y - peak - cc, i, y + peak, mbrwp);
                      if(!l_peak.isNull())
                      {
                        pen.setColor(peak_color);
                        p.setPen(pen);
                        p.drawLine(l_peak);
                      }

                      QLine l_rms = clipQLine(i, y - rms - cc, i, y + rms, mbrwp);
                      if(!l_rms.isNull())
                      {
                        pen.setColor(rms_color);
                        p.setPen(pen);
                        p.drawLine(l_rms);
                      }

                      y += 2 * h;
                    }
                }

            // Only if there's something to draw.
            if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
            {
              const int hn = hh / ev_channels;
              const int hhn = hn / 2;
              for (int i = 0; i < ev_channels; ++i) {
                    const int h2     = hn * i;
                    const int center = hhn + h2;
                    if(center >= y1 && center < y2)
                    {
                      pen.setColor(QColor(i & 1 ? right_ch_color : left_ch_color));
                      p.setPen(pen);
                      p.drawLine(wsx, center, wex, center);
                    }
                  }
            }

            // REMOVE Tim. wave. Added.
            {
              p.setPen(Qt::NoPen);
              // Draw remaining 'hidden events' decorations with 'jagged' edges...
              QColor partColor;
              if (wp->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
                  partColor = wp->track()->color();
              else
                  partColor = MusEGlobal::config.partColors[wp->colorIndex()];
              int color_brightness;
              if (MusECore::isColorBright(partColor) && !item->isSelected())
                  color_brightness=96; //0;    // too light: use dark color
              else
                  color_brightness=180; //255;   // too dark: use lighter color
              QColor c(color_brightness,color_brightness,color_brightness, MusEGlobal::config.globalAlphaBlend);
              const int gradS = qBound(0, MusEGlobal::config.partGradientStrength, 200);
              p.setBrush(MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS));

              const int h = mbbr.height();
              const double s = double(h) / 4.0;

              const int xs_0 = mbbr.x();
              const int xe_0 = xs_0 + mbbr.width();
              int xs_1 = xs_0 + 1;
              if(xs_1 > xe_0)
                xs_1 = xe_0;
              int xs_j = xs_0 + 8;
              if(xs_j > xe_0)
                xs_j = xe_0;

              int xe_1 = xe_0 - 1;
              if(xe_1 < xs_0)
                xe_1 = xs_0;
              int xe_j = xe_0 - 8;
              if(xe_j < xs_0)
                xe_j = xs_0;

              const int ys_0 = mbbr.y();

              const int y0 = ys_0;
              const int y2 = y0 + lrint(s * 2.0);
              const int y4 = y0 + h;

              QPoint points[8];
              int pts;

              //if(het & MusECore::Part::RightEventsHidden)
              //if(-ev_spos >= smps || (smps > ev_spos &&
              if(smps > ev_spos &&
                absEPosFrames + f.unConvertPosition(smps - ev_spos) > absEEndFrames)
              {
                pts = 0;
                points[pts++] = QPoint(xe_0, y0);
                points[pts++] = QPoint(xe_0, y4);
                points[pts++] = QPoint(xe_j, y2);

                p.drawConvexPolygon(points, pts);
              }

      //         if(het & MusECore::Part::LeftEventsHidden)
              if(ev_spos > 0)
              {
                pts = 0;
                points[pts++] = QPoint(xs_0, y0);
                points[pts++] = QPoint(xs_j, y2);
                points[pts++] = QPoint(xs_0, y4);

                p.drawConvexPolygon(points, pts);
              }
            }

          }
        }

        const int h2 = hh / 2;
        if(h2 >= y1 && h2 < y2)
        {
          pen.setColor(QColor(Qt::black));
          p.setPen(pen);
          // Draw the complete line only if there are an even number of channels (space for the line in the middle).
          // Ensure a complete line is drawn even if there is no sound file or channels.
          if((ev_channels & 1) == 0)
            p.drawLine(sx, h2, ex, h2);
          else
          {
            // Draw only the required two segments of the line.
            if(wave_visible)
            {
              if(sx < wsx)
                p.drawLine(sx, h2, wsx - 1, h2);
              if(wex < ex)
                p.drawLine(wex + 1, h2, ex, h2);
            }
          }
        }

      //
      // Draw custom dashed borders around the wave event
      //

      QColor color(item->isSelected() ? Qt::white : Qt::black);
      QPen penH(color);
      QPen penV(color);
      penH.setCosmetic(true);
      penV.setCosmetic(true);
      QVector<qreal> customDashPattern;
      customDashPattern << 4.0 << 6.0;
      penH.setDashPattern(customDashPattern);
      penV.setDashPattern(customDashPattern);
      penV.setDashOffset(2.0);
      // FIXME: Some shifting still going on. Values likely not quite right here.
      //int xdiff = sx - r.x();
      int xdiff = sx - mbbx;
      if(xdiff > 0)
      {
        int doff = xdiff % 10;
        penH.setDashOffset(doff);
      }
      // Tested OK. Each segment drawn only when necessary.
      if(y1 <= 0)
      {
        p.setPen(penH);
        p.drawLine(sx, 0, ex, 0);
      }
      if(y2 >= hh - 1)
      {
        p.setPen(penH);
        p.drawLine(sx, hh - 1, ex, hh - 1);
      }

      //fprintf(stderr, "...Checking left edge: ubbx:%d ux:%d ux_2:%d\n", ubbx, ux, ux_2);
      if(ubbx >= ux && ubbx < ux_2)
      {
        //fprintf(stderr, "...Drawing left edge at mbbx:%d mby_exp:%d mby_2exp:%d\n", mbbx, mby_exp, mby_2exp);

        p.setPen(penV);
        p.drawLine(mbbx, mby_exp, mbbx, mby_2exp);
      }


      //fprintf(stderr, "...Checking right edge: ubbx_2:%d ux:%d ux_2:%d\n", ubbx_2, ux, ux_2);
      if(ubbx_2 >= ux && ubbx_2 < ux_2)
      {
        //fprintf(stderr, "...Drawing right edge at mbbx_2:%d mby_exp:%d mby_2exp:%d\n", mbbx_2, mby_exp, mby_2exp);

        p.setPen(penV);
        p.drawLine(mbbx_2, mby_exp, mbbx_2, mby_2exp);
      }

      // Done. Restore and return.
      p.setWorldMatrixEnabled(wmtxen);
}

//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------

void WaveCanvas::drawTopItem(QPainter& p, const QRect& rect, const QRegion&)
{
  
  // TODO TODO: Convert this routine to new drawing system pulled from master? 20190121

  QRect mr = map(rect);
  
  DEBUG_WAVECANVAS(stderr, "WaveCanvas::drawTopItem: rect.x:%d rect.w:%d mr.x:%d mr.w:%d\n",
                   rect.x(), rect.width(), mr.x(), mr.width());

  p.save();
  p.setWorldMatrixEnabled(false);
          
  for(MusEGui::ciCItem i = items.begin(); i != items.end(); ++i) 
  {
    //if(!(i->second->isSelected()))
    //  continue;
    WEvent* e = static_cast<WEvent*>(i->second);
    drawStretchAutomation(p, mr, e);
  }        
  
  p.restore();

}

//---------------------------------------------------------
//   drawStretchAutomation
//---------------------------------------------------------

void WaveCanvas::drawStretchAutomation(QPainter& p, const QRect& rr, WEvent* item) const
{
    const MusECore::Event event = item->event();
    if(event.type() != MusECore::Wave)
      return;
    
    const MusECore::SndFileR sf = event.sndFile();
    if(sf.isNull())
      return;
    
    const MusECore::StretchList* sl = sf.stretchList();
    if(!sl)
      return;


    //const bool wave_sr_differs = sf.sampleRateDiffers();
    const double wave_sr_ratio = sf.sampleRateRatio();

    p.setBrush(Qt::NoBrush);

    QColor c;
    QPen pen;
    int xpixel;
    QVector<qreal> pattern;
    pattern << 4 << 4;
    const StretchSelectedList_t& ssl = _stretchAutomation._stretchSelectedList;
    ciStretchSelectedItemPair res;
    for(MusECore::ciStretchListItem is = sl->begin(); is != sl->end(); ++is)
    {
      // Do not recognize or draw the item at zeroth frame.
      if(is->first == 0)
        continue;
  
      const MusECore::StretchListItem& sli = is->second;
      //if((_tool == StretchTool && (sli._type & MusECore::StretchListItem::StretchEvent)) ||
      //   (_tool == SamplerateTool && (sli._type & MusECore::StretchListItem::SamplerateEvent)))
      {
        xpixel = mapx(sl->squish((double)is->first) / wave_sr_ratio + item->x());
        
        DEBUG_WAVECANVAS(stderr, "drawStretchAutomation: rr.x:%d rr.w:%d xpixel:%d\n", rr.x(), rr.width(), xpixel);
        
        if(sli._type & MusECore::StretchListItem::StretchEvent)
        {
          //if(_tool == StretchTool)
            c = Qt::magenta;
          //else 
          //  c = Qt::darkMagenta;
          
          res = ssl.equal_range(is->first); // FIXME Calls non-constant version? Want constant version.
          for(ciStretchSelectedItem ise = res.first; ise != res.second; ++ise)
          {
            if(ise->first == is->first && ise->second._sndFile.stretchList() == sl && ise->second._type == MusECore::StretchListItem::StretchEvent)
            {
              c = Qt::white;
              break;
            }
          }
          
          //c.setAlpha(200);
          pen.setColor(c);
          pen.setDashPattern(pattern);
          p.setPen(pen);
          p.drawLine(xpixel, rr.top() - 2, xpixel, rr.bottom() - 2);
        }
        
        if(sli._type & MusECore::StretchListItem::SamplerateEvent)
        {
          //if(_tool == SamplerateTool)
            c = Qt::cyan;
          //else 
          //  c = Qt::darkCyan;

          res = ssl.equal_range(is->first); // FIXME Calls non-constant version? Want constant version.
          for(ciStretchSelectedItem ise = res.first; ise != res.second; ++ise)
          {
            if(ise->first == is->first && ise->second._sndFile.stretchList() == sl && ise->second._type == MusECore::StretchListItem::SamplerateEvent)
            {
              c = Qt::white;
              break;
            }
          }
          
          //c.setAlpha(200);
          pen.setColor(c);
          pen.setDashPattern(pattern);
          // Offset to help distinguish from stretch lines.
          pen.setDashOffset(4.0);
          p.setPen(pen);
          // Draw reverse direction to help distinguish from stretch lines.
          p.drawLine(xpixel, rr.bottom() - 2, xpixel, rr.top() - 2);
        }
      }
    }    
}

MusECore::iStretchListItem WaveCanvas::stretchListHitTest(int types, QPoint pt, WEvent* wevent)
{
  const MusECore::Event event = wevent->event();
  if(event.type() != MusECore::Wave)
    return MusECore::iStretchListItem();
  
  const MusECore::SndFileR sf = event.sndFile();
  if(sf.isNull())
    return MusECore::iStretchListItem();
  
  MusECore::StretchList* stretchList = sf.stretchList();
  if(!stretchList)
    return MusECore::iStretchListItem();

  //const bool wave_sr_differs = sf.sampleRateDiffers();
  const double wave_sr_ratio = sf.sampleRateRatio();

  const int pt_x = pt.x();
  const int wevent_x = wevent->x();
  int closest_dist = _stretchAutomationPointDetectDist;
  MusECore::iStretchListItem closest_ev = stretchList->end();
  for(MusECore::iStretchListItem is = stretchList->begin(); is != stretchList->end(); ++is)
  {
    // Do not recognize or draw the item at zeroth frame.
    if(is->first == 0)
      continue;
  
    const MusECore::StretchListItem& se = is->second;
    if(!(se._type & types))
      continue;
    
    const double newSqFrame = se._finSquishedFrame / wave_sr_ratio;
    const int xpixel = mapx(newSqFrame + wevent_x);
    const int pt_pixel = mapx(pt_x);
    
    const int x_diff = (xpixel > pt_pixel) ? (xpixel - pt_pixel) : (pt_pixel - xpixel);
    if(x_diff <= closest_dist)
    {
      closest_dist = x_diff;
      closest_ev = is;
    }
  }
  
  return closest_ev;
}

void WaveCanvas::setStretchAutomationCursor(QPoint pt)
{
  if(_tool != StretchTool && _tool != SamplerateTool)
    return;
  
  CItem* item = items.find(pt);
  if(!item)
  {
    setCursor();
    return;
  }
  WEvent* wevent = static_cast<WEvent*>(item);

  const MusECore::Event event = wevent->event();
  if(event.type() != MusECore::Wave)
  {
    setCursor();
    return;
  }

  MusECore::SndFileR sf = event.sndFile();
  if(sf.isNull())
  {
    setCursor();
    return;
  }

  MusECore::StretchList* sl = sf.stretchList();
  if(!sl)
  {
    setCursor();
    return;
  }

  MusECore::StretchListItem::StretchEventType type;
  if(_tool == StretchTool)
    type = MusECore::StretchListItem::StretchEvent;
  else // if(_tool == SamplerateTool)
    type = MusECore::StretchListItem::SamplerateEvent;

  MusECore::iStretchListItem isli_hit_test = stretchListHitTest(type, pt, wevent);
  if(isli_hit_test == sl->end())
    setCursor();
  else
    QWidget::setCursor(Qt::SizeHorCursor);
}

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void WaveCanvas::drawMoving(QPainter& p, const CItem* item, const QRect& mr, const QRegion&)
    {
      const QRect ur = mapDev(mr);
      QRect ur_item = QRect(item->mp().x(), item->mp().y(), item->width(), item->height());
      ur_item = ur_item.intersected(ur);
      if(!ur_item.isValid())
        return;
      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);
      p.setBrush(QColor(0, 128, 0, 128));  // TODO: Pick a better colour, or use part colours, or grey?
      p.drawRect(ur_item);
    }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void WaveCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if ((_tool != PointerTool) && (event->button() != Qt::LeftButton)) {
            mousePress(event);
            return;
            }
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

MusECore::Undo WaveCanvas::moveCanvasItems(CItemMap& items, int /*dp*/, int dx, DragType dtype, bool rasterize)
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
      int y = 0;
      QPoint newpos = QPoint(x, y);
      if(rasterize)
        newpos = raster(newpos);
      
      // Test moving the item...
      WEvent* wevent = (WEvent*) ci;
      MusECore::Event event    = wevent->event();
      x              = newpos.x();
      if(x < 0)
        x = 0;
      // Normally frame to tick methods round down. But here we need it to 'snap'
      //  the frame from either side of a tick to the tick. So round to nearest.
      int nframe = (rasterize ? MusEGlobal::tempomap.tick2frame(
        editor->rasterVal(MusEGlobal::tempomap.frame2tick(x, 0, MusECore::LargeIntRoundNearest))) : x) - part->frame();
      if(nframe < 0)
        nframe = 0;
      int diff = nframe + event.lenFrame() - part->lenFrame();
      
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
                        
                        int x = ci->pos().x();
                        int nx = x + dx;
                        int ny = 0;
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
                                                
                        if(moving.size() == 1) 
                                                itemReleased(curItem, newpos);

                        if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
                                                selectItem(ci, false);
                }  

    for(MusECore::iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
    {
      MusECore::Part* opart = ip2c->first;
      int diff = ip2c->second.xdiff;
      
      //schedule_resize_all_same_len_clone_parts(opart, opart->lenTick() + diff, operations);
      schedule_resize_all_same_len_clone_parts(opart, opart->lenFrame() + diff, operations);
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

bool WaveCanvas::moveItem(MusECore::Undo& operations, CItem* item, const QPoint& pos, DragType dtype, bool rasterize)
      {
      // Events within an event list cannot be cloned, only copied or moved! Each must have a unique ID.
      if (dtype == MOVE_CLONE)
        return false;

      WEvent* wevent = (WEvent*) item;
      MusECore::Event event    = wevent->event();
      MusECore::Part* part = wevent->part();

      const MusECore::MuseCount_t PPosEType    = MUSE_TIME_UINT_TO_INT64 part->posValue(event.pos().type());
      const MusECore::MuseCount_t PFrame       = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::FRAMES);

      const MusECore::MuseCount_t ePos         = MUSE_TIME_UINT_TO_INT64 event.posValue();

      const MusECore::MuseCount_t absEPos      = PPosEType + ePos;
      const MusECore::MuseCount_t absEFrame    = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEPos, event.pos().type(), MusECore::Pos::FRAMES);

      MusECore::MuseCount_t newEPos            = ePos;

      MusECore::MuseCount_t newAbsEPos         = absEPos;
      MusECore::MuseCount_t newAbsEFrame       = absEFrame;

      // TODO: WaveCanvas items are currently in FRAMES. If TICKS/FRAMES timeline is ever added, convert this.
      newAbsEFrame = pos.x();
      // Limit the left edge of the event to the left edge of the part.
      if(newAbsEFrame < PFrame)
        newAbsEFrame = PFrame;
      newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEFrame, MusECore::Pos::FRAMES, event.pos().type());
      newEPos = newAbsEPos - PPosEType;
      if(newEPos < 0)
        newEPos = 0;

      if(rasterize)
      {
        // Normally frame to tick methods round down. But here we need it to 'snap'
        //  the frame from either side of a tick to the tick. So round to nearest.
        const MusECore::MuseCount_t newAbsETick =
          MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
            MusEGlobal::tempomap.frame2tick(newAbsEFrame, nullptr, MusECore::LargeIntRoundNearest));

        newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, MusECore::Pos::TICKS, event.pos().type());
        newEPos = newAbsEPos - PPosEType;
        if(newEPos < 0)
          newEPos = 0;
      }

      if(dtype == MOVE_COPY)
      {
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::SelectEvent, event, part, false, event.selected()));
        MusECore::Event newEvent = event.duplicate();
        newEvent.setPosValue(newEPos);
        newEvent.setLenValue(event.lenValue());
        newEvent.setSelected(true);
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, newEvent, part, false, false));
      }
      else
      {
        if(event.posValue() != newEPos)
        {
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEventProperties,
            part,
            event,
            newEPos,
            event.lenValue(),
            event.spos(),
            // Do the port cached controllers, per event.
            true,
            // Include all clone port cached controllers as well.
            true
          ));
        }
      }
      return true;
}

//---------------------------------------------------------
//   newItem(p, state)
//---------------------------------------------------------

CItem* WaveCanvas::newItem(const QPoint& p, int key_modifiers)
      {
      int frame  = p.x();
      if(frame < 0)
        frame = 0;
      // Normally frame to tick methods round down. But here we need it to 'snap'
      //  the frame from either side of a tick to the tick. So round to nearest.
      if(!(key_modifiers & Qt::ShiftModifier))
        frame = MusEGlobal::tempomap.tick2frame(
          editor->rasterVal1(MusEGlobal::tempomap.frame2tick(frame, 0, MusECore::LargeIntRoundNearest)));
      int len   = p.x() - frame;
      frame     -= curPart->frame();
      if (frame < 0)
            return 0;
      MusECore::Event e =  MusECore::Event(MusECore::Wave);
      e.setFrame(frame);
      e.setLenFrame(len);
      WEvent* we = new WEvent(e, curPart, height());
      return we;
      }

void WaveCanvas::newItem(CItem* item, bool noSnap)
      {
      WEvent* wevent = (WEvent*) item;
      MusECore::Event event    = wevent->event();
      MusECore::Part* part = wevent->part();
      int pframe = part->frame();
      int x = item->x();
      if (x<pframe)
            x=pframe;
      int w = item->width();

      if (!noSnap) {
            // Normally frame to tick methods round down. But here we need it to 'snap'
            //  the frame from either side of a tick to the tick. So round to nearest.
            x = MusEGlobal::tempomap.tick2frame(
              editor->rasterVal1(MusEGlobal::tempomap.frame2tick(x, 0, MusECore::LargeIntRoundNearest)));
            w = MusEGlobal::tempomap.tick2frame(
              editor->rasterVal(MusEGlobal::tempomap.frame2tick(x + w, 0, MusECore::LargeIntRoundNearest))) - x;
            if (w == 0)
                  w = MusEGlobal::tempomap.tick2frame(editor->raster());
            }
      if (x<pframe)
            x=pframe;
      event.setFrame(x - pframe);
      event.setLenFrame(w);
      event.setSelected(true);

      MusECore::Undo operations;
      int diff = event.endFrame() - part->lenFrame();
      
      if (! ((diff > 0) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
      {
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent,event, part, false, false));
        
        if (diff > 0)// part must be extended?
        {
              schedule_resize_all_same_len_clone_parts(part, event.endFrame(), operations);
              printf("newItem: extending\n");
        }
        
        MusEGlobal::song->applyOperationGroup(operations);
      }
      else // forbid action by not applying it   
          //this forces an update of the itemlist, which is necessary
          //to remove "forbidden" events from the list again
          //otherwise, if a moving operation was forbidden,
          //the canvas would still show the movement
          songChanged(SC_EVENT_INSERTED);
      }

// REMOVE Tim. wave. Added.
// //---------------------------------------------------------
// //   initItemTempValues
// //---------------------------------------------------------
//
// void WaveCanvas::initItemTempValues(CItem* item)
// {
//   WEvent* wevent = (WEvent*) item;
//   MusECore::Part* part = wevent->part();
//   MusECore::Event event = wevent->event();
//   wevent->setTmpOffset(0);
//   wevent->setTmpPartPos(part ? part->posValue() : 0);
//   wevent->setTmpPartLen(part ? part->lenValue() : 0);
//   wevent->setTmpPos(event.posValue());
//   wevent->setTmpLen(event.lenValue());
//   wevent->setTmpWaveSPos(event.spos());
// }

// REMOVE Tim. wave. Added.
// void WaveCanvas::adjustItemSize(CItem* item, int pos, bool /*left*/, bool noSnap, bool ctrl)
// {
// //   if(!noSnap)
// //     // Ignore the return y, which may be altered, we only want the x.
// //     pos = raster(QPoint(pos, item->y())).x();
// //
// //   const MusECore::Part* part = item->part();
// //   if(part)
// //   {
// //     const unsigned ppos = part->frame();
// //     if(pos < (int)ppos)
// //       pos = ppos;
// //   }
// //   item->horizResize(pos, left);
//
//   adjustItemTempValues(item, pos, noSnap, ctrl);
// }

// REMOVE Tim. wave. Added.
//---------------------------------------------------------
//   adjustItemSize
//---------------------------------------------------------

//void WaveCanvas::adjustItemTempValues(CItem* item, int pos, bool noSnap, bool ctrl, bool /*alt*/)
void WaveCanvas::adjustItemSize(CItem* item, int pos, bool left, bool noSnap, bool ctrl, bool /*alt*/, QRegion *region)
      {
//       if(resizeDirection != MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT &&
//         resizeDirection != MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//         return;

//       WEvent* wevent = (WEvent*) item;

      MusECore::Part* itemPart = item->part();

      const MusECore::Pos::TType itemPartTType = itemPart->type();

//       const MusECore::MuseCount_t itemPPos = MUSE_TIME_UINT_TO_INT64 itemPart->posValue();
//       const MusECore::MuseCount_t tmpItemPPos = item->tmpPartPos();
//       const MusECore::MuseCount_t tmpItemPLen = item->tmpPartLen();
//       const MusECore::MuseCount_t tmpItemPEnd = tmpItemPPos + tmpItemPLen;

// Added
//       const MusECore::Part::PartType originalPartType = originalPart->partType();
//       const MusECore::Pos::TType originalPartTType = originalPart->type();
//
//       const MusECore::Pos::TType newPosOrLenType = MusECore::Pos::TType::FRAMES;
      const MusECore::Pos::TType canvasTType = MusECore::Pos::FRAMES;

      MusECore::Event event = item->event();
      const MusECore::Pos::TType ePosTType     = event.pos().type();
      const MusECore::MuseCount_t PPosETType   = MUSE_TIME_UINT_TO_INT64 itemPart->posValue(ePosTType);
      const MusECore::MuseCount_t pPosCTType   = MUSE_TIME_UINT_TO_INT64 itemPart->posValue(canvasTType);
      //const MusECore::MuseCount_t PPosEType =
      //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos(), part->type(), ePosType);
      //const MusECore::MuseCount_t PFrame =
      //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos(), part->type(), MusECore::Pos::FRAMES);
      //const MusECore::MuseCount_t PEndEType =
      //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos() + wevent->tmpPartLen(), part->type(), ePosType);
// Changed
//       const MusECore::MuseCount_t PEndEType =
//         MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(part->posValue() + wevent->tmpPartLen(), part->type(), ePosType);
      const MusECore::MuseCount_t PEndETType =
        MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(itemPart->posValue() + itemPart->lenValue(), itemPartTType, ePosTType);

      if(!noSnap)
        // Ignore the return y, which may be altered, we only want the x.
        pos = raster(QPoint(pos, item->y())).x();

// Added
//       MusECore::MuseCount_t newPosOrigPartTType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//         pos, newPosOrLenType, originalPartTType);
//
//       MusECore::MuseCount_t newPosEType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//         pos, newPosOrLenType, ePosType);

//       const MusECore::MuseCount_t ePos         = MUSE_TIME_UINT_TO_INT64 event.posValue();
//       const MusECore::MuseCount_t eLen         = MUSE_TIME_UINT_TO_INT64 event.lenValue();
//       const MusECore::MuseCount_t eSPos        = event.spos();
      const MusECore::MuseCount_t ePos         = item->tmpPos();
      const MusECore::MuseCount_t eLen         = item->tmpLen();
      const MusECore::MuseCount_t eSPos        = item->tmpWaveSPos();


//       const double ePos         = wevent->tmpPos();
//       const double eLen         = wevent->tmpLen();
//       const double eSPos        = wevent->tmpWaveSPos();

      const MusECore::MuseCount_t absEPos       = PPosETType + ePos;
      const MusECore::MuseCount_t absEPosCTType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEPos, ePosTType, canvasTType);
      const MusECore::MuseCount_t absEEnd       = absEPos + eLen;
      const MusECore::MuseCount_t absEEndCTType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEEnd, ePosTType, canvasTType);

// Changed
      MusECore::MuseCount_t newEPos            = ePos;
      MusECore::MuseCount_t newELen            = eLen;
      MusECore::MuseCount_t newESPos           = eSPos;
//       double newEPos            = ePos;
//       double newELen            = eLen;
//       double newESPos           = eSPos;

      MusECore::MuseCount_t newAbsEPos         = absEPos;
      MusECore::MuseCount_t newAbsEPosCTType   = absEPosCTType;
      MusECore::MuseCount_t newAbsEEnd         = absEEnd;
      MusECore::MuseCount_t newAbsEEndCTType   = absEEndCTType;

//       newAbsEFrame = wevent->x();
//       newAbsEEndFrame = newAbsEFrame + wevent->width();

//       if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
      if (left)
      {
// Changed
        newAbsEPosCTType = pos;
//         newAbsEPos = newPosEType;
      }
//       else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
      else
      {
// Changed
        newAbsEEndCTType = pos;
//         newAbsEEnd = newPosEType;
      }


// Changed
      // Limit the left edge of the event to the left edge of the part.
      if(newAbsEPosCTType < pPosCTType)
      {
        // Adjust the end frame.
        newAbsEPosCTType = pPosCTType;
      }
//       // Limit the left edge of the event to the left edge of the part.
//       if(newAbsEPos < PPosEType)
//       {
//         // Adjust the end frame.
//         newAbsEPos = PPosEType;
//       }

      //newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEFrame, MusECore::Pos::FRAMES, ePosType);

//       if (noSnap)
      {
// Removed
        newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPosCTType, canvasTType, ePosTType);
        newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndCTType, canvasTType, ePosTType);
      }
//       else
//       {
//         // Normally frame to tick methods round down. But here we need it to 'snap'
//         //  the frame from either side of a tick to the tick. So round to nearest.
//         // TODO: WaveCanvas items are currently in FRAMES. If TICKS/FRAMES timeline is ever added, convert this.
//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//         {
//           const MusECore::MuseCount_t newAbsETick =
//             MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
//               MusEGlobal::tempomap.frame2tick(newAbsEFrame, nullptr, MusECore::LargeIntRoundNearest));
//           newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, MusECore::Pos::TICKS, ePosType);
//           newAbsEFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, ePosType, MusECore::Pos::FRAMES);
//           newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndFrame, MusECore::Pos::FRAMES, ePosType);
//         }
//         else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//         {
//           newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEFrame, MusECore::Pos::FRAMES, ePosType);
//           const MusECore::MuseCount_t newAbsEEndTick =
//             MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
//               MusEGlobal::tempomap.frame2tick(newAbsEEndFrame, nullptr, MusECore::LargeIntRoundNearest));
//           newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, ePosType);
//           newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, MusECore::Pos::FRAMES);
//         }
//       }

      newEPos = newAbsEPos - PPosETType;
      if(newEPos < 0)
        newEPos = 0;

//       if (noSnap)
//       {
//         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndFrame, MusECore::Pos::FRAMES, ePosType);
//       }
//       else
//       {
//         // Normally frame to tick methods round down. But here we need it to 'snap'
//         //  the frame from either side of a tick to the tick. So round to nearest.
//         // TODO: WaveCanvas items are currently in FRAMES. If TICKS/FRAMES timeline is ever added, convert this.
//         const MusECore::MuseCount_t newAbsEEndTick =
//           MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
//             MusEGlobal::tempomap.frame2tick(newAbsEEndFrame, nullptr, MusECore::LargeIntRoundNearest));
//
//         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, ePosType);
//       }

//       // The Canvas limits the distance to 1 rather than 0, so let's catch that.
//       if(newAbsEEnd - newAbsEPos <= 1)
//       {
//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//         {
//           MusECore::MuseCount_t newAbsETick =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, MusECore::Pos::TICKS) -
//               editor->raster();
//           const MusECore::MuseCount_t PTick = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::TICKS);
//           if(newAbsETick < PTick)
//           {
//             newAbsETick = PTick;
//             const MusECore::MuseCount_t newAbsEEndTick = newAbsETick + editor->raster();
//             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, ePosType);
//             newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, MusECore::Pos::FRAMES);
//           }
//           newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, MusECore::Pos::TICKS, ePosType);
//           if(newAbsEPos < PPosEType)
//             newAbsEPos = PPosEType;
//           newEPos = newAbsEPos - PPosEType;
//           newAbsEFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, ePosType, MusECore::Pos::FRAMES);
//         }
//         else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//         {
//           const MusECore::MuseCount_t newAbsEEndTick =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, ePosType, MusECore::Pos::TICKS) +
//               editor->raster();
//           newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, ePosType);
//           newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, MusECore::Pos::FRAMES);
//         }
//       }

      newELen = newAbsEEnd - newAbsEPos;
      if(newELen < 1)
        newELen = 1;

//       MusECore::Undo operations;

//       if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
      if (left) {
          if(!ctrl) {
//             newESPos = eSPos + (newAbsEFrame - absEFrame);
            newESPos = event.sndFile().convertPosition(event.sndFile().unConvertPosition(eSPos) + (newAbsEPosCTType - absEPosCTType));
            if(newESPos < 0)
              newESPos = 0;
          }
      }
      else {
          if(ctrl) {
//             newESPos = eSPos - (newAbsEEndFrame - absEEndFrame);
            newESPos = event.sndFile().convertPosition(event.sndFile().unConvertPosition(eSPos) - (newAbsEEndCTType - absEEndCTType));
            if(newESPos < 0)
              newESPos = 0;
          }
      }

      const QRect curr_ud_rect(item->bbox());
//       const MusECore::MuseCount_t newAbsEEndPType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, itemPartTType);

//       const bool doExtendPart = newAbsEEnd > part->endValue(ePosType);

      // While adjusting visually, only extend the part if it is the same as the given item's part.
      const bool doExtendPart = /*(part == itemPart) &&*/ newAbsEEnd > PEndETType;
//       const bool doExtendPart = /*(part == itemPart) &&*/ (newAbsEEndPType > tmpItemPEnd);

      if (! (doExtendPart && (itemPart->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
      {
        const MusECore::MuseCount_t newPartLen =
//           MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, itemPart->type()) - itemPart->posValue();
          MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosTType, itemPartTType) - itemPart->posValue();

        if (doExtendPart)// part must be extended?
        {
//           const MusECore::MuseCount_t newPartLen =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - part->posValue();
            //MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - wevent->tmpPartPos();
//           // This automatically takes care of port cached controllers.
//           schedule_resize_all_same_len_clone_parts(part, newPartLen, operations);
          //fprintf(stderr, "adjustItemTempValues: extending part\n");
          item->setTmpPartLen(newPartLen);
        }
        else
        {
          // Limit it to the part's current length.
          item->setTmpPartLen(qMax(newPartLen, MUSE_TIME_UINT_TO_INT64 itemPart->lenValue()));
        }

// Added
        // Update the wave canvas item's bounding box and position.
        // NOTE: Some accuracy may be lost AT BOTH BORDERS if converting the
        //        event's dimensions from frames to ticks here. Currently that
        //        is the case with wave events on a tick-only based canvas.
        //       Currently that won't happen on the wave canvas, but some day
        //        it might support ticks.
//         const MusECore::MuseCount_t newEPosValCType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//           newEPos, ePosType, canvasTType);
//         const MusECore::MuseCount_t newLenValCType = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//           newEPos + newELen, ePosType, canvasTType) - newEPosValCType;

//         wevent->setPos(QPoint(newEPosValCType, wevent->y()));
//         wevent->setBBox(QRect(newEPosValCType, wevent->y(), newLenValCType, wevent->height()));

        const MusECore::MuseCount_t newPosValCType = newAbsEPosCTType - pPosCTType;
        const MusECore::MuseCount_t newLenValCType = newAbsEEndCTType - newAbsEPosCTType;

        item->setPos(QPoint(newPosValCType, item->y()));
        item->setBBox(QRect(newPosValCType, item->y(), newLenValCType, item->height()));
// Keep this remarked section:
        // If we want to use more accurate temporary position and length values,
        //  which are in the same time type as the event.
        item->setTmpPos(newEPos);
        item->setTmpLen(newELen);
        item->setTmpWaveSPos(newESPos);
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
          fprintf(stderr, "WaveCanvas::adjustItemSize curr_ud_rect x:%d y:%d w:%d h:%d\n",
                  curr_ud_rect.x(), curr_ud_rect.y(), curr_ud_rect.width(), curr_ud_rect.height());
          fprintf(stderr, "                            new_ud_rect x:%d y:%d w:%d h:%d\n",
                  new_ud_rect.x(), new_ud_rect.y(), new_ud_rect.width(), new_ud_rect.height());
          fprintf(stderr, "WaveCanvas::adjustItemSize new_ud_rect_m x:%d y:%d w:%d h:%d\n",
                  new_ud_rect_m.x(), new_ud_rect_m.y(), new_ud_rect_m.width(), new_ud_rect_m.height());
          fprintf(stderr, "                                     rect x:%d y:%d w:%d h:%d\n",
                  rect().x(), rect().y(), rect().width(), rect().height());
          fprintf(stderr, "                                  newPosVal:%ld newLenVal:%ld\n",
                  newEPos, newELen);

      *region += new_ud_rect_m;
//       *region += new_ud_rect;
    }

      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

// REMOVE Tim. wave. Changed.
// void WaveCanvas::resizeItem(CItem* item, bool noSnap, bool ctrl)         // experimental changes to try dynamically extending parts
//       {
//       WEvent* wevent = (WEvent*) item;
//       MusECore::Event event = wevent->event();
//
//       MusECore::Part* part = wevent->part();
//
//       const MusECore::MuseCount_t PPosEType    = MUSE_TIME_UINT_TO_INT64 part->posValue(event.pos().type());
//       const MusECore::MuseCount_t PFrame       = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::FRAMES);
//
//       const MusECore::MuseCount_t ePos         = MUSE_TIME_UINT_TO_INT64 event.posValue();
//       const MusECore::MuseCount_t eLen         = MUSE_TIME_UINT_TO_INT64 event.lenValue();
//       const MusECore::MuseCount_t eSPos        = event.spos();
//
//       const MusECore::MuseCount_t absEPos      = PPosEType + ePos;
//       const MusECore::MuseCount_t absEFrame    = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEPos, event.pos().type(), MusECore::Pos::FRAMES);
//       const MusECore::MuseCount_t absEEnd      = absEPos + eLen;
//       const MusECore::MuseCount_t absEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEEnd, event.pos().type(), MusECore::Pos::FRAMES);
//
//       MusECore::MuseCount_t newEPos            = ePos;
//       MusECore::MuseCount_t newELen            = eLen;
//       MusECore::MuseCount_t newESPos           = eSPos;
//
//       MusECore::MuseCount_t newAbsEPos         = absEPos;
//       MusECore::MuseCount_t newAbsEFrame       = absEFrame;
//       MusECore::MuseCount_t newAbsEEnd         = absEEnd;
//       MusECore::MuseCount_t newAbsEEndFrame    = absEEndFrame;
//
//       newAbsEFrame = wevent->x();
//       newAbsEEndFrame = newAbsEFrame + wevent->width();
//       // Limit the left edge of the event to the left edge of the part.
//       if(newAbsEFrame < PFrame)
//       {
//         // Adjust the end frame.
//         newAbsEFrame = PFrame;
//       }
//       newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEFrame, MusECore::Pos::FRAMES, event.pos().type());
//       newEPos = newAbsEPos - PPosEType;
//       if(newEPos < 0)
//         newEPos = 0;
//
//       if (noSnap)
//       {
//         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndFrame, MusECore::Pos::FRAMES, event.pos().type());
//       }
//       else
//       {
//         // Normally frame to tick methods round down. But here we need it to 'snap'
//         //  the frame from either side of a tick to the tick. So round to nearest.
//         // TODO: WaveCanvas items are currently in FRAMES. If TICKS/FRAMES timeline is ever added, convert this.
//         const MusECore::MuseCount_t newAbsEEndTick =
//           MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
//             MusEGlobal::tempomap.frame2tick(newAbsEEndFrame, nullptr, MusECore::LargeIntRoundNearest));
//
//         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
//       }
//
//       // The Canvas limits the distance to 1 rather than 0, so let's catch that.
//       if(newAbsEEnd - newAbsEPos <= 1)
//       {
//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//         {
//           MusECore::MuseCount_t newAbsETick =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::TICKS) -
//               editor->raster();
//           const MusECore::MuseCount_t PTick = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::TICKS);
//           if(newAbsETick < PTick)
//           {
//             newAbsETick = PTick;
//             const MusECore::MuseCount_t newAbsEEndTick = newAbsETick + editor->raster();
//             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
//             newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::FRAMES);
//           }
//           newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, MusECore::Pos::TICKS, event.pos().type());
//           if(newAbsEPos < PPosEType)
//             newAbsEPos = PPosEType;
//           newEPos = newAbsEPos - PPosEType;
//           newAbsEFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, event.pos().type(), MusECore::Pos::FRAMES);
//         }
//         else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//         {
//           const MusECore::MuseCount_t newAbsEEndTick =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, event.pos().type(), MusECore::Pos::TICKS) +
//               editor->raster();
//           newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
//           newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::FRAMES);
//         }
//       }
//
//       newELen = newAbsEEnd - newAbsEPos;
//       if(newELen < 1)
//         newELen = 1;
//
//       MusECore::Undo operations;
//
//       if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
//           if(!ctrl) {
//             newESPos = event.spos() + (newAbsEFrame - absEFrame);
//             if(newESPos < 0)
//               newESPos = 0;
//           }
//       }
//       else {
//           if(ctrl) {
//             newESPos = event.spos() - (newAbsEEndFrame - absEEndFrame);
//             if(newESPos < 0)
//               newESPos = 0;
//           }
//       }
//
//       const bool doExtendPart = newAbsEEnd > part->endValue(event.pos().type());
//
//       if (! ((doExtendPart) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
//       {
//         if (doExtendPart)// part must be extended?
//         {
//           const MusECore::MuseCount_t newPartLen =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), part->type()) - part->posValue();
//           // This automatically takes care of port cached controllers.
//           schedule_resize_all_same_len_clone_parts(part, newPartLen, operations);
//           fprintf(stderr, "resizeItem: extending\n");
//         }
//
//         // Anything changed?
//         const bool posChanged = event.posValue() != newEPos;
//         if(posChanged || event.lenValue() != newELen || event.spos() != newESPos)
//         {
//           operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEventProperties,
//             part,
//             event,
//             newEPos,
//             newELen,
//             newESPos,
//             // May be redundant, but force it to do the port cached controllers, per event.
//             // 'schedule_resize_all_same_len_clone_parts' above is currently hard-wired to
//             //  always handle the port cached controllers, BUT it may exclude some parts.
//             // Only if the event's position changed - Length and SPos have no meaning for controllers.
//             posChanged,
//             // Include all clone port cached controllers as well.
//             posChanged
//           ));
//         }
//
//       }
//
//       if(operations.empty())
//         //this forces an update of the itemlist, which is necessary
//         //to remove "forbidden" events from the list again
//         //otherwise, if a moving operation was forbidden,
//         //the canvas would still show the movement
//         songChanged(SC_EVENT_MODIFIED);
//       else
//         MusEGlobal::song->applyOperationGroup(operations);
//       }

// void WaveCanvas::resizeItem(CItem* item, bool /*noSnap*/, bool /*ctrl*/)         // experimental changes to try dynamically extending parts
//       {
//       WEvent* wevent = (WEvent*) item;
//       MusECore::Event event = wevent->event();
//
//       MusECore::Part* part = wevent->part();
//
// // Added
//       const MusECore::Pos::TType canvasTType = MusECore::Pos::FRAMES;
//
//       MusECore::Pos::TType eventTType = event.pos().type();
//       //const MusECore::MuseCount_t PPosEType    = MUSE_TIME_UINT_TO_INT64 part->posValue(ePosType);
//       //const MusECore::MuseCount_t PPosEType =
//       //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos(), part->type(), ePosType);
// //       const MusECore::MuseCount_t PFrame       = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::FRAMES);
//       //const MusECore::MuseCount_t PEndEType =
//       //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos() + wevent->tmpPartLen(), part->type(), ePosType);
//       const MusECore::MuseCount_t newPEnd = MUSE_TIME_UINT_TO_INT64 part->posValue() + wevent->tmpPartLen();
//       //const MusECore::MuseCount_t newPEndEType =
//       //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newPEnd, part->type(), ePosType);
//
// //       const MusECore::MuseCount_t ePos         = MUSE_TIME_UINT_TO_INT64 event.posValue();
// //       const MusECore::MuseCount_t eLen         = MUSE_TIME_UINT_TO_INT64 event.lenValue();
// //       const MusECore::MuseCount_t eSPos        = event.spos();
//
// //       const MusECore::MuseCount_t absEPos      = PPosEType + ePos;
// //       const MusECore::MuseCount_t absEFrame    = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEPos, event.pos().type(), MusECore::Pos::FRAMES);
// //       const MusECore::MuseCount_t absEEnd      = absEPos + eLen;
// //       const MusECore::MuseCount_t absEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(absEEnd, event.pos().type(), MusECore::Pos::FRAMES);
//
// //       MusECore::MuseCount_t newEPos            = ePos;
// //       MusECore::MuseCount_t newELen            = eLen;
// //       MusECore::MuseCount_t newESPos           = eSPos;
//
// // Changed
// //       MusECore::MuseCount_t newEPos            = wevent->tmpPos();
// //       MusECore::MuseCount_t newELen            = wevent->tmpLen();
// //       MusECore::MuseCount_t newESPos           = wevent->tmpWaveSPos();
//       MusECore::MuseCount_t newEPos  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//         wevent->x(), canvasTType, eventTType);
//       MusECore::MuseCount_t newELen  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//         wevent->x() + wevent->width(), canvasTType, eventTType) - newEPos;
//       MusECore::MuseCount_t newESPos = wevent->tmpWaveSPos();
//
//       //MusECore::MuseCount_t newAbsEPos         = PPosEType + newEPos;
//       //MusECore::MuseCount_t newAbsEEnd         = newAbsEPos + newELen;
//
// //       newAbsEFrame = wevent->x();
// //       newAbsEEndFrame = newAbsEFrame + wevent->width();
// //       // Limit the left edge of the event to the left edge of the part.
// //       if(newAbsEFrame < PFrame)
// //       {
// //         // Adjust the end frame.
// //         newAbsEFrame = PFrame;
// //       }
// //       newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEFrame, MusECore::Pos::FRAMES, event.pos().type());
// //       newEPos = newAbsEPos - PPosEType;
// //       if(newEPos < 0)
// //         newEPos = 0;
// //
// //       if (noSnap)
// //       {
// //         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndFrame, MusECore::Pos::FRAMES, event.pos().type());
// //       }
// //       else
// //       {
// //         // Normally frame to tick methods round down. But here we need it to 'snap'
// //         //  the frame from either side of a tick to the tick. So round to nearest.
// //         // TODO: WaveCanvas items are currently in FRAMES. If TICKS/FRAMES timeline is ever added, convert this.
// //         const MusECore::MuseCount_t newAbsEEndTick =
// //           MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
// //             MusEGlobal::tempomap.frame2tick(newAbsEEndFrame, nullptr, MusECore::LargeIntRoundNearest));
// //
// //         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
// //       }
// //
// //       // The Canvas limits the distance to 1 rather than 0, so let's catch that.
// //       if(newAbsEEnd - newAbsEPos <= 1)
// //       {
// //         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
// //         {
// //           MusECore::MuseCount_t newAbsETick =
// //             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::TICKS) -
// //               editor->raster();
// //           const MusECore::MuseCount_t PTick = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::TICKS);
// //           if(newAbsETick < PTick)
// //           {
// //             newAbsETick = PTick;
// //             const MusECore::MuseCount_t newAbsEEndTick = newAbsETick + editor->raster();
// //             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
// //             newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::FRAMES);
// //           }
// //           newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, MusECore::Pos::TICKS, event.pos().type());
// //           if(newAbsEPos < PPosEType)
// //             newAbsEPos = PPosEType;
// //           newEPos = newAbsEPos - PPosEType;
// //           newAbsEFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, event.pos().type(), MusECore::Pos::FRAMES);
// //         }
// //         else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
// //         {
// //           const MusECore::MuseCount_t newAbsEEndTick =
// //             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, event.pos().type(), MusECore::Pos::TICKS) +
// //               editor->raster();
// //           newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
// //           newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::FRAMES);
// //         }
// //       }
// //
// //       newELen = newAbsEEnd - newAbsEPos;
// //       if(newELen < 1)
// //         newELen = 1;
// //
// //       MusECore::Undo operations;
// //
// //       if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
// //           if(!ctrl) {
// //             newESPos = event.spos() + (newAbsEFrame - absEFrame);
// //             if(newESPos < 0)
// //               newESPos = 0;
// //           }
// //       }
// //       else {
// //           if(ctrl) {
// //             newESPos = event.spos() - (newAbsEEndFrame - absEEndFrame);
// //             if(newESPos < 0)
// //               newESPos = 0;
// //           }
// //       }
//
//       MusECore::Undo operations;
//
// //       const bool doExtendPart = newAbsEEnd > part->endValue(ePosType);
//       //const bool doExtendPart = newAbsEEnd > PEndEType;
//       const bool doExtendPart = newPEnd > part->endValue();
//
//       if (! ((doExtendPart) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
//       {
//         if (doExtendPart)// part must be extended?
//         {
//           //const MusECore::MuseCount_t newPartLen =
//           //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - part->posValue();
//             //MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - wevent->tmpPartPos();
// //           // This automatically takes care of port cached controllers.
// //           schedule_resize_all_same_len_clone_parts(part, newPartLen, operations);
//           fprintf(stderr, "resizeItem: extending\n");
//           // This automatically takes care of port cached controllers.
//           schedule_resize_all_same_len_clone_parts(part, newPEnd - part->posValue(), operations);
//         }
//
//         // Anything changed?
//         const bool posChanged = event.posValue() != newEPos;
//         if(posChanged || event.lenValue() != newELen || event.spos() != newESPos)
//         {
//           operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEventProperties,
//             part,
//             event,
//             newEPos,
//             newELen,
//             newESPos,
//             // May be redundant, but force it to do the port cached controllers, per event.
//             // 'schedule_resize_all_same_len_clone_parts' above is currently hard-wired to
//             //  always handle the port cached controllers, BUT it may exclude some parts.
//             // Only if the event's position changed - Length and SPos have no meaning for controllers.
//             posChanged,
//             // Include all clone port cached controllers as well.
//             posChanged
//           ));
//         }
//
//       }
//
//       if(operations.empty())
//         //this forces an update of the itemlist, which is necessary
//         //to remove "forbidden" events from the list again
//         //otherwise, if a moving operation was forbidden,
//         //the canvas would still show the movement
//         songChanged(SC_EVENT_MODIFIED);
//       else
//         MusEGlobal::song->applyOperationGroup(operations);
//       }

void WaveCanvas::resizeItem(CItem* item, bool /*noSnap*/, bool ctrl)         // experimental changes to try dynamically extending parts
      {
      WEvent* wevent = (WEvent*) item;
      MusECore::Event event = wevent->event();

      MusECore::Part* part = wevent->part();

// Added
      const MusECore::Pos::TType canvasTType = MusECore::Pos::FRAMES;
      //const MusECore::Pos::TType partTType = part->type();

      MusECore::Pos::TType eventTType = event.pos().type();
      const MusECore::MuseCount_t PPosEType    = MUSE_TIME_UINT_TO_INT64 part->posValue(eventTType);
      const MusECore::MuseCount_t PPosCType    = MUSE_TIME_UINT_TO_INT64 part->posValue(canvasTType);
      //const MusECore::MuseCount_t PPosEType =
      //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos(), partTType, eventTType);
      const MusECore::MuseCount_t PFrame       = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::FRAMES);
      //const MusECore::MuseCount_t PEndEType =
      //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(wevent->tmpPartPos() + wevent->tmpPartLen(), part->type(), ePosType);
      const MusECore::MuseCount_t newPEnd = MUSE_TIME_UINT_TO_INT64 part->posValue() + wevent->tmpPartLen();
      //const MusECore::MuseCount_t newPEndEType =
      //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newPEnd, part->type(), ePosType);

      const MusECore::MuseCount_t ePos         = MUSE_TIME_UINT_TO_INT64 event.posValue();
      const MusECore::MuseCount_t eLen         = MUSE_TIME_UINT_TO_INT64 event.lenValue();
      const MusECore::MuseCount_t eSPos        = event.spos();

      const MusECore::MuseCount_t absEPos      = PPosEType + ePos;
      const MusECore::MuseCount_t absEFrame    = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
        absEPos, eventTType, MusECore::Pos::FRAMES);

      const MusECore::MuseCount_t absEEnd      = absEPos + eLen;
      const MusECore::MuseCount_t absEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
        absEEnd, event.pos().type(), MusECore::Pos::FRAMES);

//       MusECore::MuseCount_t newEPos            = ePos;
//       MusECore::MuseCount_t newELen            = eLen;
//       MusECore::MuseCount_t newESPos           = eSPos;

// Changed
//       MusECore::MuseCount_t newEPos            = wevent->tmpPos();
//       MusECore::MuseCount_t newELen            = wevent->tmpLen();
//       MusECore::MuseCount_t newESPos           = wevent->tmpWaveSPos();

//       MusECore::MuseCount_t newEPos  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//         wevent->x(), canvasTType, eventTType);
//       MusECore::MuseCount_t newELen  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
//         wevent->x() + wevent->width(), canvasTType, eventTType) - newEPos;
//       MusECore::MuseCount_t newESPos = wevent->tmpWaveSPos();
//
//       MusECore::MuseCount_t newAbsEPos         = PPosEType + newEPos;
//       MusECore::MuseCount_t newAbsEEnd         = newAbsEPos + newELen;
      MusECore::MuseCount_t newEPos  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
        PPosCType + wevent->x(), canvasTType, eventTType);
      MusECore::MuseCount_t newELen  = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
        wevent->x() + wevent->width(), canvasTType, eventTType) - newEPos;
      //MusECore::MuseCount_t newESPos = wevent->tmpWaveSPos();
      MusECore::MuseCount_t newESPos = eSPos;

//       newAbsEFrame = wevent->x();
//       newAbsEEndFrame = newAbsEFrame + wevent->width();
//       const MusECore::MuseCount_t newAbsEFrame = wevent->x();
//       const MusECore::MuseCount_t newAbsEEndFrame = newAbsEFrame + wevent->width();
      MusECore::MuseCount_t newAbsEFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
        PPosCType + wevent->x(), canvasTType, MusECore::Pos::FRAMES);
      MusECore::MuseCount_t newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(
        PPosCType + wevent->x() + wevent->width(), canvasTType, MusECore::Pos::FRAMES);

      // Limit the left edge of the event to the left edge of the part.
      if(newAbsEFrame < PFrame + 1)
      {
        // Adjust the end frame.
        newAbsEFrame = PFrame + 1;
      }

//       newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEFrame, MusECore::Pos::FRAMES, event.pos().type());
//       newEPos = newAbsEPos - PPosEType;
//       if(newEPos < 0)
//         newEPos = 0;
//
//       if (noSnap)
//       {
//         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndFrame, MusECore::Pos::FRAMES, event.pos().type());
//       }
//       else
//       {
//         // Normally frame to tick methods round down. But here we need it to 'snap'
//         //  the frame from either side of a tick to the tick. So round to nearest.
//         // TODO: WaveCanvas items are currently in FRAMES. If TICKS/FRAMES timeline is ever added, convert this.
//         const MusECore::MuseCount_t newAbsEEndTick =
//           MUSE_TIME_UINT_TO_INT64 editor->rasterVal(
//             MusEGlobal::tempomap.frame2tick(newAbsEEndFrame, nullptr, MusECore::LargeIntRoundNearest));
//
//         newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
//       }
//
//       // The Canvas limits the distance to 1 rather than 0, so let's catch that.
//       if(newAbsEEnd - newAbsEPos <= 1)
//       {
//         if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
//         {
//           MusECore::MuseCount_t newAbsETick =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::TICKS) -
//               editor->raster();
//           const MusECore::MuseCount_t PTick = MUSE_TIME_UINT_TO_INT64 part->posValue(MusECore::Pos::TICKS);
//           if(newAbsETick < PTick)
//           {
//             newAbsETick = PTick;
//             const MusECore::MuseCount_t newAbsEEndTick = newAbsETick + editor->raster();
//             newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
//             newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::FRAMES);
//           }
//           newAbsEPos = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsETick, MusECore::Pos::TICKS, event.pos().type());
//           if(newAbsEPos < PPosEType)
//             newAbsEPos = PPosEType;
//           newEPos = newAbsEPos - PPosEType;
//           newAbsEFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, event.pos().type(), MusECore::Pos::FRAMES);
//         }
//         else if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT)
//         {
//           const MusECore::MuseCount_t newAbsEEndTick =
//             MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEPos, event.pos().type(), MusECore::Pos::TICKS) +
//               editor->raster();
//           newAbsEEnd = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEndTick, MusECore::Pos::TICKS, event.pos().type());
//           newAbsEEndFrame = MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, event.pos().type(), MusECore::Pos::FRAMES);
//         }
//       }
//
//       newELen = newAbsEEnd - newAbsEPos;
//       if(newELen < 1)
//         newELen = 1;
//
//       MusECore::Undo operations;
//
//       if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
//           if(!ctrl) {
//             newESPos = event.spos() + (newAbsEFrame - absEFrame);
//             if(newESPos < 0)
//               newESPos = 0;
//           }
//       }
//       else {
//           if(ctrl) {
//             newESPos = event.spos() - (newAbsEEndFrame - absEEndFrame);
//             if(newESPos < 0)
//               newESPos = 0;
//           }
//       }






      if (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT) {
          if(!ctrl) {
//             newESPos = eSPos + (newAbsEFrame - absEFrame);
            newESPos = event.sndFile().convertPosition(event.sndFile().unConvertPosition(eSPos) + (newAbsEFrame - absEFrame));
            if(newESPos < 0)
              newESPos = 0;
          }
      }
      else {
          if(ctrl) {
//             newESPos = eSPos - (newAbsEEndFrame - absEEndFrame);
            newESPos = event.sndFile().convertPosition(event.sndFile().unConvertPosition(eSPos) - (newAbsEEndFrame - absEEndFrame));
            if(newESPos < 0)
              newESPos = 0;
          }
      }






      MusECore::Undo operations;

//       const bool doExtendPart = newAbsEEnd > part->endValue(ePosType);
      //const bool doExtendPart = newAbsEEnd > PEndEType;
      const bool doExtendPart = newPEnd > part->endValue();

      if (! ((doExtendPart) && (part->hasHiddenEvents() & MusECore::Part::RightEventsHidden)) ) //operation is allowed
      {
        if (doExtendPart)// part must be extended?
        {
          //const MusECore::MuseCount_t newPartLen =
          //  MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - part->posValue();
            //MUSE_TIME_UINT_TO_INT64 MusECore::Pos::convert(newAbsEEnd, ePosType, part->type()) - wevent->tmpPartPos();
//           // This automatically takes care of port cached controllers.
//           schedule_resize_all_same_len_clone_parts(part, newPartLen, operations);
          fprintf(stderr, "resizeItem: extending\n");
          // This automatically takes care of port cached controllers.
          schedule_resize_all_same_len_clone_parts(part, newPEnd - part->posValue(), operations);
        }

        // Anything changed?
        const bool posChanged = event.posValue() != newEPos;
        if(posChanged || event.lenValue() != newELen || event.spos() != newESPos)
        {
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEventProperties,
            part,
            event,
            newEPos,
            newELen,
            newESPos,
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

      if(operations.empty())
        //this forces an update of the itemlist, which is necessary
        //to remove "forbidden" events from the list again
        //otherwise, if a moving operation was forbidden,
        //the canvas would still show the movement
        songChanged(SC_EVENT_MODIFIED);
      else
        MusEGlobal::song->applyOperationGroup(operations);
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool WaveCanvas::deleteItem(CItem* item)
      {
      WEvent* wevent = (WEvent*) item;
      if (wevent->part() == curPart) {
            MusECore::Event ev = wevent->event();
            // Indicate do undo, and do not do port controller values and clone parts.
            MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent, ev, curPart, false, false));
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   adjustWaveOffset
//---------------------------------------------------------

void WaveCanvas::adjustWaveOffset()
{
  bool have_selected = false;
  int init_offset = 0;
  
  for (iCItem k = items.begin(); k != items.end(); ++k) 
  {
    if (k->second->isSelected())
    {
      have_selected = true;
      init_offset = k->second->event().spos();
      break;
    }
  }

  if(!have_selected)
  {
    QMessageBox::information(this, 
        QString("MusE"),
        QWidget::tr("No wave events selected."));
    return;
  }

  bool ok = false;
  int offset = QInputDialog::getInt(this, 
                                    tr("Adjust Wave Offset"), 
                                    tr("Wave offset (frames)"), 
                                    init_offset, 
                                    0, 2147483647, 1, 
                                    &ok);
  if(!ok)
    return;    
  
  MusECore::Undo operations;

  // FIXME: Respect clones! If operating on two selected clones of the same part, an extra event is created!
  //        Check - Is it really this code's problem? Seems so, other operations like moving an event seem OK.
  for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
  {
    if(ici->second->isSelected())
    {
      MusECore::Event oldEvent = ici->second->event();
      if(oldEvent.spos() != offset)
      {
        MusECore::Part* part = ici->second->part();
        MusECore::Event newEvent = oldEvent.clone();
        newEvent.setSpos(offset);
        // Do not do port controller values and clone parts. 
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, oldEvent, part, false, false));
      }
    }
  }
  
  MusEGlobal::song->applyOperationGroup(operations);
  
  redraw();
}
      
//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void WaveCanvas::drawCanvas(QPainter& p, const QRect& rect, const QRegion& rg)
      {
      if (MusEGlobal::config.canvasShowGrid)
      {
        //---------------------------------------------------
        // vertical lines
        //---------------------------------------------------

        drawTickRaster(p, rect, rg, editor->raster(), true, false, false,
                      MusEGlobal::config.midiCanvasBeatColor, // color sequence slightly done by trial and error..
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasFineColor,
                      MusEGlobal::config.midiCanvasBarColor
                      );
      }
      }

//---------------------------------------------------------
//   waveCmd
//---------------------------------------------------------

void WaveCanvas::waveCmd(int cmd)
      {
      // TODO: New WaveCanvas: Convert this routine to frames.  
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

                  const MusECore::EventList& el = part->events();
                  MusECore::Undo operations;

                  std::list <MusECore::Event> elist;
                  for (MusECore::ciEvent e = el.lower_bound(pos[0] - part->tick()); e != el.end(); ++e)
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
                  const MusECore::EventList& el = part->events();

                  std::list<MusECore::Event> elist;
                  for (MusECore::ciEvent e = el.lower_bound(pos[0]); e != el.end(); ++e)
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
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void WaveCanvas::cmd(int cmd)
      {
      int modifyoperation = -1;
      double paramA = 0.0;
      switch (cmd) {
            case CMD_SELECT_ALL:     // select all
                  if (tool() == RangeTool) 
                  {
                    if (!editor->parts()->empty()) {
                          MusECore::iPart iBeg = editor->parts()->begin();
                          MusECore::iPart iEnd = editor->parts()->end();
                          iEnd--;
                          MusECore::WavePart* beg = (MusECore::WavePart*) iBeg->second;
                          MusECore::WavePart* end = (MusECore::WavePart*) iEnd->second;
                          selectionStart = beg->frame();
                          selectionStop  = end->frame() + end->lenFrame();
                          redraw();
                          }
                  }
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        if (!k->second->isSelected())
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_NONE:     // select none
                  selectionStart = selectionStop = 0;
                  deselectAll();
                  break;
            case CMD_SELECT_INVERT:     // invert selection
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        selectItem(k->second, !k->second->isSelected());
                        }
                  break;
            case CMD_SELECT_ILOOP:     // select inside loop
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        WEvent* wevent = (WEvent*)(k->second);
                        MusECore::Part* part     = wevent->part();
                        MusECore::Event event    = wevent->event();
                        unsigned tick  = event.tick() + part->tick();
                        if (tick < MusEGlobal::song->lpos() || tick >= MusEGlobal::song->rpos())
                              selectItem(k->second, false);
                        else
                              selectItem(k->second, true);
                        }
                  break;
            case CMD_SELECT_OLOOP:     // select outside loop
                  for (iCItem k = items.begin(); k != items.end(); ++k) {
                        WEvent* wevent = (WEvent*)(k->second);
                        MusECore::Part* part     = wevent->part();
                        MusECore::Event event    = wevent->event();
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
                 
            case CMD_ADJUST_WAVE_OFFSET:
                  adjustWaveOffset();
                  break;

            case CMD_EDIT_EXTERNAL:
                  modifyoperation = EDIT_EXTERNAL;
                  break;

            case CMD_EDIT_COPY:
                  modifyoperation = COPY;
                  break;
            case CMD_EDIT_CUT:
                  modifyoperation = CUT;
                  break;
            case CMD_EDIT_PASTE:
                  modifyoperation = PASTE;
                  break;

            case CMD_MUTE:
                  modifyoperation = MUTE;
                  break;

            case CMD_NORMALIZE:
                  modifyoperation = NORMALIZE;
                  break;

            case CMD_FADE_IN:
                  modifyoperation = FADE_IN;
                  break;

            case CMD_FADE_OUT:
                  modifyoperation = FADE_OUT;
                  break;

            case CMD_REVERSE:
                  modifyoperation = REVERSE;
                  break;

            case CMD_GAIN_FREE: {
                  EditGain* editGain = new EditGain(this, lastGainvalue);
                  if (editGain->exec() == QDialog::Accepted) {
                        lastGainvalue = editGain->getGain();
                        modifyoperation = GAIN;
                        paramA = (double)lastGainvalue / 100.0;
                        }
                  delete editGain;
                  }
                  break;

            case CMD_GAIN_200:
                  modifyoperation = GAIN;
                  paramA = 2.0;
                  break;

            case CMD_GAIN_150:
                  modifyoperation = GAIN;
                  paramA = 1.5;
                  break;

            case CMD_GAIN_75:
                  modifyoperation = GAIN;
                  paramA = 0.75;
                  break;

            case CMD_GAIN_50:
                  modifyoperation = GAIN;
                  paramA = 0.5;
                  break;

            case CMD_GAIN_25:
                  modifyoperation = GAIN;
                  paramA = 0.25;
                  break;

            case CMD_CREATE_PART_REGION:
                  {
                      // create a new part and put in the copy buffer
                      MusECore::Part* pt = editor->curCanvasPart();
                      if (pt == 0 || pt->track()->type() != MusECore::Track::WAVE)
                          return;
                      MusECore::WavePart *origPart = (MusECore::WavePart*)pt;
                      if (MusEGlobal::song->lpos() < origPart->tick() || MusEGlobal::song->rpos() > origPart->endTick())
                      {
                          QMessageBox::warning(this, tr("Part creation failed"),
                                       tr("Left and right position markers must be placed inside the current part."),
                                       QMessageBox::Ok, QMessageBox::Ok);
                          return;
                      }
                      MusECore::WavePart *tempPart = new MusECore::WavePart(origPart->track());
                      unsigned origFrame = origPart->frame();
                      unsigned frameDistance = MusEGlobal::song->lPos().frame() - origFrame;
                      tempPart->setPos(MusEGlobal::song->lpos());
                      tempPart->setLenTick(MusEGlobal::song->rpos() - MusEGlobal::song->lpos());
                      // loop through the events and set them accordingly
                      for (MusECore::ciEvent iWaveEvent = origPart->events().begin(); iWaveEvent != origPart->events().end(); iWaveEvent++)
                      {
                          // TODO: handle multiple events correctly,
                          // the math for subsequent events isn't correct
                          const MusECore::Event& ev = iWaveEvent->second;
                          MusECore::Event newEvent = ev.duplicate();
                          newEvent.setSpos(ev.spos() + frameDistance);
                          newEvent.setLenTick(MusEGlobal::song->rpos() - MusEGlobal::song->lpos());
                          tempPart->addEvent(newEvent);
                      }
                      std::set<const MusECore::Part*> partList;
                      partList.insert(tempPart);

                      QMimeData *mimeData =  MusECore::parts_to_mime(partList);
                      QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
                      QMessageBox::information(this, tr("Part created"),
                                   tr("The selected region has been copied to the clipboard and can be pasted in the arranger."),
                                   QMessageBox::Ok, QMessageBox::Ok);
                  }
                  break;
            case CMD_ERASE_MEASURE:
            case CMD_DELETE_MEASURE:
            case CMD_CREATE_MEASURE:
                  break;
            default:
//                  printf("unknown ecanvas cmd %d\n", cmd);
                  break;
            }
            
      if (modifyoperation != -1) {
            if (selectionStart == selectionStop && modifyoperation!=PASTE) {
                  printf("No selection. Ignoring\n"); //@!TODO: Disable menu options when no selection
                  QMessageBox::information(this, 
                     QString("MusE"),
                     QWidget::tr("No selection. Ignoring"));

                  return;
                  }
            
            //if(!modifyWarnedYet)
            //{
            //  modifyWarnedYet = true;
            //  if(QMessageBox::warning(this, QString("Muse"),
            //     tr("Warning! Muse currently operates directly on the sound file.\n"
            //        "Undo is supported, but NOT after exit, WITH OR WITHOUT A SAVE!"), tr("&Ok"), tr("&Cancel"),
            //     QString::null, 0, 1 ) != 0)
            //   return;
            //}
            modifySelection(modifyoperation, selectionStart, selectionStop, paramA);
            }
            
      itemSelectionsChanged();
      redraw();
      }

//---------------------------------------------------------
//   getSelection
//---------------------------------------------------------
// REMOVE Tim. wave. Changed. TODO: Work in progress. Editing does not work correctly if there is an offset or the part is partially hidden.
// MusECore::WaveSelectionList WaveCanvas::getSelection(unsigned startpos, unsigned stoppos)
//       {
//       MusECore::WaveSelectionList selection;
// 
//       for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
//             MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
//             unsigned part_offset = wp->frame();
//             
//             const MusECore::EventList& el = wp->events();
// 
//             for (MusECore::ciEvent e = el.begin(); e != el.end(); ++e) {
//                   MusECore::Event event  = e->second;
//                   if (event.empty())
//                         continue;
//                   MusECore::SndFileR file = event.sndFile();
//                   if (file.isNull())
//                         continue;
// 
//                   // Respect part end: Don't modify stuff outside of part boundary.
//                   unsigned elen = event.lenFrame();
//                   if(event.frame() + event.lenFrame() >= wp->lenFrame())
//                   {
//                     // Adjust apparent operation length:
//                     if(event.frame() > wp->lenFrame())
//                       elen = 0;
//                     else
//                       elen = wp->lenFrame() - event.frame();
//                   }
//                   
//                   unsigned event_offset = event.frame() + part_offset;
//                   unsigned event_startpos  = event.spos();
//                   unsigned event_length = elen + event.spos();
//                   unsigned event_end    = event_offset + event_length;
//                   //printf("startpos=%d stoppos=%d part_offset=%d event_offset=%d event_startpos=%d event_length=%d event_end=%d\n",
//                   // startpos, stoppos, part_offset, event_offset, event_startpos, event_length, event_end);
// 
//                   if (!(event_end <= startpos || event_offset > stoppos)) {
//                         int tmp_sx = startpos - event_offset + event_startpos;
//                         int tmp_ex = stoppos  - event_offset + event_startpos;
//                         unsigned sx;
//                         unsigned ex;
// 
//                         tmp_sx < (int)event_startpos ? sx = event_startpos : sx = tmp_sx;
//                         tmp_ex > (int)event_length   ? ex = event_length   : ex = tmp_ex;
// 
//                         //printf("Event data affected: %d->%d filename:%s\n", sx, ex, file.name().toLocal8Bit().constData());
//                         MusECore::WaveEventSelection s;
//                         s.event = event;  
//                         s.startframe = sx;
//                         s.endframe   = ex+1;
//                         //printf("sx=%d ex=%d\n",sx,ex);
//                         selection.push_back(s);
//                         }
//                   }
//             }
// 
//             return selection;
//       }

MusECore::WaveSelectionList WaveCanvas::getSelection(unsigned startpos, unsigned stoppos)
      {
#ifdef ALLOW_LEFT_HIDDEN_EVENTS

      MusECore::WaveSelectionList selection;

      for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
            MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
            const int part_offset = wp->frame();
            const int part_len = wp->lenFrame();
            
            const MusECore::EventList& el = wp->events();

            for (MusECore::ciEvent e = el.begin(); e != el.end(); ++e) {
                  MusECore::Event event  = e->second;
                  if (event.empty())
                        continue;
                  MusECore::SndFileR file = event.sndFile();
                  if (file.isNull())
                        continue;

                  // Respect part end: Don't modify stuff outside of part boundary.
                  int eframe = event.frame();
                  int elen = event.lenFrame();
                  // Adjust apparent operation start and length:
// REMOVE Tim. wave. Added. TODO: Work in progress. Editing does not work correctly if there is an offset or the part is partially hidden.
//                   if(eframe < 0)
//                   {
//                     elen += eframe;
//                     if(elen < 0)
//                       continue;
//                     eframe = 0;
//                   }
                  if(eframe >= part_len)
                    break;
                  if(eframe + elen <= 0)
                    continue;
                  if(eframe + elen >= part_len)
                    elen = part_len - eframe;
                  
                  const int event_offset = eframe + part_offset;
                  const int event_startpos  = event.spos();
// REMOVE Tim. wave. Changed. This seemed wrong and it caused a crash because events that were before the selection area were included.
//                   const int event_length = elen + event_startpos;
                  const int event_length = elen - event_startpos;
                  const int event_end    = event_offset + event_length;
                  //printf("startpos=%d stoppos=%d part_offset=%d event_offset=%d event_startpos=%d event_length=%d event_end=%d\n",
                  // startpos, stoppos, part_offset, event_offset, event_startpos, event_length, event_end);

                  if (event_end > (int)startpos && event_offset < (int)stoppos) {
                        int tmp_sx = (int)startpos - event_offset + event_startpos;
                        int tmp_ex = (int)stoppos  - event_offset + event_startpos;
                        int sx;
                        int ex;

                        tmp_sx < event_startpos ? sx = event_startpos : sx = tmp_sx;
                        tmp_ex > event_length   ? ex = event_length   : ex = tmp_ex;

                        //printf("Event data affected: %d->%d filename:%s\n", sx, ex, file.name().toLocal8Bit().constData());
                        MusECore::WaveEventSelection s;
                        s.event = event;  
                        s.startframe = sx;
                        s.endframe   = ex+1;
                        //printf("sx=%d ex=%d\n",sx,ex);
                        selection.push_back(s);
                        }
                  }
            }

            return selection;

#else

      MusECore::WaveSelectionList selection;

      for (MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) {
            MusECore::WavePart* wp = (MusECore::WavePart*)(ip->second);
            const unsigned part_offset = wp->frame();
            const unsigned part_len = wp->lenFrame();
            
            const MusECore::EventList& el = wp->events();

            for (MusECore::ciEvent e = el.begin(); e != el.end(); ++e) {
                  MusECore::Event event  = e->second;
                  if (event.empty())
                        continue;
                  MusECore::SndFileR file = event.sndFile();
                  if (file.isNull())
                        continue;

                  // Respect part end: Don't modify stuff outside of part boundary.
                  const unsigned eframe = event.frame();
                  unsigned elen = event.lenFrame();
                  if(eframe + elen >= part_len)
                  {
                    // Adjust apparent operation length:
                    if(eframe > part_len)
                      elen = 0;
                    else
                      elen = part_len - eframe;
                  }
                  
                  const unsigned event_offset = eframe + part_offset;
                  const unsigned event_startpos  = event.spos();
                  const unsigned event_length = elen + event.spos();
                  const unsigned event_end    = event_offset + event_length;
                  //printf("startpos=%d stoppos=%d part_offset=%d event_offset=%d event_startpos=%d event_length=%d event_end=%d\n",
                  // startpos, stoppos, part_offset, event_offset, event_startpos, event_length, event_end);

                  if (!(event_end <= startpos || event_offset > stoppos)) {
                        int tmp_sx = startpos - event_offset + event_startpos;
                        int tmp_ex = stoppos  - event_offset + event_startpos;
                        unsigned sx;
                        unsigned ex;

                        tmp_sx < (int)event_startpos ? sx = event_startpos : sx = tmp_sx;
                        tmp_ex > (int)event_length   ? ex = event_length   : ex = tmp_ex;

                        //printf("Event data affected: %d->%d filename:%s\n", sx, ex, file.name().toLocal8Bit().constData());
                        MusECore::WaveEventSelection s;
                        s.event = event;  
                        s.startframe = sx;
                        s.endframe   = ex+1;
                        //printf("sx=%d ex=%d\n",sx,ex);
                        selection.push_back(s);
                        }
                  }
            }

            return selection;
#endif
      }


//---------------------------------------------------------
//   modifySelection
//---------------------------------------------------------
void WaveCanvas::modifySelection(int operation, unsigned startpos, unsigned stoppos, double paramA)
      {
        if (operation == PASTE) {
          // we need to redefine startpos and stoppos
          if (copiedPart =="")
            return;
          MusECore::SndFile pasteFile(copiedPart);
          pasteFile.openRead();
          startpos = pos[0];
          stoppos = startpos+ pasteFile.samples(); // possibly this is wrong if there are tempo changes
          pasteFile.close();
          pos[0]=stoppos;
        }

        //
        // Copy on Write: Check if some files need to be copied, either because they are not 
        //  writable, or more than one independent (non-clone) wave event shares a wave file.
        //
        
        MusECore::WaveSelectionList selection = getSelection(startpos, stoppos);
        std::vector<MusECore::SndFileR> copy_files_proj_dir;
        for(MusECore::iWaveSelection i = selection.begin(); i != selection.end(); i++) 
        {
          MusECore::WaveEventSelection w = *i;
          if(w.event.empty())
            continue;
          MusECore::SndFileR file = w.event.sndFile();
          if(file.isNull())
            continue;
          if(sndFileCheckCopyOnWrite(file))
          {
            std::vector<MusECore::SndFileR>::iterator i = copy_files_proj_dir.begin();
            for( ; i != copy_files_proj_dir.end(); ++i)
            {
              if(i->canonicalPath() == file.canonicalPath())
                break; 
            }  
            if(i == copy_files_proj_dir.end())
              copy_files_proj_dir.push_back(file);
          }
        }
        if(!copy_files_proj_dir.empty())
        {
          CopyOnWriteDialog* dlg = new CopyOnWriteDialog();
          for(std::vector<MusECore::SndFileR>::iterator i = copy_files_proj_dir.begin(); i != copy_files_proj_dir.end(); ++i)
          {
            qint64 sz = QFile(i->canonicalPath()).size();
            QString s;
            if(sz > 1048576)
              s += QString::number(sz / 1048576) + "MB ";
            else
            if(sz > 1024)
              s += QString::number(sz / 1024) + "KB ";
            else
              s += QString::number(sz) + "B ";
            s += i->canonicalPath();
            dlg->addProjDirFile(s);
          }
          int rv = dlg->exec();
          delete dlg;
          if(rv != QDialog::Accepted)
            return;
          // Has a project been created yet?
          if(MusEGlobal::museProject == MusEGlobal::museProjectInitPath) // && MusEGlobal::config.useProjectSaveDialog
          { 
            // No project, we need to create one.
            if(!MusEGlobal::muse->saveAs())
              return; // No project, don't want to copy without one.
            //setFocus(); // For some reason focus is given away to Arranger
          }
          for(MusECore::iWaveSelection i = selection.begin(); i != selection.end(); i++)
          {
            MusECore::WaveEventSelection w = *i;
            if(w.event.empty())
              continue;
            MusECore::SndFileR file = w.event.sndFile();
            if(file.isNull() || !sndFileCheckCopyOnWrite(file)) // Make sure to re-check
              continue;
            QString filePath = MusEGlobal::museProject + QString("/") + file.name();
            QString newFilePath;
            if(MusECore::getUniqueFileName(filePath, newFilePath))
            {
              {
                QFile qf(file.canonicalPath());
                if(!qf.copy(newFilePath)) // Copy the file
                {
                  printf("MusE Error: Could not copy to new sound file (file exists?): %s\n", newFilePath.toLocal8Bit().constData());
                  continue;  // Let's not overwrite an existing file
                }
              }  
              QFile nqf(newFilePath);
              // Need to make sure some permissions are set...
              QFile::Permissions pm = nqf.permissions();
              if(!(pm & QFile::ReadOwner))
              {
                pm |= QFile::ReadOwner;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set read owner permissions on new sound file: %s\n", newFilePath.toLocal8Bit().constData());
                  continue; 
                }
              }
              if(!(pm & QFile::WriteOwner))
              {
                pm |= QFile::WriteOwner;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set write owner permissions on new sound file: %s\n", newFilePath.toLocal8Bit().constData());
                  continue; 
                }
              }
              if(!(pm & QFile::ReadUser))
              {
                pm |= QFile::ReadUser;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set read user permissions on new sound file: %s\n", newFilePath.toLocal8Bit().constData());
                  continue; 
                }
              }
              if(!(pm & QFile::WriteUser))
              {
                pm |= QFile::WriteUser;
                if(!nqf.setPermissions(pm))
                {
                  printf("MusE Error: Could not set write user permissions on new sound file: %s\n", newFilePath.toLocal8Bit().constData());
                  continue; 
                }
              }
              MusECore::SndFile* newSF = new MusECore::SndFile(newFilePath);
              MusECore::SndFileR newSFR(newSF);  // Create a sndFileR for the new file
              if(newSFR.openRead())  
              {
                printf("MusE Error: Could not open new sound file: %s\n", newSFR.canonicalPath().toLocal8Bit().constData());
                continue; // newSF will be deleted when newSFR goes out of scope and is deleted
              }
              MusEGlobal::audio->msgIdle(true); 
              w.event.sndFile().close();             // Close the old file.
              // NOTE: For now, don't bother providing undo for this. Reason: If the user undoes
              //  and then modifies again, it will prompt to create new copies each time. There is
              //  no mechanism ("touched"?) to tell if an existing copy would be suitable to just 'drop in'.
              // It would help if we deleted the wave file copies upon undo, but not too crazy about that. 
              // So since the copy has already been created and "there it is", we might as well use it.
              // It means that events and even undo items BEFORE this operation will point to this 
              //  NEW wave file (as if they always did). It also means the user CANNOT change back 
              //  to the old file...    Oh well, this IS Copy On Write.
              // FIXME: Find a conceptual way to make undo work with or without deleting the copies. 
              w.event.setSndFile(newSFR);            // Set the new file.
              MusEGlobal::audio->msgIdle(false); 
            }
          }
        }
         
         MusEGlobal::song->startUndo();
         for (MusECore::iWaveSelection i = selection.begin(); i != selection.end(); i++) {
               MusECore::WaveEventSelection w = *i;
               if(w.event.empty())
                 continue;
               MusECore::SndFileR file         = w.event.sndFile();
               if(file.isNull())
                 continue;
               unsigned sx            = w.startframe;
               unsigned ex            = w.endframe;
               unsigned file_channels = file.channels();

               QString tmpWavFile;
               if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", tmpWavFile)) {
                     break;
                     }

               MusEGlobal::audio->msgIdle(true); // Not good with playback during operations
               MusECore::SndFile tmpFile(tmpWavFile);
               tmpFile.setFormat(file.format(), file_channels, file.samplerate());
               if (tmpFile.openWrite()) {
                     MusEGlobal::audio->msgIdle(false);
                     printf("Could not open temporary file...\n");
                     break;
                     }

               //
               // Write out data that will be changed to temp file
               //
               unsigned tmpdatalen = ex - sx;
               off_t    tmpdataoffset = sx;
               float*   tmpdata[file_channels];

               for (unsigned i=0; i<file_channels; i++) {
                     tmpdata[i] = new float[tmpdatalen];
                     }
               file.seek(tmpdataoffset, 0);
               file.readWithHeap(file_channels, tmpdata, tmpdatalen);
               file.close();
               tmpFile.write(file_channels, tmpdata, tmpdatalen, MusEGlobal::config.liveWaveUpdate);
               tmpFile.close();

               switch(operation)
               {
                     case MUTE:
                           muteSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case NORMALIZE:
                           normalizeSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case FADE_IN:
                           fadeInSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case FADE_OUT:
                           fadeOutSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case REVERSE:
                           reverseSelection(file_channels, tmpdata, tmpdatalen);
                           break;

                     case GAIN:
                           applyGain(file_channels, tmpdata, tmpdatalen, paramA);
                           break;
                     case CUT:
                           copySelection(file_channels, tmpdata, tmpdatalen, true, file.format(), file.samplerate());
                           break;
                     case COPY:
                           copySelection(file_channels, tmpdata, tmpdatalen, false, file.format(), file.samplerate());
                           break;
                     case PASTE:
                           {
                           MusECore::SndFile pasteFile(copiedPart);
                           pasteFile.openRead();
                           pasteFile.seek(tmpdataoffset, 0);
                           pasteFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
                           }
                           break;

                     case EDIT_EXTERNAL:
                           editExternal(file.format(), file.samplerate(), file_channels, tmpdata, tmpdatalen);
                           break;

                     default:
                           printf("Error: Default state reached in modifySelection\n");
                           break;

               }

               file.openWrite();
               file.seek(tmpdataoffset, 0);
               file.write(file_channels, tmpdata, tmpdatalen, MusEGlobal::config.liveWaveUpdate);
               file.update();
               file.close();
               file.openRead();

               for (unsigned i=0; i<file_channels; i++) {
                     delete[] tmpdata[i];
                     }

               // Undo handling
               MusEGlobal::song->cmdChangeWave(w.event, tmpWavFile, sx, ex);
               MusEGlobal::audio->msgIdle(false); // Not good with playback during operations
               }
         MusEGlobal::song->endUndo(SC_CLIP_MODIFIED);
         redraw();
      }

//---------------------------------------------------------
//   copySelection
//---------------------------------------------------------
void WaveCanvas::copySelection(unsigned file_channels, float** tmpdata, unsigned length, bool blankData, unsigned format, unsigned sampleRate)
{
      if (copiedPart!="") {
        QFile::remove(copiedPart);
      }
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", copiedPart)) {
            return;
            }

      MusECore::SndFile tmpFile(copiedPart);
      tmpFile.setFormat(format, file_channels, sampleRate);
      tmpFile.openWrite();
      tmpFile.write(file_channels, tmpdata, length, MusEGlobal::config.liveWaveUpdate);
      tmpFile.close();

      if (blankData) {
        // Set everything to 0!
        for (unsigned i=0; i<file_channels; i++) {
              for (unsigned j=0; j<length; j++) {
                    tmpdata[i][j] = 0;
                    }
              }
        }
}

//---------------------------------------------------------
//   muteSelection
//---------------------------------------------------------
void WaveCanvas::muteSelection(unsigned channels, float** data, unsigned length)
      {
      // Set everything to 0!
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = 0;
                  }
            }
      }

//---------------------------------------------------------
//   normalizeSelection
//---------------------------------------------------------
void WaveCanvas::normalizeSelection(unsigned channels, float** data, unsigned length)
      {
      float loudest = 0.0;

      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  if (data[i][j]  > loudest)
                        loudest = data[i][j];
                  }
            }

      double scale = 0.99 / (double)loudest;

      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   fadeInSelection
//---------------------------------------------------------
void WaveCanvas::fadeInSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) j / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   fadeOutSelection
//---------------------------------------------------------
void WaveCanvas::fadeOutSelection(unsigned channels, float** data, unsigned length)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  double scale = (double) (length - j) / (double)length ;
                  data[i][j] = (float) ((double)data[i][j] * scale);
                  }
            }
      }

//---------------------------------------------------------
//   reverseSelection
//---------------------------------------------------------
void WaveCanvas::reverseSelection(unsigned channels, float** data, unsigned length)
      {
      if(length <= 1)    
        return;
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length/2; j++) {
                  float tmpl = data[i][j];
                  float tmpr = data[i][length - j - 1];
                  data[i][j] = tmpr;
                  data[i][length - j - 1] = tmpl;
                  }
            }
      }
//---------------------------------------------------------
//   applyGain
//---------------------------------------------------------
void WaveCanvas::applyGain(unsigned channels, float** data, unsigned length, double gain)
      {
      for (unsigned i=0; i<channels; i++) {
            for (unsigned j=0; j<length; j++) {
                  data[i][j] = (float) ((double)data[i][j] * gain);
                  }
            }
      }

//---------------------------------------------------------
//   editExternal
//---------------------------------------------------------
void WaveCanvas::editExternal(unsigned file_format, unsigned file_samplerate, unsigned file_channels, float** tmpdata, unsigned tmpdatalen)
      {
      // Create yet another tmp-file
      QString exttmpFileName;
      if (!MusEGlobal::getUniqueTmpfileName("tmp_musewav",".wav", exttmpFileName)) {
            printf("Could not create temp file - aborting...\n");
            return;
            }

      MusECore::SndFile exttmpFile(exttmpFileName);
      exttmpFile.setFormat(file_format, file_channels, file_samplerate);
      if (exttmpFile.openWrite()) {
            printf("Could not open temporary file...\n");
            return;
            }
      // Write out change-data to this file:
      exttmpFile.write(file_channels, tmpdata, tmpdatalen, MusEGlobal::config.liveWaveUpdate);
      exttmpFile.close();

      printf("Temporary file: %s\n", qPrintable(exttmpFileName));

      QProcess proc;
      QStringList arguments;
      arguments << exttmpFileName;
      proc.start(MusEGlobal::config.externalWavEditor, arguments);

      // Wait forever. This freezes MusE until returned.
      // FIXME TODO: Try to make something that does it asynchronously while MusE still runs?
      //             Hm, that'd be quite hard... (More like inter-app 'live collaboration' support).
      if(!proc.waitForFinished(-1))
      {
        QMessageBox::warning(this, tr("MusE - external editor failed"),
              tr("MusE was unable to launch the external editor\ncheck if the editor setting in:\n"
              "Global Settings->Audio:External Waveditor\nis set to a valid editor."));
      }

      if(proc.exitStatus() != QProcess::NormalExit)
      {
        std::fprintf(stderr, "\nError: Launch external wave editor: Exit status: %d File: %s\n", 
                      proc.exitStatus(), MusEGlobal::config.externalWavEditor.toLocal8Bit().constData());
      }

      if(proc.exitCode() != 0)
      {
        std::fprintf(stderr, "\nError: Launch external wave editor: Exit code: %d File: %s\n", 
                      proc.exitCode(), MusEGlobal::config.externalWavEditor.toLocal8Bit().constData());
      }

      if (exttmpFile.openRead()) {
          printf("Could not reopen temporary file!\n");
          }
      else {
          // Re-read file again
          exttmpFile.seek(0, 0);
          size_t sz = exttmpFile.readWithHeap(file_channels, tmpdata, tmpdatalen);
          if (sz != tmpdatalen) {
                  // File must have been shrunken - not good. Alert user.
                  QMessageBox::critical(this, tr("MusE - file size changed"),
                      tr("When editing in external editor - you should not change the filesize\nsince it must fit the selected region.\n\nMissing data is muted"));
                  for (unsigned i=0; i<file_channels; i++) {
                      for (unsigned j=sz; j<tmpdatalen; j++) {
                              tmpdata[i][j] = 0;
                              }
                      }
                  }
          }
      QDir dir = exttmpFile.dirPath();
      dir.remove(exttmpFileName);
      dir.remove(exttmpFile.basename() + ".wca");
      }

      
      
//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void WaveCanvas::startDrag(CItem* /* item*/, DragType t)
{
   QMimeData* md = MusECore::selected_events_to_mime(MusECore::partlist_to_set(editor->parts()), 1);

   if (md) {
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

void WaveCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      //event->accept(Q3TextDrag::canDecode(event));
      event->acceptProposedAction();  // TODO CHECK Tim.
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void WaveCanvas::dragMoveEvent(QDragMoveEvent*)
      {
      //printf("drag move %x\n", this); DELETETHIS (whole function?)
      //event->acceptProposedAction();  
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void WaveCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
      //printf("drag leave\n");         DELETETHIS (whole function?)
      //event->acceptProposedAction();  
      }

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void WaveCanvas::curPartChanged()
      {
      EventCanvas::curPartChanged();
      editor->setWindowTitle(getCaption());
      }

//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void WaveCanvas::modifySelected(NoteInfo::ValType type, int val, bool delta_mode)
      {
      // TODO: New WaveCanvas: Convert this routine to frames and remove unneeded operations. 
      QList< QPair<QUuid,MusECore::Event> > already_done;
      MusECore::Undo operations;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!(i->second->isSelected()))
                  continue;
            WEvent* e   = (WEvent*)(i->second);
            MusECore::Event event = e->event();
            if (event.type() != MusECore::Note)
                  continue;

            MusECore::WavePart* part = (MusECore::WavePart*)(e->part());
            
            if (already_done.contains(QPair<QUuid,MusECore::Event>(part->clonemaster_uuid(), event)))
              continue;
            
            MusECore::Event newEvent = event.clone();

            switch (type) {
                  case NoteInfo::VAL_TIME:
                        {
                        int newTime = val;
                        if(delta_mode)
                          newTime += event.tick();
                        else
                          newTime -= part->tick();
                        if (newTime < 0)
                           newTime = 0;
                        newEvent.setTick(newTime);
                        }
                        break;
                  case NoteInfo::VAL_LEN:
                        {
                        int len = val;
                        if(delta_mode)
                          len += event.lenTick();
                        if (len < 1)
                              len = 1;
                        newEvent.setLenTick(len);
                        }
                        break;
                  case NoteInfo::VAL_VELON:
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
                  case NoteInfo::VAL_VELOFF:
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
                  case NoteInfo::VAL_PITCH:
                        {
                        int pitch = val;
                        if(delta_mode)
                          pitch += event.pitch();
                        if (pitch > 127)
                              pitch = 127;
                        else if (pitch < 0)
                              pitch = 0;
                        newEvent.setPitch(pitch);
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

void WaveCanvas::resizeEvent(QResizeEvent* ev)
      {
      // Readjust all wave canvas item heights  
      bool do_redraw = false;
      for (iCItem k = items.begin(); k != items.end(); ++k) 
      {
        if(k->second->height() != ev->size().height())
        {
          k->second->setHeight(ev->size().height());
          do_redraw = true;
        }
      }
  
      if (ev->size().width() != ev->oldSize().width())
            emit newWidth(ev->size().width());
      EventCanvas::resizeEvent(ev);
  
      if(do_redraw)
        redraw();
      }

//---------------------------------------------------------
//   genItemPopup
//---------------------------------------------------------

QMenu* WaveCanvas::genItemPopup(CItem* item)
      {
      QMenu* eventPopup = new QMenu(this);

      eventPopup->addAction(new MenuTitleItem(tr("Wave event:"), eventPopup));

      eventPopup->addSeparator();
      QAction *act_settings = eventPopup->addAction(tr("Converter settings"));
      act_settings->setData(0);
      act_settings->setEnabled(item && !item->event().sndFile().isNull());

      genCanvasPopup(eventPopup);
      return eventPopup;
      }

//---------------------------------------------------------
//   itemPopup
//---------------------------------------------------------

void WaveCanvas::itemPopup(CItem* /*item*/, int n, const QPoint& /*pt*/)
{
   if(n >= TOOLS_ID_BASE)
   {
      canvasPopup(n);
      return;
   }

  switch(n)
  {
    case 0:     // Settings
    if(curItem && !curItem->event().sndFile().isNull())
    {
      if(MusECore::AudioConverterSettingsGroup* settings = curItem->event().sndFile().audioConverterSettings())
      {
        MusECore::AudioConverterSettingsGroup* wrk_set = new MusECore::AudioConverterSettingsGroup(true); // Local settings.
        wrk_set->assign(*settings);
        AudioConverterSettingsDialog dialog(this, 
                                            &MusEGlobal::audioConverterPluginList, 
                                            wrk_set, 
                                            true); // Local settings.
        if(dialog.exec() == QDialog::Accepted)
        {
          MusECore::PendingOperationList operations;
          MusEGlobal::song->modifyAudioConverterSettingsOperation(
            curItem->event().sndFile(),
            wrk_set,
            MusEGlobal::defaultAudioConverterSettings,
            true,  // Local settings.
            operations);

          if(operations.empty())
          {
            delete wrk_set;
          }
          else
          {
            MusEGlobal::audio->msgExecutePendingOperations(operations, true);
            //MusEGlobal::song->update(SC_);
          }
        }
        else
        {
          delete wrk_set;
        }
        
      }
    }
    break;
        
    default:
        printf("unknown action %d\n", n);
        break;
  }
}


void WaveCanvas::setRangeToSelection() {

    if (selectionStop > selectionStart) {
        int tick_min = MusEGlobal::tempomap.frame2tick(selectionStart, 0, MusECore::LargeIntRoundNearest);
        int tick_max = MusEGlobal::tempomap.frame2tick(selectionStop, 0, MusECore::LargeIntRoundNearest);
        MusECore::Pos p1(tick_min, true);
        MusECore::Pos p2(tick_max, true);

        if (p1 < MusEGlobal::song->lPos()) {
            MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
            MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
        } else {
            MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
            MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
        }
    }
}

// REMOVE Tim. wave. Added.
void WaveCanvas::setColorMode(int mode) {
  colorMode = mode;
  redraw();
}


} // namespace MusEGui

