//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pcanvas.cpp,v 1.48.2.26 2009/11/22 11:08:33 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <values.h>
#include <uuid/uuid.h>
#include <math.h>

#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qdragobject.h>
#include <qpopupmenu.h>
#include <qurl.h>
#include <qmenudata.h>

#include "widgets/tools.h"
#include "pcanvas.h"
#include "midieditor.h"
#include "globals.h"
#include "icons.h"
#include "event.h"
#include "xml.h"
#include "wave.h"
#include "audio.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "app.h"
#include "filedialog.h"

const char* partColorNames[] = {
      "Default",
      "Refrain",
      "Bridge",
      "Intro",
      "Coda",
      "Chorus",
      "Solo",
      "Brass",
      "Percussion",
      "Drums",
      "Guitar",
      "Bass",
      "Flute",
      "Strings",
      "Keyboard",
      "Piano",
      "Saxophon",
      };

//---------------------------------------------------------
//   ColorListItem
//---------------------------------------------------------

class ColorListItem : public QCustomMenuItem {
      QColor color;
      int h;
      int fontheight;
      QString label;
      virtual QSize sizeHint() { return QSize(80, h); }
      virtual void paint(QPainter* p, const QColorGroup&, bool /*act*/, bool /*enabled*/, int x, int y, int /*w*/, int h)
            {
            p->fillRect(x+5, y+2, h-4, h-4, QBrush(color));
            p->drawText(x+5 + h - 4 + 3, y+(fontheight * 3) / 4, label);
            }

   public:
      ColorListItem(const QColor& c, int _h,  int _fh, const char* txt)
         : color(c), h(_h), fontheight(_fh), label(txt) {
            }
      QString text() const { return QString("PartColor"); }
      };

//---------------------------------------------------------
//   NPart
//---------------------------------------------------------

NPart::NPart(Part* e) : CItem(Event(), e)
      {
      int th = track()->height();
      int y  = track()->y();
      setPos(QPoint(e->tick(), y + 1));
      setBBox(QRect(e->tick(), y + 1, e->lenTick(), th));
      }

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

PartCanvas::PartCanvas(int* r, QWidget* parent, int sx, int sy)
   : Canvas(parent, sx, sy)
      {
      setAcceptDrops(true);
      _raster = r;

      setFocusPolicy(StrongFocus);
      // Defaults:
      lineEditor = 0;
      editMode   = false;

      tracks = song->tracks();
      setMouseTracking(true);
      drag          = DRAG_OFF;
      curColorIndex = 0;
      partsChanged();
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int PartCanvas::y2pitch(int y) const
      {
      TrackList* tl = song->tracks();
      int yy  = 0;
      int idx = 0;
      for (iTrack it = tl->begin(); it != tl->end(); ++it, ++idx) {
            int h = (*it)->height();
            // if ((y >= yy) && (y < yy+h))
            if (y < yy+h)
                  break;
            yy += h;
            }
      return idx;
      }

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int PartCanvas::pitch2y(int p) const
      {
      TrackList* tl = song->tracks();
      int yy  = 0;
      int idx = 0;
      for (iTrack it = tl->begin(); it != tl->end(); ++it, ++idx) {
            if (idx == p)
                  break;
            yy += (*it)->height();
            }
      return yy;
      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PartCanvas::leaveEvent(QEvent*)
      {
      emit timeChanged(MAXINT);
      }

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void PartCanvas::returnPressed()
      {
      lineEditor->hide();
      Part* oldPart = editPart->part();
      Part* newPart = oldPart->clone();
      // Added by Tim. p3.3.6
      //printf("PartCanvas::returnPressed before msgChangePart oldPart refs:%d Arefs:%d newPart refs:%d Arefs:%d\n", oldPart->events()->refCount(), oldPart->events()->arefCount(), newPart->events()->refCount(), newPart->events()->arefCount());
      
      newPart->setName(lineEditor->text());
      // Indicate do undo, and do port controller values but not clone parts. 
      //audio->msgChangePart(oldPart, newPart);
      audio->msgChangePart(oldPart, newPart, true, true, false);
      // Added by Tim. p3.3.6
      //printf("PartCanvas::returnPressed after msgChangePart oldPart refs:%d Arefs:%d newPart refs:%d Arefs:%d\n", oldPart->events()->refCount(), oldPart->events()->arefCount(), newPart->events()->refCount(), newPart->events()->arefCount());
      
      editMode = false;
      }

//---------------------------------------------------------
//   viewMouseDoubleClick
//---------------------------------------------------------

void PartCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if (_tool != PointerTool) {
            viewMousePressEvent(event);
            return;
            }
      QPoint cpos = event->pos();
      curItem     = items.find(cpos);
      bool shift  = event->state() & ShiftButton;
      if (curItem) {
            if (event->button() == QMouseEvent::LeftButton && shift) {
                  editPart = (NPart*)curItem;
                  QRect r = map(curItem->bbox());
                  if (lineEditor == 0) {
                        lineEditor = new QLineEdit(this);
                        lineEditor->setFrame(true);
                        }
                  editMode = true;
                  lineEditor->setGeometry(r);
                  lineEditor->setText(editPart->name());
                  lineEditor->setFocus();
                  lineEditor->show();
                  }
            else if (event->button() == QMouseEvent::LeftButton) {
                  deselectAll();
                  selectItem(curItem, true);
                  emit dclickPart(((NPart*)(curItem))->track());
                  }
            }
      //
      // double click creates new part between left and
      // right mark

      else {
            TrackList* tl = song->tracks();
            iTrack it;
            int yy = 0;
            int y = event->y();
            for (it = tl->begin(); it != tl->end(); ++it) {
                  int h = (*it)->height();
                  if (y >= yy && y < (yy + h))
                        break;
                  yy += h;
                  }
            if (pos[2] - pos[1] > 0 && it != tl->end()) {
                  Track* track = *it;
                  switch(track->type()) {
                        case Track::MIDI:
                        case Track::DRUM:
                              {
                              MidiPart* part = new MidiPart((MidiTrack*)track);
                              part->setTick(pos[1]);
                              part->setLenTick(pos[2]-pos[1]);
                              part->setName(track->name());
                              NPart* np = new NPart(part);
                              items.add(np);
                              deselectAll();
                              part->setSelected(true);
                              audio->msgAddPart(part);
                              }
                              break;
                        case Track::WAVE:
                        case Track::AUDIO_OUTPUT:
                        case Track::AUDIO_INPUT:
                        case Track::AUDIO_GROUP:
                        case Track::AUDIO_AUX:
                        case Track::AUDIO_SOFTSYNTH:
                              break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   startUndo
//---------------------------------------------------------

void PartCanvas::startUndo(DragType)
      {
      song->startUndo();
      }

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void PartCanvas::endUndo(DragType t, int flags)
      {
      song->endUndo(flags | ((t == MOVE_COPY || t == MOVE_CLONE)
         ? SC_PART_INSERTED : SC_PART_MODIFIED));
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void PartCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype, int*)
{      
  /*
  if(editor->parts()->empty())
    return;
    
  //struct p2c
  //{
  //  Part* newp;
  //  int   xdiff;
  //} 
  
  //std::set<Part*> parts2change;
  //typedef std::set<Part*>::iterator iptc;
  std::map<Part*, Part*> parts2change;
  typedef std::map<Part*, Part*>::iterator iP2C;
  
  int modified = 0;
  for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
  {
    Part* part = ip->second;
    if(!part)
      continue;
    
    int npartoffset = 0;
    for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
    {
      CItem* ci = ici->second;
      //Part* pt = ci->part();
      //if(!pt)
      if(ci->part() != part)
        continue;
      
      int x = ci->pos().x() + dx;
      int y = pitch2y(y2pitch(ci->pos().y()) + dp);
      QPoint newpos = raster(QPoint(x, y));
      
      // Test moving the item...
      
      //int offset = testMoveItem(ci, newpos, dragtype);
      NEvent* nevent = (NEvent*) ci;
      Event event    = nevent->event();
      //int npitch     = y2pitch(newpos.y());
      x              = newpos.x();
      if (x < 0)
            x = 0;
      
      int ntick = editor->rasterVal(x) - part->tick();
      if (ntick < 0)
            ntick = 0;
      int diff = ntick + event.lenTick() - part->lenTick();
      
      // If moving the item would require a new part size...
      if(diff > npartoffset)
        npartoffset = diff;
    }
        
    if(npartoffset > 0)
    {    
      // Create new part...
      // if there are several events that are moved outside the part, it will be recreated for each
      // so the part _in_ the event will not be valid, ask the authority.
      Part* newPart = part->clone();
      //Part* newPart = Canvas::part()->clone();

      newPart->setLenTick(newPart->lenTick() + npartoffset);
      audio->msgChangePart(part, newPart,false);

      modified = SC_PART_MODIFIED;

      // BUG FIX: #1650953
      // Added by T356.
      // Fixes posted "select and drag past end of part - crashing" bug
      for(iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
      {
        if(ip->second == part)
        {
          editor->parts()->erase(ip);
          break;
        }
      }
      
      editor->parts()->add(newPart);
      if(parts2change.find(part) == parts2change.end())
        parts2change.insert(std::pair<Part*, Part*> (part, newPart));
      
//      part = newPart; // reassign
//      item->setPart(part);
//      item->setEvent(newEvent);
//      curPart = part;
//      curPartId = curPart->sn();

    }
  }
*/    
    
//    int modified = 0;
  for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
  {
    CItem* ci = ici->second;
    
    // If this item's part is in the parts2change list, change the item's part to the new part.
    //Part* pt = ci->part();
    //iP2C ip2c = parts2change.find(pt);
    //if(ip2c != parts2change.end())
    //  ci->setPart(ip2c->second);
    
    int x = ci->pos().x();
    int y = ci->pos().y();
    int nx = x + dx;
    int ny = pitch2y(y2pitch(y) + dp);
    QPoint newpos = raster(QPoint(nx, ny));
    selectItem(ci, true);
    
    if(moveItem(ci, newpos, dtype))
          ci->move(newpos);
    if(moving.size() == 1) {
          itemReleased(curItem, newpos);
          }
    if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
          selectItem(ci, false);
  }  
  
  
  //if(pflags)
  //  *pflags = modified;
}
      
//---------------------------------------------------------
//   moveItem
//    return false, if copy/move not allowed
//---------------------------------------------------------

// Changed by T356.
//bool PartCanvas::moveItem(CItem* item, const QPoint& newpos, DragType t, int*)
bool PartCanvas::moveItem(CItem* item, const QPoint& newpos, DragType t)
      {
      NPart* npart    = (NPart*) item;
      Part* spart     = npart->part();
      Track* track    = npart->track();
      unsigned dtick  = newpos.x();
      unsigned ntrack = y2pitch(item->mp().y());
      Track::TrackType type = track->type();
      if (tracks->index(track) == ntrack && (dtick == spart->tick())) {
            return false;
            }
      if (ntrack >= tracks->size()) {
            ntrack = tracks->size();
            Track* newTrack = song->addTrack(int(type));
            if (type == Track::WAVE) {
                  WaveTrack* st = (WaveTrack*) track;
                  WaveTrack* dt = (WaveTrack*) newTrack;
                  dt->setChannels(st->channels());
                  }
            emit tracklistChanged();
            }
      Track* dtrack = tracks->index(ntrack);

      if (dtrack->type() != type) {
            QMessageBox::critical(this, QString("MusE"),
               tr("Cannot copy/move/clone to different Track-Type"));
            return false;
            }

      Part* dpart;
      //bool clone = (t == MOVE_CLONE) || (spart->events()->arefCount() > 1);
      //bool clone = (t == MOVE_CLONE);
      bool clone = (t == MOVE_CLONE || (t == MOVE_COPY && spart->events()->arefCount() > 1));
      
      if(t == MOVE_MOVE)
      {
        // This doesn't increment aref count, and doesn't chain clones.
        // It also gives the new part a new serial number, but it is 
        //  overwritten with the old one by Song::changePart(), from Audio::msgChangePart() below. 
        dpart = spart->clone();
        dpart->setTrack(dtrack);
      }  
      else
        // This increments aref count if cloned, and chains clones.
        // It also gives the new part a new serial number.
        dpart = dtrack->newPart(spart, clone);

      dpart->setTick(dtick);

      // Added by Tim. p3.3.6
      //printf("PartCanvas::moveItem before add/changePart clone:%d spart:%p events:%p refs:%d Arefs:%d sn:%d dpart:%p events:%p refs:%d Arefs:%d sn:%d\n", clone, spart, spart->events(), spart->events()->refCount(), spart->events()->arefCount(), spart->sn(), dpart, dpart->events(), dpart->events()->refCount(), dpart->events()->arefCount(), dpart->sn());
      
      if(t == MOVE_MOVE) 
        item->setPart(dpart);
      //if (!clone) {
      if (t == MOVE_COPY && !clone) {
            //
            // Copy Events
            //
            EventList* se = spart->events();
            EventList* de = dpart->events();
            for (iEvent i = se->begin(); i != se->end(); ++i) {
                  Event oldEvent = i->second;
                  Event ev = oldEvent.clone();
                  de->add(ev);
                  }
            }
      if (t == MOVE_COPY || t == MOVE_CLONE) {
            // These will not increment ref count, and will not chain clones...
            if (dtrack->type() == Track::WAVE)
                  audio->msgAddPart((WavePart*)dpart,false);
            else
                  audio->msgAddPart(dpart,false);
            }
      else if (t == MOVE_MOVE) {
            dpart->setSelected(spart->selected());
            // These will increment ref count if not a clone, and will chain clones...
            if (dtrack->type() == Track::WAVE)
                  // Indicate no undo, and do not do port controller values and clone parts. 
                  //audio->msgChangePart((WavePart*)spart, (WavePart*)dpart,false);
                  audio->msgChangePart((WavePart*)spart, (WavePart*)dpart, false, false, false);
            else
                  // Indicate no undo, and do port controller values but not clone parts. 
                  //audio->msgChangePart(spart, dpart, false);
                  audio->msgChangePart(spart, dpart, false, true, false);
            
            spart->setSelected(false);
            }
      // Added by Tim. p3.3.6
      //printf("PartCanvas::moveItem after add/changePart spart:%p events:%p refs:%d Arefs:%d dpart:%p events:%p refs:%d Arefs:%d\n", spart, spart->events(), spart->events()->refCount(), spart->events()->arefCount(), dpart, dpart->events(), dpart->events()->refCount(), dpart->events()->arefCount());
      
      if (song->len() < (dpart->lenTick() + dpart->tick()))
            song->setLen(dpart->lenTick() + dpart->tick());
      //endUndo(t);
      return true;
      }

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint PartCanvas::raster(const QPoint& p) const
      {
      int y = pitch2y(y2pitch(p.y()));
      int x = p.x();
      if (x < 0)
            x = 0;
      x = sigmap.raster(x, *_raster);
      if (x < 0)
            x = 0;
      return QPoint(x, y);
      }

//---------------------------------------------------------
//   partsChanged
//---------------------------------------------------------

void PartCanvas::partsChanged()
      {
      items.clear();
      int idx = 0;
      for (iTrack t = tracks->begin(); t != tracks->end(); ++t) {
            PartList* pl = (*t)->parts();
            for (iPart i = pl->begin(); i != pl->end(); ++i) {
                  NPart* np = new NPart(i->second);
                  items.add(np);
                  if (i->second->selected()) {
                        selectItem(np, true);
                        }
                  }
            ++idx;
            }
      redraw();
      }

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void PartCanvas::updateSelection()
      {
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            NPart* part = (NPart*)(i->second);
            part->part()->setSelected(i->second->isSelected());
            }
      emit selectionChanged();
      redraw();
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PartCanvas::resizeItem(CItem* i, bool noSnap)
      {
      Track* t = ((NPart*)(i))->track();
      Part*  p = ((NPart*)(i))->part();

      int pos = p->tick() + i->width();
      int snappedpos = p->tick();
      if (!noSnap) {
            snappedpos = sigmap.raster(pos, *_raster);
            }
      unsigned int newwidth = snappedpos - p->tick();
      if (newwidth == 0)
            newwidth = sigmap.rasterStep(p->tick(), *_raster);

      song->cmdResizePart(t, p, newwidth);
      }

//---------------------------------------------------------
//   newItem
//    first create local Item
//---------------------------------------------------------

CItem* PartCanvas::newItem(const QPoint& pos, int)
      {
      int x = pos.x();
      if (x < 0)
            x = 0;
      x = sigmap.raster(x, *_raster);
      unsigned trackIndex = y2pitch(pos.y());
      if (trackIndex >= tracks->size())
            return 0;
      Track* track = tracks->index(trackIndex);
      if(!track)
        return 0;
        
      Part* pa  = 0;
      NPart* np = 0;
      switch(track->type()) {
            case Track::MIDI:
            case Track::DRUM:
                  pa = new MidiPart((MidiTrack*)track);
                  pa->setTick(x);
                  pa->setLenTick(0);
                  break;
            case Track::WAVE:
                  pa = new WavePart((WaveTrack*)track);
                  pa->setTick(x);
                  pa->setLenTick(0);
                  break;
            case Track::AUDIO_OUTPUT:
            case Track::AUDIO_INPUT:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_AUX:
            case Track::AUDIO_SOFTSYNTH:
                  return 0;
            }
      pa->setName(track->name());
      pa->setColorIndex(curColorIndex);
      np = new NPart(pa);
      return np;
      }

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

void PartCanvas::newItem(CItem* i, bool noSnap)
      {
      Part*  p = ((NPart*)(i))->part();

      int len = i->width();
      if (!noSnap)
            len = sigmap.raster(len, *_raster);
      if (len == 0)
            len = sigmap.rasterStep(p->tick(), *_raster);
      p->setLenTick(len);
      p->setSelected(true);
      audio->msgAddPart(p);
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PartCanvas::deleteItem(CItem* i)
      {
      Part*  p = ((NPart*)(i))->part();
      audio->msgRemovePart(p); //Invokes songChanged which calls partsChanged which makes it difficult to delete them there
      return true;
      }

//---------------------------------------------------------
//   splitItem
//---------------------------------------------------------

void PartCanvas::splitItem(CItem* item, const QPoint& pt)
      {
      NPart* np = (NPart*) item;
      Track* t = np->track();
      Part*  p = np->part();
      int x = pt.x();
      if (x < 0)
            x = 0;
      song->cmdSplitPart(t, p, sigmap.raster(x, *_raster));
      }

//---------------------------------------------------------
//   glueItem
//---------------------------------------------------------

void PartCanvas::glueItem(CItem* item)
      {
      NPart* np = (NPart*) item;
      Track* t = np->track();
      Part*  p = np->part();
      song->cmdGluePart(t, p);
      }

//---------------------------------------------------------
//   genItemPopup
//---------------------------------------------------------

QPopupMenu* PartCanvas::genItemPopup(CItem* item)
      {
      NPart* npart = (NPart*) item;
      Track::TrackType trackType = npart->track()->type();

      QPopupMenu* partPopup = new QPopupMenu(this);

      partPopup->insertItem(*editcutIconSet, tr("C&ut"), 4);
      partPopup->setAccel(CTRL+Key_X, 4);

      partPopup->insertItem(*editcopyIconSet, tr("&Copy"), 5);
      partPopup->setAccel(CTRL+Key_C, 5);

      partPopup->insertSeparator();
      int rc = npart->part()->events()->arefCount();
      QString st = QString(tr("s&elect "));
      if(rc > 1)
        st += (QString().setNum(rc) + QString(" "));
      st += QString(tr("clones"));
      partPopup->insertItem(st, 18);
      
      partPopup->insertSeparator();
      partPopup->insertItem(tr("rename"), 0);
      QPopupMenu* colorPopup = new QPopupMenu(this);
      partPopup->insertItem(tr("color"), colorPopup);

      // part color selection
      const QFontMetrics& fm = colorPopup->fontMetrics();
      int h = fm.lineSpacing();

      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            ColorListItem* item = new ColorListItem(config.partColors[i], h, fontMetrics().height(), partColorNames[i]);
            colorPopup->insertItem(item, 20+i);
            }

      partPopup->insertItem(*deleteIcon, tr("delete"), 1);
      partPopup->insertItem(*cutIcon, tr("split"),  2);
      partPopup->insertItem(*glueIcon, tr("glue"),   3);
      partPopup->insertItem(tr("de-clone"), 15);

      partPopup->insertSeparator();
      switch(trackType) {
            case Track::MIDI:
                  partPopup->insertItem(*pianoIconSet, tr("pianoroll"), 10);
                  partPopup->insertItem(*edit_listIcon, tr("list"), 12);
                  partPopup->insertItem(tr("export"), 16);
                  break;
            case Track::DRUM:
                  partPopup->insertItem(*edit_listIcon, tr("list"), 12);
                  partPopup->insertItem(*edit_drummsIcon, tr("drums"), 13);
                  partPopup->insertItem(tr("export"), 16);
                  break;
            case Track::WAVE:
                  partPopup->insertItem(*edit_waveIcon, tr("wave edit"), 14);
                  partPopup->insertItem(tr("export"), 16);
                  partPopup->insertItem(tr("file info"), 17);
                  break;
            case Track::AUDIO_OUTPUT:
            case Track::AUDIO_INPUT:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_AUX:
            case Track::AUDIO_SOFTSYNTH:
                  break;
            }

      partPopup->setItemEnabled(18, rc > 1);
      partPopup->setItemEnabled(1, true);
      partPopup->setItemEnabled(4, true);
      partPopup->setItemEnabled(15, rc > 1);
      
      return partPopup;
      }

//---------------------------------------------------------
//   itemPopup
//---------------------------------------------------------

void PartCanvas::itemPopup(CItem* item, int n, const QPoint& pt)
      {
      PartList* pl = new PartList;
      NPart* npart = (NPart*)(item);
      pl->add(npart->part());
      switch(n) {
            case 0:     // rename
                  {
                  editPart = npart;
                  QRect r = map(curItem->bbox());
                  if (lineEditor == 0) {
                        lineEditor = new QLineEdit(this);
                        lineEditor->setFrame(true);
                        }
                  lineEditor->setText(editPart->name());
                  lineEditor->setFocus();
                  lineEditor->show();
                  lineEditor->setGeometry(r);
                  editMode = true;
                  }
                  break;
            case 1:     // delete
                  deleteItem(item);
                  break;
            case 2:     // split
                  splitItem(item, pt);
                  break;
            case 3:     // glue
                  glueItem(item);
                  break;
            case 4:
                  copy(pl);
                  audio->msgRemovePart(npart->part());
                  break;
            case 5:
                  copy(pl);
                  break;
            case 10:    // pianoroll edit
                  emit startEditor(pl, 0);
                  return;
            case 12:    // list edit
                  emit startEditor(pl, 1);
                  return;
            case 13:    // drum edit
                  emit startEditor(pl, 3);
                  return;
            case 14:    // wave edit
                  {
                    // Changed to allow multiple selected parts to be shown. By T356
                    // Slightly inefficient to add (above), then clear here.
                    // Should really only add npart->part() to pl only if NOT here.
                    // Removed. Added wave editor menu item instead.
                    //pl->clear();
                    //PartList* ptl = npart->track()->parts();
                    //for(ciPart pi = ptl->begin(); pi != ptl->end(); pi++)
                    //{
                    //  if(pi->second->selected())
                    //    pl->add(pi->second);
                    //}  
                    emit startEditor(pl, 4);
                  }
                  return;
            case 15:    // declone
                  {
                  Part* spart  = npart->part();
                  Track* track = npart->track();
                  Part* dpart  = track->newPart(spart, false);
                  // Added by Tim. p3.3.6
                  //printf("PartCanvas::itemPopup: #1 spart %s %p next:%s %p prev:%s %p\n", spart->name().latin1(), spart, spart->nextClone()->name().latin1(), spart->nextClone(), spart->prevClone()->name().latin1(), spart->prevClone()); 
                  //printf("PartCanvas::itemPopup: #1 dpart %s %p next:%s %p prev:%s %p\n", dpart->name().latin1(), dpart, dpart->nextClone()->name().latin1(), dpart->nextClone(), dpart->prevClone()->name().latin1(), dpart->prevClone()); 

                  EventList* se = spart->events();
                  EventList* de = dpart->events();
                  for (iEvent i = se->begin(); i != se->end(); ++i) {
                        Event oldEvent = i->second;
                        Event ev = oldEvent.clone();
                        de->add(ev);
                        }
                  song->startUndo();
                  // Indicate no undo, and do port controller values but not clone parts. 
                  //audio->msgChangePart(spart, dpart, false);
                  audio->msgChangePart(spart, dpart, false, true, false);
                  // Added by Tim. p3.3.6
                  //printf("PartCanvas::itemPopup: #2 spart %s %p next:%s %p prev:%s %p\n", spart->name().latin1(), spart, spart->nextClone()->name().latin1(), spart->nextClone(), spart->prevClone()->name().latin1(), spart->prevClone()); 
                  //printf("PartCanvas::itemPopup: #2 dpart %s %p next:%s %p prev:%s %p\n", dpart->name().latin1(), dpart, dpart->nextClone()->name().latin1(), dpart->nextClone(), dpart->prevClone()->name().latin1(), dpart->prevClone()); 

                  song->endUndo(SC_PART_MODIFIED);
                  break; // Has to be break here, right?
                  }
            case 16: // Export to file
                  {
                  const Part* part = item->part();
                  bool popenFlag = false;
                  //QString fn = getSaveFileName(QString(""), part_file_pattern, this, tr("MusE: save part"));
                  QString fn = getSaveFileName(QString(""), part_file_save_pattern, this, tr("MusE: save part"));
                  if (!fn.isEmpty()) {
                        FILE* fp = fileOpen(this, fn, ".mpt", "w", popenFlag, false, false);
                        if (fp) {
                              Xml tmpXml = Xml(fp);
                              //part->write(0, tmpXml);
                              // Write the part. Indicate that it's a copy operation - to add special markers, 
                              //  and force full wave paths.
                              part->write(0, tmpXml, true, true);
                              fclose(fp);
                              }
                        }
                  break;
                  }
                  
            case 17: // File info
                  {
                    Part* p = item->part();
                    EventList* el = p->events();
                    QString str = tr("Part name") + ": " + p->name() + "\n" + tr("Files") + ":";
                    for (iEvent e = el->begin(); e != el->end(); ++e) 
                    {
                      Event event = e->second;
                      SndFileR f  = event.sndFile();
                      if (f.isNull())
                        continue;
                      //str.append("\n" + f.path());
                      str.append(QString("\n@") + QString().setNum(event.tick()) + QString(" len:") + 
                        QString().setNum(event.lenTick()) + QString(" ") + f.path());
                    }  
                    QMessageBox::information(this, "File info", str, "Ok", 0);
                    break;
                  }
            case 18: // Select clones
                  {
                    Part* part = item->part();
                    
                    // Traverse and process the clone chain ring until we arrive at the same part again.
                    // The loop is a safety net.
                    Part* p = part; 
                    int j = part->cevents()->arefCount();
                    if(j > 0)
                    {
                      for(int i = 0; i < j; ++i)
                      {
                        // Added by Tim. p3.3.6
                        //printf("PartCanvas::itemPopup i:%d %s %p events %p refs:%d arefs:%d\n", i, p->name().latin1(), p, part->cevents(), part->cevents()->refCount(), j); 
                        
                        p->setSelected(true);
                        p = p->nextClone();
                        if(p == part)
                          break;
                      }
                      //song->update();
                      song->update(SC_SELECTION);
                    }
                    
                    break;
                  }
            case 20 ... NUM_PARTCOLORS+20:
                  {
                    curColorIndex = n - 20;
                    bool selfound = false;
                    //Loop through all parts and set color on selected:
                    for (iCItem i = items.begin(); i != items.end(); i++) {
                          if (i->second->isSelected()) {
                                selfound = true;
                                i->second->part()->setColorIndex(curColorIndex);
                                }
                          }
                          
                    // If no items selected, use the one clicked on.
                    if(!selfound)
                      item->part()->setColorIndex(curColorIndex);
                                
                    redraw();
                    break;
                  }
            default:
                  printf("unknown action %d\n", n);
                  break;
            }
      delete pl;
      }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void PartCanvas::mousePress(QMouseEvent* event)
      {
      if (event->state() & ShiftButton) {
            return;
            }
      QPoint pt = event->pos();
      CItem* item = items.find(pt);
      if (item == 0)
            return;
      switch (_tool) {
            default:
                  emit trackChanged(item->part()->track());
                  break;
            case CutTool:
                  splitItem(item, pt);
                  break;
            case GlueTool:
                  glueItem(item);
                  break;
            case MuteTool:
                  {
                  NPart* np = (NPart*) item;
                  Part*  p = np->part();
                  p->setMute(!p->mute());
                  redraw();
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void PartCanvas::mouseRelease(const QPoint&)
      {
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void PartCanvas::mouseMove(const QPoint& pos)
      {
      int x = pos.x();
      if (x < 0)
            x = 0;
      emit timeChanged(sigmap.raster(x, *_raster));
      }

//---------------------------------------------------------
//   y2Track
//---------------------------------------------------------

Track* PartCanvas::y2Track(int y) const
      {
      TrackList* l = song->tracks();
      int ty = 0;
      for (iTrack it = l->begin(); it != l->end(); ++it) {
            int h = (*it)->height();
            if (y >= ty && y < ty + h)
                  return *it;
            ty += h;
            }
      return 0;
      }

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void PartCanvas::keyPress(QKeyEvent* event)
      {
      int key = event->key();
      if (editMode) {
            returnPressed();
            return;
            }

      if (event->state() & ShiftButton)
            key += SHIFT;
      if (event->state() & AltButton)
            key += ALT;
      if (event->state() & ControlButton)
            key += CTRL;

      if (key == shortcuts[SHRT_DELETE].key) {
            if (getCurrentDrag()) {
                  //printf("dragging!!\n");
                  return;
                  }
                
            song->startUndo();
            song->msgRemoveParts();
            song->endUndo(SC_PART_REMOVED);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            int frames = pos[0] - sigmap.rasterStep(pos[0], *_raster);
            if (frames < 0)
                  frames = 0;
            Pos p(frames,true);
            song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC].key) {
            Pos p(pos[0] + sigmap.rasterStep(pos[0], *_raster), true);
            song->setPos(0, p, true, true, true); //CDW
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_POINTER].key) {
            emit setUsedTool(PointerTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PENCIL].key) {
            emit setUsedTool(PencilTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_RUBBER].key) {
            emit setUsedTool(RubberTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_SCISSORS].key) {
            emit setUsedTool(CutTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_GLUE].key) {
            emit setUsedTool(GlueTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_MUTE].key) {
            emit setUsedTool(MuteTool);
            return;
            }

      //
      // Shortcuts that require selected parts from here
      //
      if (!curItem) { //TODO: Fix a curItem from selected parts if song is loaded and has selected parts.
            event->ignore();
            return;
            }

      CItem* newItem = 0;
      bool singleSelection = isSingleSelection();
      bool add = false;
      //Locators to selection
      if (key == shortcuts[SHRT_LOCATORS_TO_SELECTION].key) {
            CItem *leftmost = 0, *rightmost = 0;
            for (iCItem i = items.begin(); i != items.end(); i++) {
            if (i->second->isSelected()) {
                  // Check leftmost:
                  if (!leftmost)
                        leftmost = i->second;
                  else
                        if (leftmost->x() > i->second->x())
                              leftmost = i->second;

                  // Check rightmost:
                  if (!rightmost)
                        rightmost = i->second;
                  else
                        if (rightmost->x() < i->second->x())
                              rightmost = i->second;
                  }
            }

            int left_tick = leftmost->part()->tick();
            int right_tick = rightmost->part()->tick() + rightmost->part()->lenTick();
            Pos p1(left_tick, true);
            Pos p2(right_tick, true);
            song->setPos(1, p1);
            song->setPos(2, p2);
            return;
            }

      // Select part to the right
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
            if (key == shortcuts[SHRT_SEL_RIGHT_ADD].key)
                  add = true;

            Part* part = curItem->part();
            Track* track = part->track();
            unsigned int tick = part->tick();
            bool afterthis = false;
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  NPart* npart = (NPart*)(i->second);
                  Part* ipart = npart->part();
                  if (ipart->track() != track)
                        continue;
                  if (ipart->tick() < tick)
                        continue;
                  if (ipart == part)
                  {
                        afterthis = true;
                        continue;
                  }      
                  if(afterthis)
                  {
                      newItem = i->second;
                      break;
                  }
                  }
            }
      // Select part to the left
      else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key) {
            if (key == shortcuts[SHRT_SEL_LEFT_ADD].key)
                  add = true;

            Part* part = curItem->part();
            Track* track = part->track();
            unsigned int tick = part->tick();

            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  NPart* npart = (NPart*)(i->second);
                  Part* ipart = npart->part();

                  if (ipart->track() != track)
                        continue;
                  if (ipart->tick() > tick)
                        continue;
                  if (ipart == part)
                        break;
                  newItem = i->second;
                  }
            }
      // Select nearest part on track above
      else if (key == shortcuts[SHRT_SEL_ABOVE].key || key == shortcuts[SHRT_SEL_ABOVE_ADD].key) {
            if (key == shortcuts[SHRT_SEL_ABOVE_ADD].key)
                  add = true;
            //To get an idea of which track is above us:
            int stepsize = rmapxDev(1);
            Track* track = curItem->part()->track();//top->part()->track();
            track = y2Track(track->y() - 1);

            //If we're at topmost, leave
            if (!track)
                  return;

            int middle = curItem->x() + curItem->part()->lenTick()/2;
            CItem *aboveL = 0, *aboveR = 0;
            //Upper limit: song end, lower limit: song start
            int ulimit  = song->len();
            int llimit = 0;

            while (newItem == 0) {
                  int y = track->y() + 2;
                  int xoffset = 0;
                  int xleft   = middle - xoffset;
                  int xright  = middle + xoffset;
                  while ((xleft > llimit || xright < ulimit)  && (aboveL == 0) && (aboveR == 0)) {
                        xoffset += stepsize;
                        xleft  = middle - xoffset;
                        xright = middle + xoffset;
                        if (xleft >= 0)
                              aboveL = items.find(QPoint(xleft,y));
                        if (xright <= ulimit)
                              aboveR = items.find(QPoint(xright,y));
                        }

                  if ((aboveL || aboveR) != 0) { //We've hit something
                        CItem* above  = 0;
                        above = (aboveL !=0) ? aboveL : aboveR;
                        newItem = above;
                        }
                  else { //We didn't hit anything. Move to track above, if there is one
                        track = y2Track(track->y() - 1);
                        if (track == 0)
                              return;
                        }
                  }
                  emit trackChanged(track);
            }
      // Select nearest part on track below
      else if (key == shortcuts[SHRT_SEL_BELOW].key || key == shortcuts[SHRT_SEL_BELOW_ADD].key) {
            if (key == shortcuts[SHRT_SEL_BELOW_ADD].key)
                  add = true;

            //To get an idea of which track is below us:
            int stepsize = rmapxDev(1);
            Track* track = curItem->part()->track();//bottom->part()->track();
            track = y2Track(track->y() + track->height() + 1 );
            int middle = curItem->x() + curItem->part()->lenTick()/2;
            //If we're at bottommost, leave
            if (!track)
                  return;

            CItem *belowL = 0, *belowR = 0;
            //Upper limit: song end , lower limit: song start
            int ulimit = song->len();
            int llimit = 0;
            while (newItem == 0) {
                  int y = track->y() + 1;
                  int xoffset = 0;
                  int xleft   = middle - xoffset;
                  int xright  = middle + xoffset;
                  while ((xleft > llimit || xright < ulimit)  && (belowL == 0) && (belowR == 0)) {
                        xoffset += stepsize;
                        xleft  = middle - xoffset;
                        xright = middle + xoffset;
                        if (xleft >= 0)
                              belowL = items.find(QPoint(xleft,y));
                        if (xright <= ulimit)
                              belowR = items.find(QPoint(xright,y));
                        }

                  if ((belowL || belowR) != 0) { //We've hit something
                        CItem* below = 0;
                        below = (belowL !=0) ? belowL : belowR;
                        newItem = below;
                        }
                  else {
                        //Get next track below, or abort if this is the lowest
                        track = y2Track(track->y() + track->height() + 1 );
                        if (track == 0)
                              return;
                        }
                  }
                  emit trackChanged(track);
            }
      else if (key == shortcuts[SHRT_EDIT_PART].key && curItem) { //This should be the other way around - singleSelection first.
            if (!singleSelection) {
                  event->ignore();
                  return;
                  }
            PartList* pl = new PartList;
            NPart* npart = (NPart*)(curItem);
            Track* track = npart->part()->track();
            pl->add(npart->part());
            int type = 0;

            //  Check if track is wave or drum,
            //  else track is midi

            switch (track->type()) {
                  case Track::DRUM:
                        type = 3;
                        break;

                  case Track::WAVE:
                        type = 4;
                        break;

                  case Track::MIDI:
                  case Track::AUDIO_OUTPUT:
                  case Track::AUDIO_INPUT:
                  case Track::AUDIO_GROUP:
                  case Track::AUDIO_AUX:
                  case Track::AUDIO_SOFTSYNTH: //TODO
                        break;
                  }
            emit startEditor(pl, type);
            }
      else {
            event->ignore();  // give global accelerators a chance
            return;
            }


      // Check if anything happened to the selected parts
      if (newItem) {
            //If this is a single selection, toggle previous item
            if (singleSelection && !add)
                  selectItem(curItem, false);
            else if(!add)
                  deselectAll();

            curItem = newItem;
            selectItem(newItem, true);

            //Check if we've hit the upper or lower boundaries of the window. If so, set a new position
            if (newItem->x() < mapxDev(0)) {
                  int curpos = pos[0];
                  setPos(0,newItem->x(),true);
                  setPos(0,curpos,false); //Dummy to put the current position back once we've scrolled
                  }
            else if (newItem->x() > mapxDev(width())) {
                  int curpos = pos[0];
                  setPos(0,newItem->x(),true);
                  setPos(0,curpos,false); //Dummy to put the current position back once we've scrolled
                  }
            redraw();
            }
      }

//---------------------------------------------------------
//   drawPart
//    draws a part
//---------------------------------------------------------

void PartCanvas::drawItem(QPainter& p, const CItem* item, const QRect& rect)
      {
      int from   = rect.x();
      int to     = from + rect.width();

      Part* part = ((NPart*)item)->part();
      int pTick  = part->tick();
      from      -= pTick;
      to        -= pTick;
      if(from < 0)
        from = 0;
      if((unsigned int)to > part->lenTick())
        to = part->lenTick();  

      QRect r    = item->bbox();
      //QRect r    = item->bbox().intersect(rect);
      int i      = part->colorIndex();

      // Added by Tim. p3.3.6
      //printf("PartCanvas::drawItem %s evRefs:%d pTick:%d pLen:%d bb.x:%d bb.w:%d rect.x:%d rect.w:%d r.x:%d r.w:%d\n", part->name().latin1(), part->events()->arefCount(), pTick, part->lenTick(), item->bbox().x(), item->bbox().width(), rect.x(), rect.width(), r.x(), r.width());
  
      // Must be reasonable about very low negative x values! With long songs > 15min
      //  and with high horizontal magnification, 'ghost' drawings appeared,
      //  apparently the result of truncation later (xp = -65006 caused ghosting
      //  at bar 245 with magnification at max.), even with correct clipping region
      //  applied to painter in View::paint(). Tim.  Apr 5 2009 
      // Quote: "Warning: Note that QPainter does not attempt to work around 
      //  coordinate limitations in the underlying window system. Some platforms may 
      //  behave incorrectly with coordinates as small as +/-4000."
      //if(r.isEmpty())
      //  return;
        
      p.setPen(black);
      if (part->mute()) {
            p.setBrush(gray);
            p.drawRect(r);
            return;
            }
      if (item->isMoving()) {
            p.setBrush(gray);
            p.drawRect(r);
            }
      //else if (part->mute())
      //      return;
      else if (part->selected()) {
            bool clone = part->events()->arefCount() > 1;
            //p.setPen(config.partColors[i]);
            p.setPen(QPen(config.partColors[i], 2, clone ? DashLine : SolidLine));
            p.setBrush(black);
            p.drawRect(r);
            }
      else {
            bool clone = part->events()->arefCount() > 1;
            p.setPen(QPen(black, 2, clone ? DashLine : SolidLine));
            p.setBrush(config.partColors[i]);
            p.drawRect(r);
            }
      
      MidiPart* mp = 0;
      WavePart* wp = 0;
      Track::TrackType type = part->track()->type();
      if (type == Track::WAVE) {
            wp =(WavePart*)part;
            }
      else {
            mp = (MidiPart*)part;
            }

      if (config.canvasShowPartType & 2) {      // show events
            if (mp) 
            {
                  // Do not allow this, causes segfault.
                  if(from <= to)
                  {
                    p.setPen(darkGray);
                    EventList* events = mp->events();
                    iEvent ito(events->lower_bound(to));
                    
                    for (iEvent i = events->lower_bound(from); i != ito; ++i) {
                          EventType type = i->second.type();
                          if (
                            ((config.canvasShowPartEvent & 1) && (type == Note))
                            || ((config.canvasShowPartEvent & 2) && (type == PAfter))
                            || ((config.canvasShowPartEvent & 4) && (type == Controller))
                            || ((config.canvasShowPartEvent &16) && (type == CAfter))
                            || ((config.canvasShowPartEvent &64) && (type == Sysex || type == Meta))
                            ) {
                                int t = i->first + pTick;
                                int th = part->track()->height();
                                if(t >= r.left() && t <= r.right())
                                  p.drawLine(t, r.y()+2, t, r.y()+th-4);
                                }
                          }
                  }      
            }
            else if (wp)
                  drawWavePart(p, rect, wp, r);
            }

      else {      // show Cakewalk Style
            if (mp) {
                  p.setPen(darkGray);
                  EventList* events = mp->events();
                  iEvent ito(events->lower_bound(to));
                  // Added by Tim. P3.3.6
                  //printf("PartCanvas::drawItem pTick:%d from:%d to:%d part len:%d\n", pTick, from, to, part->lenTick());
                  
                  for (iEvent i = events->begin(); i != ito; ++i) {
                        int t  = i->first + pTick;
                        int te = t + i->second.lenTick();

                        if (t > (to + pTick))
                        {
                          // Added by Tim. P3.3.6
                          printf("PartCanvas::drawItem t:%d > to:%d + pTick:%d i->first:%d\n", t, to, pTick, i->first);
                          
                          break;
                        }
                        
                        if (te < (from + pTick))
                              continue;

                        if (te > (to + pTick))
                              te = to + pTick;

                        EventType type = i->second.type();
                        if (type == Note) {
                              int pitch = i->second.pitch();
                              int th = int(part->track()->height() * 0.75); // only draw on three quarters
                              int hoffset = (part->track()->height() - th ) / 2; // offset from bottom
                              int y     =  hoffset + (r.y() + th - (pitch * (th) / 127));
                              p.drawLine(t, y, te, y);
                              }
                        }
                  }
            else if (wp)
                  drawWavePart(p, rect, wp, r);
            }
      if (config.canvasShowPartType & 1) {     // show names
            // draw name
            // FN: Set text color depending on part color (black / white)
            int part_r, part_g, part_b, brightness;
            config.partColors[i].getRgb(&part_r, &part_g, &part_b);
            brightness =  part_r*29 + part_g*59 + part_b*12;
            if (brightness < 12000 || part->selected())
              p.setPen(white);   /* too dark: use white for text color */
            else
              p.setPen(black);  /* otherwise use black */
            QRect rr = map(r);
            rr.setX(rr.x() + 3);
            p.save();
            p.setFont(config.fonts[1]);
            p.setWorldXForm(false);
            p.drawText(rr, AlignVCenter|AlignLeft, part->name());
            p.restore();
            }
      }

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void PartCanvas::drawMoving(QPainter& p, const CItem* item, const QRect&)
      {
        //if(!item->isMoving()) 
        //  return;
        p.setPen(black);
        p.setBrush(NoBrush);
        p.drawRect(item->mp().x(), item->mp().y()+1, item->width(), item->height());
      }

//---------------------------------------------------------
//   drawWavePart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void PartCanvas::drawWavePart(QPainter& p,
   const QRect& bb, WavePart* wp, const QRect& _pr)
      {
      QRect rr = p.worldMatrix().map(bb);
      QRect pr = p.worldMatrix().map(_pr);

      p.save();
      p.resetXForm();

      int x2 = 1;
      int x1  = rr.x() > pr.x() ? rr.x() : pr.x();
      x2 += rr.right() < pr.right() ? rr.right() : pr.right();

      if (x1 < 0)
            x1 = 0;
      if (x2 > width())
            x2 = width();
      int hh = pr.height();
      int h  = hh/2;
      int y  = pr.y() + h;

      EventList* el = wp->events();
      for (iEvent e = el->begin(); e != el->end(); ++e) {
            int cc = hh % 2 ? 0 : 1;
            Event event = e->second;
            SndFileR f  = event.sndFile();
            if (f.isNull())
                  continue;
            unsigned channels = f.channels();
            if (channels == 0) {
                  printf("drawWavePart: channels==0! %s\n", f.name().latin1());
                  continue;
                  }

            int xScale;
            int pos;
            int tickstep = rmapxDev(1);
            int postick = tempomap.frame2tick(wp->frame() + event.frame());
            int eventx = mapx(postick);
            int drawoffset;
            if((x1 - eventx) < 0)
              drawoffset = 0;
            else
              drawoffset = rmapxDev(x1 - eventx);
              postick += drawoffset;
            pos = event.spos() + tempomap.tick2frame(postick) - wp->frame() - event.frame();
            
            int i;
            if(x1 < eventx)
              i = eventx;
            else  
              i = x1;
            int ex = mapx(tempomap.frame2tick(wp->frame() + event.frame() + event.lenFrame()));
            if(ex > x2)
              ex = x2;
            if (h < 20) {
                  //
                  //    combine multi channels into one waveform
                  //
                  for (; i < ex; i++) {
                        SampleV sa[channels];
                        xScale = tempomap.deltaTick2frame(postick, postick + tickstep);
                        f.read(sa, xScale, pos);
                        postick += tickstep;
                        pos += xScale;
                        int peak = 0;
                        int rms  = 0;
                        for (unsigned k = 0; k < channels; ++k) {
                              if (sa[k].peak > peak)
                                    peak = sa[k].peak;
                              rms += sa[k].rms;
                              }
                        rms /= channels;
                        peak = (peak * (hh-2)) >> 9;
                        rms  = (rms  * (hh-2)) >> 9;
                        p.setPen(QColor(darkGray));
                        p.drawLine(i, y - peak - cc, i, y + peak);
                        p.setPen(QColor(black));
                        p.drawLine(i, y - rms - cc, i, y + rms);
                        }
                  }
            else {
                  //
                  //  multi channel display
                  //
                  int hm = hh / (channels * 2);
                  int cc = hh % (channels * 2) ? 0 : 1;
                  for (; i < ex; i++) {
                        y  = pr.y() + hm;
                        SampleV sa[channels];
                        xScale = tempomap.deltaTick2frame(postick, postick + tickstep);
                        f.read(sa, xScale, pos);
                        postick += tickstep;
                        pos += xScale;
                        for (unsigned k = 0; k < channels; ++k) {
                              int peak = (sa[k].peak * (hm - 1)) >> 8;
                              int rms  = (sa[k].rms  * (hm - 1)) >> 8;
                              p.setPen(QColor(darkGray));
                              p.drawLine(i, y - peak - cc, i, y + peak);
                              p.setPen(QColor(black));
                              p.drawLine(i, y - rms - cc, i, y + rms);
                              
                              y  += 2 * hm;
                              }
                        }
                  }
            }
      p.restore();
      }
//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void PartCanvas::cmd(int cmd)
      {
      PartList pl;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!i->second->isSelected())
                  continue;
            NPart* npart = (NPart*)(i->second);
            pl.add(npart->part());
            }
      switch (cmd) {
            case CMD_CUT_PART:
                  copy(&pl);
                  song->startUndo();
                  
                  bool loop;
                  do
                  {
                    loop = false;
                    for (iCItem i = items.begin(); i != items.end(); ++i) {
                          if (!i->second->isSelected())
                                continue;
                          NPart* p = (NPart*)(i->second);
                          Part* part = p->part();
                          audio->msgRemovePart(part);
                          
                          loop = true;
                          break;
                        }
                  } while (loop);
                  song->endUndo(SC_PART_REMOVED);
                  break;
            case CMD_COPY_PART:
                  copy(&pl);
                  break;
            case CMD_PASTE_PART:
                  paste(false, false);
                  break;
            case CMD_PASTE_CLONE_PART:
                  paste(true, false);
                  break;
            case CMD_PASTE_PART_TO_TRACK:
                  paste();
                  break;
            case CMD_PASTE_CLONE_PART_TO_TRACK:
                  paste(true);
                  break;
            }
      }

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void PartCanvas::copy(PartList* pl)
      {
      //printf("void PartCanvas::copy(PartList* pl)\n");
      if (pl->empty())
            return;
      // Changed by T356. Support mixed .mpt files.
      //bool isWave = pl->begin()->second->track()->type() == Track::WAVE;
      bool wave = false;
      bool midi = false;
      for(ciPart p = pl->begin(); p != pl->end(); ++p) 
      {
        if(p->second->track()->isMidiTrack())
          midi = true;
        else
        if(p->second->track()->type() == Track::WAVE)
          wave = true;
        if(midi && wave)
          break;  
      }
      if(!(midi || wave))
        return;
        
      //---------------------------------------------------
      //    write parts as XML into tmp file
      //---------------------------------------------------

      FILE* tmp = tmpfile();
      if (tmp == 0) {
            fprintf(stderr, "PartCanvas::copy() fopen failed: %s\n",
               strerror(errno));
            return;
            }
      Xml xml(tmp);

      // Clear the copy clone list.
      cloneList.clear();
      //copyCloneList.clear();
      
      int level = 0;
      int tick = 0;
      for (ciPart p = pl->begin(); p != pl->end(); ++p) {
            // Indicate this is a copy operation. Also force full wave paths.
            //p->second->write(level, xml);
            p->second->write(level, xml, true, true);
            
            int endTick = p->second->endTick();
            if (endTick > tick)
                  tick = endTick;
            }
      Pos p(tick, true);
      song->setPos(0, p);

      //---------------------------------------------------
      //    read tmp file into QTextDrag Object
      //---------------------------------------------------

      fflush(tmp);
      struct stat f_stat;
      if (fstat(fileno(tmp), &f_stat) == -1) {
            fprintf(stderr, "PartCanvas::copy() fstat failed:<%s>\n",
               strerror(errno));
            fclose(tmp);
            return;
            }
      int n = f_stat.st_size;
      char* fbuf  = (char*)mmap(0, n+1, PROT_READ|PROT_WRITE,
         MAP_PRIVATE, fileno(tmp), 0);
      fbuf[n] = 0;
      QTextDrag* drag = new QTextDrag(QString(fbuf));
      // Changed by T356. Support mixed .mpt files.
      //drag->setSubtype(QCString(isWave ? "wavepartlist" : "midipartlist"));
      if(midi && wave)
        drag->setSubtype(QCString("mixedpartlist"));
      else
      if(midi)
        drag->setSubtype(QCString("midipartlist"));
      else
      if(wave)
        drag->setSubtype(QCString("wavepartlist"));
        
      QApplication::clipboard()->setData(drag, QClipboard::Clipboard);
      munmap(fbuf, n);
      fclose(tmp);
      }

//---------------------------------------------------------
//   pasteAt
//---------------------------------------------------------

int PartCanvas::pasteAt(const QString& pt, Track* track, int pos, bool clone, bool toTrack)
      {
      //printf("int PartCanvas::pasteAt(const QString& pt, Track* track, int pos)\n");
      const char* ptxt = pt.latin1();
      Xml xml(ptxt);
      bool firstPart=true;
      int  posOffset=0;
      //int  finalPos=0;
      int  finalPos = pos;
      int  notDone = 0;
      int  done = 0;
      bool end = false;

      song->startUndo();
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        end = true;
                        break;
                  case Xml::TagStart:
                        if (tag == "part") {
                              /*
                              Part* p = 0;
                              if(clone)
                              {
                                if(!(p = readClone(xml, track, toTrack)))
                                  break;
                              }
                              else
                              {
                                if (track->type() == Track::MIDI || track->type() == Track::DRUM)
                                  p = new MidiPart((MidiTrack*)track);
                                else if (track->type() == Track::WAVE)
                                  p = new WavePart((WaveTrack*)track);
                                else
                                  break;
                                p->read(xml, 0, toTrack);
                              }
                              */
                              
                              // Read the part.
                              Part* p = 0;
                              p = readXmlPart(xml, track, clone, toTrack);
                              // If it could not be created...
                              if(!p)
                              {
                                // Increment the number of parts not done and break.
                                ++notDone;
                                break;
                              } 
                              
                              // Increment the number of parts done.
                              ++done;
                              
                              if (firstPart) {
                                    firstPart=false;
                                    posOffset=pos-p->tick();
                                    }
                              p->setTick(p->tick()+posOffset);
                              finalPos=p->tick()+p->lenTick();
                              //pos += p->lenTick();
                              audio->msgAddPart(p,false);
                              }
                        else
                              xml.unknown("PartCanvas::pasteAt");
                        break;
                  case Xml::TagEnd:
                        break;
                  default:
                              end = true;
                        break;
                }
                if(end)
                  break;
            }
      
      song->endUndo(SC_PART_INSERTED);
      //return pos;
      
      if(notDone)
      {
        int tot = notDone + done;
        QMessageBox::critical(this, QString("MusE"),
           QString().setNum(notDone) + (tot > 1 ? (tr(" out of ") + QString().setNum(tot)) : QString("")) + 
           (tot > 1 ? tr(" parts") : tr(" part")) + 
           tr(" could not be pasted.\nLikely the selected track is the wrong type."));
      }
      
      return finalPos;
      }

/*
//---------------------------------------------------------
//   PartCanvas::readPart
//---------------------------------------------------------

Part* PartCanvas::readPart(Xml& xml, Track* track, bool doClone, bool toTrack)
      {
      int id = -1;
      Part* npart = 0;
      uuid_t uuid; 
      uuid_clear(uuid);
      bool uuidvalid = false;
      bool clone = true;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return npart;
                  case Xml::TagStart:
                        // If the part has not been created yet...
                        if(!npart)
                        {
                          // Attribute section did not create a clone from any matching part. Create a non-clone part now.
                          if(!track)
                          {
                            xml.skip("part");
                            return 0;
                          }  
                          if (track->type() == Track::MIDI || track->type() == Track::DRUM)
                            npart = new MidiPart((MidiTrack*)track);
                          else if (track->type() == Track::WAVE)
                            npart = new WavePart((WaveTrack*)track);
                          else
                          {
                            xml.skip("part");
                            return 0;
                          }  
                          
                          // Signify a new non-clone part was created.
                          // Even if the original part was itself a clone, clear this because the
                          //  attribute section did not create a clone from any matching part.
                          clone = false;
                          
                          // If an id or uuid was found, add the part to the clone list 
                          //  so that subsequent parts can look it up and clone from it...
                          if(id != -1)
                          {
                            ClonePart ncp(npart, id);
                            cloneList.push_back(ncp);
                          }
                          else  
                          if(uuidvalid)
                          {
                            ClonePart ncp(npart);
                            // New ClonePart creates its own uuid, but we need to replace it.
                            uuid_copy(ncp.uuid, uuid);
                            cloneList.push_back(ncp);
                          }
                        }
                        
                        if (tag == "name")
                              npart->setName(xml.parse1());
                        else if (tag == "poslen") {
                              ((PosLen*)npart)->read(xml, "poslen");
                              }
                        else if (tag == "pos") {
                              Pos pos;
                              pos.read(xml, "pos");  // obsolete
                              npart->setTick(pos.tick());
                              }
                        else if (tag == "len") {
                              Pos len;
                              len.read(xml, "len");  // obsolete
                              npart->setLenTick(len.tick());
                              }
                        else if (tag == "selected")
                              npart->setSelected(xml.parseInt());
                        else if (tag == "color")
                              npart->setColorIndex(xml.parseInt());
                        else if (tag == "mute")
                              npart->setMute(xml.parseInt());
                        else if (tag == "event") 
                        {
                              // If a new non-clone part was created, accept the events...
                              if(!clone)
                              {
                                EventType type = Wave;
                                if(track->isMidiTrack())
                                  type = Note;
                                Event e(type);
                                e.read(xml);
                                // stored tickpos for event has absolute value. However internally
                                // tickpos is relative to start of part, we substract tick().
                                // TODO: better handling for wave event
                                e.move( -npart->tick() );
                                int tick = e.tick();  
                                
                                // Do not discard events belonging to clone parts,
                                //  at least not yet. A later clone might have a longer, 
                                //  fully accommodating part length!
                                //if ((tick < 0) || (tick >= (int) lenTick())) {
                                //if ((tick < 0) || ( id == -1 && !clone && (tick >= (int)lenTick()) )) 
                                // No way to tell at the moment whether there will be clones referencing this...
                                // No choice but to accept all events past 0.
                                if(tick < 0) 
                                {
                                  //printf("readClone: warning: event not in part: %d - %d -%d, discarded\n",
                                  printf("readClone: warning: event at tick:%d not in part:%s, discarded\n",
                                    tick, npart->name().latin1());
                                }
                                else 
                                {
                                  npart->events()->add(e);
                                }      
                              }
                              else
                                // ...Otherwise a clone was created, so we don't need the events.
                                xml.skip(tag);
                        }
                        else
                              xml.unknown("PartCanvas::readClone");
                        break;
                  case Xml::Attribut:
                        if (tag == "cloneId")
                        {
                          id = xml.s2().toInt();
                          if(id != -1)
                          {
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(i->id == id) 
                              {
                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                if(!doClone && i->cp->cevents()->arefCount() <= 1)
                                  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }  
                        }      
                        else if (tag == "uuid")
                        {
                          uuid_parse(xml.s2().latin1(), uuid);
                          if(!uuid_is_null(uuid))
                          {
                            uuidvalid = true;
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(uuid_compare(uuid, i->uuid) == 0) 
                              {
                                Track* cpt = i->cp->track();
                                // If we want to paste to the given track...
                                if(toTrack)
                                {
                                  // If the given track type is not the same as the part's 
                                  //  original track type, we can't continue. Just return.
                                  if(!track || cpt->type() != track->type())
                                  {
                                    xml.skip("part");
                                    return 0;
                                  }  
                                }
                                else
                                // ...else we want to paste to the part's original track.
                                {
                                  // Make sure the track exists (has not been deleted).
                                  if((cpt->isMidiTrack() && song->midis()->find(cpt) != song->midis()->end()) || 
                                     (cpt->type() == Track::WAVE && song->waves()->find(cpt) != song->waves()->end()))
                                    track = cpt;   
                                  else
                                  // Track was not found. Try pasting to the given track, as above...
                                  {
                                    if(!track || cpt->type() != track->type())
                                    {
                                      // No luck. Just return.
                                      xml.skip("part");
                                      return 0;
                                    }  
                                  }
                                }
                                
                                // If it's a regular paste (not paste clone), and the original part is
                                //  not a clone, defer so that a new copy is created in TagStart above.
                                if(!doClone && i->cp->cevents()->arefCount() <= 1)
                                  break;
                                  
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }  
                          }
                        }      
                        //else if(tag == "isclone")        // Ignore
                        //  clone = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "part") 
                          return npart;
                  default:
                        break;
                  }
            }
  return npart;            
}
*/

/*
//---------------------------------------------------------
//   PartCanvas::readClone
//---------------------------------------------------------

Part* PartCanvas::readClone(Xml& xml, Track* track, bool toTrack)
      {
      int id = -1;
      Part* npart = 0;
      uuid_t uuid; 
      uuid_clear(uuid);
      bool uuidvalid = false;
      bool clone = true;

      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return npart;
                  case Xml::TagStart:
                        // If the part has not been created yet...
                        if(!npart)
                        {
                          // Attribute section did not create a clone from any matching part. Create a non-clone part now.
                          if (track->type() == Track::MIDI || track->type() == Track::DRUM)
                            npart = new MidiPart((MidiTrack*)track);
                          else if (track->type() == Track::WAVE)
                            npart = new WavePart((WaveTrack*)track);
                          else
                            return 0;
                            
                          // Signify a new non-clone part was created.
                          // Even if the original part was itself a clone, clear this because the
                          //  attribute section did not create a clone from any matching part.
                          clone = false;
                          
                          // If an id or uuid was found, add the part to the clone list 
                          //  so that subsequent parts can look it up and clone from it...
                          if(id != -1)
                          {
                            ClonePart ncp(npart, id);
                            cloneList.push_back(ncp);
                          }
                          else  
                          if(uuidvalid)
                          {
                            ClonePart ncp(npart);
                            // New ClonePart creates its own uuid, but we need to replace it.
                            uuid_copy(ncp.uuid, uuid);
                            cloneList.push_back(ncp);
                          }
                        }
                        
                        if (tag == "name")
                              npart->setName(xml.parse1());
                        else if (tag == "poslen") {
                              ((PosLen*)npart)->read(xml, "poslen");
                              }
                        else if (tag == "pos") {
                              Pos pos;
                              pos.read(xml, "pos");  // obsolete
                              npart->setTick(pos.tick());
                              }
                        else if (tag == "len") {
                              Pos len;
                              len.read(xml, "len");  // obsolete
                              npart->setLenTick(len.tick());
                              }
                        else if (tag == "selected")
                              npart->setSelected(xml.parseInt());
                        else if (tag == "color")
                              npart->setColorIndex(xml.parseInt());
                        else if (tag == "mute")
                              npart->setMute(xml.parseInt());
                        else if (tag == "event") 
                        {
                              // If a new non-clone part was created, accept the events...
                              if(!clone)
                              {
                                EventType type = Wave;
                                if(track->isMidiTrack())
                                  type = Note;
                                Event e(type);
                                e.read(xml);
                                // stored tickpos for event has absolute value. However internally
                                // tickpos is relative to start of part, we substract tick().
                                // TODO: better handling for wave event
                                e.move( -npart->tick() );
                                int tick = e.tick();  
                                
                                // Do not discard events belonging to clone parts,
                                //  at least not yet. A later clone might have a longer, 
                                //  fully accommodating part length!
                                //if ((tick < 0) || (tick >= (int) lenTick())) {
                                //if ((tick < 0) || ( id == -1 && !clone && (tick >= (int)lenTick()) )) 
                                // No way to tell at the moment whether there will be clones referencing this...
                                // No choice but to accept all events past 0.
                                if(tick < 0) 
                                {
                                  //printf("readClone: warning: event not in part: %d - %d -%d, discarded\n",
                                  printf("readClone: warning: event at tick:%d not in part:%s, discarded\n",
                                    tick, npart->name().latin1());
                                }
                                else 
                                {
                                  npart->events()->add(e);
                                }      
                              }
                              else
                                // ...Otherwise a clone was created, so we don't need the events.
                                xml.skip(tag);
                        }
                        else
                              xml.unknown("PartCanvas::readClone");
                        break;
                  case Xml::Attribut:
                        if (tag == "cloneId")
                        {
                          id = xml.s2().toInt();
                          if(id != -1)
                          {
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(i->id == id) 
                              {
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }
                          }  
                        }      
                        else if (tag == "uuid")
                        {
                          uuid_parse(xml.s2().latin1(), uuid);
                          if(!uuid_is_null(uuid))
                          {
                            uuidvalid = true;
                            for(iClone i = cloneList.begin(); i != cloneList.end(); ++i) 
                            {
                              // Is a matching part found in the clone list?
                              if(uuid_compare(uuid, i->uuid) == 0) 
                              {
                                // If we want to paste to the part's original track...
                                if(!toTrack)
                                {
                                  // Make sure the track exists (has not been deleted).
                                  if((i->cp->track()->isMidiTrack() && song->midis()->find(i->cp->track()) != song->midis()->end()) || 
                                    (i->cp->track()->type() == Track::WAVE && song->waves()->find(i->cp->track()) != song->waves()->end()))
                                    track = i->cp->track();
                                }
                                // This makes a clone, chains the part, and increases ref counts.
                                npart = track->newPart((Part*)i->cp, true);
                                break;
                              }
                            }  
                          }
                        }      
                        //else if(tag == "isclone")        // Ignore
                        //  clone = xml.s2().toInt();
                        break;
                  case Xml::TagEnd:
                        if (tag == "part") 
                          return npart;
                  default:
                        break;
                  }
            }
  return npart;            
}
*/

//---------------------------------------------------------
//   paste
//    paste part to current selected track at cpos
//---------------------------------------------------------

//void PartCanvas::paste()
void PartCanvas::paste(bool clone, bool toTrack)
{
      Track* track = 0;
      // If we want to paste to a selected track...
      if(toTrack)
      {  
        TrackList* tl = song->tracks();
        for (iTrack i = tl->begin(); i != tl->end(); ++i) {
              if ((*i)->selected()) {
                    if (track) {
                          QMessageBox::critical(this, QString("MusE"),
                            tr("Cannot paste: multiple tracks selected"));
                          return;
                          }
                    else
                          track = *i;
                    }
              }
        if (track == 0) {
              QMessageBox::critical(this, QString("MusE"),
                tr("Cannot paste: no track selected"));
              return;
              }
      }

      QClipboard* cb  = QApplication::clipboard();
      QMimeSource* ms = cb->data(QClipboard::Clipboard);

      bool midiPart = false;
      bool wavePart = false;

      // If we want to paste to a selected track...
      if(toTrack)
      {
        for (int i = 0; const char* format = ms->format(i); ++i) {
              format = ms->format(i);
              if (strcmp(format, "text/midipartlist") == 0) {
                    if (!track->isMidiTrack()) {
                          QMessageBox::critical(this, QString("MusE"),
                                tr("Can only paste to midi/drum track"));
                          return;
                          }
                    midiPart = true;
                    }
              else if (strcmp(format, "text/wavepartlist") == 0) {
                    if (track->type() != Track::WAVE) {
                          QMessageBox::critical(this, QString("MusE"),
                          tr("Can only paste to wave track"));
                          return;
                          }
                    wavePart = true;
                    }
              // Added by T356. Support mixed .mpt files.
              else if (strcmp(format, "text/mixedpartlist") == 0) {
                    if (!track->isMidiTrack() && track->type() != Track::WAVE) {
                          QMessageBox::critical(this, QString("MusE"),
                          tr("Can only paste to midi or wave track"));
                          return;
                          }
                    midiPart = true;
                    wavePart = true;
                    }
              }
  
        if (!(midiPart || wavePart)) {
              QMessageBox::critical(this, QString("MusE"),
                tr("Cannot paste: wrong data type"));
              return;
              }
      }      
            
      QCString subtype = 0;
      QString txt = cb->text(subtype);
      if (!txt.isEmpty()) 
      {
        Pos p(pasteAt(txt, track, song->vcpos(), clone, toTrack), true);
        song->setPos(0, p);
      }
    }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PartCanvas::startDrag(CItem* item, DragType t)
      {
      //printf("PartCanvas::startDrag(CItem* item, DragType t)\n");
      NPart* p = (NPart*)(item);
      Part* part = p->part();

      //---------------------------------------------------
      //    write part as XML into tmp file
      //---------------------------------------------------

      FILE* tmp = tmpfile();
      if (tmp == 0) {
            fprintf(stderr, "PartCanvas::startDrag() fopen failed: %s\n",
               strerror(errno));
            return;
            }
      Xml xml(tmp);
      int level = 0;
      part->write(level, xml);

      //---------------------------------------------------
      //    read tmp file into QTextDrag Object
      //---------------------------------------------------

      fflush(tmp);
      struct stat f_stat;
      if (fstat(fileno(tmp), &f_stat) == -1) {
            fprintf(stderr, "PartCanvas::startDrag fstat failed:<%s>\n",
               strerror(errno));
            fclose(tmp);
            return;
            }
      int n = f_stat.st_size + 1;
      char* fbuf  = (char*)mmap(0, n, PROT_READ|PROT_WRITE,
         MAP_PRIVATE, fileno(tmp), 0);
      fbuf[n] = 0;
      QTextDrag* drag = new QTextDrag(QString(fbuf), this);
      drag->setSubtype("partlist");
      if (t == MOVE_COPY || t == MOVE_CLONE)
            drag->dragCopy();
      else
            drag->dragMove();
      munmap(fbuf, n);
      fclose(tmp);
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void PartCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      event->accept(QTextDrag::canDecode(event));
      }

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void PartCanvas::dragMoveEvent(QDragMoveEvent*)
      {
//      printf("drag move %x\n", this);
      }

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void PartCanvas::dragLeaveEvent(QDragLeaveEvent*)
      {
//      printf("drag leave\n");
      }

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void PartCanvas::viewDropEvent(QDropEvent* event)
      {
      //printf("void PartCanvas::viewDropEvent(QDropEvent* event)\n");
      if (event->source() == this) {
            printf("no local DROP\n");
            return;
            }
      int type = 0;     // 0 = unknown, 1 = partlist, 2 = uri-list
      QString text;
      for (int i = 0; ; ++i) {
            const char* p= event->format(i);
            if (p == 0)
                  break;
            if (strncmp(p, "text/partlist", 13) == 0) {
                  type = 1;
                  break;
                  }
            else if (strcmp(p, "text/uri-list") == 0) {
                  type = 2;
                  break;
                  }
            else {
                  if (debugMsg)
                        printf("unknown drop format <%s>\n", p);
                  }
            }
      if (type == 0)
            return;
      
      // Make a backup of the current clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      CloneList copyCloneList = cloneList;
      // Clear the clone list to prevent any dangerous associations with
      //  current non-original parts.
      cloneList.clear();
      
      if (QTextDrag::decode(event, text)) {
            if (type == 1) {
                  int x = sigmap.raster(event->pos().x(), *_raster);
                  if (x < 0)
                        x = 0;
                  unsigned trackNo = y2pitch(event->pos().y());
                  Track* track = 0;
                  if (trackNo < tracks->size())
                        track = tracks->index(trackNo);
                  if (track)
                        pasteAt(text, track, x);
                  }
            else if (type == 2) {
                  text = text.stripWhiteSpace();
                  if (text.endsWith(".wav",false) || text.endsWith(".ogg",false) || text.endsWith(".mpt", false) )
                      {
                      int x = sigmap.raster(event->pos().x(), *_raster);
                      if (x < 0)
                            x = 0;
                      unsigned trackNo = y2pitch(event->pos().y());
                      Track* track = 0;
                      if (trackNo < tracks->size())
                            track = tracks->index(trackNo);
                      if (track)
                          {
                          QUrl url(text);
                          QString newPath = url.path();
                          if (track->type() == Track::WAVE && (text.endsWith(".wav", false) || (text.endsWith(".ogg", false))))
                              {
                              unsigned tick = x;
                              muse->importWaveToTrack(newPath, tick, track);
                              }
                           // Changed by T356. Support mixed .mpt files.
                           //else if ((track->type() == Track::MIDI || track->type() == Track::DRUM) && text.endsWith(".mpt", false))
                           else if ((track->isMidiTrack() || track->type() == Track::WAVE) && text.endsWith(".mpt", false))
                              {
                              unsigned tick = x;
                              muse->importPartToTrack(newPath, tick, track);
                              }
                          }
                      }
                  else if(text.endsWith(".med",false))
                      {
                      QUrl url(text);
                      emit dropSongFile(url.path());
                      }
                  else if(text.endsWith(".mid",false))
                      {
                      QUrl url(text);
                      emit dropMidiFile(url.path());
                      }
                  else
                      {
                      printf("dropped... something...  no hable...\n");
                      }
                  }
            }
      // Restore backup of the clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      cloneList.clear();
      cloneList = copyCloneList;
      }

//---------------------------------------------------------
//   drawCanvas
//---------------------------------------------------------

void PartCanvas::drawCanvas(QPainter& p, const QRect& rect)
      {
      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();

      //////////
      // GRID //
      //////////
      QColor baseColor(config.partCanvasBg.light(110));
      p.setPen(baseColor);

      //--------------------------------
      // vertical lines
      //-------------------------------
      //printf("raster=%d\n", *_raster);
      if (config.canvasShowGrid) {
          int bar, beat;
          unsigned tick;
          switch (*_raster) {
                case 0:     // measure
                      sigmap.tickValues(x, &bar, &beat, &tick);
                      for (;;) {
                            int xt = sigmap.bar2tick(bar++, 0, 0);
                            if (xt >= x + w)
                                  break;
                            if (!((bar-1) % 4))
                                p.setPen(baseColor.dark(130));
                            else
                                p.setPen(baseColor);
                            p.drawLine(xt, y, xt, y+h);
                      }
                      break;
                case 1:           // no raster
                      break;
                case 768:         // 1/2
                case 384:         // 1/4
                case 192:         // 1/8
                case 96:          // 1/16
                {
                      int r = *_raster;
                      int rr = rmapx(r);
                      while (rr < 4) {
                            r *= 2;
                            rr = rmapx(r);
                      }

                      for (int xt = x; xt < (x + w); xt += r)
                            p.drawLine(xt, y, xt+1, y+h);
                }
                break;
          }
      }
      //--------------------------------
      // horizontal lines
      //--------------------------------

      TrackList* tl = song->tracks();
      int yy = 0;
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
            if (yy > y + h)
                  break;
            Track* track = *it;
            if (/*config.canvasShowGrid ||*/ !track->isMidiTrack()) {
              p.setPen(baseColor.dark(130));
              p.drawLine(x, yy, x + w, yy);
              p.setPen(baseColor);
            }
            if (!track->isMidiTrack() && (track->type() != Track::WAVE)) {
                  QRect r = rect & QRect(x, yy, w, track->height());
                  drawAudioTrack(p, r, (AudioTrack*)track);
                  p.setPen(baseColor);
                  }
            yy += track->height();
            }
      }

//---------------------------------------------------------
//   drawAudioTrack
//---------------------------------------------------------

void PartCanvas::drawAudioTrack(QPainter& p, const QRect& r, AudioTrack*)
      {
      p.setPen(QPen(black, 2, SolidLine));
      p.setBrush(gray);
      p.drawRect(r);
      }

