//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlcanvas.cpp,v 1.15.2.10 2009/11/14 03:37:48 terminator356 Exp $
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

#include <stdio.h>
#include <limits.h>

#include <QApplication>
#include <QPainter>
#include <QCursor>
#include <QMouseEvent>
#include <QAction>
#include <QToolTip>

#include "ctrlcanvas.h"
#include "app.h"
#include "globals.h"
#include "ctrledit.h"
#include "midieditor.h"
#include "icons.h"
#include "midiport.h"
#include "song.h"
#include "midictrl.h"
#include "audio.h"
#include "gconfig.h"
#include "ctrlpanel.h"
#include "midiedit/drummap.h"
#include "drumedit.h"
#include "drummap.h"
#include "functions.h"
#include "popupmenu.h"
#include "menutitleitem.h"

#define ABS(x)  ((x) < 0) ? -(x) : (x)

static MusECore::MidiCtrlValList veloList(MusECore::CTRL_VELOCITY);    // dummy

namespace MusEGui {

// static
const int CtrlCanvas::overlayTextOffsetFromOrg = 2;

//---------------------------------------------------------
//   computeVal
//---------------------------------------------------------

static int computeVal(MusECore::MidiController* mc, int y, int height)
      {
      int min; int max;
      if(mc->num() == MusECore::CTRL_PROGRAM)
      {
        min = 1;
        max = 128;
      }
      else
      {
        min = mc->minVal();
        max = mc->maxVal();
      }
      int val = max - (y * (max-min) / height);
      if (val < min)
            val = min;
      if (val > max)
            val = max;
      if(mc->num() != MusECore::CTRL_PROGRAM)
        val += mc->bias();
      return val;
      }

//---------------------------------------------------------
//   computeY
//---------------------------------------------------------

static int computeY(const MusECore::MidiController* mc, int val, int height)
      {
      int min; int max; 
      if(mc->num() == MusECore::CTRL_PROGRAM)
      {
        min = 1;
        max = 128;
      }
      else
      {
        min = mc->minVal();
        max = mc->maxVal();
      }
      
      //printf("computeY #1 min:%d max:%d val:%d bias:%d height:%d\n", min, max, val, mc->bias(), height);
      
      if(mc->num() != MusECore::CTRL_PROGRAM)
        val -= mc->bias();
      
      if (val < min)
            val = min;
      if (val > max)
            val = max;
      
      int y = min == max ? 0 : (max-val)*height/(max-min);
      
      //printf("computeY #2 y:%d min:%d max:%d val:%d bias:%d height:%d\n", y, min, max, val, mc->bias(), height);
      
      return y;
      }

//---------------------------------------------------------
//   CEvent
//---------------------------------------------------------

CEvent::CEvent() : CItem()
{
  _part = nullptr;
  _val = 0;
  ex = 0;
}

CEvent::CEvent(const MusECore::Event& e, MusECore::Part* pt, int v) : CItem()
{
  _part = pt;
  _event = e;
  _val   = v;
  ex     = !e.empty() ? e.tick() : 0;
}

bool CEvent::isObjectInRange(const MusECore::Pos& p0, const MusECore::Pos& p1) const
{
  MusECore::Pos pos = _event.pos();
  if(_part)
    pos += (*_part);
  return pos >= p0 && pos < p1;
}

//---------------------------------------------------------
//   intersects
//---------------------------------------------------------

bool CEvent::intersectsController(const MusECore::MidiController* mc, const QRect& r, const int tickstep, const int wh) const
{
      if(_event.empty())
        return false;
      int y1 = computeY(mc, _val, wh);
      
      int tick1 = _event.tick() + _part->tick();
      if(ex == -1)
        return tick1 < (r.x() + r.width()) && y1 < (r.y() + r.height());
      
      int tick2 = ex + _part->tick();
      
      // Velocities don't use EX (set equal to event tick, giving zero width here),
      //  and velocities are drawn three pixels wide, so adjust the width now. 
      // Remember, each drawn pixel represents one tickstep which varies with zoom.
      // So that's 3 x tickstep for each velocity line.
      // Hmm, actually, for better pin-point accuracy just use one tickstep for now.
      if(MusECore::midiControllerType(mc->num()) == MusECore::MidiController::Velo)
        tick2 += tickstep;
      
      QRect er(tick1, y1, tick2 - tick1, wh - y1);   

      return r.intersects(er);
}

bool CEvent::containsPoint(const MusECore::MidiController* mc, const QPoint& p, const int tickstep, const int wh) const
{
  if(_event.empty())
    return false;
  const int y1 = computeY(mc, _val, wh);
  
  const int tick1 = _event.tick() + _part->tick();
  if(ex == -1)
    return tick1 <= p.x() && y1 <= p.y();
  
  int tick2 = ex + _part->tick();
  
  // Velocities don't use EX (set equal to event tick, giving zero width here),
  //  and velocities are drawn three pixels wide, so adjust the width now. 
  // Remember, each drawn pixel represents one tickstep which varies with zoom.
  // So that's 3 x tickstep for each velocity line.
  // Hmm, actually, for better pin-point accuracy just use one tickstep for now.
  if(MusECore::midiControllerType(mc->num()) == MusECore::MidiController::Velo)
    tick2 += tickstep;
  
  return p.x() >= tick1 && p.x() < tick2 && y1 <= p.y();
}

//---------------------------------------------------------
//   contains
//---------------------------------------------------------

bool CEvent::containsXRange(int x1, int x2) const
      {
      int tick1 = !_event.empty() ? _event.tick() + _part->tick() : 0;
      if(ex == -1)
        return (tick1 < x2);
      
      int tick2 = ex + _part->tick();
      return ((tick1 >= x1 && tick1 < x2)
         //|| (tick2 >= x1 && tick2 < x2) DELETETHIS?
         || (tick2 > x1 && tick2 < x2)
         || (tick1 < x1 && tick2 >= x2));
      }

//---------------------------------------------------------
//   eventWithLength
//   See header comments about this small Event hack...
//---------------------------------------------------------

MusECore::Event CEvent::eventWithLength() const
{ 
  // Grab a clone of the event.
  MusECore::Event new_e = _event.clone();
  
  // Synthesize a length value.
  // This is HACK for copy/paste and the value must be reset
  //  to zero after usage.
  const unsigned int pos_val = new_e.posValue();
  unsigned int len = 0;
  if(ex >= 0)
  {
    if((unsigned int)ex > pos_val)
      len = ex - pos_val;
    else
      // It's an error, but give it a minimum length of 1.
      len = 1;
  }
  new_e.setLenValue(len);
  
  return new_e;
}

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

CtrlCanvas::CtrlCanvas(MidiEditor* e, QWidget* parent, int xmag,
   const char* name, CtrlPanel* pnl) : View(parent, xmag, 1, name)
      {
      // Reset these since our parent will typically turn them on for speed.
      // Definitely not static contents. Full repaint desired upon resize
      //  because the contents need to scale. Without this, contents were not
      //  updating on Mint Cinnamon when adjusting the vertical splitters.
      setAttribute(Qt::WA_StaticContents, false);
      setStatusTip(tr("Control canvas: Use Pencil tool to edit events and Draw tool to adjust them gradually. Hold Ctrl to affect only existing events."));

      if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
          setBg(MusEGlobal::config.midiControllerViewBg);
          setBg(QPixmap());
      }
      else {
            setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
      }
      setFocusPolicy(Qt::StrongFocus);
      _cursorOverrideCount = 0;
      setFont(MusEGlobal::config.fonts[3]);  
      //_cursorShape = Qt::ArrowCursor;
      _mouseGrabbed = false;
      curItem = nullptr;
      _movingItemUnderCursor = nullptr;
      editor = e;
      _panel = pnl;
      drag   = DRAG_OFF;
      _dragType = MOVE_MOVE;
      tool   = MusEGui::PointerTool;
      //button = Qt::NoButton;
      
      _dragFirstXPos = 0;
      line1x = line1y = line2x = line2y = 0;
      drawLineMode = false;
      _bgAlpha = static_cast<quint8>(MusEGlobal::config.globalAlphaBlend / 2);

      // Initialize the position markers.
      pos[0] = MusEGlobal::song->cPos().tick();
      pos[1] = MusEGlobal::song->lPos().tick();
      pos[2] = MusEGlobal::song->rPos().tick();
      
      noEvents=false;
      _perNoteVeloMode = MusEGlobal::config.velocityPerNote;
      if(_panel)
        _panel->setVeloPerNoteMode(_perNoteVeloMode);
      
      filterTrack=false;

      ctrl   = &veloList;
      _controller = &MusECore::veloCtrl;
      _cnum  = MusECore::CTRL_VELOCITY;    
      _dnum  = MusECore::CTRL_VELOCITY;    
      _didx  = MusECore::CTRL_VELOCITY;      
      connect(MusEGlobal::song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));

      setMouseTracking(true);
      curPart = nullptr;
      curTrack = nullptr;
      if (!editor->parts()->empty())
            setCurTrackAndPart();

      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
      
      setCurDrumPitch(editor->curDrumInstrument());
      //printf("CtrlCanvas::CtrlCanvas curDrumPitch:%d\n", curDrumPitch);
                          
      connect(editor, SIGNAL(curDrumInstrumentChanged(int)), SLOT(setCurDrumPitch(int)));
      updateItems();
      }

CtrlCanvas::~CtrlCanvas()
{
  // Just in case the ref count is not 0. This is our last chance to clear 
  //  our contribution to QApplication::setOverrideCursor references.
  showCursor();
  
  items.clearDelete();
}
   
//---------------------------------------------------------
//   setPanel
//---------------------------------------------------------

void CtrlCanvas::setPanel(CtrlPanel* pnl)
{
  _panel = pnl;
  if(_panel)
    _panel->setVeloPerNoteMode(_perNoteVeloMode);
}

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void CtrlCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)
      {
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
                              int ppos =  val - rmapxDev(width()/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < 0) {
                              int ppos =  val - rmapxDev(width()*3/4);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        break;
                  case MusECore::Song::CONTINUOUS:
                        if (npos > (width()*5)/8) {
                              int ppos =  pos[idx] - rmapxDev(width()*5/8);
                              if (ppos < 0)
                                    ppos = 0;
                              emit followEvent(ppos);
                              opos = mapx(pos[idx]);
                              npos = mapx(val);
                              }
                        else if (npos < (width()*3)/8) {
                              int ppos =  pos[idx] - rmapxDev(width()*3/8);
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
      redraw(QRect(x, 0, w, height()));
      }

//---------------------------------------------------------
//   setMidiController
//---------------------------------------------------------

void CtrlCanvas::setMidiController(int num)
      {
      _cnum = num;    
      partControllers(curPart, _cnum, &_dnum, &_didx, &_controller, &ctrl, &_ctrlInfo);
      
      if(_panel)
      {
        if(_cnum == MusECore::CTRL_VELOCITY)              
          _panel->setHWController(curTrack, &MusECore::veloCtrl);
        else 
          _panel->setHWController(curTrack, _controller);
      }
    }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void CtrlCanvas::keyPressEvent(QKeyEvent *event)
{
  switch(event->key())
  {
    case Qt::Key_Control:
      _dragType = MOVE_COPY;
      setCursor();
      event->accept();
      return;
    break;
    
    case Qt::Key_Escape:
      if(!moving.empty())
      {
        cancelMouseOps();
        setCursor();
        event->accept();
        return;
      }
    break;
    
    default:
    break;
  }
  
  event->ignore();
  View::keyPressEvent(event);
}

//---------------------------------------------------------
//   keyReleaseEvent
//---------------------------------------------------------

void CtrlCanvas::keyReleaseEvent(QKeyEvent *event)
{
  switch(event->key())
  {
    case Qt::Key_Control:
      _dragType = MOVE_MOVE;
      setCursor();
      event->accept();
      return;
    break;
    
    default:
    break;
  }
  
  event->ignore();
  View::keyReleaseEvent(event);
}
    
//---------------------------------------------------------
//   enterEvent
//---------------------------------------------------------

void CtrlCanvas::enterEvent(QEvent*)
      {
        setCursor();
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void CtrlCanvas::leaveEvent(QEvent*)
      {
      emit xposChanged(INT_MAX);
      emit yposChanged(-1);
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint CtrlCanvas::raster(const QPoint& p) const
      {
      if(!editor)
        return p;
      int x = p.x();
      if (x < 0)
            x = 0;
      x = editor->rasterVal(x);
      return QPoint(x, p.y());
      }

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void CtrlCanvas::deselectAll()
      {
        // To save time searching the potentially large 'items' list, a selection list is used.
        for(iCItemList i = selection.begin(); i != selection.end(); ++i)
            (*i)->setSelected(false);

        // Removed. Let itemSelectionsChanged() handle it later.
        //selection.clear();
      }

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void CtrlCanvas::selectItem(CEvent* e)
      {
      e->setSelected(true);
      for (iCItemList i = selection.begin(); i != selection.end(); ++i) {
            if (*i == e) {
                    // It was found in the list. Just return.
                    // It will be selected by now, from setSelected() above.
                    return;
                  }
            }
            
      // It's not in the selection list. Add it now.
      selection.push_back(e);
      }

//---------------------------------------------------------
//   deselectItem
//---------------------------------------------------------

void CtrlCanvas::deselectItem(CEvent* e)
      {
      e->setSelected(false);
      // The item cannot be removed yet from the selection list.
      // Only itemSelectionsChanged() does that.
      }

//---------------------------------------------------------
//   deselectSelection
//---------------------------------------------------------

void CtrlCanvas::removeSelection(CEvent* e)
{
      for (iCItemList i = selection.begin(); i != selection.end(); ++i) {
            if (*i == e) {
                  selection.erase(i);
                  break;
                  }
            }
}

//---------------------------------------------------------
//   applyYOffset
//---------------------------------------------------------

void CtrlCanvas::applyYOffset(MusECore::Event& e, int yoffset) const
{
  if(!curPart)
    return;

  //fprintf(stderr, "min: %d max: %d bias:%d yoffset: %d cur val: %d \n", _ctrlInfo.min, _ctrlInfo.max, _ctrlInfo.bias, yoffset, e.dataB());
  
  // Y offset is top to bottom. Reverse it.
  int new_v = e.dataB() - yoffset;
  const int min = _ctrlInfo.min + _ctrlInfo.bias;
  const int max = _ctrlInfo.max + _ctrlInfo.bias;
  if(new_v < min)
    new_v = min;
  else if(new_v > max)
    new_v = max;
  e.setB(new_v);
}

//---------------------------------------------------------
//   tagItems
//---------------------------------------------------------

void CtrlCanvas::tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const
{ 
  if(!curPart)
    return;
  
  const bool tagSelected = options._flags & MusECore::TagSelected;
  const bool tagMoving   = options._flags & MusECore::TagMoving;
  const bool tagAllItems = options._flags & MusECore::TagAllItems;
  const bool tagAllParts = options._flags & MusECore::TagAllParts;
  const bool range       = options._flags & MusECore::TagRange;
  const MusECore::Pos& p0 = options._p0;
  const MusECore::Pos& p1 = options._p1;
  
  // Protect against divide by zero.
  const int offset_y = rmapyDev(height() == 0 ? 0 : _curDragOffset.y() * (_ctrlInfo.max - _ctrlInfo.min) / height());

  MusECore::Event new_e;
  CEvent* item;
  MusECore::Part* part;
  if(range)
  {
    if(tagAllItems || tagAllParts)
    {
      for(ciCItemList i = items.cbegin(); i != items.cend(); ++i)
      {
        item = static_cast<CEvent*>(*i);
        part = item->part();
        if(!tagAllParts && (part != curPart || (part && part->track() != curTrack)))
          continue;
        if(!(tagAllItems
             || (tagSelected && item->isSelected())
             || (tagMoving && item->isMoving())))
          continue;
        if(item->isObjectInRange(p0, p1))
        {
          // Grab a clone of the event with a synthesized length
          //  representing the visual length of the 'bar' on the graph.
          // This is a HACK of the Event length for this temporary purpose.
          // Normally the length is always zero for controllers.
          // The caller (xml copy/paste routines etc.) resets this back to zero
          //  upon reception of the event when it is done with the information.
          new_e = item->eventWithLength();
          if(tagMoving && item->isMoving())
            applyYOffset(new_e, offset_y);
          tag_list->add(part, new_e);
        }
      }
    }
    else
    {
      if(tagSelected)
      {
        for(ciCItemList i = selection.cbegin(); i != selection.cend(); ++i)
        {
          item = static_cast<CEvent*>(*i);
          part = item->part();
          if(part != curPart || (part && part->track() != curTrack))
            continue;
          if(item->isObjectInRange(p0, p1))
          {
            new_e = item->eventWithLength();
            tag_list->add(part, new_e);
          }
        }
      }
      
      if(tagMoving)
      {
        for(ciCItemList i = moving.cbegin(); i != moving.cend(); ++i)
        {
          item = static_cast<CEvent*>(*i);
          part = item->part();
          if(part != curPart || (part && part->track() != curTrack))
            continue;
          if(item->isObjectInRange(p0, p1))
          {
            // Avoid duplicates found in moving list.
            if(tagSelected && selection.cfind(item) != selection.cend())
              continue;
            new_e = item->eventWithLength();
            applyYOffset(new_e, offset_y);
            tag_list->add(part, new_e);
          }
        }
      }
    }
  }
  else
  {
    if(tagAllItems || tagAllParts)
    {
      for(ciCItemList i = items.cbegin(); i != items.cend(); ++i)
      {
        item = static_cast<CEvent*>(*i);
        part = item->part();
        if(!tagAllParts && (part != curPart || (part && part->track() != curTrack)))
          continue;
        if(!(tagAllItems
             || (tagSelected && item->isSelected())
             || (tagMoving && item->isMoving())))
          continue;
        new_e = item->eventWithLength();
        if(tagMoving && item->isMoving())
          applyYOffset(new_e, offset_y);
        tag_list->add(part, new_e);
      }
    }
    else
    {
      if(tagSelected)
      {
        for(ciCItemList i = selection.cbegin(); i != selection.cend(); ++i)
        {
          item = static_cast<CEvent*>(*i);
          part = item->part();
          if(part != curPart || (part && part->track() != curTrack))
            continue;
          new_e = item->eventWithLength();
          tag_list->add(part, new_e);
        }
      }
      
      if(tagMoving)
      {
        for(ciCItemList i = moving.cbegin(); i != moving.cend(); ++i)
        {
          item = static_cast<CEvent*>(*i);
          // Avoid duplicates found in selection list.
          if(tagSelected && selection.cfind(item) != selection.cend())
            continue;
          part = item->part();
          if(part != curPart || (part && part->track() != curTrack))
            continue;
          new_e = item->eventWithLength();
          applyYOffset(new_e, offset_y);
          tag_list->add(part, new_e);
        }
      }
    }
  }
}

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void CtrlCanvas::setController(int num)
{
  setMidiController(num);
  updateItems();
}


//---------------------------------------------------------
//   setCurTrackAndPart
//---------------------------------------------------------

bool CtrlCanvas::setCurTrackAndPart()
{
  bool changed = false;
  MusECore::MidiPart* part = 0;
  MusECore::MidiTrack* track = 0;
  
  if(!editor->parts()->empty()) 
  {
    MusECore::Part* pt = editor->curCanvasPart();
    if(pt && pt->track())
    {
      if(pt->track()->isMidiTrack())
      {
        part = (MusECore::MidiPart*)pt;
        track = part->track();
      }  
    }
  }
    
  if(part != curPart)
  {
    curPart = part;
    changed = true;
  }  
  
  if(track != curTrack)
  {
    curTrack = track;
    changed = true;
  }  
  
  return changed;
}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void CtrlCanvas::configChanged()    
{ 
  if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
        setBg(MusEGlobal::config.midiControllerViewBg);
        setBg(QPixmap());
  }
  else {
        setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
  }
  songChanged(SC_CONFIG); 
}

//---------------------------------------------------------
//   songChanged
//    all marked parts are added to the internal event list
//---------------------------------------------------------

void CtrlCanvas::songChanged(MusECore::SongChangedStruct_t type)
{
  if(editor->deleting())  // Ignore while while deleting to prevent crash.
    return; 
  
  if(type & SC_CONFIG)
  {
    setBg(MusEGlobal::config.midiControllerViewBg);
    setFont(MusEGlobal::config.fonts[3]);  
  }
  
  bool changed = false;
  if(type & (SC_CONFIG | SC_PART_MODIFIED | SC_SELECTION))
    changed = setCurTrackAndPart();
            
  // Although changing the instrument/device in the
  //  config window generates a type of -1, we can eliminate
  //  some other useless calls using SC_CONFIG, which was not used 
  //  anywhere else in muse before now, except song header.
  if((type & (SC_CONFIG | SC_MIDI_INSTRUMENT | SC_DRUM_SELECTION | SC_PIANO_SELECTION | SC_DRUMMAP)) ||
     ((type & (SC_PART_MODIFIED | SC_SELECTION)) && changed))
    setMidiController(_cnum);
  
  if(!curPart)         
    return;
              
  if(type & (SC_CONFIG | SC_MIDI_INSTRUMENT | SC_DRUM_SELECTION | SC_PIANO_SELECTION |
     SC_DRUMMAP | SC_PART_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED))
    updateItems();
  else if(type & SC_SELECTION)
  {
    // Prevent race condition: Ignore if the change was ultimately sent by the canvas itself.
    if(type._sender != this)
      updateItemSelections();
  }
}

//---------------------------------------------------------
//   partControllers
//   num is the controller number, in 'MidiController terms' (lo-byte = 0xff for per-note controllers).
//---------------------------------------------------------

void CtrlCanvas::partControllers(const MusECore::MidiPart* part, int num, int* dnum, int* didx,
                                 MusECore::MidiController** mc, MusECore::MidiCtrlValList** mcvl,
                                 CtrlCanvasInfoStruct* ctrlInfo)
{
  if(num == MusECore::CTRL_VELOCITY) // special case
  {    
    if(mcvl)
      *mcvl = &veloList;
    if(mc)
      *mc = &MusECore::veloCtrl;
    if(dnum)
      *dnum = num;
    if(didx)
      *didx = num;
    if(ctrlInfo)
      *ctrlInfo = CtrlCanvasInfoStruct();
  }
  else 
  {
    if(!part)         
    {
      if(mcvl)
        *mcvl = 0;
      if(mc)
        *mc = 0;
      if(dnum)
        *dnum = 0;
      if(didx)
        *didx = 0;
      if(ctrlInfo)
        *ctrlInfo = CtrlCanvasInfoStruct();
      return;
    }
    
    MusECore::MidiTrack* mt = part->track();
    MusECore::MidiPort* mp = nullptr;
    MusECore::MidiController* mp_mc = nullptr;
    int chan;
    int di = 0;
    int n = 0;
    bool is_newdrum_ctl = false;

    if((curDrumPitch >= 0) && ((num & 0xff) == 0xff))
    {
      di = (num & ~0xff) | curDrumPitch;
      if(mt->type() == MusECore::Track::DRUM)
      {
        is_newdrum_ctl = true;
        n = (num & ~0xff) | mt->drummap()[curDrumPitch].anote;
        // Default to track port if -1 and track channel if -1.
        int mport = mt->drummap()[curDrumPitch].port;
        if(mport == -1)
          mport = mt->outPort();
        mp = &MusEGlobal::midiPorts[mport];
        chan = mt->drummap()[curDrumPitch].channel;
        if(chan == -1)
          chan = mt->outChannel();
      }
      else if(mt->type() == MusECore::Track::MIDI) 
      {
        n = di; // Simple one-to-one correspondence. There is no 'mapping' for piano roll midi - yet.
        mp = &MusEGlobal::midiPorts[mt->outPort()];
        chan = mt->outChannel();
      }
    }    
    else
    {
       di = num;
       n = num;
       mp = &MusEGlobal::midiPorts[mt->outPort()];          
       chan = mt->outChannel();
    }
    
    if(mp)
      mp_mc = mp->midiController(n, chan);
    
    if(dnum)
      *dnum = n;
          
    if(didx)
      *didx = di;
      
    if(mc)
      *mc = mp_mc;
    
    if(ctrlInfo)
    {
      int min = 0;
      int max = 127;
      int bias = 0;
      if(n == MusECore::CTRL_PROGRAM)
      {
        min = 1;
        max = 128;
        bias = 0;
      }
      else
      if(mp_mc)
      {
        min  = mp_mc->minVal();
        max  = mp_mc->maxVal();
        bias  = mp_mc->bias();
      }
      
      ctrlInfo->fin_ctrl_num = n;
      ctrlInfo->is_newdrum_ctl = is_newdrum_ctl;
      ctrlInfo->min = min;
      ctrlInfo->max = max;
      ctrlInfo->bias = bias;
    }
      
    if(mcvl)
    {
      MusECore::MidiCtrlValList* tmcvl = 0;
      MusECore::MidiCtrlValListList* cvll = mp->controller();
      for(MusECore::iMidiCtrlValList i = cvll->begin(); i != cvll->end(); ++i) 
      {
        if(i->second->num() == n) 
        {
          tmcvl = i->second;
          break;
        }
      }
      *mcvl = tmcvl;
      
    }    
  }
}

//---------------------------------------------------------
//   itemSelectionsChanged
//---------------------------------------------------------

bool CtrlCanvas::itemSelectionsChanged(MusECore::Undo* operations, bool deselectAll)
{
      MusECore::Undo ops;
      MusECore::Undo* opsp = operations ? operations : &ops;
  
      bool item_selected;
      bool obj_selected;
      bool changed=false;
      
      // If we are deselecting all, globally deselect all events,
      //  and don't bother individually deselecting objects, below.
      if(deselectAll)
      {
        // This is a one-time operation (it has no 'undo' section).
        opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0, true));
        changed = true;
      }
      
      // To save time searching the potentially large 'items' list, a selection list is used.
      for(ciCItemList i = selection.begin(); i != selection.end() ; ) {
            CItem* item = *i;
            item_selected = item->isSelected();
            obj_selected = item->objectIsSelected();

            // Don't bother deselecting objects if we have already deselected all, above.
            if((item_selected || !deselectAll) &&
                ((item_selected != obj_selected) ||
                // Need to force this because after the 'deselect all events' command executes,
                //  if the item is selected another select needs to be executed even though it
                //  appears nothing changed here.
                (item_selected && deselectAll)))
              
              
            // Here we have a choice of whether to allow undoing of selections.
            // Disabled for now, it's too tedious in use. Possibly make the choice user settable.
            opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::SelectEvent,
                                              //item->event(), item->part(), item_selected, obj_selected, false));
                                              item->event(), item->part(), item_selected, obj_selected));
            changed=true;

            // Now it is OK to remove the item from the
            //  selection list if it is unselected.
            if(!item_selected)
              i = selection.erase(i);
            else
              ++i;
      }

      if (!operations && changed)
      {
            // Set the 'sender' to this so that we can ignore self-generated songChanged signals.
            // Here we have a choice of whether to allow undoing of selections.
            if(MusEGlobal::config.selectionsUndoable)
              MusEGlobal::song->applyOperationGroup(ops, MusECore::Song::OperationUndoMode, this);
            else
              MusEGlobal::song->applyOperationGroup(ops, MusECore::Song::OperationExecuteUpdate, this);
            
// For testing...
//               fprintf(stderr, "CtrlCanvas::updateSelection: Applied SelectPart operations, redrawing\n");
      }

      return changed;
}

//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void CtrlCanvas::updateItems()
      {
      selection.clear();
      items.clearDelete();
      moving.clear();
      cancelMouseOps();
      
      if(!editor->parts()->empty())
      {
        CEvent *newev = 0;
  
        for (MusECore::iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p) 
        {
              MusECore::Event last;
              CEvent* lastce  = 0;
              
              MusECore::MidiPart* part = (MusECore::MidiPart*)(p->second);
              
              if (filterTrack && part->track() != curTrack)
                continue;
              
              MusECore::MidiCtrlValList* mcvl;
              partControllers(part, _cnum, 0, 0, 0, &mcvl, 0);
              unsigned len = part->lenTick();

              for (MusECore::ciEvent i = part->events().begin(); i != part->events().end(); ++i) 
              {
                    const MusECore::Event& e = i->second;
                    // Do not add events which are past the end of the part.
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
                    if((int)e.tick() /*+ (int)e.lenTick()*/ < 0)
                      continue;
                    if((int)e.tick() >= (int)len)
                      break;
#else
                    if(e.tick() > len)   
                      break;
#endif
              
                    
                    if(_cnum == MusECore::CTRL_VELOCITY && e.type() == MusECore::Note) 
                    {
                          newev = 0;
                          // Zero note on vel is not allowed now.
                          int vel = e.velo();
                          if(vel == 0)
                          {
                            fprintf(stderr, "CtrlCanvas::updateItems: Warning: Event has zero note on velocity!\n");
                            vel = 1;
                          }
                          if (curDrumPitch == -1 || !_perNoteVeloMode) // and NOT >=0
                                items.add(newev = new CEvent(e, part, vel));
                          else if (e.dataA() == curDrumPitch) //same note. if curDrumPitch==-2, this never is true
                                items.add(newev = new CEvent(e, part, vel));
                          if(newev && e.selected())
                          {
                            newev->setSelected(true);
                            selection.push_back(newev);
                          }
                    }
                    else if (e.type() == MusECore::Controller) 
                    {
                      int ctl = e.dataA();
                      if(part->track() && part->track()->type() == MusECore::Track::DRUM && (_cnum & 0xff) == 0xff)
                      {
                        if(curDrumPitch < 0)
                          continue;
                        // Default to track port if -1 and track channel if -1.
                        int port = part->track()->drummap()[ctl & 0x7f].port;
                        if(port == -1)
                          port = part->track()->outPort();
                        int chan = part->track()->drummap()[ctl & 0x7f].channel;
                        if(chan == -1)
                          chan = part->track()->outChannel();

                        int cur_port = part->track()->drummap()[curDrumPitch].port;
                        if(cur_port == -1)
                          cur_port = part->track()->outPort();
                        int cur_chan = part->track()->drummap()[curDrumPitch].channel;
                        if(cur_chan == -1)
                          cur_chan = part->track()->outChannel();

                        if((port != cur_port) || (chan != cur_chan))
                          continue;
                        ctl = (ctl & ~0xff) | part->track()->drummap()[ctl & 0x7f].anote;
                      }

                      if(ctl == _dnum)
                      {
                          if(mcvl && last.empty()) 
                          {
                                lastce = new CEvent(MusECore::Event(), part, mcvl->value(part->tick()));
                                items.add(lastce);
                          }
                          if (lastce)
                                lastce->setEX(e.tick());
                          lastce = new CEvent(e, part, e.dataB());
                          lastce->setEX(-1);
                          items.add(lastce);
                          if(e.selected())
                          {
                            lastce->setSelected(true);
                            selection.push_back(lastce);
                          }
                          last = e;
                      }
                    }    
              }
        }
      }  
      redraw();
    }

//---------------------------------------------------------
//   updateItemSelections
//---------------------------------------------------------

void CtrlCanvas::updateItemSelections()
      {
      selection.clear();
      cancelMouseOps();
      
      bool obj_selected;
      for(ciCItemList i = items.begin(); i != items.end(); ++i) {
            // To save time searching the potentially large 'items' list, a selection list is used.
            CItem* item = *i;
            obj_selected = item->objectIsSelected();
            item->setSelected(obj_selected);
            if(obj_selected)
              selection.push_back(item);
      }
      redraw();
}

//---------------------------------------------------------
//   startMoving
//    copy selection-List to moving-List
//---------------------------------------------------------

void CtrlCanvas::startMoving(const QPoint& pos, int dir, bool rasterize)
      {
      CItem* first_item = nullptr;
      for (iCItemList i = items.begin(); i != items.end(); ++i) {
            CItem* item = *i;
            if (item->isSelected() && item->part() == curPart) {
                  if(!item->isMoving())
                  {
                    item->setMoving(true);
                    moving.add(item);
                  }
                  
                  // Find the item with the lowest event time (the 'first' item by time).
                  if(!first_item || item->event().tick() < first_item->event().tick())
                    first_item = item;
                  }
            }
            
      _dragFirstXPos = 0;
      if(first_item)
      {
        const MusECore::Part* part = first_item->part();
        if(part)
        {
          const MusECore::Event& ev = first_item->event();
          _dragFirstXPos = !ev.empty() ? ev.tick() + part->tick() : 0;
        }
      }
            
      moveItems(pos, dir, rasterize);
      }

//---------------------------------------------------------
//   moveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void CtrlCanvas::moveItems(const QPoint& pos, int dir, bool rasterize)
      {
      if(!curPart)
        return;
      
      int dist_x = pos.x() - start.x();
      int dist_y = pos.y() - start.y();

      if (dir == 1)
            dist_y = 0;
      else if (dir == 2)
            dist_x = 0;

      int dx = _mouseDist.x() + dist_x;
      int dy = _mouseDist.y() + dist_y;
      
      if(dir != 2)
      {
        int tick = _dragFirstXPos + dx;
        if(tick < 0)
          tick = 0;
        if(rasterize)
          tick = editor->rasterVal(tick);
        dx = tick - _dragFirstXPos;
      }
      
      unsigned int first_pos_offset;
      if(_dragFirstXPos > curPart->posValue())
        first_pos_offset = _dragFirstXPos - curPart->posValue();
      else
        first_pos_offset = curPart->posValue();
      
      // Limit the left movement.
      if(dx < 0 && (unsigned int)-dx > first_pos_offset)
      {
        dx = -first_pos_offset;
        // Reset the accumulating mouse distance.
        _mouseDist.setX(-first_pos_offset);
      }
        
      // Limit the up/down movement.
      if(start.y() + dy < 0)
      {
        dy = -start.y();
        _mouseDist.setY(-start.y());
      }
      else if(/*start.y() +*/ dy  >= height())
      {
        dy = height() /*- start.y()*/ - 1;
        _mouseDist.setY(height() /*- start.y()*/ - 1);
      }
      
      _curDragOffset = QPoint(dx, dy);

// For testing...
//       fprintf(stderr, "_mouseDist x:%d y:%d dist_x:%d dist_y:%d _curDragOffset x:%d y:%d"
//         " _dragFirstXPos:%u first_pos_offset:%d\n",
//               _mouseDist.x(), _mouseDist.y(), dist_x, dist_y,
//               _curDragOffset.x(), _curDragOffset.y(), _dragFirstXPos,
//               first_pos_offset);
      
      redraw();
      }

void CtrlCanvas::endMoveItems()
{
  if(!curPart)
    return;
  
  // Limit the paste position at zero.
  unsigned int pos = 0;
  if(_curDragOffset.x() > 0 || (_dragFirstXPos > ((unsigned int) -_curDragOffset.x())))
    pos = _dragFirstXPos + _curDragOffset.x();
  
  // Tag only moving items, regardless of selection (avoid the redundant
  //  search in the selection list).
  MusECore::TagEventList tag_list;
  tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagMoving));
  
  // HACK tagItems() returns a list of cloned events with the lengths set to the visual lengths.
  //      We only use it for temporary things like copy/paste.
  //      The lengths will be reset to zero after when done, in the paste_items_at() function.
  //      Normally an event's length is ALWAYS zero for all controller events.
  
  // Whee ! Now paste the items directly from the list. No Xml conversion required.
  MusECore::paste_items_at(
    // Part list to search for given parts.
    //part_to_set(curPart),
    std::set<const MusECore::Part*>(),
    // List of events to paste.
    &tag_list,
    // The paste position.
    MusECore::Pos(pos, true),  // true = ticks.
    // Max distance before new part created.
    3072,
    // Pasting options including whether to cut, how to erase existing target events,
    //  and whether to create a new part.
    MusECore::FunctionOptionsStruct(
      (_dragType == MOVE_MOVE ? MusECore::FunctionCutItems : MusECore::FunctionNoOptions)
      | (MusEGlobal::config.midiCtrlGraphMergeErase ? MusECore::FunctionEraseItems : MusECore::FunctionNoOptions)
      | (MusEGlobal::config.midiCtrlGraphMergeEraseWysiwyg ? MusECore::FunctionEraseItemsWysiwyg : MusECore::FunctionNoOptions)
      | (MusEGlobal::config.midiCtrlGraphMergeEraseInclusive ? MusECore::FunctionEraseItemsInclusive : MusECore::FunctionNoOptions)
      | (MusECore::FunctionPasteNeverNewPart)
      ),
    // Paste into this part instead of the original part(s).
    //nullptr,
    curPart,
    // Number of copies to paste.
    1,
    // Separation between copies.
    3072,
    // Choose which events to paste.
    MusECore::ControllersRelevant,
    // If pasting controllers, paste into this controller number if not -1.
    // If the source has multiple controllers, user will be asked which one to paste.
    _cnum
    );
  
  // Be sure to clear the items' moving flag and the moving list!
  if(!moving.empty())
  {
    for(iCItemList i = moving.begin(); i != moving.end(); ++i)
      (*i)->setMoving(false);
    moving.clear();
  }
  
  if(drag != DRAG_OFF)
    drag = DRAG_OFF;
 
  _curDragOffset = QPoint(0, 0);
  _mouseDist = QPoint(0, 0);
  
  redraw();
}

void CtrlCanvas::setCursor()
{
    showCursor();
    switch (drag) {
    case DRAGX_MOVE:
    case DRAGX_COPY:
        View::setCursor(QCursor(Qt::SizeHorCursor));
        break;

    case DRAGY_MOVE:
    case DRAGY_COPY:
        View::setCursor(QCursor(Qt::SizeVerCursor));
        break;

    case DRAG_MOVE:
    case DRAG_COPY:
        // Bug in KDE cursor theme? On some distros this cursor is actually another version of a closed hand! From 'net:
        // "It might be a problem in the distribution as Qt uses the cursor that is provided by X.org/xcursor extension with name "size_all".
        //  We fixed this issue by setting the KDE cursor theme to "System theme" "
        View::setCursor(QCursor(Qt::SizeAllCursor));
        break;

    case DRAG_RESIZE:
        View::setCursor(QCursor(Qt::SizeHorCursor));
        break;

    case DRAG_PAN:
        if(MusEGlobal::config.borderlessMouse)
            showCursor(false); // CAUTION
        else
            View::setCursor(QCursor(Qt::ClosedHandCursor));
        break;

    case DRAG_ZOOM:
        if(MusEGlobal::config.borderlessMouse)
            showCursor(false); // CAUTION
        break;

    case DRAG_DELETE:
    case DRAG_COPY_START:
    case DRAG_MOVE_START:
    case DRAG_NEW:
    case DRAG_LASSO_START:
    case DRAG_LASSO:
    case DRAG_OFF:
        switch(tool) {
        case PencilTool:
            QWidget::setCursor(*pencilCursor);
//             QWidget::setCursor(*magnetCursor);
            break;
        case RubberTool:
            QWidget::setCursor(*deleteCursor);
            break;
        case GlueTool:
            QWidget::setCursor(QCursor(Qt::ForbiddenCursor));
            break;
        case CutTool:
            QWidget::setCursor(QCursor(Qt::ForbiddenCursor));
            break;
        case MuteTool:
            QWidget::setCursor(QCursor(Qt::ForbiddenCursor));
            break;
        case DrawTool:
            QWidget::setCursor(*drawCursor);
            break;
        case PanTool:
            QWidget::setCursor(QCursor(Qt::ForbiddenCursor));
            break;
        case ZoomTool:
            QWidget::setCursor(QCursor(Qt::ForbiddenCursor));
            break;
        default:
            if(moving.empty())
            {
                View::setCursor(QCursor(Qt::ArrowCursor));
            }
            else
            {
                if(_movingItemUnderCursor)
                {
                    View::setCursor(QCursor(Qt::SizeAllCursor));
                }
                else
                {
                    if(_dragType == MOVE_MOVE)
                        View::setCursor(*editpasteSCursor);
                    else
                        View::setCursor(*editpasteCloneSCursor);
                }
            }
            break;
        }
        break;
    }
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void CtrlCanvas::viewMousePressEvent(QMouseEvent* event)
      {
// For testing...
//       fprintf(stderr, "CtrlCanvas::viewMousePressEvent\n");
        
      if(!_controller || curDrumPitch==-2)
      {
        cancelMouseOps();
        return;
      }
        
      Qt::MouseButton button = event->button();
      
      // Ignore event if (another) button is already active:
      if (event->buttons() ^ button) {
            return;
            }

      setMouseGrab(false);
      
      start = event->pos();
      MusEGui::Tool activeTool = tool;
      
      bool ctrlKey = event->modifiers() & Qt::ControlModifier;
      int xpos = start.x();
      if(xpos < 0)
        xpos = 0;
      int ypos = start.y();

      MusECore::MidiController::ControllerType type = MusECore::midiControllerType(_controller->num());
      const bool velo = type == MusECore::MidiController::Velo;
      
      _operations.clear();

      const int tickstep = rmapxDev(1);
      curItem = findCurrentItem(start, tickstep, height());
      
      if (button == Qt::RightButton)
      {
        // No stay-open for now. 
        // TODO How to write a click handler to support stay-open for
        //       the 'options' items but not the 'actions' items ?
        PopupMenu* itemPopupMenu = new PopupMenu(this, false);
        populateMergeOptions(itemPopupMenu);
        itemPopupMenu->setToolTipsVisible(true);
        QAction *act = itemPopupMenu->exec(event->globalPosition().toPoint());
        int idx = -1;
        bool is_checked = false;
        if(act && act->data().isValid())
        {
          idx = act->data().toInt();
          is_checked = act->isChecked();
        }
        delete itemPopupMenu;
        switch(idx)
        {
          case ContextIdCancelDrag:
            cancelMouseOps();
          break;
          
          case ContextIdMerge:
              if(!moving.empty())
              {
                // Force the drag type before merge.
                _dragType = MOVE_MOVE;
                // Merge the moving items...
                endMoveItems();
                setCursor();
                // Return. The user must click again to select anything.
                return;
              }
          break;
          
          case ContextIdMergeCopy:
              if(!moving.empty())
              {
                // Force the drag type before merge.
                _dragType = MOVE_COPY;
                // Merge the moving items...
                endMoveItems();
                setCursor();
                // Return. The user must click again to select anything.
                return;
              }
          break;
          
          case ContextIdErase:
            MusEGlobal::config.midiCtrlGraphMergeErase = is_checked;
          break;
          
          case ContextIdEraseWysiwyg:
            MusEGlobal::config.midiCtrlGraphMergeEraseWysiwyg = is_checked;
          break;
          
          case ContextIdEraseInclusive:
            MusEGlobal::config.midiCtrlGraphMergeEraseInclusive = is_checked;
          break;
          
          default:
          break;
        }
      }
      else if (button == Qt::LeftButton)
      {
        // If selecting controller empty space or a new item,
        //  we must first merge whatever items may have been
        //  moved before anything further is done.
        if(!velo && (!curItem || !curItem->isSelected() || !curItem->isMoving()))
        {
          if(!moving.empty())
          {
            // Merge the moving items...
            endMoveItems();
            
            setCursor();
            // Return. The user must click again to select anything.
            return;
          }
        }
        
        switch (activeTool) {
              case MusEGui::PointerTool:
                    if(curPart)      
                    {
                          if (!velo && curItem && (curItem->isMoving() || curItem->isSelected())) {
                                // If the current item is not already moving, then this
                                //  is the grand start of a move. Here we must reset
                                //  these 'accumulating' variables!
                                if(!curItem->isMoving())
                                {
                                  clearMoving();
                                }
                            
                                // Alt alone is usually reserved for moving a window in X11. Ignore shift + alt.
                                if (ctrlKey)
                                      drag = DRAG_COPY_START;
                                else
                                      drag = DRAG_MOVE_START;
                                }
                          else
                                drag = DRAG_LASSO_START;
                      
                          setCursor();
                          setMouseGrab(true); // CAUTION
                    }
                    
                    
                    break;

            case MusEGui::PencilTool:
                    if (!ctrlKey && !velo) {
                                drag = DRAG_NEW;
                                newVal(xpos, ypos);
                          }
                    else {
                          drag = DRAG_RESIZE;
                          changeVal(xpos, xpos, ypos);
                          }
//                    setCursor();
                    break;

              case MusEGui::RubberTool:
                    if (!velo) {
                          drag = DRAG_DELETE;
                          deleteVal(xpos, xpos, ypos);
                          }
                    setCursor();
                    break;

              case MusEGui::DrawTool:
                    if (drawLineMode) {
                          line2x = xpos;
                          line2y = ypos;
                          if (!ctrlKey && !velo)
                                newValRamp(line1x, line1y, line2x, line2y);
                          else
                                changeValRamp(line1x, line1y, line2x, line2y);
                          drawLineMode = false;
                          }
                    else {
                          line2x = line1x = xpos;
                          line2y = line1y = ypos;
                          drawLineMode = true;
                          }
                    redraw();
                    break;

              default:
                    break;
              }
           }
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseMoveEvent(QMouseEvent* event)
      {
      _movingItemUnderCursor = nullptr;

      if(!_controller || curDrumPitch==-2)
      {
        // Be sure to cancel any relative stuff. Otherwise it's left in a bad state.
        cancelMouseOps();
        return;
      }
        
      // Drag not off and left mouse button not pressed? It's an error.
      // Meaning likely mouseRelease was not called (which CAN happen).
      if(drag != DRAG_OFF && !(event->buttons() & Qt::LeftButton))
      {
// For testing...
//         fprintf(stderr, "CtrlCanvas::viewMouseMoveEvent: calling cancelMouseOps()\n");
        
        // Be sure to cancel any relative stuff. Otherwise it's left in a bad state.
        cancelMouseOps();
      }
      
      Qt::KeyboardModifiers modifiers = event->modifiers();
      bool shift = modifiers & Qt::ShiftModifier;
      
      QPoint pos  = event->pos();
      const QPoint dist = pos - start;
      const int ax = ABS(dist.x());
      const int ay = ABS(dist.y());
      const bool is_moving = (ax >= 2) || (ay > 2);

      switch (drag) {
            case DRAG_LASSO_START:
                  if (!is_moving)
                    break;
                  drag = DRAG_LASSO;
                  setCursor();
                  // fallthrough
            case DRAG_LASSO:
                  lasso.setRect(start.x(), start.y(), dist.x(), dist.y());
                  redraw();
                  break;
            case DRAG_RESIZE:
                  changeVal(start.x(), pos.x(), pos.y());
                  start = pos;
                  break;

            case DRAG_MOVE_START:
            case DRAG_COPY_START:
            {
                  if (!is_moving)
                        break;

                  int dir = 0;
                  if (shift) {
                        if (ax > ay) {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGX_MOVE;
                              else
                                    drag = DRAGX_COPY;
                              dir = 1;
                              }
                        else {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAGY_MOVE;
                              else
                                    drag = DRAGY_COPY;
                              dir = 2;
                              }
                        }
                  else
                  {
                        if (drag == DRAG_MOVE_START)
                              drag = DRAG_MOVE;
                        else
                              drag = DRAG_COPY;
                  }
                  setCursor();
                  if (curItem && !curItem->isSelected()) {
                        if (drag == DRAG_MOVE)
                              deselectAll();
                        selectItem(curItem);
                        itemSelectionsChanged(nullptr, drag == DRAG_MOVE);
                        redraw();
                        }
                        
                  startMoving(pos, dir, !shift);
                  break;
            }

            case DRAG_MOVE:
            case DRAG_COPY:
                  // TODO
//                   if(!scrollTimer)
                    moveItems(pos, 0, !shift);
                  break;

            case DRAGX_MOVE:
            case DRAGX_COPY:
                  // TODO
//                   if(!scrollTimer)
//                     moveItems(pos, 1, false);
                    moveItems(pos, 1, !shift);
                  break;

            case DRAGY_MOVE:
            case DRAGY_COPY:
                  // TODO
//                   if(!scrollTimer)
//                     moveItems(pos, 2, false);
                    moveItems(pos, 2, !shift);
                  break;

            case DRAG_NEW:
                  newVal(start.x(), start.y(), pos.x(), pos.y());
                  start = pos;
                  break;

            case DRAG_DELETE:
                  deleteVal(start.x(), pos.x(), pos.y());
                  start = pos;
                  break;

            case DRAG_OFF:
            {
              _movingItemUnderCursor = nullptr;
              const int tickstep = rmapxDev(1);
              for(iCItemList i = moving.begin(); i != moving.end(); ++i)
              {
                CEvent* item = static_cast<CEvent*>(*i);
                if(item->part() != curPart)
                  continue;
                // Be sure to subtract the drag offset from the given point.
                if(item->containsPoint(_controller, pos - _curDragOffset, tickstep, height()))
                {
                  _movingItemUnderCursor = item;
                  break;
                }
              }
              
              // Set the cursor but only if items are moving.
              if(!moving.empty())
                setCursor();
            }
            break;

            default:
                  break;
            }
      if(pos.x() < 0)
        pos.setX(0);
      if (tool == MusEGui::DrawTool && drawLineMode) {
            line2x = pos.x();
            line2y = pos.y();
            redraw();
            }
      emit xposChanged(editor->rasterVal(pos.x()));
      
      
      int val = computeVal(_controller, pos.y(), height());
      emit yposChanged(val);

      //_mouseDist = event->pos();

      if (MusEGlobal::config.showNoteTooltips)
      {
          const QPoint p = event->globalPosition().toPoint();
          QToolTip::showText(QPoint(p.x(), p.y() + 20), tr("Value: ") + QString::number(val));
      }
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseReleaseEvent(QMouseEvent* event)
      {
// For testing...
//       fprintf(stderr, "CtrlCanvas::viewMouseReleaseEvent\n");
      
      // We want only the left mouse release events. Ignore anything else.
      // Do nothing, even if the mouse is grabbed or we have a moving list.
      if(event->button() != Qt::LeftButton) 
        return;

      // Immediately cancel any mouse grabbing.
      // Because for example there are a few message boxes that may appear
      //  in the subsequent code, and the mouse will not work in them if it
      //  is still grabbed.
      setMouseGrab(false);

      const QPoint pos = event->pos();
      Qt::KeyboardModifiers modifiers = event->modifiers();
      bool ctrlKey = modifiers & Qt::ControlModifier;
      const int xpos = start.x();
      const int ypos = start.y();
      const int tickstep = rmapxDev(1);

      // Now that the mouse is up we must contribute the
      //  distance moved to the 'accumulating' variable.
      _mouseDist += (pos - start);

      switch (drag) {
            case DRAG_MOVE_START:
            case DRAG_COPY_START:
                  // Don't change anything if the current item is moving.
                  if(!curItem || !curItem->isMoving())
                  {
                    if (!ctrlKey)
                          deselectAll();
                    if(curItem)
                    {
                      if (ctrlKey && curItem->isSelected())
                        deselectItem(curItem);
                      else
                        selectItem(curItem);
                    }

                    itemSelectionsChanged(nullptr, !ctrlKey);
                    redraw();
                  }
                  
                  break;
                  
            case DRAG_COPY:
            case DRAGX_COPY:
            case DRAGY_COPY:
            case DRAG_MOVE:
            case DRAGX_MOVE:
            case DRAGY_MOVE:
                  break;

            case DRAG_LASSO_START:
            {
                  lasso = QRect(xpos, ypos, tickstep, rmapyDev(1));
            }
            //fallthrough
            case DRAG_LASSO:
            {
                  // Don't change anything if the current item is moving.
                  if(!curItem || !curItem->isMoving())
                  {
                    if (!ctrlKey)
                    {
                      deselectAll();
                    }
                    
                    if(_controller)  
                    {
                      lasso = lasso.normalized();
                      int h = height();
                      CEvent* item;
                      for (iCItemList i = items.begin(); i != items.end(); ++i) {
                            item = static_cast<CEvent*>(*i);
                            if(item->part() != curPart)
                              continue;
                            
                            if (item->intersectsController(_controller, lasso, tickstep, h)) {
                            // If the lasso is empty treat it as a single click.
                                  if (ctrlKey && item->isSelected())
                                    deselectItem(item);
                                  else
                                    selectItem(item);
                                }  
                            }
                      drag = DRAG_OFF;
                      itemSelectionsChanged(nullptr, !ctrlKey);
                    }
                    
                    redraw();
                  }
            }
            break;

            case DRAG_RESIZE:
            case DRAG_NEW:
            case DRAG_DELETE:
            // Also in case ramp was drawn, check if any operations are required.
            default:
                  // Set the 'sender' to this so that we can ignore self-generated songChanged signals.
                  MusEGlobal::song->applyOperationGroup(_operations, MusECore::Song::OperationUndoMode, this);
                  break;
            }

      // Done with any operations. Clear the list.
      _operations.clear();

      drag = DRAG_OFF;
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void CtrlCanvas::wheelEvent(QWheelEvent* ev)
{
  emit redirectWheelEvent(ev);
}

//---------------------------------------------------------
//   newValRamp
//---------------------------------------------------------

void CtrlCanvas::newValRamp(int x1, int y1, int x2, int y2)
      {
      if(!curPart || !_controller)         
        return;
      
      if(x1 < 0)
        x1 = 0;
      if(x2 < 0)
        x2 = 0;
      
      if(x2 - x1 < 0)
      {
        int a = x1;
        x1 = x2;
        x2 = a;
        a = y1;
        y1 = y2;
        y2 = a;
      }
      
      int xx1 = editor->rasterVal1(x1);
      int xx2 = editor->rasterVal2(x2);
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);
      
      int type = _controller->num();

      bool useRaster = false;
      int raster = editor->raster();
      if (raster == 1)          // set reasonable raster
      {
        raster = MusEGlobal::config.division/16;  // Let's use 64th notes, for a bit finer resolution. Tim.
        useRaster = true;
      }  

      // delete existing events

      unsigned curPartTick = curPart->tick();
      int lastpv = MusECore::CTRL_VAL_UNKNOWN;
      for (ciCItemList i = items.begin(); i != items.end(); ++i) {
            CItem* ev = *i;
            if (ev->part() != curPart)
              continue;
            MusECore::Event event = ev->event();
            if (event.empty())
                  continue;
            int x = event.tick() + curPartTick;
            if (x < xx1)
                  continue;
            if (x >= xx2)
                  break;
            // Do port controller values and clone parts. 
            _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent, event, curPart, true, true));
            }

      if (ctrl)  
        lastpv = ctrl->hwVal();
        
      unsigned curPartLen = curPart->lenTick();
      
      // insert new events
      for (int x = xx1, step; x < xx2 ; x += step )    
      {
            step = useRaster ? raster : editor->rasterVal2(x + 1) - x;
            
            int y    = x + step >= xx2 || x2 == x1 ? y2 : (x == xx1 ? y1 : (((y2 - y1) * (x + step/2 - x1)) / (x2 - x1)) + y1);
            int nval = computeVal(_controller, y, height());
            
            unsigned tick = x - curPartTick;
            // Do not add events which are past the end of the part.
            if (tick >= curPartLen)
              break;
            MusECore::Event event(MusECore::Controller);
            event.setTick(tick);
            event.setA(_didx);
            if (type == MusECore::CTRL_PROGRAM)
            {
              if (lastpv == MusECore::CTRL_VAL_UNKNOWN)
                event.setB(nval - 1);
              else  
                event.setB((lastpv & 0xffff00) | (nval - 1));
            }
            else  
              event.setB(nval);
            // Do port controller values and clone parts. 
            _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, event, curPart, true, true));
            }
      }

//---------------------------------------------------------
//   changeValRamp
//---------------------------------------------------------

void CtrlCanvas::changeValRamp(int x1, int y1, int x2, int y2)
      {
      if(!curPart || !_controller)
        return;
      
      int h   = height();
      int type = _controller->num();

      for (ciCItemList i = items.begin(); i != items.end(); ++i) {
            CEvent* ev = static_cast<CEvent*>(*i);
            if (ev->containsXRange(x1, x2)) {
                  if (ev->part() != curPart)
                    continue;

                  MusECore::Event event = ev->event();
                  if (event.empty())
                        continue;

                  int x    = event.tick() + curPart->tick();
                  int y    = (x2==x1) ? y1 : (((y2-y1)*(x-x1))/(x2-x1))+y1;
                  int nval = computeVal(_controller, y, h);
                  if (type == MusECore::CTRL_PROGRAM)
                  {
                    if (event.dataB() == MusECore::CTRL_VAL_UNKNOWN)
                      --nval;
                    else  
                      nval = (event.dataB() & 0xffff00) | (nval - 1);
                  }
                    
                  ev->setVal(nval);
                  
                  if (type == MusECore::CTRL_VELOCITY) {
                        if(nval > 127) nval = 127;
                        else if(nval <= 0) nval = 1;  // Zero note on vel is not allowed.
                    
                        if ((event.velo() != nval)) {
                              MusECore::Event newEvent = event.clone();
                              newEvent.setVelo(nval);
                              // Do not do port controller values and clone parts. 
                              _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, curPart, false, false));
                              }
                        }
                  else {
                        if (!event.empty()) {
                              if ((event.dataB() != nval)) {
                                    MusECore::Event newEvent = event.clone();
                                    newEvent.setB(nval);
                                    // Do port controller values and clone parts. 
                                    _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent, newEvent, event, curPart, true, true));
                                    }
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   changeVal
//---------------------------------------------------------

void CtrlCanvas::changeVal(int x1, int x2, int y)
      {
      if(!curPart || !_controller)         
        return;
      
      bool changed = false;
      int newval = computeVal(_controller, y, height());
      int type = _controller->num();

      for (ciCItemList i = items.begin(); i != items.end(); ++i) {
            CEvent* ev = static_cast<CEvent*>(*i);
            if (!ev->containsXRange(x1, x2))
                  continue;
            if(ev->part() != curPart)
              continue;
            MusECore::Event event      = ev->event();

            if (type == MusECore::CTRL_VELOCITY) {
                  if(newval > 127) newval = 127;
                  else if(newval <= 0) newval = 1;  // Zero note on vel is not allowed.
                  
                  if ((event.velo() != newval)) {
                        ev->setVal(newval);
                        MusECore::Event newEvent = event.clone();
                        newEvent.setVelo(newval);
                        // Indicate do not do port controller values and clone parts.
                        _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,
                                          newEvent, event, curPart, false, false));
                        changed = true;
                        }
                  }
            else {
                  if (!event.empty()) {
                        int nval = newval;
                        if(type == MusECore::CTRL_PROGRAM)
                        {
                          if(event.dataB() == MusECore::CTRL_VAL_UNKNOWN)
                            --nval;
                          else  
                            nval = (event.dataB() & 0xffff00) | (nval - 1);
                        }
                        ev->setVal(nval);
                        
                        if ((event.dataB() != nval)) {
                              MusECore::Event newEvent = event.clone();
                              newEvent.setB(nval);
                              // Indicate do port controller values and clone parts.
                              _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,
                                                newEvent, event, curPart, true, true));
                              changed = true;
                              }
                        }
                  }
            }
      if (changed)
            redraw();
      }

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int y)
      {
      if(!curPart || !_controller)         
        return;
      
      if(x1 < 0)
        x1 = 0;
      int xx1  = editor->rasterVal1(x1);
      int xx2  = editor->rasterVal2(x1);
      // If x1 happens to lie directly on a raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x1 + 1);
        
      int newval = computeVal(_controller, y, height());
      int type = _controller->num();

      bool found        = false;
      bool do_redraw = false;
      iCItemList ice_tmp;
      iCItemList prev_ev = items.end();     // End is OK with multi-part, this is just an end 'invalid' marker at first.
      iCItemList insertPoint = items.end(); // Similar case here. 
      bool curPartFound = false;

      int lastpv = MusECore::CTRL_VAL_UNKNOWN;
      if(ctrl)
        lastpv = ctrl->hwVal();
        
      int partTick = curPart->tick();
      for (iCItemList i = items.begin(); i != items.end() ; ) 
      {
            CEvent* ev = static_cast<CEvent*>(*i);
            if(ev->part() != curPart)
            {
              if(curPartFound)
              {
                // Make sure insertPoint doesn't lie beyond next part's first event.
                if(insertPoint == items.end())
                  insertPoint = i;
                // Speed-up: If the current part was already processed, we are done.
                break;
              }
              ++i;
              continue;
            }  
            curPartFound = true;
            
            MusECore::Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int ax = event.tick() + partTick;
            
            if (ax < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            if (ax >= xx2)
            {
              insertPoint = i;
              break;
            }
            
            int nval = newval;
            if(type == MusECore::CTRL_PROGRAM)
            {
              if(event.dataB() == MusECore::CTRL_VAL_UNKNOWN)
              {
                if(lastpv == MusECore::CTRL_VAL_UNKNOWN)
                  --nval;
                else  
                  nval = (lastpv & 0xffff00) | (nval - 1);
              }
              else  
                nval = (event.dataB() & 0xffff00) | (nval - 1);
            }
              
            if (ax == xx1) 
            {
              // change event
              found = true;
              ev->setVal(nval);
              if ((event.dataB() != nval)) 
              {
                MusECore::Event newEvent = event.clone();
                newEvent.setB(nval);
                
// For testing...
//                 fprintf(stderr, "CtrlCanvas::newVal: Found. Pushing ModifyEvent\n");
                
                // Indicate do port controller values and clone parts.
                _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyEvent,
                                  newEvent, event, curPart, true, true));
              
                // Now that the modify command has been sent, 'event' will stay alive
                //  (its reference count stays alive) in the operation/undo lists etc,
                //  so it is now safe to overwrite the canvas item's event.
                // This is important so that successive modify commands use newEvent as the 'old event',
                //  otherwise the undo/operation system will complain about 'double deletes' etc.
                // The undo/operation system then optimizes the entire sequence down to just one modify command.
                ev->setEvent(newEvent);
                
                do_redraw = true;      
              }
              prev_ev = i;
              ++i;
            }
            else if (ax < xx2) 
            {
                  // We must remove the item from the selected list.
                  removeSelection(ev);
                  // Indicate do port controller values and clone parts.
                  
// For testing...
//                   fprintf(stderr, "CtrlCanvas::newVal: Found. Pushing DeleteEvent\n");
                
                  _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                             event, curPart, true, true));
                  
                  // Now that the delete command has been sent, 'event' will stay alive
                  //  (its reference count stays alive) in the operation/undo lists etc,
                  //  so it is now safe to delete the canvas item and its event.
                  delete ev;
                  
                  // Erase the item from the item list and get the next item.
                  i = items.erase(i);
                  ev = static_cast<CEvent*>(*i);
                  
                  // Is there a previous item?
                  if(prev_ev != items.end())
                  {
                    // Have we reached the end of the list, or the end of the current part's items within the item list?
                    // Then it's current part's last drawn item. EX is 'infinity'. (As far as the part knows. Another part may overlay later.) 
                    // Else EX is current item's tick. (By now, ((*i)->event() should be valid - it must be not empty to call tick()).
                    static_cast<CEvent*>(*prev_ev)->setEX(i == items.end() || ev->part() != curPart ? -1 : ev->event().tick()); 
                  }
                  
                  do_redraw = true;    // Let songChanged handle the redraw upon SC_SELECTION.
                  
                  prev_ev = i;
            }
      }
      
      if (!found) 
      {
            // new event
            int tick = xx1 - curPart->tick();
            // Do not add events which are past the end of the part.
            if((unsigned)tick < curPart->lenTick())
            {
              MusECore::Event event(MusECore::Controller);
              event.setTick(tick);
              event.setA(_didx);
              if(type == MusECore::CTRL_PROGRAM)
              {
                if(lastpv == MusECore::CTRL_VAL_UNKNOWN)
                  event.setB(newval - 1);
                else    
                  event.setB((lastpv & 0xffff00) | (newval - 1));
              }
              else  
                event.setB(newval);
              
              // Indicate do port controller values and clone parts. 
              
// For testing...
//               fprintf(stderr, "CtrlCanvas::newVal: Not found. Pushing AddEvent\n");
                
              _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, 
                                event, curPart, true, true));
              
              CEvent* newev = new CEvent(event, curPart, event.dataB());
              insertPoint = items.insert(insertPoint, newev);
              //selectItem(newev); // TODO: There are advantages, but do we really want new items to be selected?
              
              if(insertPoint != items.begin())
              {
                ice_tmp = insertPoint;
                --ice_tmp;
                static_cast<CEvent*>(*ice_tmp)->setEX(tick);
              }  
              ice_tmp = insertPoint;
              ++ice_tmp;
              static_cast<CEvent*>(*insertPoint)->setEX(ice_tmp == items.end() || 
                                    (*ice_tmp)->part() != curPart ? 
                                    -1 : (*ice_tmp)->event().tick());
              do_redraw = true;              
            }  
      }
      
      if(do_redraw)                 // Let songChanged handle the redraw upon SC_SELECTION.
        redraw();
      }

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int y1, int x2, int y2)
      {
      if(!curPart || !_controller)         
        return;
      
      if(x1 < 0)
        x1 = 0;
      if(x2 < 0)
        x2 = 0;
      
      if(x2 - x1 < 0)
      {
        int a = x1;
        x1 = x2;
        x2 = a;
        a = y1;
        y1 = y2;
        y2 = a;
      }
      
      // I wanted to avoid redundantly drawing the first x position since it is already done via mouseDown.
      // But with the raster 'off' there were some problems not catching some items, and jagged drawing.
      // Possibly there are slight differences between rasterVal1() and rasterVal2() - not reciprocals of each other? 
      // Also the loops below were getting complicated to ignore that first position.
      // So I draw from the first x. (TODO The idea may work now since I wrote this - more work was done.) Tim.
      
      int xx1 = editor->rasterVal1(x1);
      
      int xx2 = editor->rasterVal2(x2);
      
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);
      
      int type = _controller->num();

      bool useRaster = false;
      int raster = editor->raster();
      if (raster == 1)          // set reasonable raster
      {
        raster = MusEGlobal::config.division/16;  // Let's use 64th notes, for a bit finer resolution. Tim.
        useRaster = true;
      }  

      bool do_redraw = false;
      
      // delete existing events

      iCItemList prev_ev = items.end();     // End is OK with multi-part, this is just an end 'invalid' marker at first.
      iCItemList insertPoint = items.end(); // Similar case here. 
      iCItemList ice_tmp;
      bool curPartFound = false;
      int lastpv = MusECore::CTRL_VAL_UNKNOWN;
      int partTick = curPart->tick();
      for (iCItemList i = items.begin(); i != items.end() ; ) 
      {
            CEvent* ev = static_cast<CEvent*>(*i);
            if(ev->part() != curPart)
            {
              if(curPartFound)
              {
                // Make sure insertPoint doesn't lie beyond next part's first event.
                if(insertPoint == items.end())
                  insertPoint = i;
                // Speed-up: If the current part was already processed, we are done.
                break;
              }
              ++i;
              continue;
            }  
            curPartFound = true;
            
            MusECore::Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int x = event.tick() + partTick;
            
            if (x < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            if (x >= xx2)
            {
              insertPoint = i;
              break;
            }
            
            // We must remove the item from the selected list.
            removeSelection(ev);
            // Indicate do port controller values and clone parts.
            
// For testing...
//             fprintf(stderr, "CtrlCanvas::newVal 2: Pushing DeleteEvent\n");
                
            _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                             event, curPart, true, true));
            
            delete (ev);
            i = items.erase(i);           
            ev = static_cast<CEvent*>(*i);
            // Is there a previous item?
            if(prev_ev != items.end())
            {
              // Have we reached the end of the list, or the end of the current part's items within the item list?
              // Then it's current part's last drawn item. EX is 'infinity'. (As far as the part knows. Another part may overlay later.) 
              // Else EX is current item's tick. (By now, ((*i)->event() should be valid - it must be not empty to call tick()).
              static_cast<CEvent*>(*prev_ev)->setEX(i == items.end() || ev->part() != curPart ? -1 : ev->event().tick()); 
            }
            
            do_redraw = true;    // Let songChanged handle the redraw upon SC_SELECTION.
            
            prev_ev = i;
      }

      if(ctrl)  
        lastpv = ctrl->hwVal();
        
      // insert new events
      for (int x = xx1, step; x < xx2 ; x += step )    
      {
            step = useRaster ? raster : editor->rasterVal2(x + 1) - x;
            
            int y    = x + step >= xx2 || x2 == x1 ? y2 : (x == xx1 ? y1 : (((y2 - y1) * (x + step/2 - x1)) / (x2 - x1)) + y1);
            int nval = computeVal(_controller, y, height());
            
            int tick = x - partTick;
            // Do not add events which are past the end of the part.
            if((unsigned)tick >= curPart->lenTick())
              break;
            MusECore::Event event(MusECore::Controller);
            event.setTick(tick);
            event.setA(_didx);
            if(type == MusECore::CTRL_PROGRAM)
            {
              if(lastpv == MusECore::CTRL_VAL_UNKNOWN)
                event.setB(nval - 1);
              else  
                event.setB((lastpv & 0xffff00) | (nval - 1));
            }
            else  
              event.setB(nval);
            
            // Indicate do port controller values and clone parts. 
            
// For testing...
//             fprintf(stderr, "CtrlCanvas::newVal 2: Pushing AddEvent\n");
                
            _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddEvent, 
                              event, curPart, true, true));
            
            CEvent* newev = new CEvent(event, curPart, event.dataB());
            insertPoint = items.insert(insertPoint, newev);
            //selectItem(newev); // TODO: There are advantages, but do we really want new items to be selected?
            
            if(insertPoint != items.begin())
            {
              ice_tmp = insertPoint;
              --ice_tmp;
              static_cast<CEvent*>(*ice_tmp)->setEX(tick);
            }  
            ice_tmp = insertPoint;
            ++ice_tmp;
            static_cast<CEvent*>(*insertPoint)->setEX(ice_tmp == items.end() || 
                                  (*ice_tmp)->part() != curPart ? 
                                  -1 : (*ice_tmp)->event().tick());
            ++insertPoint;
            do_redraw = true;
            }
              
      if(do_redraw)                 //
        redraw();
      }

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

void CtrlCanvas::deleteVal(int x1, int x2, int)
      {
      if(!curPart)         
        return;
      
      if(x1 < 0)
        x1 = 0;
      if(x2 < 0)
        x2 = 0;
      
      if(x2 - x1 < 0)
      {
        int a = x1;
        x1 = x2;
        x2 = a;
      }
      
      int xx1 = editor->rasterVal1(x1);
      int xx2 = editor->rasterVal2(x2);
      // If x1 and x2 happen to lie directly on the same raster, xx1 will equal xx2, 
      //  which is not good - there should always be a spread. Nudge by +1 and recompute.
      if(xx1 == xx2)
        xx2  = editor->rasterVal2(x2 + 1);

      int partTick = curPart->tick();
      xx1 -= partTick;
      xx2 -= partTick;

      iCItemList prev_ev = items.end();
      bool curPartFound = false;
      bool do_redraw = false;
      
      for (iCItemList i = items.begin(); i != items.end() ;) 
      {
            CEvent* ev = static_cast<CEvent*>(*i);
            if(ev->part() != curPart)
            {
              // Speed-up: If the current part was already processed, we are done.
              if(curPartFound)
                break;
              ++i;
              continue;
            }
            curPartFound = true;
            
            MusECore::Event event = ev->event();
            if (event.empty())
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            int x = event.tick();
            if (x < xx1)
            {
              prev_ev = i;
              ++i;
              continue;
            }      
            if (x >= xx2)
              break;
            
            // We must remove the item from the selected list.
            removeSelection(ev);
            // Indicate do port controller values and clone parts.
            _operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeleteEvent,
                             event, curPart, true, true));
            
            delete (ev);
            i = items.erase(i);           
            ev = static_cast<CEvent*>(*i);
            // Is there a previous item?
            if(prev_ev != items.end())
            {
              // Have we reached the end of the list, or the end of the current part's items within the item list?
              // Then it's current part's last drawn item. EX is 'infinity'. (As far as the part knows. Another part may overlay later.) 
              // Else EX is current item's tick. (By now, ((*i)->event() should be valid - it must be not empty to call tick()).
              static_cast<CEvent*>(*prev_ev)->setEX(i == items.end() || ev->part() != curPart ? -1 : ev->event().tick()); 
            }
            do_redraw = true;              
            
            prev_ev = i;
      }
            
      if(do_redraw)
        redraw();                 // Let songChanged handle the redraw upon SC_SELECTION.  
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlCanvas::setTool(int t)
      {
      if (tool == MusEGui::Tool(t))
            return;
      tool = MusEGui::Tool(t);
      switch(tool) {
            case MusEGui::DrawTool:
                  drawLineMode = false;
                  break;
            default:
                  break;
            }

      cancelMouseOps();
      setCursor();
      }

//---------------------------------------------------------
//   pFillBackgrounds
//---------------------------------------------------------

void CtrlCanvas::pFillBackgrounds(QPainter& p, const QRect& rect, const MusECore::MidiPart* part)
{
  if(!part)         
    return;
  
  int x = rect.x() - 1;   // compensate for 3 pixel line width
  int w = rect.width() + 2;
  int wh = height();
  QColor dark_gray_color = Qt::darkGray;
  dark_gray_color.setAlpha(_bgAlpha);
  
  MusECore::MidiTrack* mt = part->track();
  MusECore::MidiPort* mp;
  int chan;
  int cnum = _cnum;
  bool is_newdrum_ctl = (mt->type() == MusECore::Track::DRUM) && (curDrumPitch >= 0) && ((_cnum & 0xff) == 0xff);

  if(is_newdrum_ctl)
  {
    // Default to track port if -1 and track channel if -1.
    int mport = mt->drummap()[curDrumPitch].port;
    if(mport == -1)
      mport = mt->outPort();
    mp = &MusEGlobal::midiPorts[mport];
    cnum = (_cnum & ~0xff) | mt->drummap()[curDrumPitch].anote;
    chan = mt->drummap()[curDrumPitch].channel;
    if(chan == -1)
      chan = mt->outChannel();
  }
  else
  {
    mp = &MusEGlobal::midiPorts[mt->outPort()];
    chan = mt->outChannel();
  }
  
  MusECore::MidiController* mc = mp->midiController(cnum, chan);
  
  int min;
  int max;
  int bias;
  if(cnum == MusECore::CTRL_PROGRAM)
  {
    min = 1;
    max = 128;
    bias = 0;
  }
  else
  {
    min  = mc->minVal();
    max  = mc->maxVal();
    bias  = mc->bias();
  }
  int x1   = rect.x();
  int lval = MusECore::CTRL_VAL_UNKNOWN;
  for (iCItemList i = items.begin(); i != items.end(); ++i) 
  {
    CEvent* e = static_cast<CEvent*>(*i);
    // Draw unselected part controller events (lines) on top of selected part events (bars).
    if(e->part() != part)
      continue;
    MusECore::Event ev = e->event();
    // Draw drum controllers from another drum on top of ones from this drum.
    if(is_newdrum_ctl && ev.type() == MusECore::Controller && ev.dataA() != _didx)
      continue;
    int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
    int val = e->val();
    int pval = val;
    if(cnum == MusECore::CTRL_PROGRAM)
    {
      if((val & 0xff) == 0xff)
        // What to do here? prog = 0xff should not be allowed, but may still be encountered.
        pval = 1;
      else  
        pval = (val & 0x7f) + 1;
    }
    if (tick <= x) {
          if (val == MusECore::CTRL_VAL_UNKNOWN)
                lval = MusECore::CTRL_VAL_UNKNOWN;
          else
          {
                if(cnum == MusECore::CTRL_PROGRAM)
                  lval = wh - ((pval - min - bias) * wh / (max - min));
                else  
                  lval = wh - ((val - min - bias) * wh / (max - min));
          }
          continue;
          }
    if (tick > x+w)
          break;
    
    if (lval == MusECore::CTRL_VAL_UNKNOWN)
      p.fillRect(x1, 0, tick - x1, wh, dark_gray_color);
    
    x1 = tick;
    if (val == MusECore::CTRL_VAL_UNKNOWN)
          lval = MusECore::CTRL_VAL_UNKNOWN;
    else
    {
          if(cnum == MusECore::CTRL_PROGRAM)
            lval = wh - ((pval - min - bias) * wh / (max - min));
          else  
            lval = wh - ((val - min - bias) * wh / (max - min));
    } 
  }
  
  if (lval == MusECore::CTRL_VAL_UNKNOWN)
    p.fillRect(x1, 0, (x+w) - x1, wh, dark_gray_color);
}

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void CtrlCanvas::drawMoving(QPainter& p, const QRect& rect, const QRegion& /*region*/, const MusECore::MidiPart* part)
{
  int x = rect.x();   // compensate for 3 pixel line width
  int w = rect.width();
  int wh = height();
//  const QColor selection_color(0, 160, 255, MusEGlobal::config.globalAlphaBlend);
  QColor selection_color(MusEGlobal::config.ctrlGraphSel);
  selection_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  QColor graph_fg_color = MusEGlobal::config.ctrlGraphFg;
  graph_fg_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  QColor gray_color = Qt::gray;
  gray_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  
  if(!part)         
    return;
  
  QPen pen;
  pen.setCosmetic(true);
  
  CtrlCanvasInfoStruct info_out;
  partControllers(curPart, _cnum, 0, 0, 0, 0, &info_out);
  const int cnum = info_out.fin_ctrl_num;
  const bool is_newdrum_ctl = info_out.is_newdrum_ctl;    
  const int min = info_out.min;
  const int max = info_out.max;
  const int bias = info_out.bias;
  
  int lval = MusECore::CTRL_VAL_UNKNOWN;
  int item_x, item_endx;
  int tick;
  unsigned int part_tick;
  QColor color;
  for (iCItemList i = moving.begin(); i != moving.end(); ++i) 
  {
    CEvent* e = static_cast<CEvent*>(*i);
    // Draw unselected part controller events (lines) on top of selected part events (bars).
    if(e->part() != part)
      continue;
    MusECore::Event ev = e->event();
    if(ev.empty())
      continue;
    // Draw drum controllers from another drum on top of ones from this drum.
    if(is_newdrum_ctl && ev.type() == MusECore::Controller && ev.dataA() != _didx)
      continue;

    part_tick = e->part()->tick();
    
    tick = ev.tick() + part_tick + _curDragOffset.x();
    if(tick < 0)
      tick = 0;
    item_x = mapx(tick);
    
    item_endx = x + w;
    if(e->EX() >= 0)
    {
      int end_tick = e->EX() + part_tick + _curDragOffset.x();
      if(end_tick < 0)
        end_tick = 0;
      item_endx = mapx(end_tick);
    }

    if(item_x >= (x + w) || item_endx <= x)
      continue;
    
    const int val = e->val();
    int pval = val;
    if(cnum == MusECore::CTRL_PROGRAM)
    {
      if((val & 0xff) == 0xff)
        // What to do here? prog = 0xff should not be allowed, but may still be encountered.
        pval = 1;
      else  
        pval = (val & 0x7f) + 1;
    }
    
    color = selection_color;
    if(pval == MusECore::CTRL_VAL_UNKNOWN)
    {
      pval = max;
      color = gray_color;
    }
    else
    {
      pval -= bias;
      if(pval < min)
        pval = min;
      if(pval > max)
        pval = max;
    }
    
    lval = wh - ((pval - min) * wh / (max - min));

    lval += mapy(_curDragOffset.y());
    if(lval < 0)
      lval = 0;
    if(lval >= wh)
      lval = wh - 1;

    if(item_x < x)
      item_x = x;
    if(item_endx > (x + w))
      item_endx = x + w;
    
    p.fillRect(item_x, lval, item_endx - item_x, wh - lval, color);
    
  }
}

//---------------------------------------------------------
//   pdrawItems
//---------------------------------------------------------

void CtrlCanvas::pdrawItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, bool velo, bool fg)
{
  int x = rect.x() - 1;   // compensate for 3 pixel line width
  int w = rect.width() + 2;
  int wh = height();
//  const QColor selection_color(0, 160, 255, MusEGlobal::config.globalAlphaBlend);
  QColor selection_color(MusEGlobal::config.ctrlGraphSel);
  selection_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  QColor graph_fg_color = MusEGlobal::config.ctrlGraphFg;
  graph_fg_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  QColor gray_color = Qt::gray;
  gray_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  QColor light_gray_color = Qt::lightGray;
  light_gray_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
  QColor fill_color;
  
  noEvents=true;

  if(velo) 
  {
    noEvents=false;
    for(iCItemList i = items.begin(); i != items.end(); ++i) 
    {
      CEvent* e = static_cast<CEvent*>(*i);
      // Draw selected part velocity events on top of unselected part events.
      if(e->part() != part)
        continue;
      MusECore::Event event = e->event();
      int tick = mapx(event.tick() + e->part()->tick());
      if (tick <= x)
            continue;
      if (tick > x+w)
            break;
      int y1 = wh - (e->val() * wh / 128);
      // fg means 'draw selected parts'.
      if(fg)
      {
        if(e->isSelected())
          //p.setPen(QPen(Qt::blue, 3));
          p.setPen(QPen(selection_color, 3));
        else
          p.setPen(QPen(graph_fg_color, 3));
      }  
      else  
        p.setPen(QPen(Qt::darkGray, 3));
      p.drawLine(tick, wh, tick, y1);
    }
  }
  else
  {
    if(!part)         
      return;
    
    QPen pen;
    pen.setCosmetic(true);
    
    MusECore::MidiTrack* mt = part->track();
    MusECore::MidiPort* mp;
    int chan;
    int cnum = _cnum;
    bool is_newdrum_ctl = (mt->type() == MusECore::Track::DRUM) && (curDrumPitch >= 0) && ((_cnum & 0xff) == 0xff);

    if(is_newdrum_ctl)
    {
      // Default to track port if -1 and track channel if -1.
      int mport = mt->drummap()[curDrumPitch].port;
      if(mport == -1)
        mport = mt->outPort();
      mp = &MusEGlobal::midiPorts[mport];
      cnum = (_cnum & ~0xff) | mt->drummap()[curDrumPitch].anote;
      chan = mt->drummap()[curDrumPitch].channel;
      if(chan == -1)
        chan = mt->outChannel();
    }
    else
    {
      mp = &MusEGlobal::midiPorts[mt->outPort()];
      chan = mt->outChannel();
    }
    
    MusECore::MidiController* mc = mp->midiController(cnum, chan);
    
    int min;
    int max;
    int bias;
    if(cnum == MusECore::CTRL_PROGRAM)
    {
      min = 1;
      max = 128;
      bias = 0;
    }
    else
    {
      min  = mc->minVal();
      max  = mc->maxVal();
      bias  = mc->bias();
    }
    int x1   = rect.x();
    int lval = MusECore::CTRL_VAL_UNKNOWN;
    bool selected = false;
    bool is_moving = false;
    for (iCItemList i = items.begin(); i != items.end(); ++i) 
    {
      noEvents=false;
      CEvent* e = static_cast<CEvent*>(*i);
      // Draw unselected part controller events (lines) on top of selected part events (bars).
      if(e->part() != part)
        continue;
      MusECore::Event ev = e->event();
      // Draw drum controllers from another drum on top of ones from this drum.
      if(is_newdrum_ctl && ev.type() == MusECore::Controller && ev.dataA() != _didx)
        continue;
      int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
      int val = e->val();
      int pval = val;
      if(cnum == MusECore::CTRL_PROGRAM)
      {
        if((val & 0xff) == 0xff)
          // What to do here? prog = 0xff should not be allowed, but may still be encountered.
          pval = 1;
        else  
          pval = (val & 0x7f) + 1;
      }
      if (tick <= x) {
            if (val == MusECore::CTRL_VAL_UNKNOWN)
                  lval = MusECore::CTRL_VAL_UNKNOWN;
            else
            {
                  if(cnum == MusECore::CTRL_PROGRAM)
                    lval = wh - ((pval - min - bias) * wh / (max - min));
                  else  
                    lval = wh - ((val - min - bias) * wh / (max - min));
            }
            selected = e->isSelected();
            is_moving = e->isMoving();
            continue;
            }
      if (tick > x+w)
            break;

      if (lval != MusECore::CTRL_VAL_UNKNOWN)
      {
        if(fg)
        {  
          pen.setColor(gray_color);
          p.setPen(pen);
          p.drawLine(x1, lval, tick, lval);
        }  
        else
        {
          fill_color = is_moving ? light_gray_color : (selected ? selection_color : graph_fg_color);
          p.fillRect(x1, lval, tick - x1, wh - lval, fill_color);
        }
      }
      
      x1 = tick;
      if (val == MusECore::CTRL_VAL_UNKNOWN)
            lval = MusECore::CTRL_VAL_UNKNOWN;
      else
      {
            if(cnum == MusECore::CTRL_PROGRAM)
              lval = wh - ((pval - min - bias) * wh / (max - min));
            else  
              lval = wh - ((val - min - bias) * wh / (max - min));
      } 
      selected = e->isSelected();     
      is_moving = e->isMoving();
    }
    
    if (lval != MusECore::CTRL_VAL_UNKNOWN)
    {
      if(fg)
      {  
        pen.setColor(gray_color);
        p.setPen(pen);
        p.drawLine(x1, lval, x + w, lval);
      }  
      else
      {
        fill_color = is_moving ? light_gray_color : (selected ? selection_color : graph_fg_color);
        p.fillRect(x1, lval, (x+w) - x1, wh - lval, fill_color);
      }
    }
  }       
}

//---------------------------------------------------------
//   pdrawExtraDrumCtrlItems
//---------------------------------------------------------

void CtrlCanvas::pdrawExtraDrumCtrlItems(QPainter& p, const QRect& rect, const MusECore::MidiPart* part, int drum_ctl)
{
  int x = rect.x() - 1;   // compensate for 3 pixel line width
  int w = rect.width() + 2;
  int wh = height();

  QPen pen;
  pen.setCosmetic(true);
  
  noEvents=true;

  {
    if(!part)         
      return;
    
    MusECore::MidiTrack* mt = part->track();
    MusECore::MidiPort* mp;
    int chan;
    int cnum = _cnum;
    bool is_newdrum_ctl = (mt->type() == MusECore::Track::DRUM) && (curDrumPitch >= 0) && ((_cnum & 0xff) == 0xff);

    if(is_newdrum_ctl)
    {
      // Default to track port if -1 and track channel if -1.
      int mport = mt->drummap()[curDrumPitch].port;
      if(mport == -1)
        mport = mt->outPort();
      mp = &MusEGlobal::midiPorts[mport];
      cnum = (_cnum & ~0xff) | mt->drummap()[curDrumPitch].anote;
      chan = mt->drummap()[curDrumPitch].channel;
      if(chan == -1)
        chan = mt->outChannel();
    }
    else
    {
      mp = &MusEGlobal::midiPorts[mt->outPort()];
      chan = mt->outChannel();
    }
    
    MusECore::MidiController* mc = mp->midiController(cnum, chan);
    
    int min;
    int max;
    int bias;
    if(cnum == MusECore::CTRL_PROGRAM)
    {
      min = 1;
      max = 128;
      bias = 0;
    }
    else
    {
      min  = mc->minVal();
      max  = mc->maxVal();
      bias  = mc->bias();
    }
    int x1   = rect.x();
    int lval = MusECore::CTRL_VAL_UNKNOWN;
    for (iCItemList i = items.begin(); i != items.end(); ++i) 
    {
      noEvents=false;
      CEvent* e = static_cast<CEvent*>(*i);
      // Draw unselected part controller events (lines) on top of selected part events (bars).
      if(e->part() != part)
        continue;
      MusECore::Event ev = e->event();
      // Draw drum controllers from another drum on top of ones from this drum.
      // FIXME TODO Finish this off, not correct yet.
      if(drum_ctl == -1 && is_newdrum_ctl && ev.type() == MusECore::Controller && ev.dataA() != _didx)
        continue;
      if(drum_ctl != -1 && (!is_newdrum_ctl || (ev.type() == MusECore::Controller && ev.dataA() == _didx)))
        continue;
      int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
      int val = e->val();
      int pval = val;
      if(cnum == MusECore::CTRL_PROGRAM)
      {
        if((val & 0xff) == 0xff)
          // What to do here? prog = 0xff should not be allowed, but may still be encountered.
          pval = 1;
        else  
          pval = (val & 0x7f) + 1;
      }
      if (tick <= x) {
            if (val == MusECore::CTRL_VAL_UNKNOWN)
                  lval = MusECore::CTRL_VAL_UNKNOWN;
            else
            {
                  if(cnum == MusECore::CTRL_PROGRAM)
                    lval = wh - ((pval - min - bias) * wh / (max - min));
                  else  
                    lval = wh - ((val - min - bias) * wh / (max - min));
            }
            continue;
            }
      if (tick > x+w)
            break;
      
      if (lval != MusECore::CTRL_VAL_UNKNOWN)
      {
        pen.setColor(Qt::gray);
        p.setPen(pen);
        p.drawLine(x1, lval, tick, lval);
      }
      
      x1 = tick;
      if (val == MusECore::CTRL_VAL_UNKNOWN)
            lval = MusECore::CTRL_VAL_UNKNOWN;
      else
      {
            if(cnum == MusECore::CTRL_PROGRAM)
              lval = wh - ((pval - min - bias) * wh / (max - min));
            else  
              lval = wh - ((val - min - bias) * wh / (max - min));
      } 
    }

    if (lval != MusECore::CTRL_VAL_UNKNOWN)
    {
      pen.setColor(Qt::gray);
      p.setPen(pen);
      p.drawLine(x1, lval, x + w, lval);
    }
  }       
}

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void CtrlCanvas::pdraw(QPainter& p, const QRect& rect, const QRegion& region)
      {
      if(!_controller)   
        return;

      QPen pen;
      pen.setCosmetic(true);
      
      int x = rect.x() - 1;   // compensate for 3 pixel line width
      int y = rect.y();
      int w = rect.width() + 2;
      int h = rect.height();
      
      //---------------------------------------------------
      // draw Canvas Items
      //---------------------------------------------------

      bool velo = (MusECore::midiControllerType(_controller->num()) == MusECore::MidiController::Velo);
      
      if(!velo)
        // Fill backgrounds of 'unknown' value areas.
        pFillBackgrounds(p, rect, curPart);
      
      //---------------------------------------------------
      // draw the grid and markers now - before velocity items
      //---------------------------------------------------
      p.save();
      View::pdraw(p, rect);
      p.restore();
      
      pen.setColor(MusEGlobal::config.rangeMarkerColor);
      p.setPen(pen);
      int xp = mapx(pos[1]);
      if (xp >= x && xp < x+w) {
            p.drawLine(xp, y, xp, y+h);
            }
      xp = mapx(pos[2]);
      if (xp >= x && xp < x+w) {
            p.drawLine(xp, y, xp, y+h);
            }
      // Draw the red main position cursor last, on top of the others.
      xp = mapx(pos[0]);
      if (xp >= x && xp < x+w) {
            pen.setColor(MusEGlobal::config.positionMarkerColor);
            p.setPen(pen);
            p.drawLine(xp, y, xp, y+h);
            }

      if(!velo)
        // Draw non-fg non-velocity items for the current part
        pdrawItems(p, rect, curPart, false, false);
        
      for(MusECore::iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip) 
      {
        MusECore::MidiPart* part = (MusECore::MidiPart*)(ip->second);
        if(part == curPart || (filterTrack && part->track() != curTrack))
          continue;
        // Draw items for all parts - other than current part
        pdrawItems(p, rect, part, velo, !velo);
      }
      
      // Special: Draw fg drum controller items for non-current selected drum, for the current part
      // FIXME TODO Finish this off, not correct yet.
      if(curPart && curPart->track() && curPart->track()->type() == MusECore::Track::DRUM &&
         curDrumPitch >= 0 && ((_cnum & 0xff) == 0xff))
      {
        // Default to track port if -1 and track channel if -1.
        int port = curPart->track()->drummap()[curDrumPitch].port;
        if(port == -1)
          port = curPart->track()->outPort();
        int anote = curPart->track()->drummap()[curDrumPitch].anote;
        for(int i = 0; i < DRUM_MAPSIZE; ++i)
        {
          int iport = curPart->track()->drummap()[i].port;
          if(iport == -1)
            iport = curPart->track()->outPort();
          if(i != curDrumPitch && iport == port && curPart->track()->drummap()[i].anote == anote)
            pdrawExtraDrumCtrlItems(p, rect, curPart, anote);
        }
      }

      if(velo) 
      {
        // Draw fg velocity items for the current part
        pdrawItems(p, rect, curPart, true, true);
      }

      //---------------------------------------------------
      //    draw outlines of potential drop places of moving items
      //---------------------------------------------------

      if(!velo)
        drawMoving(p, rect, region, curPart);

      //---------------------------------------------------
      //    draw lasso
      //---------------------------------------------------

      if (drag == DRAG_LASSO) {
            setPainter(p);
            pen.setColor(Qt::blue);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(lasso);
            }
      }

//---------------------------------------------------------
//   drawOverlay
//---------------------------------------------------------

void CtrlCanvas::drawOverlay(QPainter& p, const QRect&, const QRegion&)
      {
      const QString s = _controller ? _controller->name() : QString("");
      
      p.setFont(font());
      
      p.setPen(palette().color(QPalette::WindowText));
      
      int y = fontMetrics().lineSpacing() + 2;
      
      const int txt_x = -xorg + overlayTextOffsetFromOrg;

      p.drawText(txt_x, y, s);
      }

//---------------------------------------------------------
//   overlayRect
//    returns geometry of overlay rectangle
//---------------------------------------------------------

QRect CtrlCanvas::overlayRect() const
{
      const QFontMetrics fm = fontMetrics();
      QRect r = fm.boundingRect(_controller ? _controller->name() : QString(""));
      
      int y = fm.lineSpacing() + 2;
      const int txt_x = -xorg + overlayTextOffsetFromOrg;
      r.translate(txt_x, y);

      return r;
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void CtrlCanvas::draw(QPainter& p, const QRect& rect, const QRegion& rg)
      {
      if (MusEGlobal::config.canvasShowGrid)
      {
        drawTickRaster(p, rect, rg, editor->raster(),
                      false, false, false,
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasBeatColor,
                      MusEGlobal::config.midiCanvasFineColor,
                      MusEGlobal::config.midiCanvasBarColor
                      );
      }

      //---------------------------------------------------
      //    draw line tool
      //---------------------------------------------------

      if ((tool == MusEGui::DrawTool) && drawLineMode) {
            QPen pen;
            pen.setCosmetic(true);
            pen.setColor(Qt::black);
            p.setPen(pen);
            p.drawLine(line1x, line1y, line2x, line2y);
            }
      }

//---------------------------------------------------------
//   drumPitchChanged
//---------------------------------------------------------

bool CtrlCanvas::drumPitchChanged()
{
  // Is it a drum controller?
  if((curDrumPitch >= 0) && ((_cnum & 0xff) == 0xff))
  {
    // Recompose the canvas according to the new selected pitch.
    setMidiController(_cnum);
    updateItems();
    return true;
  }
  return false;
}

//---------------------------------------------------------
//   findCurrentItem
//---------------------------------------------------------

CEvent* CtrlCanvas::findCurrentItem(const QPoint& p, const int tickstep, const int h)
{
  // Look for items that are moving first, since they are displayed
  //  on top of existing items until merged at some later time.
  for (iCItemList i = moving.begin(); i != moving.end(); ++i) {
        CEvent* item = static_cast<CEvent*>(*i);
        if(item->part() != curPart)
          continue;
        
        // Just to be thorough, the item should be marked as moving.
        if(!item->isMoving())
          continue;
        
        // Be sure to subtract the drag offset from the given point.
        if(item->containsPoint(_controller, p - _curDragOffset, tickstep, h)) {
              return item;
            }  
        }

  for (iCItemList i = items.begin(); i != items.end(); ++i) {
        CEvent* item = static_cast<CEvent*>(*i);
        if(item->part() != curPart)
          continue;
        
        // Disregard items that are moving.
        if(item->isMoving())
          continue;
        
        if(item->containsPoint(_controller, p, tickstep, h)) {
              return item;
            }  
        }
  return nullptr;
}

void CtrlCanvas::showCursor(bool show)
{ 
  if(_cursorOverrideCount > 1)
    fprintf(stderr, "MusE Warning: _cursorOverrideCount > 1 in CtrlCanvas::showCursor(%d)\n", show);

  if(show)
  {  
    while(_cursorOverrideCount > 0)
    {
      QApplication::restoreOverrideCursor();
      _cursorOverrideCount--;
    }
  }
  else
  {
    _cursorOverrideCount++;
    QApplication::setOverrideCursor(Qt::BlankCursor); // CAUTION
  }
}

//---------------------------------------------------------
//   setMouseGrab
//---------------------------------------------------------

void CtrlCanvas::setMouseGrab(bool grabbed)
{
  if(grabbed && !_mouseGrabbed)
  {
    _mouseGrabbed = true;
    grabMouse(); // CAUTION
  }
  else if(!grabbed && _mouseGrabbed)
  {
    releaseMouse();
    _mouseGrabbed = false;
  }
}

//---------------------------------------------------------
//   clearMoving
//---------------------------------------------------------

bool CtrlCanvas::clearMoving()
{
  bool changed = false;
  // Be sure to clear the moving list and especially the item moving flags!
  if(!moving.empty())
  {
    for(iCItemList i = moving.begin(); i != moving.end(); ++i)
      (*i)->setMoving(false);
    moving.clear();
    changed = true;
  }
  _curDragOffset = QPoint(0, 0);
  _mouseDist = QPoint(0, 0);
  _dragType = MOVE_MOVE;
  return changed;
}

//---------------------------------------------------------
//   cancelMouseOps
//---------------------------------------------------------

bool CtrlCanvas::cancelMouseOps()
{
  bool changed = false;
  
  // Make sure this is done. See mousePressEvent.
  showCursor();
  setMouseGrab(false);
  
  // Be sure to clear the moving list and especially the item moving flags!
  if(clearMoving())
    changed = true;
  
  // Be sure to clear the operations list!
  if(!_operations.empty())
  {
    _operations.clear();
    changed = true;
  }

  if(drag != DRAG_OFF)
  {
    drag = DRAG_OFF;
    changed = true;
  }
 
  if(_dragType != MOVE_MOVE)
  {
    _dragType = MOVE_MOVE;
    changed = true;
  }
  
  redraw();
  
  return changed;
}

//---------------------------------------------------------
//   setCurDrumPitch
//---------------------------------------------------------

bool CtrlCanvas::setCurDrumPitch(int instrument)
{
      DrumEdit* drumedit = dynamic_cast<DrumEdit*>(editor);
      if (drumedit == nullptr)
        curDrumPitch = instrument;
      else // new style drummap mode
      {
        // Crash protection by Tim. 
        // FIXME: Still, drum list is blank, editor can't edit. Other values of instrument or curDrumPitch just crash too.
        // Seems only with drum tracks that were created by importing a midi file (then changed to use fluidsynth device?).
        if(instrument == -1)  curDrumPitch = -1;
        
        else if (drumedit->get_instrument_map()[instrument].tracks.contains(curTrack))
          curDrumPitch = drumedit->get_instrument_map()[instrument].pitch;
        else
          curDrumPitch = -2; // this means "invalid", but not "unused"
      }

      return drumPitchChanged();
}

void CtrlCanvas::curPartHasChanged(MusECore::Part*)
{
  // If the current part or track changed, setup the midi controller variables.
  if(setCurTrackAndPart())
    setMidiController(_cnum);
  
  // Rebuild if setCurDrumPitch() doesn't already do it.
  if(!setCurDrumPitch(editor->curDrumInstrument()))
    updateItems();
  
//   songChanged(SC_EVENT_MODIFIED);
}

void CtrlCanvas::setPerNoteVeloMode(bool v)
{
  if(v == _perNoteVeloMode)
    return;
  _perNoteVeloMode = v;
  if(_cnum == MusECore::CTRL_VELOCITY)
    updateItems();
}

//---------------------------------------------------
//  populateMergeOptions
//---------------------------------------------------

void CtrlCanvas::populateMergeOptions(PopupMenu* menu)
{
  menu->addAction(new MenuTitleItem(tr("Merge Options"), menu));
  
  QAction* act = menu->addAction(QIcon(*midiCtrlMergeEraseIcon), tr("Erase Target"));
  act->setData(ContextIdErase);
  act->setCheckable(true);
  act->setChecked(MusEGlobal::config.midiCtrlGraphMergeErase);
  act->setToolTip(tr("Erase target events between source events"));
  
  act = menu->addAction(QIcon(*midiCtrlMergeEraseWysiwygIcon), tr("Erase Target WYSIWYG"));
  act->setData(ContextIdEraseWysiwyg);
  act->setCheckable(true);
  act->setChecked(MusEGlobal::config.midiCtrlGraphMergeEraseWysiwyg);
  act->setToolTip(tr("Include last source item width when erasing"));
  
  act = menu->addAction(QIcon(*midiCtrlMergeEraseInclusiveIcon), tr("Erase Target Inclusive"));
  act->setData(ContextIdEraseInclusive);
  act->setCheckable(true);
  act->setChecked(MusEGlobal::config.midiCtrlGraphMergeEraseInclusive);
  act->setToolTip(tr("Include entire source range when erasing"));
  
  menu->addAction(new MenuTitleItem(tr("Merge Actions"), menu));
  
  const bool is_mv = !moving.empty();
  
  act = menu->addAction(*pasteSVGIcon, tr("Merge Dragged Items"));
  act->setData(ContextIdMerge);
  act->setCheckable(false);
//  act->setToolTip(tr("Merge the dragged items"));
  act->setEnabled(is_mv);
  
  act = menu->addAction(*copySVGIcon, tr("Merge Copy of Dragged Items"));
  act->setData(ContextIdMergeCopy);
  act->setCheckable(false);
//  act->setToolTip(tr("Merge a copy of the dragged items"));
  act->setEnabled(is_mv);
  
  act = menu->addAction(*clearSVGIcon, tr("Cancel Drag"));
  act->setData(ContextIdCancelDrag);
  act->setCheckable(false);
//  act->setToolTip(tr("Cancel dragging the items"));
  act->setEnabled(is_mv);
}

//---------------------------------------------------
//  mergeDraggedItems
//  Merges any dragged items. Merges copies of items if 'copy' is true. Otherwise moves the items.
//---------------------------------------------------

bool CtrlCanvas::mergeDraggedItems(bool /*copy*/)
{
  
  return true;
}

} // namespace MusEGui
