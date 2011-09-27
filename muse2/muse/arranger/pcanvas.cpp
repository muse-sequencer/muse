//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pcanvas.cpp,v 1.48.2.26 2009/11/22 11:08:33 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on users DOT sourceforge DOT net)
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
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <values.h>
#include <uuid/uuid.h>
#include <math.h>
#include <map>

#include <QClipboard>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
//#include <QPainter>
#include <QUrl>
#include <QPoint>

#include "fastlog.h"
#include "widgets/tools.h"
#include "arranger.h"
#include "arrangerview.h"
#include "structure.h"
#include "pcanvas.h"
#include "midieditor.h"
#include "globals.h"
#include "icons.h"
#include "event.h"
//#include "xml.h"
#include "wave.h"
#include "audio.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "app.h"
#include "filedialog.h"
#include "marker/marker.h"
#include "mpevent.h"
#include "midievent.h"
#include "midi.h"
#include "midictrl.h"
#include "utils.h"
#include "dialogs.h"
#include "widgets/pastedialog.h"

//#define ABS(x)  ((x) < 0) ? -(x) : (x))
//#define ABS(x) (x>=0?x:-x)
#define ABS(x) (abs(x))

using std::set;

namespace MusEArranger {

//---------------------------------------------------------
//   NPart
//---------------------------------------------------------

NPart::NPart(Part* e) : MusEWidget::CItem(Event(), e)
      {
      leftBorderTouches = false;
      rightBorderTouches = false;
      
      int y  = track()->y();
      setPos(QPoint(e->tick(), y));
      setBBox(QRect(e->tick(), y, e->lenTick(), track()->height()));
      }

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

PartCanvas::PartCanvas(int* r, QWidget* parent, int sx, int sy)
   : MusEWidget::Canvas(parent, sx, sy)
      {
      setAcceptDrops(true);
      _raster = r;

      setFocusPolicy(Qt::StrongFocus);
      // Defaults:
      lineEditor = 0;
      editMode   = false;

      tracks = song->tracks();
      setMouseTracking(true);
      drag          = DRAG_OFF;
      curColorIndex = 0;
      //automation.currentCtrl = 0;
      automation.currentCtrlValid = false;
      automation.controllerState = doNothing;
      automation.moveController = false;
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
      if (editMode) {
          //this check is neccessary, because it returnPressed may be called
          //twice. the second call would cause a crash, however!
          Part* oldPart = editPart->part();
          Part* newPart = oldPart->clone();
          //printf("PartCanvas::returnPressed before msgChangePart oldPart refs:%d Arefs:%d newPart refs:%d Arefs:%d\n", oldPart->events()->refCount(), oldPart->events()->arefCount(), newPart->events()->refCount(), newPart->events()->arefCount());
          
          newPart->setName(lineEditor->text());
          // Indicate do undo, and do port controller values but not clone parts. 
          audio->msgChangePart(oldPart, newPart, true, true, false);
          
          editMode = false;
          }
      }

//---------------------------------------------------------
//   viewMouseDoubleClick
//---------------------------------------------------------

void PartCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
      {
      if (_tool != MusEWidget::PointerTool) {
            viewMousePressEvent(event);
            return;
            }
      QPoint cpos = event->pos();
      curItem     = items.find(cpos);
      bool ctrl  = event->modifiers() & Qt::ControlModifier;
      if (curItem) {
            if (event->button() == Qt::LeftButton && ctrl) {
                  editPart = (NPart*)curItem;
                  QRect r = map(curItem->bbox());
                  if (lineEditor == 0) {
                        lineEditor = new QLineEdit(this);
                        lineEditor->setFrame(true);
                        connect(lineEditor, SIGNAL(editingFinished()),SLOT(returnPressed()));
                        }
                  editMode = true;
                  lineEditor->setGeometry(r);
                  lineEditor->setText(editPart->name());
                  lineEditor->setFocus();
                  lineEditor->show();
                  }
            else if (event->button() == Qt::LeftButton) {
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
//   update
//---------------------------------------------------------

void PartCanvas::updateSong(DragType t, int flags)
      {
      song->update(flags | ((t == MOVE_COPY || t == MOVE_CLONE)
         ? SC_PART_INSERTED : SC_PART_MODIFIED));
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void PartCanvas::moveCanvasItems(MusEWidget::CItemList& items, int dp, int dx, DragType dtype)
{      
  Undo operations;
  
  for(MusEWidget::iCItem ici = items.begin(); ici != items.end(); ++ici) 
  {
    MusEWidget::CItem* ci = ici->second;
    
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
    
    UndoOp operation=moveItem(ci, newpos, dtype);
    if (operation.type != UndoOp::DoNothing)
    {
    	ci->move(newpos);
      operations.push_back(operation);
    }
    
    if(moving.size() == 1) {
          itemReleased(curItem, newpos);
          }
    if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
          selectItem(ci, false);
  }
  
  song->applyOperationGroup(operations);
  partsChanged();
}
      
//---------------------------------------------------------
//   moveItem
//    return false, if copy/move not allowed
//---------------------------------------------------------

// Changed by T356.
UndoOp PartCanvas::moveItem(MusEWidget::CItem* item, const QPoint& newpos, DragType t)
      {
      UndoOp result;
      NPart* npart    = (NPart*) item;
      Part* spart     = npart->part();
      Track* track    = npart->track();
      unsigned dtick  = newpos.x();
      unsigned ntrack = y2pitch(item->mp().y());
      Track::TrackType type = track->type();
      if (tracks->index(track) == ntrack && (dtick == spart->tick())) {
            return UndoOp(UndoOp::DoNothing);
            }
      if (ntrack >= tracks->size()) {
            ntrack = tracks->size();
            if (MusEGlobal::debugMsg)
                printf("PartCanvas::moveItem - add new track\n");
            Track* newTrack = song->addTrack(type);  // Add at end of list.
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
            return UndoOp(UndoOp::DoNothing);
            }

      Part* dpart;
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

      if(t == MOVE_MOVE) 
        item->setPart(dpart);
      if (t == MOVE_COPY && !clone) {
            dpart->events()->incARef(-1); // the later song->applyOperationGroup() will increment it
                                          // so we must decrement it first :/

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
            // TODO FINDMICH: is this still correct (by flo93)? i doubt it!
            result=UndoOp(UndoOp::AddPart,dpart);
            }
      else if (t == MOVE_MOVE) {
            dpart->setSelected(spart->selected());
            // These will increment ref count if not a clone, and will chain clones...
            // TODO FINDMICH: is this still correct (by flo93)? i doubt it!
            result=UndoOp(UndoOp::ModifyPart,spart, dpart, true, false);
            
            spart->setSelected(false);
            }
      // else // will never happen -> result will always be defined
      
      if (song->len() < (dpart->lenTick() + dpart->tick()))
            song->setLen(dpart->lenTick() + dpart->tick());

      return result;
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
      x = AL::sigmap.raster(x, *_raster);
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
      for (iTrack t = tracks->begin(); t != tracks->end(); ++t) {
            PartList* pl = (*t)->parts();
            for (iPart i = pl->begin(); i != pl->end(); ++i) {
                  Part* part = i->second;
                  NPart* np = new NPart(part);
                  items.add(np);
                  if (i->second->selected()) {
                        selectItem(np, true);
                        }
                  
                  // Check for touching borders. p4.0.29
                  Part* pp;
                  for(ciPart ii = pl->begin(); ii != pl->end(); ++ii) 
                  {
                    pp = ii->second;
                    if(pp == part)  // Ignore this part
                      continue;
                    if(pp->tick() > part->endTick())
                      break;
                    if(pp->endTick() == part->tick())
                      np->leftBorderTouches = true;
                    if(pp->tick() == part->endTick())
                      np->rightBorderTouches = true;
                  }      
                }
            }
      redraw();
      }

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void PartCanvas::updateSelection()
      {
      for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
            NPart* part = (NPart*)(i->second);
            part->part()->setSelected(i->second->isSelected());
      }
      emit selectionChanged();
      redraw();
      }

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PartCanvas::resizeItem(MusEWidget::CItem* i, bool noSnap, bool ctrl)
      {
      Track* t = ((NPart*)(i))->track();
      Part*  p = ((NPart*)(i))->part();

      int pos = p->tick() + i->width();
      int snappedpos = p->tick();
      if (!noSnap) {
            snappedpos = AL::sigmap.raster(pos, *_raster);
            }
      unsigned int newwidth = snappedpos - p->tick();
      if (newwidth == 0)
            newwidth = AL::sigmap.rasterStep(p->tick(), *_raster);

      song->cmdResizePart(t, p, newwidth, !ctrl);
      }

//---------------------------------------------------------
//   newItem
//    first create local Item
//---------------------------------------------------------

MusEWidget::CItem* PartCanvas::newItem(const QPoint& pos, int)
      {
      int x = pos.x();
      if (x < 0)
            x = 0;
      x = AL::sigmap.raster(x, *_raster);
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

void PartCanvas::newItem(MusEWidget::CItem* i, bool noSnap)
      {
      Part*  p = ((NPart*)(i))->part();

      int len = i->width();
      if (!noSnap)
            len = AL::sigmap.raster(len, *_raster);
      if (len == 0)
            len = AL::sigmap.rasterStep(p->tick(), *_raster);
      p->setLenTick(len);
      p->setSelected(true);
      audio->msgAddPart(p, true); //indicate undo
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PartCanvas::deleteItem(MusEWidget::CItem* i)
      {
      Part*  p = ((NPart*)(i))->part();
      audio->msgRemovePart(p, true); //Invokes songChanged which calls partsChanged which makes it difficult to delete them there
      return true;
      }

//---------------------------------------------------------
//   splitItem
//---------------------------------------------------------

void PartCanvas::splitItem(MusEWidget::CItem* item, const QPoint& pt)
      {
      NPart* np = (NPart*) item;
      Track* t = np->track();
      Part*  p = np->part();
      int x = pt.x();
      if (x < 0)
            x = 0;
      song->cmdSplitPart(t, p, AL::sigmap.raster(x, *_raster));
      }

//---------------------------------------------------------
//   glueItem
//---------------------------------------------------------

void PartCanvas::glueItem(MusEWidget::CItem* item)
      {
      NPart* np = (NPart*) item;
      Track* t = np->track();
      Part*  p = np->part();
      song->cmdGluePart(t, p);
      }

//---------------------------------------------------------
//   genItemPopup
//---------------------------------------------------------

QMenu* PartCanvas::genItemPopup(MusEWidget::CItem* item)
      {
      NPart* npart = (NPart*) item;
      Track::TrackType trackType = npart->track()->type();

      QMenu* partPopup = new QMenu(this);

      QAction *act_cut = partPopup->addAction(*editcutIconSet, tr("C&ut"));
      act_cut->setData(4);
      act_cut->setShortcut(Qt::CTRL+Qt::Key_X);

      QAction *act_copy = partPopup->addAction(*editcopyIconSet, tr("&Copy"));
      act_copy->setData(5);
      act_copy->setShortcut(Qt::CTRL+Qt::Key_C);

      partPopup->addSeparator();
      int rc = npart->part()->events()->arefCount();
      QString st = QString(tr("s&elect "));
      if(rc > 1)
        st += (QString().setNum(rc) + QString(" "));
      st += QString(tr("clones"));
      QAction *act_select = partPopup->addAction(st);
      act_select->setData(18);
      
      partPopup->addSeparator();
      QAction *act_rename = partPopup->addAction(tr("rename"));
      act_rename->setData(0);
      
      QMenu* colorPopup = partPopup->addMenu(tr("color"));

      // part color selection
      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            QAction *act_color = colorPopup->addAction(MusEUtil::colorRect(MusEConfig::config.partColors[i], 80, 80), MusEConfig::config.partColorNames[i]);
            act_color->setData(20+i);
            }

      QAction *act_delete = partPopup->addAction(QIcon(*deleteIcon), tr("delete")); // ddskrjo added QIcon to all
      act_delete->setData(1);
      QAction *act_split = partPopup->addAction(QIcon(*cutIcon), tr("split"));
      act_split->setData(2);
      QAction *act_glue = partPopup->addAction(QIcon(*glueIcon), tr("glue"));
      act_glue->setData(3);
      QAction *act_declone = partPopup->addAction(tr("de-clone"));
      act_declone->setData(15);

      partPopup->addSeparator();
      switch(trackType) {
            case Track::MIDI: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin->startPianoEditAction);
                  partPopup->addMenu(MusEGlobal::muse->arranger()->parentWin->scoreSubmenu);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin->startScoreEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin->startListEditAction);
                  QAction *act_mexport = partPopup->addAction(tr("save part to disk"));
                  act_mexport->setData(16);
                  }
                  break;
            case Track::DRUM: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin->startDrumEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin->startListEditAction);
                  QAction *act_dexport = partPopup->addAction(tr("save part to disk"));
                  act_dexport->setData(16);
                  }
                  break;
            case Track::WAVE: {
                  QAction *act_wedit = partPopup->addAction(QIcon(*edit_waveIcon), tr("wave edit"));
                  act_wedit->setData(14);
                  QAction *act_wexport = partPopup->addAction(tr("save part to disk"));
                  act_wexport->setData(16);
                  QAction *act_wfinfo = partPopup->addAction(tr("file info"));
                  act_wfinfo->setData(17);
                  }
                  break;
            case Track::AUDIO_OUTPUT:
            case Track::AUDIO_INPUT:
            case Track::AUDIO_GROUP:
            case Track::AUDIO_AUX:
            case Track::AUDIO_SOFTSYNTH:
                  break;
            }

      act_select->setEnabled( rc > 1);
      act_delete->setEnabled( true);
      act_cut->setEnabled( true);
      act_declone->setEnabled( rc > 1);
      
      return partPopup;
      }

//---------------------------------------------------------
//   itemPopup
//---------------------------------------------------------

void PartCanvas::itemPopup(MusEWidget::CItem* item, int n, const QPoint& pt)
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
                        connect(lineEditor, SIGNAL(editingFinished()),SLOT(returnPressed()));
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

            case 14:    // wave edit
                    emit startEditor(pl, 4);
                  return;
            case 15:    // declone
                  {
                  Part* spart  = npart->part();
                  Track* track = npart->track();
                  Part* dpart  = track->newPart(spart, false);

                  EventList* se = spart->events();
                  EventList* de = dpart->events();
                  for (iEvent i = se->begin(); i != se->end(); ++i) {
                        Event oldEvent = i->second;
                        Event ev = oldEvent.clone();
                        de->add(ev);
                        }
                  // Indicate undo, and do port controller values but not clone parts. 
                  // changed by flo93: removed start and endUndo, instead changed first bool to true
                  audio->msgChangePart(spart, dpart, true, true, false);
                  break; // Has to be break here, right?
                  }
            case 16: // Export to file
                  {
                  const Part* part = item->part();
                  bool popenFlag = false;
                  QString fn = getSaveFileName(QString(""), MusEGlobal::part_file_save_pattern, this, tr("MusE: save part"));
                  if (!fn.isEmpty()) {
                        FILE* fp = fileOpen(this, fn, ".mpt", "w", popenFlag, false, false);
                        if (fp) {
                              Xml tmpXml = Xml(fp);
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
                    QString str = tr("Part name: %1\nFiles:").arg(p->name());
                    for (iEvent e = el->begin(); e != el->end(); ++e) 
                    {
                      Event event = e->second;
                      SndFileR f  = event.sndFile();
                      if (f.isNull())
                        continue;
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
                        p->setSelected(true);
                        p = p->nextClone();
                        if(p == part)
                          break;
                      }
                      song->update(SC_SELECTION);
                    }
                    
                    break;
                  }
            case 20 ... NUM_PARTCOLORS+20:
                  {
                    curColorIndex = n - 20;
                    bool selfound = false;
                    //Loop through all parts and set color on selected:
                    for (MusEWidget::iCItem i = items.begin(); i != items.end(); i++) {
                          if (i->second->isSelected()) {
                                selfound = true;
                                i->second->part()->setColorIndex(curColorIndex);
                                }
                          }
                          
                    // If no items selected, use the one clicked on.
                    if(!selfound)
                      item->part()->setColorIndex(curColorIndex);
                    
                    song->update(SC_PART_MODIFIED);
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
    if (event->modifiers() & Qt::ControlModifier) {
            return;
            }
      QPoint pt = event->pos();
      MusEWidget::CItem* item = items.find(pt);

      //if (item == 0 && _tool!=MusEWidget::AutomationTool) // FINDMICHJETZT. neccessary? (flo93)
      //      return;

      switch (_tool) {
            default:
                  if (item)
                      emit trackChanged(item->part()->track());
                  else
                      emit trackChanged(NULL);
                  break;
            case MusEWidget::CutTool:
                  if (item) splitItem(item, pt);
                  break;
            case MusEWidget::GlueTool:
                  if (item) glueItem(item);
                  break;
            case MusEWidget::MuteTool:
                  {
                  if (item) {
                      NPart* np = (NPart*) item;
                      Part*  p = np->part();
                      p->setMute(!p->mute());
                      redraw();
                      break;
                      }
                  }
            case MusEWidget::AutomationTool:
                    if (automation.controllerState != doNothing)
                        automation.moveController=true;
                    break;
            }
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void PartCanvas::mouseRelease(const QPoint&)
      {
          // clear all the automation parameters
          automation.moveController=false;
          automation.controllerState = doNothing;
          //automation.currentCtrl=0;
          automation.currentCtrlValid = false;
          automation.currentTrack=0;
          automation.currentCtrlList=0;
      }

//---------------------------------------------------------
//   viewMousevent
//---------------------------------------------------------

void PartCanvas::mouseMove(QMouseEvent* event)
      {
      int x = event->pos().x();
      if (x < 0)
            x = 0;

      if (_tool == MusEWidget::AutomationTool)
          processAutomationMovements(event->pos(), event->modifiers() & Qt::ShiftModifier);

      emit timeChanged(AL::sigmap.raster(x, *_raster));
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

//      if (_tool == MusEWidget::AutomationTool) { // can't get the cursor pos to work right, skipping for now
//        // clear all the automation parameters
//        automation.moveController=false;
//        automation.controllerState = doNothing;
//        automation.currentCtrl=0;
//        automation.currentTrack=0;
//        automation.currentCtrlList=0;
//
//        processAutomationMovements(mapDev(QCursor::pos()),event->key()& Qt::Key_Control);
//      }
      if (editMode)
            {
            if ( key == Qt::Key_Return || key == Qt::Key_Enter ) 
                  {
                  //returnPressed(); commented out by flo
                  return;
                  }
            else if ( key == Qt::Key_Escape )
                  {
                  lineEditor->hide();
                  editMode = false;
                  return;
                  }
            }

      if (event->modifiers() &  Qt::ShiftModifier)
            key +=  Qt::SHIFT;
      if (event->modifiers() &  Qt::AltModifier)
            key +=  Qt::ALT;
      if (event->modifiers() &  Qt::ControlModifier)
            key +=  Qt::CTRL;

      if (key == shortcuts[SHRT_DELETE].key) {
            if (getCurrentDrag())
                  return;
                
            song->msgRemoveParts();
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            int spos = pos[0];
            if(spos > 0) 
            {
              spos -= 1;     // Nudge by -1, then snap down with raster1.
              spos = AL::sigmap.raster1(spos, *_raster);
            }  
            if(spos < 0)
              spos = 0;
            Pos p(spos,true);
            song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC].key) {
            int spos = AL::sigmap.raster2(pos[0] + 1, *_raster);    // Nudge by +1, then snap up with raster2.
            Pos p(spos,true);
            song->setPos(0, p, true, true, true); 
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key) {
            int spos = pos[0] - AL::sigmap.rasterStep(pos[0], *_raster);
            if(spos < 0)
              spos = 0;
            Pos p(spos,true);
            song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key) {
            Pos p(pos[0] + AL::sigmap.rasterStep(pos[0], *_raster), true);
            song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_POINTER].key) {
            emit setUsedTool(MusEWidget::PointerTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PENCIL].key) {
            emit setUsedTool(MusEWidget::PencilTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_RUBBER].key) {
            emit setUsedTool(MusEWidget::RubberTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_SCISSORS].key) {
            emit setUsedTool(MusEWidget::CutTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key) {
            emit setUsedTool(MusEWidget::AutomationTool);
            return;
            }      else if (key == shortcuts[SHRT_TOOL_GLUE].key) {
            emit setUsedTool(MusEWidget::GlueTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_MUTE].key) {
            emit setUsedTool(MusEWidget::MuteTool);
            return;
            }
      else if (key == shortcuts[SHRT_SEL_TRACK_ABOVE].key) {
            emit selectTrackAbove();
            return;
            }
      else if (key == shortcuts[SHRT_SEL_TRACK_BELOW].key) {
            emit selectTrackBelow();
            return;
            }
      //
      // Shortcuts that require selected parts from here
      //
      if (!curItem) {
          if (items.size()==0) {
              event->ignore();  // give global accelerators a chance
              return;
          }
          for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
              NPart* part = (NPart*)(i->second);
              if (part->isSelected()) {
                curItem=part;
                break;
              }
          }
          if (!curItem)
            curItem = (NPart*)items.begin()->second; // just grab the first part
      }

      MusEWidget::CItem* newItem = 0;
      bool singleSelection = isSingleSelection();
      bool add = false;
      //Locators to selection
      if (key == shortcuts[SHRT_LOCATORS_TO_SELECTION].key) {
            MusEWidget::CItem *leftmost = 0, *rightmost = 0;
            for (MusEWidget::iCItem i = items.begin(); i != items.end(); i++) {
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
            for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
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

            for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
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
            if (!track) {
              printf("no track above!\n");
                  return;
                }
            int middle = curItem->x() + curItem->part()->lenTick()/2;
            MusEWidget::CItem *aboveL = 0, *aboveR = 0;
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
                        MusEWidget::CItem* above  = 0;
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

            MusEWidget::CItem *belowL = 0, *belowR = 0;
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
                        MusEWidget::CItem* below = 0;
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

#if 0
void PartCanvas::drawItem(QPainter& p, const MusEWidget::CItem* item, const QRect& rect)
      {
      int from   = rect.x();
      int to     = from + rect.width();

      //printf("from %d to %d\n", from,to);
      Part* part = ((NPart*)item)->part();
      int pTick  = part->tick();
      from      -= pTick;
      to        -= pTick;
      if(from < 0)
        from = 0;
      if((unsigned int)to > part->lenTick())
        to = part->lenTick();  

      // Item bounding box x is in tick coordinates, same as rectangle.
      if(item->bbox().intersect(rect).isNull())
      {
        //printf("PartCanvas::drawItem rectangle is null\n");
        return;
      }
      
      QRect r    = item->bbox();
      bool clone = part->events()->arefCount() > 1;
      QBrush brush;
      
      //QRect rr = map(r);
      //QRect rr = p.transform().mapRect(r);
      //printf("PartCanvas::drawItem called map rx:%d rw:%d  rrx:%d rrw:%d\n", r.x(), r.width(), rr.x(), rr.width());  
      //printf("PartCanvas::drawItem called map rx:%d rw:%d\n", r.x(), r.width());  
      //p.save();
      //p.setWorldMatrixEnabled(false);
      
      // NOTE: Optimization: For each item, hasHiddenEvents() is called once in Canvas::draw(), and we use cachedHasHiddenEvents().
      // Not used for now.
      //int het = part->cachedHasHiddenEvents();
      int het = part->hasHiddenEvents();
        
      int y_1 = rmapyDev(1);  // Hack, try to replace.
      double xs_0 =  r.x();
      double xe_0 =  xs_0 + r.width();
      double xs_m0 = rmapx_f(xs_0);
      double xe_m0 = rmapx_f(xe_0);
      double xs_1 = rmapxDev_f(xs_m0 + 1.0);
      if(xs_1 > xe_0)
        xs_1 = xe_0;
      double xs_2 = rmapxDev_f(xs_m0 + 2.0);
      if(xs_2 > xe_0)
        xs_2 = xe_0;
      double xs_6 = rmapxDev_f(xs_m0 + 6.0);
      if(xs_6 > xe_0)
        xs_6 = xe_0;
      
      double xe_1 = rmapxDev_f(xe_m0 - 1.0);
      if(xe_1 < xs_0)
        xe_1 = xs_0;
      double xe_2 = rmapxDev_f(xe_m0 - 2.0);
      if(xe_2 < xs_0)
        xe_2 = xs_0;
      double xe_6 = rmapxDev_f(xe_m0 - 6.0);
      if(xe_6 < xs_0)
        xe_6 = xs_0;
      
      double ys_0 =  r.y();
      double ye_0 =  ys_0 + r.height();
      double ys_m0 = rmapy_f(ys_0);
      double ye_m0 = rmapy_f(ye_0);
      double ys_1 = rmapyDev_f(ys_m0 + 1.0);
      if(ys_1 > ye_0)
        ys_1 = ye_0;
      double ys_2 = rmapyDev_f(ys_m0 + 2.0);
      if(ys_2 > ye_0)
        ys_2 = ye_0;
      
      double ye_1 = rmapyDev_f(ye_m0 - 1.0);
      if(ye_1 < ys_0)
        ye_1 = ys_0;
      double ye_2 = rmapyDev_f(ye_m0 - 2.0);
      if(ye_2 < ys_0)
        ye_2 = ys_0;
      
      int cidx = part->colorIndex();
      if (item->isMoving()) 
      {
            QColor c(Qt::gray);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            QLinearGradient gradient(r.topLeft(), r.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else 
      if (part->selected()) 
      {
          QColor c(Qt::black);
          c.setAlpha(MusEConfig::config.globalAlphaBlend);
          QLinearGradient gradient(r.topLeft(), r.bottomLeft());
          // Use a colour only about 20% lighter than black, rather than the 50% we use in MusEUtil::gGradientFromQColor
          //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
          //c.setRgba(64, 64, 64, c.alpha());        
          gradient.setColorAt(0, QColor(51, 51, 51, MusEConfig::config.globalAlphaBlend));
          gradient.setColorAt(1, c);
          brush = QBrush(gradient);
      }
      else
      if (part->mute()) 
      {
            QColor c(Qt::white);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            QLinearGradient gradient(r.topLeft(), r.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
            
            // Not good. Aliasing missing lines happening at different mags. 
            // And it's too much. If notes + automation is displayed (by removing 'return' below), cross hatch interferes.
            // TODO: Maybe try to draw a nice gradient SINGLE cross (or STAR) from corner to corner instead. 
            // Then remove the 'return' below and let it draw the name and notes. 
            //brush.setStyle(Qt::DiagCrossPattern);
            //p.fillRect(rf, brush);
      }
      else
      {
            QColor c(MusEConfig::config.partColors[cidx]);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            brush = QBrush(MusEUtil::gGradientFromQColor(c, r.topLeft(), r.bottomLeft()));
      }  
      
      double h = r.height();
      double s = h / 4.0;
      double y0 = r.y();
      double y1 = y0 + s;
      double y2 = y0 + s * 2.0;
      double y3 = y0 + s * 3.0;
      double y4 = y0 + h;
      
      QPointF points[16];   
      int pts;
      
      //
      // Fill the part rectangles, accounting for hidden events by using 'jagged' edges...
      //
      
      p.setBrush(brush);
      p.setPen(Qt::NoPen);
      if(het)
      {
        pts = 0;
        if(het == (Part::LeftEventsHidden | Part::RightEventsHidden))
        {
          points[pts++] = QPointF(xs_0, y0);
          points[pts++] = QPointF(xe_0, y0);
          points[pts++] = QPointF(xe_6, y1);
          points[pts++] = QPointF(xe_0, y2);
          points[pts++] = QPointF(xe_6, y3);
          points[pts++] = QPointF(xe_0, y4);
          points[pts++] = QPointF(xs_0, y4);
          points[pts++] = QPointF(xs_6, y3);
          points[pts++] = QPointF(xs_0, y2);
          points[pts++] = QPointF(xs_6, y1);
          p.drawConvexPolygon(points, pts);   // Help says may be faster on some platforms (X11).
        }
        else
        if(het == Part::LeftEventsHidden)
        {
          points[pts++] = QPointF(xs_0, y0);
          points[pts++] = QPointF(xe_0, y0);
          points[pts++] = QPointF(xe_0, y4);
          points[pts++] = QPointF(xs_0, y4);
          points[pts++] = QPointF(xs_6, y3);
          points[pts++] = QPointF(xs_0, y2);
          points[pts++] = QPointF(xs_6, y1);
          p.drawConvexPolygon(points, pts);   
        }
        else
        if(het == Part::RightEventsHidden)
        {
          points[pts++] = QPointF(xs_0, y0);
          points[pts++] = QPointF(xe_0, y0);
          
          points[pts++] = QPointF(xe_6, y1);
          points[pts++] = QPointF(xe_0, y2);
          points[pts++] = QPointF(xe_6, y3);
          points[pts++] = QPointF(xe_0, y4);
          points[pts++] = QPointF(xs_0, y4);
          p.drawConvexPolygon(points, pts);   
        }
        
        //
        // Draw remaining 'hidden events' decorations with 'jagged' edges...
        //
                      
        int part_r, part_g, part_b, brightness, color_brightness;
        MusEConfig::config.partColors[cidx].getRgb(&part_r, &part_g, &part_b);
        brightness =  part_r*29 + part_g*59 + part_b*12;
        //if ((brightness < 12000 || part->selected()) && !part->mute() && !item->isMoving())
        //  color_brightness=223;   // too dark: use lighter color 
        //else
        //  color_brightness=32;  // otherwise use dark color 
        if ((brightness >= 12000 && !part->selected()))
          color_brightness=32;    // too light: use dark color 
        else
          color_brightness=223;   // too dark: use lighter color 
        QColor c(color_brightness,color_brightness,color_brightness, MusEConfig::config.globalAlphaBlend);
        p.setBrush(QBrush(MusEUtil::gGradientFromQColor(c, r.topLeft(), r.bottomLeft())));
        //p.setBrush(QBrush(c));
        if(het & Part::RightEventsHidden)
        {
          pts = 0;
          points[pts++] = QPointF(xe_0, y0);
          points[pts++] = QPointF(xe_0, y4);
          points[pts++] = QPointF(xe_6, y3);
          points[pts++] = QPointF(xe_0, y2);
          points[pts++] = QPointF(xe_6, y1);
          p.drawConvexPolygon(points, pts);   
        }
        if(het & Part::LeftEventsHidden)
        {
          pts = 0;
          points[pts++] = QPointF(xs_0, y0);
          points[pts++] = QPointF(xs_6, y1);
          points[pts++] = QPointF(xs_0, y2);
          points[pts++] = QPointF(xs_6, y3);
          points[pts++] = QPointF(xs_0, y4);
          p.drawConvexPolygon(points, pts);   
        }
      }  
      else
      {
        p.fillRect(r, brush);
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

      if (wp)
          drawWavePart(p, rect, wp, r);
      else if (mp)
      {
          drawMidiPart(p, rect, mp->events(), (MidiTrack*)part->track(), mp, r, mp->tick(), from, to);  
      }

  #if 0
        //
        // Now draw the borders...
        // Works great but requires clones be drawn with the highest priority on top of all other parts, in Canvas::draw.
        //
        
        QPen pen(part->selected() ? MusEConfig::config.partColors[i] : Qt::black, 2.0, clone ? Qt::DotLine : Qt::SolidLine);
        pen.setCosmetic(true);
        p.setPen(pen); 
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
        
  #else 
        //
        // Now draw the borders, using custom segments...
        //
        
        // FIXME NOTE: For 1-pixel wide lines, setting pen style to anything other than solid didn't work out well. 
        // Much too screwy - the single-width lines kept getting shifted one pixel over intermittently. 
        // I tried EVERYTHING to make sure x is proper but the painter keeps shifting them over.
        // Meanwhile the fills are correct. Seems painter doesn't like line patterns, whether stock or custom.
        // Therefore I was forced to manually draw the left and right segments. 
        // It works. Which seems to be more proof that painter is handling line patterns and pen widths badly... 
        // DO NOT ERASE COMMENTED CODE BELOW for now, in case it can be fixed. Tim. p4.0.29
        
        p.setBrush(Qt::NoBrush);
        
        QColor pc((part->mute() || item->isMoving())? Qt::white : MusEConfig::config.partColors[cidx]);
        QPen penSelect1H(pc);
        QPen penSelect2H(pc, 2.0);
        QPen penSelect1V(pc);
        QPen penSelect2V(pc, 2.0);
        penSelect1H.setCosmetic(true);
        penSelect2H.setCosmetic(true);
        penSelect1V.setCosmetic(true);
        penSelect2V.setCosmetic(true);
        
        pc = Qt::black;
        QPen penNormal1H(pc);
        QPen penNormal2H(pc, 2.0);
        QPen penNormal1V(pc);
        QPen penNormal2V(pc, 2.0);
        penNormal1H.setCosmetic(true);
        penNormal2H.setCosmetic(true);
        penNormal1V.setCosmetic(true);
        penNormal2V.setCosmetic(true);
        
        QVector<qreal> customDashPattern;
        if(clone)
        {
          customDashPattern << 4.0 << 8.0;
          penSelect1H.setDashPattern(customDashPattern);
          penNormal1H.setDashPattern(customDashPattern);
          //penSelect1V.setDashPattern(customDashPattern);
          //penNormal1V.setDashPattern(customDashPattern);
          customDashPattern.clear();
          customDashPattern << 2.0 << 4.0;
          penSelect2H.setDashPattern(customDashPattern);
          penNormal2H.setDashPattern(customDashPattern);
          //penSelect2V.setDashPattern(customDashPattern);
          //penNormal2V.setDashPattern(customDashPattern);
        }  
        
        pc = Qt::white;
        QPen penHidden1(pc);
        QPen penHidden2(pc, 2.0);
        penHidden2.setCosmetic(true);
        //customDashPattern.clear();
        //customDashPattern << 2.0 << 10.0;
        //penHidden1.setDashPattern(customDashPattern);
        //customDashPattern.clear();
        //customDashPattern << 1.0 << 5.0;
        //penHidden2.setDashPattern(customDashPattern);
        
        bool lbt = ((NPart*)item)->leftBorderTouches;
        bool rbt = ((NPart*)item)->rightBorderTouches;
        
        QLineF l1( lbt?xs_1:xs_0, ys_0,   rbt?xe_1:xe_0, ys_0);                     // Top 
        //QLineF l2(rbt?xe_1:xe_0,  r.y() + (rbt?y_1:y_2) - 1,       rbt?xe_1:xe_0, r.y() + r.height() - 1);        // Right 
        QLineF l3( lbt?xs_1:xs_0, ye_0,   rbt?xe_1:xe_0, ye_0);  // Bottom
        //QLineF l4(r.x(), r.y() + (lbt?y_1:y_2),            r.x(), r.y() + r.height() - (lbt?y_1:y_2));       // Left
        
        if(het & Part::RightEventsHidden)
          p.setPen(((NPart*)item)->rightBorderTouches ? penHidden1 : penHidden2); 
        else  
        {
          if(((NPart*)item)->rightBorderTouches)              
            p.setPen(part->selected() ? penSelect1V : penNormal1V); 
          else  
            p.setPen(part->selected() ? penSelect2V : penNormal2V); 
        }  
        //p.drawLine(l2);        // Right line
        
        double xx = rbt?xe_1:xe_0; 
        if(clone)
        {
          double yinc = 7.0 * y_1;
          for(double yy = (rbt?ys_1:ys_2); yy < ye_2; yy += yinc)
          {
            double yi = (rbt?3.0:2.0) * y_1;
            if(yy + yi > ye_2)
              yi = ye_2 - yy;
            p.drawLine(QPointF(xx, yy), QPointF(xx, yy + yi));      // Right dashed line
          }   
        }
        else  
          p.drawLine(QPointF(xx, rbt?ys_1:ys_2), QPointF(xx, rbt?ye_1:ye_2));      // Right line
        
        if(het & Part::LeftEventsHidden)
          p.setPen(((NPart*)item)->leftBorderTouches ? penHidden1 : penHidden2); 
        else  
        {
          if(((NPart*)item)->leftBorderTouches)              
            p.setPen(part->selected() ? penSelect1V : penNormal1V); 
          else  
            p.setPen(part->selected() ? penSelect2V : penNormal2V); 
        }  
        //p.drawLine(l4);        //  Left line
        
        xx = xs_0;
        if(clone)
        {
          double yinc = 7.0 * y_1;
          for(double yy = (lbt?ys_1:ys_2); yy < ye_2; yy += yinc)
          {
            double yi = (lbt?3.0:2.0) * y_1;
            if(yy + yi > ye_2)
              yi = ye_2 - yy;
            p.drawLine(QPointF(xx, yy), QPointF(xx, yy + yi));      // Left dashed line
          }   
        }
        else  
          p.drawLine(QPointF(xx, lbt?ys_1:ys_2), QPointF(xx, lbt?ye_1:ye_2));      // Left line
        
        p.setPen(part->selected() ? penSelect2H : penNormal2H); 
        p.drawLine(l1);  // Top line
        p.drawLine(l3);  // Bottom line
        
  #endif
 
      //p.restore();
      
      if (MusEConfig::config.canvasShowPartType & 1) {     // show names
            // draw name
            // FN: Set text color depending on part color (black / white)
            int part_r, part_g, part_b, brightness;
            // Since we'll draw the text on the bottom (to accommodate drum 'slivers'),
            //  get the lowest colour in the gradient used to draw the part.
            QRect rr = map(r);
            rr.setX(rr.x() + 3);
            MusEUtil::gGradientFromQColor(MusEConfig::config.partColors[cidx], rr.topLeft(), rr.bottomLeft()).stops().last().second.getRgb(&part_r, &part_g, &part_b);
            brightness =  part_r*29 + part_g*59 + part_b*12;
            //bool rev = (brightness < 12000 || part->selected()) && !part->mute() && !item->isMoving();
            bool rev = brightness >= 12000 && !part->selected();
            p.save();
            p.setFont(MusEConfig::config.fonts[1]);
            p.setWorldMatrixEnabled(false);
            if (rev)
              p.setPen(Qt::white); 
            else
              p.setPen(Qt::black);   
            p.drawText(rr.translated(1, 1), Qt::AlignBottom|Qt::AlignLeft, part->name());
            if (rev)
              p.setPen(Qt::black);  
            else
              p.setPen(Qt::white);   
            p.drawText(rr, Qt::AlignBottom|Qt::AlignLeft, part->name());
            p.restore();
            }
      }
#endif

void PartCanvas::drawItem(QPainter& p, const MusEWidget::CItem* item, const QRect& rect)
      {
      int from   = rect.x();
      int to     = from + rect.width();

      //printf("from %d to %d\n", from,to);
      Part* part = ((NPart*)item)->part();
      int pTick  = part->tick();
      from      -= pTick;
      to        -= pTick;
      if(from < 0)
        from = 0;
      if((unsigned int)to > part->lenTick())
        to = part->lenTick();  

      bool clone = part->events()->arefCount() > 1;
      QBrush brush;
      
      QRect r    = item->bbox();
      // Compensation required for two pixel wide border. FIXME Prefer to do this after the map, but r is needed below.
      r.moveTop(r.y() + rmapyDev(1));  
      //QRect rr = p.transform().mapRect(r);    // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect rr = map(r);                        // Use our own map instead.                                
      
      QRect mr = map(rect);
      
      // Item bounding box x is in tick coordinates, same as rectangle.
      if((rr & mr).isNull())
      {
        //printf("PartCanvas::drawItem rectangle is null\n");
        return;
      }
      
      //p.save();
      p.setWorldMatrixEnabled(false);
      
      // NOTE: Optimization: For each item, hasHiddenEvents() is called once in Canvas::draw(), and we use cachedHasHiddenEvents().
      // Not used for now.
      //int het = part->cachedHasHiddenEvents();
      int het = part->hasHiddenEvents();
        
      int xs_0 = rr.x();
      int xe_0 = xs_0 + rr.width();
      int xs_1 = xs_0 + 1;
      if(xs_1 > xe_0)
        xs_1 = xe_0;
      int xs_2 = xs_0 + 2;
      if(xs_2 > xe_0)
        xs_2 = xe_0;
      int xs_j = xs_0 + 8;
      if(xs_j > xe_0)
        xs_j = xe_0;
      
      int xe_1 = xe_0 - 1;
      if(xe_1 < xs_0)
        xe_1 = xs_0;
      int xe_2 = xe_0 - 2;
      if(xe_2 < xs_0)
        xe_2 = xs_0;
      int xe_j = xe_0 - 8;
      if(xe_j < xs_0)
        xe_j = xs_0;
      
      int ys_0 = rr.y();          
      int ye_0 = ys_0 + rr.height();
      int ys_1 = ys_0 + 1;
      if(ys_1 > ye_0)
        ys_1 = ye_0;
      int ys_2 = ys_0 + 2;
      if(ys_2 > ye_0)
        ys_2 = ye_0;
      int ys_3 = ys_0 + 3;
      if(ys_3 > ye_0)
        ys_3 = ye_0;
      
      int ye_1 = ye_0 - 1;
      if(ye_1 < ys_0)
        ye_1 = ys_0;
      int ye_2 = ye_0 - 2;
      if(ye_2 < ys_0)
        ye_2 = ys_0;
      
      int mrxs_0 = mr.x();
      int mrxe_0 = mrxs_0 + mr.width();
      bool lbt = ((NPart*)item)->leftBorderTouches;
      bool rbt = ((NPart*)item)->rightBorderTouches;
      int lbx = lbt?xs_1:xs_0;
      int rbx = rbt?xe_1:xe_0;
      int lbx_c = lbx < mrxs_0 ? mrxs_0 : lbx;
      int rbx_c = rbx > mrxe_0 ? mrxe_0 : rbx;
      
      int cidx = part->colorIndex();
      if (item->isMoving()) 
      {
            QColor c(Qt::gray);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            QLinearGradient gradient(rr.topLeft(), rr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else 
      if (part->selected()) 
      {
          QColor c(Qt::black);
          c.setAlpha(MusEConfig::config.globalAlphaBlend);
          QLinearGradient gradient(rr.topLeft(), rr.bottomLeft());
          // Use a colour only about 20% lighter than black, rather than the 50% we use in MusEUtil::gGradientFromQColor
          //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
          //c.setRgba(64, 64, 64, c.alpha());        
          gradient.setColorAt(0, QColor(51, 51, 51, MusEConfig::config.globalAlphaBlend));
          gradient.setColorAt(1, c);
          brush = QBrush(gradient);
      }
      else
      if (part->mute()) 
      {
            QColor c(Qt::white);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            QLinearGradient gradient(rr.topLeft(), rr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else
      {
            QColor c(MusEConfig::config.partColors[cidx]);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            brush = QBrush(MusEUtil::gGradientFromQColor(c, rr.topLeft(), rr.bottomLeft()));
      }  
      
      int h = rr.height();
      double s = double(h) / 4.0;
      int y0 = ys_0;
      //int y1 = y0 + lrint(s);
      int y2 = y0 + lrint(s * 2.0);
      //int y3 = y0 + lrint(s * 3.0);
      int y4 = y0 + h;
      
      //QPoint points[12];   
      QPoint points[8];   
      int pts;
      
      //
      // Fill the part rectangles, accounting for hidden events by using 'jagged' edges...
      //
      
      p.setBrush(brush);
      p.setPen(Qt::NoPen);
      
      if(het)
      {
        // TODO: Make this part respect the requested drawing rectangle (rr & mr), for speed !
        
        pts = 0;
        if(het == (Part::LeftEventsHidden | Part::RightEventsHidden))
        {
          //points[pts++] = QPoint(xs_0, y0);
          //points[pts++] = QPoint(xe_0, y0);
          //points[pts++] = QPoint(xe_j, y1);
          //points[pts++] = QPoint(xe_0, y2);
          //points[pts++] = QPoint(xe_j, y3);
          //points[pts++] = QPoint(xe_0, y4);
          //points[pts++] = QPoint(xs_0, y4);
          //points[pts++] = QPoint(xs_j, y3);
          //points[pts++] = QPoint(xs_0, y2);
          //points[pts++] = QPoint(xs_j, y1);
          
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_j, y2);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xs_0, y4);
          points[pts++] = QPoint(xs_j, y2);
          
          p.drawConvexPolygon(points, pts);   // Help says may be faster on some platforms (X11).
        }
        else
        if(het == Part::LeftEventsHidden)
        {
          //points[pts++] = QPoint(xs_0, y0);
          //points[pts++] = QPoint(xe_0, y0);
          //points[pts++] = QPoint(xe_0, y4);
          //points[pts++] = QPoint(xs_0, y4);
          //points[pts++] = QPoint(xs_j, y3);
          //points[pts++] = QPoint(xs_0, y2);
          //points[pts++] = QPoint(xs_j, y1);
          
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xs_0, y4);
          points[pts++] = QPoint(xs_j, y2);
          
          p.drawConvexPolygon(points, pts);   
        }
        else
        if(het == Part::RightEventsHidden)
        {
          //points[pts++] = QPoint(xs_0, y0);
          //points[pts++] = QPoint(xe_0, y0);
          //points[pts++] = QPoint(xe_j, y1);
          //points[pts++] = QPoint(xe_0, y2);
          //points[pts++] = QPoint(xe_j, y3);
          //points[pts++] = QPoint(xe_0, y4);
          //points[pts++] = QPoint(xs_0, y4);
          
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_j, y2);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xs_0, y4);
          
          p.drawConvexPolygon(points, pts);   
        }
        
        //
        // Draw remaining 'hidden events' decorations with 'jagged' edges...
        //
                      
        int part_r, part_g, part_b, brightness, color_brightness;
        MusEConfig::config.partColors[cidx].getRgb(&part_r, &part_g, &part_b);
        brightness =  part_r*29 + part_g*59 + part_b*12;
        //if ((brightness < 12000 || part->selected()) && !part->mute() && !item->isMoving())
        //  color_brightness=223;   // too dark: use lighter color 
        //else
        //  color_brightness=32;  // otherwise use dark color 
        if ((brightness >= 12000 && !part->selected()))
          color_brightness=96; //0;    // too light: use dark color 
        else
          color_brightness=180; //255;   // too dark: use lighter color 
        QColor c(color_brightness,color_brightness,color_brightness, MusEConfig::config.globalAlphaBlend);
        p.setBrush(QBrush(MusEUtil::gGradientFromQColor(c, rr.topLeft(), rr.bottomLeft())));
        //p.setBrush(QBrush(c));
        if(het & Part::RightEventsHidden)
        {
          pts = 0;
          //points[pts++] = QPoint(xe_0, y0);
          //points[pts++] = QPoint(xe_0, y4);
          //points[pts++] = QPoint(xe_j, y3);
          //points[pts++] = QPoint(xe_0, y2);
          //points[pts++] = QPoint(xe_j, y1);
          
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xe_j, y2);
          
          p.drawConvexPolygon(points, pts);   
        }
        if(het & Part::LeftEventsHidden)
        {
          pts = 0;
          //points[pts++] = QPoint(xs_0, y0);
          //points[pts++] = QPoint(xs_j, y1);
          //points[pts++] = QPoint(xs_0, y2);
          //points[pts++] = QPoint(xs_j, y3);
          //points[pts++] = QPoint(xs_0, y4);
          
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xs_j, y2);
          points[pts++] = QPoint(xs_0, y4);
          
          p.drawConvexPolygon(points, pts);   
        }
      }  
      else
      {
        //p.fillRect(rr, brush);
        p.fillRect(rr & mr, brush);  // Respect the requested drawing rectangle. Gives speed boost!
      }
          
      // Draw a pattern brush on muted parts...
      if(part->mute())
      {
        p.setPen(Qt::NoPen);
        //brush.setStyle(Qt::DiagCrossPattern);
        brush.setStyle(Qt::Dense7Pattern);
        
        //p.fillRect(rr, brush);                                                       // FIXME: Some shifting going on
        //p.fillRect(QRect(rr.x(), rr.y(), rr.width() + 1, rr.height() + 1), brush);   // Same here
        p.fillRect(rr & mr, brush);   // Respect the requested drawing rectangle. Gives speed boost!                                                      
      }  
      
      p.setWorldMatrixEnabled(true);
      
      MidiPart* mp = 0;
      WavePart* wp = 0;
      Track::TrackType type = part->track()->type();
      if (type == Track::WAVE) {
            wp =(WavePart*)part;
            }
      else {
            mp = (MidiPart*)part;
            }

      if (wp)
          drawWavePart(p, rect, wp, r);
      else if (mp)
      {
          drawMidiPart(p, rect, mp->events(), (MidiTrack*)part->track(), mp, r, mp->tick(), from, to);  
      }

      p.setWorldMatrixEnabled(false);
      
  #if 0
        //
        // Now draw the borders...
        // Works great but requires clones be drawn with the highest priority on top of all other parts, in Canvas::draw.
        //
        
        QPen pen(part->selected() ? MusEConfig::config.partColors[i] : Qt::black, 2.0, clone ? Qt::DotLine : Qt::SolidLine);
        pen.setCosmetic(true);
        p.setPen(pen); 
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);
        
  //#else 
  #endif
  
  #if 1
        //
        // Now draw the borders, using custom segments...
        //
        
        p.setBrush(Qt::NoBrush);
        
        QColor pc((part->mute() || item->isMoving())? Qt::white : MusEConfig::config.partColors[cidx]);
        QPen penSelect1H(pc);
        QPen penSelect2H(pc, 2.0);
        QPen penSelect1V(pc);
        QPen penSelect2V(pc, 2.0);
        penSelect1H.setCosmetic(true);
        penSelect2H.setCosmetic(true);
        penSelect1V.setCosmetic(true);
        penSelect2V.setCosmetic(true);
        
        pc = Qt::black;
        QPen penNormal1H(pc);
        QPen penNormal2H(pc, 2.0);
        QPen penNormal1V(pc);
        QPen penNormal2V(pc, 2.0);
        penNormal1H.setCosmetic(true);
        penNormal2H.setCosmetic(true);
        penNormal1V.setCosmetic(true);
        penNormal2V.setCosmetic(true);
        
        //pc = Qt::white;
        //pc = Qt::darkGray;
        //QPen penHidden1(pc);
        //QPen penHidden2(pc, 2.0);
        //penHidden2.setCosmetic(true);
        
        QVector<qreal> customDashPattern;
        
        if(clone)
        {
          customDashPattern << 4.0 << 6.0;
          penSelect1H.setDashPattern(customDashPattern);
          penNormal1H.setDashPattern(customDashPattern);
          penSelect1V.setDashPattern(customDashPattern);  
          penNormal1V.setDashPattern(customDashPattern);  
          penSelect1V.setDashOffset(2.0);  
          penNormal1V.setDashOffset(2.0);
          //penHidden1.setDashPattern(customDashPattern);  
          customDashPattern.clear();
          customDashPattern << 2.0 << 3.0;
          penSelect2H.setDashPattern(customDashPattern);
          penNormal2H.setDashPattern(customDashPattern);
          penSelect2V.setDashPattern(customDashPattern);  
          penNormal2V.setDashPattern(customDashPattern);  
          penSelect2V.setDashOffset(1.0);  
          penNormal2V.setDashOffset(1.0);  
          //penHidden2.setDashPattern(customDashPattern);  
          
          // FIXME: Some shifting still going on. Values likely not quite right here. 
          int xdiff = mrxs_0 - lbx;
          if(xdiff > 0)
          {
            int doff = xdiff % 10;
            penSelect1H.setDashOffset(doff);
            penNormal1H.setDashOffset(doff);
            doff = xdiff % 5;
            penSelect2H.setDashOffset(doff);
            penNormal2H.setDashOffset(doff);
          }
        }  
        
        //if(het & Part::RightEventsHidden)
        //  p.setPen(((NPart*)item)->rightBorderTouches ? penHidden1 : penHidden2); 
        //else  
        {
          if(((NPart*)item)->rightBorderTouches)              
            p.setPen(part->selected() ? penSelect1V : penNormal1V); 
          else  
            p.setPen(part->selected() ? penSelect2V : penNormal2V); 
        }  
        
        if(rbx >= mrxs_0 && rbx <= mrxe_0)  // Respect the requested drawing rectangle. Gives speed boost!
        {
          QLine l2(rbx, ys_0, rbx, ye_0);            // Right 
          p.drawLine(l2);        // Right line
        }
        
        /*
        int xx = rbt?xe_1:xe_0; 
        if(clone)
        {
          int yinc = 7;
          for(int yy = (rbt?ys_1:ys_2); yy < ye_2; yy += yinc)
          {
            int yi = rbt?3:2;
            if(yy + yi > ye_2)
              yi = ye_2 - yy;
            p.drawLine(QPoint(xx, yy), QPoint(xx, yy + yi));      // Right dashed line
          }   
        }
        else  
          p.drawLine(QPoint(xx, rbt?ys_1:ys_2), QPoint(xx, rbt?ye_1:ye_2));      // Right line
        */
        
        //if(het & Part::LeftEventsHidden)
        //  p.setPen(((NPart*)item)->leftBorderTouches ? penHidden1 : penHidden2); 
        //else  
        {
          if(((NPart*)item)->leftBorderTouches)              
            p.setPen(part->selected() ? penSelect1V : penNormal1V); 
          else  
            p.setPen(part->selected() ? penSelect2V : penNormal2V); 
        }  
        
        if(xs_0 >= mrxs_0 && xs_0 <= mrxe_0)
        {
          QLine l4(xs_0, ys_0, xs_0, ye_0);            // Left
          p.drawLine(l4);        //  Left line
        }
        
        /*
        xx = xs_0;
        if(clone)
        {
          int yinc = 7;
          for(int yy = (lbt?ys_1:ys_2); yy < ye_2; yy += yinc)
          {
            int yi = lbt?3:2;
            if(yy + yi > ye_2)
              yi = ye_2 - yy;
            p.drawLine(QPoint(xx, yy), QPoint(xx, yy + yi));      // Left dashed line
          }   
        }
        else  
          p.drawLine(QPoint(xx, lbt?ys_1:ys_2), QPoint(xx, lbt?ye_1:ye_2));      // Left line
        */
        
        p.setPen(part->selected() ? penSelect2H : penNormal2H); 
        
        // Respect the requested drawing rectangle. Gives speed boost!
        QLine l1(lbx_c, ys_0, rbx_c, ys_0);  
        p.drawLine(l1);  // Top line
        QLine l3(lbx_c, ye_0, rbx_c, ye_0);  
        p.drawLine(l3);  // Bottom line
        
  #endif
      
      if (MusEConfig::config.canvasShowPartType & 1) {     // show names
            // draw name
            // FN: Set text color depending on part color (black / white)
            int part_r, part_g, part_b, brightness;
            // Since we'll draw the text on the bottom (to accommodate drum 'slivers'),
            //  get the lowest colour in the gradient used to draw the part.
            //QRect rr = map(r);
            QRect tr = rr;
            tr.setX(tr.x() + 3);
            MusEUtil::gGradientFromQColor(MusEConfig::config.partColors[cidx], tr.topLeft(), tr.bottomLeft()).stops().last().second.getRgb(&part_r, &part_g, &part_b);
            brightness =  part_r*29 + part_g*59 + part_b*12;
            //bool rev = (brightness < 12000 || part->selected()) && !part->mute() && !item->isMoving();
            bool rev = brightness >= 12000 && !part->selected();
            p.setFont(MusEConfig::config.fonts[1]);
            if (rev)
              p.setPen(Qt::white); 
            else
              p.setPen(Qt::black);   
            p.drawText(tr.translated(1, 1), Qt::AlignBottom|Qt::AlignLeft, part->name());
            if (rev)
              p.setPen(Qt::black);  
            else
              p.setPen(Qt::white);   
            p.drawText(tr, Qt::AlignBottom|Qt::AlignLeft, part->name());
            }
            
      //p.restore();
      p.setWorldMatrixEnabled(true);
      }

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void PartCanvas::drawMoving(QPainter& p, const MusEWidget::CItem* item, const QRect&)
      {
        p.setPen( Qt::black);
        Part* part = ((NPart*)item)->part();
        QColor c(part->mute() ? Qt::white : MusEConfig::config.partColors[part->colorIndex()]);        
        //c.setAlpha(MusEConfig::config.globalAlphaBlend);
        c.setAlpha(128);  // Fix this regardless of global setting. Should be OK.
        p.setBrush(c);
        p.drawRect(item->mp().x(), item->mp().y(), item->width(), item->height());
      }


//---------------------------------------------------------
//   drawMidiPart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void PartCanvas::drawMidiPart(QPainter& p, const QRect&, EventList* events, MidiTrack *mt, MidiPart *pt, const QRect& r, int pTick, int from, int to)
{
  int color_brightness;
  
  if(pt) 
  {
    int part_r, part_g, part_b, brightness;
    MusEConfig::config.partColors[pt->colorIndex()].getRgb(&part_r, &part_g, &part_b);
    brightness =  part_r*29 + part_g*59 + part_b*12;
    //if ((brightness < 12000 || pt->selected()) && !pt->mute())
    //  color_brightness=192;   // too dark: use lighter color 
    //else
    //  color_brightness=64;  // otherwise use dark color 
    if (brightness >= 12000 && !pt->selected())
      color_brightness=64; // 96;    // too bright: use dark color 
    else
      color_brightness=190; //160;   // too dark: use lighter color 
  }
  else
    color_brightness=80;
    
  if (MusEConfig::config.canvasShowPartType & 2) {      // show events
            p.setPen(QColor(color_brightness,color_brightness,color_brightness));
            // Do not allow this, causes segfault.
            if(from <= to)
            {
              iEvent ito(events->lower_bound(to));

              for (iEvent i = events->lower_bound(from); i != ito; ++i) {
                    EventType type = i->second.type();
                    if (
                      ((MusEConfig::config.canvasShowPartEvent & 1) && (type == Note))
                      || ((MusEConfig::config.canvasShowPartEvent & 2) && (type == PAfter))
                      || ((MusEConfig::config.canvasShowPartEvent & 4) && (type == Controller))
                      || ((MusEConfig::config.canvasShowPartEvent &16) && (type == CAfter))
                      || ((MusEConfig::config.canvasShowPartEvent &64) && (type == Sysex || type == Meta))
                      ) {
                          int t = i->first + pTick;
                          int th = mt->height();
                          if(t >= r.left() && t <= r.right())
                            p.drawLine(t, r.y()+2, t, r.y()+th-4);
                          }
                    }
            }
      }
  else {      // show Cakewalk Style
      using std::map;
      using std::pair;
      
      iEvent ito(events->lower_bound(to));
      bool isdrum = (mt->type() == Track::DRUM);

      // draw controllers ------------------------------------------
      p.setPen(QColor(192,192,color_brightness/2));
      for (iEvent i = events->begin(); i != ito; ++i) { // PITCH BEND
            int t  = i->first + pTick;

            EventType type = i->second.type();
            if (type == Controller) {
                  int ctrl_type=i->second.dataA();
                  int val=i->second.dataB();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == CTRL_PITCH)
                    p.drawLine(t, hoffset + r.y() + th/2, t, hoffset + r.y() - val*th/8192/2 + th/2);
            }
      }

      p.setPen(QColor(192,color_brightness/2,color_brightness/2));
      for (iEvent i = events->begin(); i != ito; ++i) { // PAN
            int t  = i->first + pTick;

            EventType type = i->second.type();
            if (type == Controller) {
                  int ctrl_type=i->second.dataA();
                  int val=i->second.dataB();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == 10)
                    p.drawLine(t, hoffset + r.y() + th - val*th/127, t, hoffset + r.y() + th);
            }
      }

      p.setPen(QColor(color_brightness/2,192,color_brightness/2));
      for (iEvent i = events->begin(); i != ito; ++i) { // VOLUME
            int t  = i->first + pTick;

            EventType type = i->second.type();
            if (type == Controller) {
                  int ctrl_type=i->second.dataA();
                  int val=i->second.dataB();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == 7)
                    p.drawLine(t, hoffset + r.y() + th - val*th/127, t, hoffset + r.y() + th);
            }
      }

      p.setPen(QColor(0,0,255));
      for (iEvent i = events->begin(); i != ito; ++i) { // PROGRAM CHANGE
            int t  = i->first + pTick;

            EventType type = i->second.type();
            if (type == Controller) {
                  int ctrl_type=i->second.dataA();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == CTRL_PROGRAM)
                    p.drawLine(t, hoffset + r.y(), t, hoffset + r.y() + th);
            }
      }





      // draw notes ------------------------------------------------

      int lowest_pitch=127;
      int highest_pitch=0;
      map<int,int> y_mapper;
      
      if (MusEConfig::config.canvasShowPartType & 4) //y-stretch?
      {
        for (iEvent i = events->begin(); i != events->end(); ++i)
        {
          if (i->second.type()==Note)
          {
            int pitch=i->second.pitch();

            if (!isdrum)
            {
              if (pitch > highest_pitch) highest_pitch=pitch;
              if (pitch < lowest_pitch) lowest_pitch=pitch;
            }
            else
            {
              y_mapper.insert(pair<int,int>(pitch, 0));
            }
          }
        }
        
        if (isdrum)
        {
          int cnt=0;
          for (map<int,int>::iterator it=y_mapper.begin(); it!=y_mapper.end(); it++)
          {
            it->second=cnt;
            cnt++;
          }
          lowest_pitch=0;
          highest_pitch=cnt-1;
        }
        
        if (lowest_pitch==highest_pitch)
        {
          lowest_pitch--;
          highest_pitch++;
        }
        
        if (MusEGlobal::heavyDebugMsg)
        {
            if (!isdrum)
                printf("DEBUG: arranger: cakewalk enabled, y-stretching from %i to %i. eventlist=%p\n",lowest_pitch, highest_pitch, events);
            else
            {
                printf("DEBUG: arranger: cakewalk enabled, y-stretching drums: ");;
                for (map<int,int>::iterator it=y_mapper.begin(); it!=y_mapper.end(); it++)
                    printf("%i ", it->first);
                printf("; eventlist=%p\n",events); 
            }
        }
      }
      else
      {
        lowest_pitch=0;
        highest_pitch=127;

        if (isdrum)
          for (int cnt=0;cnt<127;cnt++)
            y_mapper[cnt]=cnt;
        
        if (MusEGlobal::heavyDebugMsg) printf("DEBUG: arranger: cakewalk enabled, y-stretch disabled\n");
      }

      p.setPen(QColor(color_brightness,color_brightness,color_brightness));      
      for (iEvent i = events->begin(); i != ito; ++i) {
            int t  = i->first + pTick;
            int te = t + i->second.lenTick();

            if (te < (from + pTick))
                  continue;

            //if (te > (to + pTick))
            //      te = to + pTick;
            if (te >= (to + pTick))
                  te = lrint(rmapxDev_f(rmapx_f(to + pTick) - 1.0));
            
            EventType type = i->second.type();
            if (type == Note) {
                  int pitch = i->second.pitch();
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom
                  int y;
                  if (!isdrum)
                    y = hoffset + r.y() + th - (pitch-lowest_pitch)*th/(highest_pitch-lowest_pitch);
                  else
                    y = hoffset + r.y() + y_mapper[pitch]*th/(highest_pitch-lowest_pitch);
                  
                  p.drawLine(t, y, te, y);
            }
      }
  }
}

//---------------------------------------------------------
//   drawWavePart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void PartCanvas::drawWavePart(QPainter& p,
   const QRect& bb, WavePart* wp, const QRect& _pr)
      {
      //QRect rr = p.worldMatrix().mapRect(bb);    // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect rr = map(bb);                          // Use our own map instead.
      //QRect pr = p.worldMatrix().mapRect(_pr);
      QRect pr = map(_pr);

      p.save();
      p.resetTransform();

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
                  printf("drawWavePart: channels==0! %s\n", f.name().toLatin1().constData());
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
                  //printf("PartCanvas::drawWavePart i:%d ex:%d\n", i, ex);  
                  
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
                        p.setPen(QColor(Qt::darkGray));
                        p.drawLine(i, y - peak - cc, i, y + peak);
                        p.setPen(QColor(Qt::darkGray).darker());
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
                              p.setPen(QColor(Qt::darkGray));
                              p.drawLine(i, y - peak - cc, i, y + peak);
                              p.setPen(QColor(Qt::darkGray).darker());
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
      for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
            if (!i->second->isSelected())
                  continue;
            NPart* npart = (NPart*)(i->second);
            pl.add(npart->part());
            }
      switch (cmd) {
            case CMD_CUT_PART:
            {
                  copy(&pl);
                  
                  Undo operations;
                  
                  for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i) {
                        if (i->second->isSelected()) {
                              NPart* p = (NPart*)(i->second);
                              Part* part = p->part();
                              operations.push_back(UndoOp(UndoOp::DeletePart, part));
                              }
                      }
                  
                  song->applyOperationGroup(operations);
                  
                  break;
            }
            case CMD_COPY_PART:
                  copy(&pl);
                  break;
            case CMD_COPY_PART_IN_RANGE:
                  copy_in_range(&pl);
                  break;
            case CMD_PASTE_PART:
                  paste();
                  break;
            case CMD_PASTE_CLONE_PART:
                  paste(true);
                  break;
            case CMD_PASTE_DIALOG:
            case CMD_PASTE_CLONE_DIALOG:
            {
                  unsigned temp_begin = AL::sigmap.raster1(song->vcpos(),0);
                  unsigned temp_end = AL::sigmap.raster2(temp_begin + MusEUtil::get_paste_len(), 0);
                  paste_dialog->raster = temp_end - temp_begin;
                  paste_dialog->clone = (cmd == CMD_PASTE_CLONE_DIALOG);
                  
                  if (paste_dialog->exec())
                  {
                    paste_mode_t paste_mode;
                    switch (paste_dialog->insert_method)
                    {
                      case 0: paste_mode=PASTEMODE_MIX; break;
                      case 1: paste_mode=PASTEMODE_MOVEALL; break;
                      case 2: paste_mode=PASTEMODE_MOVESOME; break;
                    }

                    paste(paste_dialog->clone, paste_mode, paste_dialog->all_in_one_track,
                          paste_dialog->number, paste_dialog->raster);
                  }
                  
                  break;
            }
            case CMD_INSERT_EMPTYMEAS:
                  int startPos=song->vcpos();
                  int oneMeas=AL::sigmap.ticksMeasure(startPos);
                  Undo temp=movePartsTotheRight(startPos,oneMeas);
                  song->applyOperationGroup(temp);
                  break;
            }
      }

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void PartCanvas::copy_in_range(PartList* pl_)
{
  PartList pl;
  PartList result_pl;
  unsigned int lpos = song->lpos();
  unsigned int rpos = song->rpos();
  
  if (pl_->empty())
  {
    for (MusEWidget::iCItem i = items.begin(); i != items.end(); ++i)
    {
      Part* part=static_cast<NPart*>(i->second)->part();
      if ( (part->track()->isMidiTrack()) || (part->track()->type() == Track::WAVE) )
        pl.add(part);
    }
  }
  else
  {
    for(ciPart p = pl_->begin(); p != pl_->end(); ++p) 
      if ( (p->second->track()->isMidiTrack()) || (p->second->track()->type() == Track::WAVE) )
        pl.add(p->second);
  }
  
  if (!pl.empty() && (rpos>lpos))
  {
    for(ciPart p = pl.begin(); p != pl.end(); ++p) 
    {
      Part* part=p->second;
      Track* track=part->track();
      
      if ((part->tick() < rpos) && (part->endTick() > lpos)) //is the part in the range?
      {
        if ((lpos > part->tick()) && (lpos < part->endTick()))
        {
          Part* p1;
          Part* p2;
          
          track->splitPart(part, lpos, p1, p2);
          p1->events()->incARef(-1);
          p2->events()->incARef(-1);
          
          part=p2;
        }
        
        if ((rpos > part->tick()) && (rpos < part->endTick()))
        {
          Part* p1;
          Part* p2;
          
          track->splitPart(part, rpos, p1, p2);
          p1->events()->incARef(-1);
          p2->events()->incARef(-1);

          part=p1;
        }
        
        result_pl.add(part);
      }
    }
    
    copy(&result_pl);
  }
}

void PartCanvas::copy(PartList* pl)
      {
      if (pl->empty())
            return;
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
      
      int level = 0;
      int tick = 0;
      for (ciPart p = pl->begin(); p != pl->end(); ++p) {
            // Indicate this is a copy operation. Also force full wave paths.
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
      
      QByteArray data(fbuf);
      QMimeData* md = new QMimeData();
      
      
      if(midi && wave)
        md->setData("text/x-muse-mixedpartlist", data);   // By T356. Support mixed .mpt files.
      else
      if(midi)
        md->setData("text/x-muse-midipartlist", data);
      else
      if(wave)
        md->setData("text/x-muse-wavepartlist", data);
        
      QApplication::clipboard()->setMimeData(md, QClipboard::Clipboard);
      
      munmap(fbuf, n);
      fclose(tmp);
      }


Undo PartCanvas::pasteAt(const QString& pt, Track* track, unsigned int pos, bool clone, bool toTrack, int* finalPosPtr, set<Track*>* affected_tracks)
      {
      Undo operations;
      
      QByteArray ba = pt.toLatin1();
      const char* ptxt = ba.constData();
      Xml xml(ptxt);
      bool firstPart=true;
      int  posOffset=0;
      unsigned int  finalPos = pos;
      int  notDone = 0;
      int  done = 0;
      bool end = false;

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

                              p->events()->incARef(-1); // the later song->applyOperationGroup() will increment it
                                                        // so we must decrement it first :/
                              
                              // Increment the number of parts done.
                              ++done;
                              
                              if (firstPart) {
                                    firstPart=false;
                                    posOffset=pos-p->tick();
                                    }
                              p->setTick(p->tick()+posOffset);
                              if (p->tick()+p->lenTick()>finalPos) {
                                finalPos=p->tick()+p->lenTick();
                              }
                              p->setSelected(true);
                              operations.push_back(UndoOp(UndoOp::AddPart,p));
                              if (affected_tracks)
                                affected_tracks->insert(p->track());
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
      
      
      if(notDone)
      {
        int tot = notDone + done;
        QMessageBox::critical(this, QString("MusE"),
           (tot > 1  ?  tr("%n part(s) out of %1 could not be pasted.\nLikely the selected track is the wrong type.","",notDone).arg(tot)
                     :  tr("%n part(s) could not be pasted.\nLikely the selected track is the wrong type.","",notDone)));
      }
      
      if (finalPosPtr) *finalPosPtr=finalPos;
      return operations;
      }


//---------------------------------------------------------
//   paste
//    paste part to current selected track at cpos
//---------------------------------------------------------

void PartCanvas::paste(bool clone, paste_mode_t paste_mode, bool to_single_track, int amount, int raster)
{
      Track* track = 0;
      
      // If we want to paste to a selected track...
      if (to_single_track)
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
      const QMimeData* md = cb->mimeData(QClipboard::Clipboard);
      
      QString pfx("text/");
      QString mdpl("x-muse-midipartlist");
      QString wvpl("x-muse-wavepartlist");  
      QString mxpl("x-muse-mixedpartlist");
      QString txt;
        
      if(md->hasFormat(pfx + mdpl))
      {
        // If we want to paste to a selected track...
        if(to_single_track && !track->isMidiTrack()) 
        {
          QMessageBox::critical(this, QString("MusE"),
            tr("Can only paste to midi/drum track"));
          return;
        }
        txt = cb->text(mdpl, QClipboard::Clipboard);  
      }
      else if(md->hasFormat(pfx + wvpl))
      {
        // If we want to paste to a selected track...
        if(to_single_track && track->type() != Track::WAVE) 
        {
          QMessageBox::critical(this, QString("MusE"),
            tr("Can only paste to wave track"));
          return;
        }
        txt = cb->text(wvpl, QClipboard::Clipboard);  
      }  
      else if(md->hasFormat(pfx + mxpl))
      {
        // If we want to paste to a selected track...
        if(to_single_track && !track->isMidiTrack() && track->type() != Track::WAVE) 
        {
          QMessageBox::critical(this, QString("MusE"),
            tr("Can only paste to midi or wave track"));
          return;
        }
        txt = cb->text(mxpl, QClipboard::Clipboard);  
      }
      else
      {
        QMessageBox::critical(this, QString("MusE"),
          tr("Cannot paste: wrong data type"));
        return;
      }
      
      if (!txt.isEmpty())
      {
        int endPos=0;
        unsigned int startPos=song->vcpos();
        set<Track*> affected_tracks;

        deselectAll();
        
        Undo operations;
        for (int i=0;i<amount;i++)
        {
          Undo temp = pasteAt(txt, track, startPos + i*raster, clone, to_single_track, &endPos, &affected_tracks);
          operations.insert(operations.end(), temp.begin(), temp.end());
        }
        
        Pos p(endPos, true);
        song->setPos(0, p);
        
        if (paste_mode != PASTEMODE_MIX)
        {
          int offset;
          if (amount==1) offset = endPos-startPos;
          else           offset = amount*raster;
            
          Undo temp;
          if (paste_mode==PASTEMODE_MOVESOME)
            temp=movePartsTotheRight(startPos, offset, false, &affected_tracks);
          else
            temp=movePartsTotheRight(startPos, offset);
            
          operations.insert(operations.end(), temp.begin(), temp.end());
        }
        
        song->applyOperationGroup(operations);
      }

    }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PartCanvas::startDrag(MusEWidget::CItem* item, DragType t)
      {
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
      
      QByteArray data(fbuf);
      QMimeData* md = new QMimeData();
      
      md->setData("text/x-muse-partlist", data);
      
      // "Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object. 
      //  The QDrag must be constructed on the heap with a parent QWidget to ensure that Qt can 
      //  clean up after the drag and drop operation has been completed. "
      QDrag* drag = new QDrag(this);
      drag->setMimeData(md);
      
      if (t == MOVE_COPY || t == MOVE_CLONE)
            drag->exec(Qt::CopyAction);
      else
            drag->exec(Qt::MoveAction);
            
      munmap(fbuf, n);
      fclose(tmp);
      }

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void PartCanvas::dragEnterEvent(QDragEnterEvent* event)
      {
      event->acceptProposedAction();  // TODO CHECK Tim.
      }

//---------------------------------------------------------
//   dragvent
//---------------------------------------------------------

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void PartCanvas::viewDropEvent(QDropEvent* event)
      {
      if (MusEGlobal::debugMsg)
            printf("void PartCanvas::viewDropEvent(QDropEvent* event)\n");
      if (event->source() == this) {
            printf("local DROP\n");    
            //event->ignore();                     // TODO CHECK Tim.
            return;
            }
      int type = 0;     // 0 = unknown, 1 = partlist, 2 = uri-list
      QString text;
      
      if(event->mimeData()->hasFormat("text/partlist")) 
        type = 1;
      else 
      if(event->mimeData()->hasUrls()) 
        type = 2;
      else 
      {
        if(MusEGlobal::debugMsg && event->mimeData()->formats().size() != 0)
          printf("Drop with unknown format. First format:<%s>\n", event->mimeData()->formats()[0].toLatin1().constData());
        //event->ignore();                     // TODO CHECK Tim.
        return;  
      }
      
      // Make a backup of the current clone list, to retain any 'copy' items,
      //  so that pasting works properly after.
      CloneList copyCloneList = cloneList;
      // Clear the clone list to prevent any dangerous associations with
      //  current non-original parts.
      cloneList.clear();
      
      if (type == 1) 
      {
          printf("type1\n");
            text = QString(event->mimeData()->data("text/partlist"));
            
            int x = AL::sigmap.raster(event->pos().x(), *_raster);
            if (x < 0)
                  x = 0;
            unsigned trackNo = y2pitch(event->pos().y());
            Track* track = 0;
            if (trackNo < tracks->size())
                  track = tracks->index(trackNo);
            if (track)
            {
                  deselectAll();
                  Undo temp=pasteAt(text, track, x);
                  song->applyOperationGroup(temp);
            }
      }
      else if (type == 2) 
      {
          unsigned trackNo = y2pitch(event->pos().y());
          Track* track = 0;
          if (trackNo < tracks->size())
                track = tracks->index(trackNo);

          int x = AL::sigmap.raster(event->pos().x(), *_raster);
          if (x < 0)
                x = 0;

          foreach(QUrl url, event->mimeData()->urls())
          {
            text = url.path();
            
            if (text.endsWith(".wav",Qt::CaseInsensitive) || 
                text.endsWith(".ogg",Qt::CaseInsensitive) || 
                text.endsWith(".mpt", Qt::CaseInsensitive) )
            {

                if (!track) { // we need to create a track for this drop
                    if (text.endsWith(".mpt", Qt::CaseInsensitive)) {
                        track = song->addTrack(Track::MIDI);    // Add at end of list.
                    } else {
                        track = song->addTrack(Track::WAVE);    // Add at end of list.
                    }
                }
                if (track->type() == Track::WAVE &&
                        (text.endsWith(".wav", Qt::CaseInsensitive) || 
                          (text.endsWith(".ogg", Qt::CaseInsensitive))))
                        {
                        unsigned tick = x;
                        MusEGlobal::muse->importWaveToTrack(text, tick, track);
                        }
                      // Changed by T356. Support mixed .mpt files.
                      else if ((track->isMidiTrack() || track->type() == Track::WAVE) && text.endsWith(".mpt", Qt::CaseInsensitive))
                        {
                        unsigned tick = x;
                        MusEGlobal::muse->importPartToTrack(text, tick, track);
                        }
            }
            else if(text.endsWith(".med",Qt::CaseInsensitive))
            {
                emit dropSongFile(text);
                break; // we only support ONE drop of this kind
            }                        
            else if(text.endsWith(".mid",Qt::CaseInsensitive))
            {
                emit dropMidiFile(text);
            }
            else
            {
                printf("dropped... something...  no hable...\n");
            }
            track=0;
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
      //int y = rect.y();
      int w = rect.width();
      //int h = rect.height();
      
      // Changed to draw in device coordinate space instead of virtual, transformed space.     Tim. p4.0.30  
      
      //QRect mr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect mr = map(rect);                      // Use our own map instead.
      
      p.save();
      p.setWorldMatrixEnabled(false);
      
      int mx = mr.x();
      int my = mr.y();
      int mw = mr.width();
      int mh = mr.height();

      //////////
      // GRID //
      //////////
      QColor baseColor(MusEConfig::config.partCanvasBg.light(104));
      p.setPen(baseColor);

      //--------------------------------
      // vertical lines
      //-------------------------------
      if (MusEConfig::config.canvasShowGrid) {
          int bar, beat;
          unsigned tick;

          AL::sigmap.tickValues(x, &bar, &beat, &tick);
          for (;;) {
            int xt = AL::sigmap.bar2tick(bar++, 0, 0);
            //int xt = mapx(AL::sigmap.bar2tick(bar++, 0, 0));
            if (xt >= x + w)
            //if (xt >= mx + mw)
                  break;
            if (!((bar-1) % 4))
                p.setPen(baseColor.dark(115));
            else
                p.setPen(baseColor);
            //p.drawLine(xt, y, xt, y+h);
            int xtm = mapx(xt);
            p.drawLine(xtm, my, xtm, my+mh);

            // append
            int noDivisors=0;
            if (*_raster == MusEConfig::config.division *2)         // 1/2
                noDivisors=2;
            else if (*_raster== MusEConfig::config.division)        // 1/4
                noDivisors=4;
            else if (*_raster==MusEConfig::config.division/2)         // 1/8
                noDivisors=8;
            else if (*_raster==MusEConfig::config.division/4)          // 1/16
                noDivisors=16;
            else if (*_raster==MusEConfig::config.division/8)          // 1/16
                noDivisors=32;
            else if (*_raster==MusEConfig::config.division/16)          // 1/16
                noDivisors=64;

            int r = *_raster;
            int rr = rmapx(r);
            if (*_raster > 1) {
              while (rr < 4) {
                r *= 2;
                rr = rmapx(r);
                noDivisors=noDivisors/2;
              }
              p.setPen(baseColor);
              int xx;
              for (int t=1;t< noDivisors;t++)
              {
                //p.drawLine(xt+r*t, y, xt+r*t, y+h);
                xx = mapx(xt + r * t);
                p.drawLine(xx, my, xx, my+mh);
              }  
            }
          }
      }
      
      //--------------------------------
      // horizontal lines
      //--------------------------------

      TrackList* tl = song->tracks();
      //int yy = 0;
      int yy = -rmapy(yorg) - ypos;
      int th;
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
            //if (yy > y + h)
            if (yy > my + mh)
                  break;
            Track* track = *it;
            th = track->height();
            if (!th)
              continue;
            if (MusEConfig::config.canvasShowGrid && (track->isMidiTrack() || track->type() == Track::WAVE))   // Tim.
            {
              p.setPen(baseColor.dark(130));
              //p.drawLine(x, yy + th, x + w, yy + th);  
              p.drawLine(mx, yy + th, mx + mw, yy + th);  
              //p.setPen(baseColor);
            }
            
            // The update rectangle (rect and mr etc) is clipped at x<0 and y<0 in View::pdraw(). 
            // The 'corrupt drawing' bug of drawAudioTrack was actually because the recently added gradient 
            //  used the update rectangle, so the gradient was also being clipped at 0,0.
            // One could remove that limiter, but no, that is the correct way. So instead let's construct a 
            //  'pseudo bounding box' (half update rectangle, half bounding box), un-clipped. The gradient needs this!       
            //
            // Here is a different situation than PartCanvas::drawItem which uses un-clipped part bounding boxes and 
            //  does NOT depend on the update rectangle (except to check intersection). That's why this issue 
            //  does not show up there. Should probably try to make that routine more efficient, just like here.   Tim. p4.0.30
            QRect r(mx, yy, mw, th);
            //if(r.intersects(mr)) 
            {
              if (!track->isMidiTrack() && (track->type() != Track::WAVE)) {
                    //QRect r = rect & QRect(x, yy, w, track->height());
                    drawAudioTrack(p, mr, r, (AudioTrack*)track);
                    //p.setPen(baseColor);
              }
              
              // This was redundant drawing. Not required, done via drawTopItem in Canvas::draw
              /*
              //p.setWorldMatrixEnabled(true);
              //if (!track->isMidiTrack()) { // draw automation
              if (!track->isMidiTrack() && (track->type() != Track::WAVE)) {
                    //QRect r = rect & QRect(x, yy, w, track->height());
                    drawAutomation(p, r, (AudioTrack*)track);
                    //p.setPen(baseColor);
              }
              //p.setWorldMatrixEnabled(false);
              */
            }
            yy += th;
      }
      
      p.restore();
      //p.setWorldMatrixEnabled(true);
}

//---------------------------------------------------------
//   drawLast
//---------------------------------------------------------
void PartCanvas::drawTopItem(QPainter& p, const QRect& rect)
{
    //int x = rect.x();
    //int y = rect.y();
    //int w = rect.width();
    //int h = rect.height();

    // Changed to draw in device coordinate space instead of virtual, transformed space.     Tim. p4.0.30  
    
    //QRect mr = p.transform().mapRect(rect);  // Gives inconsistent positions. Source shows wrong operation for our needs.
    QRect mr = map(rect);                      // Use our own map instead.
    
    //printf("PartCanvas::drawTopItem x:%d y:%d w:%d h:%d\n", rect.x(), rect.y(), rect.width(), rect.height()); 
    
    int mx = mr.x();
    int my = mr.y();
    int mw = mr.width();
    int mh = mr.height();
    
    QColor baseColor(MusEConfig::config.partCanvasBg.light(104));
    //p.setPen(baseColor);

    p.save();
    p.setWorldMatrixEnabled(false);
    
    TrackList* tl = song->tracks();
    int yoff = -rmapy(yorg) - ypos;
    //int yy = 0;
    int yy = yoff;
    int th;
    for (iTrack it = tl->begin(); it != tl->end(); ++it) {
          //if (yy > y + h)
          if (yy > my + mh)
                break;
          Track* track = *it;
          th = track->height();
          if (!th)
            continue;
          if (!track->isMidiTrack()) { // draw automation
                //QRect r = rect & QRect(x, yy, w, track->height());
                QRect r(mx, yy, mw, th);
                if(r.intersects(mr)) 
                {
                  drawAutomation(p, r, (AudioTrack*)track);
                  //p.setPen(baseColor);
                }  
          }
          //yy += track->height();
          yy += th;
          }

    unsigned int startPos = audio->getStartRecordPos().tick();
    if (song->punchin())
      startPos=song->lpos();
    int startx = mapx(startPos);
    int width = mapx(song->cpos()) - mapx(startPos);

    if (song->cpos() < startPos) {
        //p.setWorldMatrixEnabled(true);
        p.restore();
        return; // no drawing if we are before punch out
    }
    if (song->punchout() && song->cpos() > song->rpos()) {
       //p.setWorldMatrixEnabled(true);
       p.restore();
       return; // no drawing if we are beyond punch out.
    }

    ///p.save();
    ///p.resetTransform();

    // write recording while it happens to get feedback
    // should be enhanced with solution that draws waveform also
    //int yPos=0;
    int yPos = yoff;
    if (song->record() && audio->isPlaying()) {
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
        Track* track = *it;
        th = track->height();
        if (!th)
          continue;
        if (track->recordFlag()) {
            QPen pen(Qt::black, 0, Qt::SolidLine);
            p.setPen(pen);
            QColor c(MusEConfig::config.partColors[0]);
            c.setAlpha(MusEConfig::config.globalAlphaBlend);
            QLinearGradient gradient(QPoint(startx,yPos), QPoint(startx,yPos+th));
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            QBrush cc(gradient);
            p.setBrush(cc);

            p.drawRect(startx,yPos, width, th);
        }
        yPos+=th;
      }
    }
    p.restore();
    //p.setWorldMatrixEnabled(true);

    // draw midi events on
    yPos=0;
    if (song->record() && audio->isPlaying()) {
      for (iTrack it = tl->begin(); it != tl->end(); ++it) {
           Track* track = *it;

           if (track->isMidiTrack() && track->recordFlag()) {
               MidiTrack *mt = (MidiTrack*)track;
               QRect partRect(startPos,yPos, song->cpos()-startPos, track->height()); // probably the wrong rect
               EventList myEventList;
               MPEventList *el = mt->mpevents();
               if (el->size()) {

                 for (iMPEvent i = el->begin(); i != el->end(); ++i) {
                    MidiPlayEvent pe = *i;

                    if (pe.isNote() && !pe.isNoteOff()) {
                      Event e(Note);
                      e.setPitch(pe.dataA());
                      e.setTick(pe.time()-startPos);
                      e.setLenTick(song->cpos()-pe.time());
                      e.setC(1); // we abuse this value to determine that this note hasn't been concluded
                      myEventList.add(e);
                    }
                    else if (pe.isNoteOff()) {
                      for (iEvent i = myEventList.begin(); i != myEventList.end(); ++i) {
                        Event &e = i->second;
                        if (e.pitch() == pe.dataA() && e.dataC() == 1) {
                          e.setLenTick(pe.time() - e.tick()- startPos);
                          e.setC(0); // reset the variable we borrowed for state handling
                          continue;
                        }
                      }
                    }
                 }
               }
               drawMidiPart(p, rect, &myEventList, mt, 0, partRect,startPos,0,song->cpos()-startPos);
           }
           yPos+=track->height();
      }
    }

}

//---------------------------------------------------------
//   drawAudioTrack
//---------------------------------------------------------
void PartCanvas::drawAudioTrack(QPainter& p, const QRect& r, const QRect& bbox, AudioTrack* /*t*/)
{
      QRect mr = r & bbox; 
      if(mr.isNull())
        return;
      int mx = mr.x();
      int my = mr.y();
      int mw = mr.width();
      int mh = mr.height();
      int mex = bbox.x();
      int mey = bbox.y();
      //int mew = bbox.width();
      int meh = bbox.height();
      
      p.setPen(Qt::black);
      QColor c(Qt::gray);
      c.setAlpha(MusEConfig::config.globalAlphaBlend);
      //QLinearGradient gradient(r.topLeft(), r.bottomLeft());
      QLinearGradient gradient(mex + 1, mey + 1, mex + 1, mey + meh - 1);    // Inside the border
      gradient.setColorAt(0, c);
      gradient.setColorAt(1, c.darker());
      QBrush brush(gradient);
      p.fillRect(mr, brush);       // p4.0.30  ...
      
      //int xx = -rmapx(xorg) - xpos;           
      //printf("PartCanvas::drawAudioTrack x:%d y:%d w:%d h:%d th:%d xx:%d\n", r.x(), r.y(), r.width(), r.height(), t->height(), xx);  
      //if(r.x() <= xx)
      //  p.drawLine(r.x(), r.y(), r.x(), r.y() + r.height());                          // The left edge
      //p.drawLine(r.x(), r.y(), r.x() + r.width(), r.y());                             // The top edge
      //p.drawLine(r.x(), r.y() + r.height(), r.x() + r.width(), r.y() + r.height());   // The bottom edge
      
      if(mex >= mx && mex <= mx + mw)
        p.drawLine(mex, my, mex, my + mh - 1);                // The left edge
      //if(mex + mew >= mx && mex + mew <= mx + mw)
      //  p.drawLine(mex + mew, my, mex + mew, my + mh - 1);  // The right edge. Not used - infinite to the right
      if(mey >= my && mey <= my + mh)
        p.drawLine(mx, mey, mx + mw - 1, mey);                // The top edge
      if(mey + meh >= my && mey + meh <= my + mh)
        p.drawLine(mx, mey + meh, mx + mw - 1, mey + meh);    // The bottom edge. Special for Audio track - draw one past bottom.
}

//---------------------------------------------------------
//   drawAutomation
//---------------------------------------------------------

void PartCanvas::drawAutomation(QPainter& p, const QRect& rr, AudioTrack *t)
{
    //QRect rr = p.worldMatrix().mapRect(r);
    //p.save();
    //p.resetTransform();

    int bottom = rr.bottom() - 2;
    int height = bottom - rr.top() - 2; // limit height

    //printf("PartCanvas::drawAutomation x:%d y:%d w:%d h:%d height:%d\n", rr.x(), rr.y(), rr.width(), rr.height(), height); 
    
    p.setBrush(Qt::NoBrush);
    
    CtrlListList* cll = t->controller();
    for(CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
    {
      CtrlList *cl = icll->second;
      if (cl->dontShow() || !cl->isVisible())
        continue;
      iCtrl ic=cl->begin();
      int oldX = mapx(0);
      if(rr.right() < oldX)
      {
        //p.restore();
        return;
      }  
      int xpixel = oldX;
      int oldY = -1;
      int ypixel = oldY;
      double min, max;
      cl->range(&min,&max);
      //bool discrete = cl->valueType() == VAL_BOOL || cl->mode() == CtrlList::DISCRETE;  
      bool discrete = cl->mode() == CtrlList::DISCRETE;  
      QPen pen1(cl->color(), 0);  
      QPen pen2(cl->color(), 2);  
      pen2.setCosmetic(true);

      // First check that there ARE automation, ic == cl->end means no automation
      if (ic == cl->end()) 
      {
        double y;   
        if (cl->valueType() == VAL_LOG ) { // use db scale for volume
          y = dbToVal(cl->curVal()); // represent volume between 0 and 1
          if (y < 0) y = 0.0;
        }
        else 
          y = (cl->curVal() - min)/(max-min);  // we need to set curVal between 0 and 1
        ypixel = oldY = bottom - rmapy_f(y) * height;
      }
      else
      {
        for (; ic !=cl->end(); ++ic)
        {
            double y = ic->second.val; 
            if (cl->valueType() == VAL_LOG ) { // use db scale for volume
              y = dbToVal(y); // represent volume between 0 and 1
              if (y < 0) y = 0.0;
            }
            else 
              y = (y-min)/(max-min);  // we need to set curVal between 0 and 1
            
            ypixel = bottom - rmapy_f(y) * height;
            xpixel = mapx(tempomap.frame2tick(ic->second.frame));
            
            if (oldY==-1) oldY = ypixel;

            //printf(" line x1:%d x2:%d prevVal:%f nextVal:%f\n", leftX, currentPixel, prevVal, nextVal); 
            p.setPen(pen1);
            if(discrete)
            {
              p.drawLine(oldX, oldY, xpixel, oldY); 
              p.drawLine(xpixel, oldY, xpixel, ypixel); 
            }
            else
              p.drawLine(oldX, oldY, xpixel, ypixel); 
                      
            if (xpixel > rr.right())
              break;

            // draw a square around the point
            //p.drawRect(mapx(tempomap.frame2tick(prevPosFrame))-2, (rr.bottom()-2)-prevVal*height-2, 5, 5);
            //p.drawRect(mapx(tempomap.frame2tick(prevPosFrame))-1, (rr.bottom()-1)-prevVal*height-2, 3, 3);
            pen2.setColor((automation.currentCtrlValid && automation.currentCtrlList == cl && 
                           automation.currentCtrlFrame == ic->second.frame) ? 
                          Qt::white : cl->color());  
            
            p.setPen(pen2);
            p.drawRect(xpixel-2, ypixel-2, 5, 5);
            oldX = xpixel;
            oldY = ypixel;
        }
      }
      if (xpixel <= rr.right())
      {
        //printf(" endline prevVal:%f\n", prevVal); 
        p.setPen(pen1);
        p.drawLine(xpixel, ypixel, rr.right(), ypixel);
      }  
    }
    //p.restore();
}


//---------------------------------------------------------
//  checkAutomation
//    compares the current mouse pointer with the automation
//    lines on the track under it.
//    if there is a controller to be moved it is marked
//    in the automation object
//    if addNewCtrl is set and a valid line is found the
//    automation object will also be set but no
//    controller added.
//---------------------------------------------------------

void PartCanvas::checkAutomation(Track * t, const QPoint &pointer, bool addNewCtrl)
{
    if (t->isMidiTrack())
      return;

    int currY;
    int trackY = t->y();
    int trackH = t->height();
    
    { int y = pointer.y();
      if(y < trackY || y >= (trackY + trackH))
        return; 
      currY =  mapy(y);  }
    
    int currX =  mapx(pointer.x());
    int circumference = 5;
    
    CtrlListList* cll = ((AudioTrack*) t)->controller();
    for(CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
      {
        CtrlList *cl = icll->second;
        if (cl->dontShow() || !cl->isVisible()) {
          continue;
        }
        iCtrl ic=cl->begin();

        int oldX = mapx(0);
        int xpixel = oldX;
        int oldY = -1;
        int ypixel = oldY;
        double min, max;
        cl->range(&min,&max);
        //bool discrete = cl->valueType() == VAL_BOOL || cl->mode() == CtrlList::DISCRETE;  // Tim
        bool discrete = cl->mode() == CtrlList::DISCRETE;  

        // First check that there IS automation, ic == cl->end means no automation
        if (ic == cl->end()) 
        {
          double y;   
          if (cl->valueType() == VAL_LOG ) { // use db scale for volume
            y = dbToVal(cl->curVal()); // represent volume between 0 and 1
            if (y < 0) y = 0.0;
          }
          else 
            y = (cl->curVal() - min)/(max-min);  // we need to set curVal between 0 and 1
          ypixel = oldY = mapy(trackY+trackH-1 - 2 - y * trackH);
        }
        else
        {  
          for (; ic !=cl->end(); ic++)
          {
             double y = ic->second.val;
             if (cl->valueType() == VAL_LOG ) { // use db scale for volume
                y = dbToVal(y); // represent volume between 0 and 1
               if (y < 0) y = 0;
             }
             else 
               y = (y-min)/(max-min);  // we need to set curVal between 0 and 1
             
             ypixel = mapy(trackY + trackH - 2 - y * trackH);
             xpixel = mapx(tempomap.frame2tick(ic->second.frame));

             if (oldY==-1) oldY = ypixel;
             //printf("     oldX:%d oldY:%d xpixel:%d ypixel:%d\n", oldX, oldY, xpixel, ypixel); 
             
             bool foundIt=false;
             if (addNewCtrl) {
                // check if we are reasonably close to a line
               //printf("xpixel=%d oldX=%d\n", xpixel, oldX);
               double firstX=oldX;
               double lastX=xpixel;
               double firstY=oldY;
               double lastY = discrete ? oldY : ypixel;

               double proportion = (currX-firstX)/(lastX-firstX);

               //if (    (currX > lastX && firstY!=lastY) // omit special cases.
               //     || firstX==lastX ) {
               if((currX < oldX) || (currX > lastX) || (firstX==lastX) ) 
               {
                    oldX = xpixel;
                    oldY = ypixel;
                    continue; // not the right region
               }

               // 10   X(15)   20
               // proportion = 0.5
               //              10
               //          /
               //      Y(5)
               //   /
               // 1
               double calcY = (lastY-firstY)*proportion+firstY;
               //printf("calcY=%f currY=%d\n", calcY, currY);
               if(ABS(calcY-currY) < circumference || (xpixel == oldX && ABS(currX-xpixel) < circumference))
                 foundIt=true;
             
             } else {
               int x1 = ABS(currX - xpixel) ;
               int y1 = ABS(currY - ypixel);
               if (x1 < circumference &&  y1 < circumference && pointer.x() > 0 && pointer.y() > 0) {
                 foundIt=true;
               }

             }
             oldX = xpixel;
             oldY = ypixel;

             if (foundIt) {
               QWidget::setCursor(Qt::CrossCursor);
               if (addNewCtrl) {
                 //automation.currentCtrl = 0;
                 automation.currentCtrlValid = false;
                 automation.controllerState = addNewController;
               }else {
                 //automation.currentCtrl=&ic->second;
                 automation.currentCtrlFrame = ic->second.frame;
                 //automation.currentCtrlVal = ic->second.val;
                 automation.currentCtrlValid = true;
                 automation.controllerState = movingController;
               }
               automation.currentCtrlList = cl;
               automation.currentTrack = t;
               return;
             }
          }
        } 

        if (addNewCtrl) {
          // check if we are reasonably close to a line, we only need to check Y
          // as the line is straight after the last controller
          //printf("post oldX:%d oldY:%d xpixel:%d ypixel:%d currX:%d currY:%d\n", oldX, oldY, xpixel, ypixel, currX, currY); 
          if(currX >= xpixel && ypixel == oldY && ABS(currY-ypixel) < circumference) {
            QWidget::setCursor(Qt::CrossCursor);
            automation.controllerState = addNewController;
            automation.currentCtrlList = cl;
            automation.currentTrack = t;
            //automation.currentCtrl = 0;
            automation.currentCtrlValid = false;
            return;
          }
        }
      }
      // if there are no hits we default to clearing all the data
      automation.controllerState = doNothing;
      //automation.currentCtrl = 0;
      automation.currentCtrlValid = false;
      automation.currentCtrlList = 0;
      automation.currentTrack = 0;
      setCursor();
}

void PartCanvas::controllerChanged(Track* t)
{
  //redraw();
  redraw((QRect(0, mapy(t->y()), width(), rmapy(t->height()))));  // TODO Check this - correct?
}

void PartCanvas::processAutomationMovements(QPoint pos, bool addPoint)
{

  if (_tool == MusEWidget::AutomationTool) {

      if (!automation.moveController) { // currently nothing going lets's check for some action.
          Track * t = y2Track(pos.y());
          if (t) {
             checkAutomation(t, pos, addPoint);
          }
          return;
      }

    // automation.moveController is set, lets rock.

    int prevFrame = 0;
    int nextFrame = -1;
    int currFrame = 0;

    if (automation.controllerState == addNewController)
    {
       //printf("adding a new ctrler!\n");
       int frame = tempomap.tick2frame(pos.x());
       // FIXME Inefficient to add with wait here, then remove and add with wait again below. Tim.
       audio->msgAddACEvent((AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), frame, 1.0 /*dummy value */);
       //song->addACEvent((AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), frame, 1.0 /*dummy value */);

       iCtrl ic=automation.currentCtrlList->begin();
       for (; ic !=automation.currentCtrlList->end(); ++ic) {
         CtrlVal &cv = ic->second;
         if (cv.frame == frame) {
           //automation.currentCtrl = &cv;
           automation.currentCtrlFrame = cv.frame;
           //automation.currentCtrlVal = cv.val;
           automation.currentCtrlValid = true;
           automation.controllerState = movingController;
           break;
         }
       }
    }

    // get previous and next frame position to give x bounds for this event.
    iCtrl ic=automation.currentCtrlList->begin();
    iCtrl iprev = ic;
    for (; ic !=automation.currentCtrlList->end(); ++ic)
    {
       CtrlVal &cv = ic->second;
       //if (&cv == automation.currentCtrl)
       if (cv.frame == automation.currentCtrlFrame)
       {
         currFrame = cv.frame;
         break;
       }  
       prevFrame = cv.frame;
       iprev = ic;
    }
    
    iCtrl icc = ic;

    if ( ++ic != automation.currentCtrlList->end()) {
      CtrlVal &cv = ic->second;
      nextFrame = cv.frame;
    }
    
    // A perfectly straight vertical line (two points with same frame) is impossible:
    //  there is only one value at t, and the next value at t+1, and so on.
    // Also these are maps, not multimaps.           p4.0.32 Tim.
    int newFrame = tempomap.tick2frame(pos.x());
    //if(currFrame == 0) 
    //  newFrame = 0;  // Force first item to stay at x = 0.
    //else 
    if (newFrame <= prevFrame) 
      newFrame=prevFrame + (icc == automation.currentCtrlList->begin() ? 0: 1);  // Only first item is allowed to go to zero x.
    if (nextFrame!=-1 && newFrame >= nextFrame) newFrame=nextFrame-1;
    
    int posy=mapy(pos.y());
    int tracky = mapy(automation.currentTrack->y());
    int trackHeight = automation.currentTrack->height();
    //printf("posy=%d tracky=%d trackHeight=%d\n", posy,tracky,trackHeight);

    int mouseY = trackHeight - (posy - tracky)-2;
    double yfraction = ((double)mouseY)/automation.currentTrack->height();

    double min, max;
    automation.currentCtrlList->range(&min,&max);
    double cvval;    
    if (automation.currentCtrlList->valueType() == VAL_LOG  ) { // use db scale for volume
       cvval = valToDb(yfraction);
       //printf("calc yfraction = %f v=%f ",yfraction,cvval);
       if (cvval< min) cvval=min;
       if (cvval>max) cvval=max;
    }
    else {
      // we need to set val between 0 and 1 (unless integer)
      cvval = yfraction * (max-min) + min;
      // 'Snap' to integer or boolean
      //if (automation.currentCtrlList->valueType() == VAL_INT || automation.currentCtrlList->valueType() == VAL_BOOL)
      if (automation.currentCtrlList->mode() == CtrlList::DISCRETE)
        cvval = rint(cvval + 0.1); // LADSPA docs say add a slight bias to avoid rounding errors. Try this.
      if (cvval< min) cvval=min;
      if (cvval>max) cvval=max;
    }
    
    automation.currentCtrlFrame = newFrame;
    //automation.currentCtrlVal = cvval;
    automation.currentCtrlValid = true;
    
    if(icc != automation.currentCtrlList->end())
      audio->msgChangeACEvent((AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), icc->second.frame, newFrame, cvval);
      //song->changeACEvent((AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), icc->second.frame, newFrame, cvval);
    else
      audio->msgAddACEvent((AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), newFrame, cvval);
      //song->addACEvent((AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), newFrame, cvval);
    
    // Not needed. Redraw is now handled by msgXXX().
    //controllerChanged(automation.currentTrack);
  }

}

double PartCanvas::dbToVal(double inDb)
{
    return (20.0*fast_log10(inDb)+60.0) / 70.0;
}
double PartCanvas::valToDb(double inV)
{
    return exp10((inV*70.0-60.0)/20.0);
}

//---------------------------------------------------------
//   endMoveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void PartCanvas::endMoveItems(const QPoint& pos, DragType dragtype, int dir)
      {
      int dp = y2pitch(pos.y()) - y2pitch(start.y());
      int dx = pos.x() - start.x();

      if (dir == 1)
            dp = 0;
      else if (dir == 2)
            dx = 0;

      moveCanvasItems(moving, dp, dx, dragtype);
      
      moving.clear();
      updateSelection();
      redraw();
      }

} // namespace MusEArranger
