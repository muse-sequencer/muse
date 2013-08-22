//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pcanvas.cpp,v 1.48.2.26 2009/11/22 11:08:33 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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
#include <limits.h>
#include <uuid/uuid.h>
#include <math.h>
#include <map>

#include <QClipboard>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>
#include <QPoint>
#include <QIcon>

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
#include "wave.h"
#include "audio.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "app.h"
#include "functions.h"
#include "filedialog.h"
#include "marker/marker.h"
#include "menutitleitem.h"
#include "mpevent.h"
#include "midievent.h"
#include "midi.h"
#include "midictrl.h"
#include "utils.h"
#include "dialogs.h"
#include "widgets/pastedialog.h"
#include "undo.h"

using MusECore::Undo;
using MusECore::UndoOp;

#define ABS(x) (abs(x))

#define EDITING_FINISHED_TIMEOUT 50 /* in milliseconds */

using std::set;

namespace MusEGui {

//---------------------------------------------------------
//   NPart
//---------------------------------------------------------

NPart::NPart(MusECore::Part* e) : CItem(MusECore::Event(), e)
      {
      leftBorderTouches = false;
      rightBorderTouches = false;
      
      _serial=e->sn();
      
      int y  = track()->y();
      setPos(QPoint(e->tick(), y));
      setBBox(QRect(e->tick(), y, e->lenTick(), track()->height()));
      }

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

PartCanvas::PartCanvas(int* r, QWidget* parent, int sx, int sy)
   : Canvas(parent, sx, sy)
      {
      setAcceptDrops(true);
      _raster = r;

      setFocusPolicy(Qt::StrongFocus);
      // Defaults:
      lineEditor = 0;
      editMode   = false;

      tracks = MusEGlobal::song->tracks();
      setMouseTracking(true);
      drag          = DRAG_OFF;
      curColorIndex = 0;
      automation.currentCtrlValid = false;
      automation.controllerState = doNothing;
      automation.moveController = false;
      partsChanged();
      }

PartCanvas::~PartCanvas()
{
}

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int PartCanvas::y2pitch(int y) const
      {
      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      int yy  = 0;
      int idx = 0;
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it, ++idx) {
            int h = (*it)->height();
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
      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      int yy  = 0;
      int idx = 0;
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it, ++idx) {
            if (idx == p)
                  break;
            yy += (*it)->height();
            }
      return yy;
      }

//---------------------------------------------------------
//   y2height
//---------------------------------------------------------

int PartCanvas::y2height(int y) const
{
      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      int yy  = 0;
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
            int h = (*it)->height();
            if (y < yy+h)
                  return h;
            yy += h;
            }
      return 20;
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PartCanvas::leaveEvent(QEvent*)
      {
      emit timeChanged(INT_MAX);
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
          MusECore::Part* part = editPart->part();
          // Indicate do undo, and do port controller values but not clone parts. 
          
          Undo operations;
          operations.push_back(UndoOp(UndoOp::ModifyPartName,part, part->name().toUtf8().data(), lineEditor->text().toUtf8().data())); // FIXME char sucks, better use QString directly.
          MusEGlobal::song->applyOperationGroup(operations);
          
          editMode = false;
          
          editingFinishedTime.start();
          }
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
      
      // double click creates new part between left and
      // right mark

      else {
            MusECore::TrackList* tl = MusEGlobal::song->tracks();
            MusECore::ciTrack it;
            int yy = 0;
            int y = event->y();
            for (it = tl->begin(); it != tl->end(); ++it) {
                  int h = (*it)->height();
                  if (y >= yy && y < (yy + h) && (*it)->isVisible())
                        break;
                  yy += h;
                  }
            if (pos[2] - pos[1] > 0 && it != tl->end()) {
                  MusECore::Track* track = *it;
                  switch(track->type()) {
                        case MusECore::Track::MIDI:
                        case MusECore::Track::DRUM:
                        case MusECore::Track::NEW_DRUM:
                              {
                              MusECore::MidiPart* part = new MusECore::MidiPart((MusECore::MidiTrack*)track);
                              part->setTick(pos[1]);
                              part->setLenTick(pos[2]-pos[1]);
                              part->setName(track->name());
                              NPart* np = new NPart(part);
                              items.add(np);
                              deselectAll();
                              part->setSelected(true);
                              MusEGlobal::audio->msgAddPart(part);
                              }
                              break;
                        case MusECore::Track::WAVE:
                        case MusECore::Track::AUDIO_OUTPUT:
                        case MusECore::Track::AUDIO_INPUT:
                        case MusECore::Track::AUDIO_GROUP:
                        case MusECore::Track::AUDIO_AUX:
                        case MusECore::Track::AUDIO_SOFTSYNTH:
                              break;
                        }
                  }
            }
      }

//---------------------------------------------------------
//   update
//---------------------------------------------------------

void PartCanvas::updateSong(DragType t, MusECore::SongChangedFlags_t flags)
      {
      MusEGlobal::song->update(flags | ((t == MOVE_COPY || t == MOVE_CLONE)
         ? SC_PART_INSERTED : SC_PART_MODIFIED));
      }

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void PartCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype, bool rasterize)
{      
  MusECore::Undo operations;
  
  for(iCItem ici = items.begin(); ici != items.end(); ++ici) 
  {
    CItem* ci = ici->second;
    
    int x = ci->pos().x();
    int y = ci->pos().y();
    int nx = x + dx;
    int ny = pitch2y(y2pitch(y) + dp);
    QPoint newpos = QPoint(nx, ny);
    if(rasterize)
      newpos = raster(newpos);
    selectItem(ci, true);
    
    bool result=moveItem(operations, ci, newpos, dtype);
    if (result)
        ci->move(newpos);
    
    if(moving.size() == 1) {
          itemReleased(curItem, newpos);
          }
    if(dtype == MOVE_COPY || dtype == MOVE_CLONE)
          selectItem(ci, false);
  }
  
  MusEGlobal::song->applyOperationGroup(operations);
  partsChanged();
}
      
//---------------------------------------------------------
//   moveItem
//    return false, if copy/move not allowed
//---------------------------------------------------------

bool PartCanvas::moveItem(MusECore::Undo& operations, CItem* item, const QPoint& newpos, DragType t)
{
    NPart* npart    = (NPart*) item;
    MusECore::Part* spart     = npart->part();
    MusECore::Track* track    = npart->track();
    MusECore::Track* dtrack=NULL;
    unsigned dtick  = newpos.x(); // FIXME TODO make subtick-compatible!
    unsigned ntrack = y2pitch(item->mp().y());
    MusECore::Track::TrackType type = track->type();
    int new_partend;
    if (tracks->index(track) == ntrack && (dtick == spart->tick())) {
        return false;
    }
    if (ntrack >= tracks->size()) {
        ntrack = tracks->size();
        if (MusEGlobal::debugMsg)
            printf("PartCanvas::moveItem - add new track\n");
        dtrack = MusEGlobal::song->addTrack(operations, type);  // Add at end of list.
        
        if (type == MusECore::Track::WAVE) {
            MusECore::WaveTrack* st = (MusECore::WaveTrack*) track;
            MusECore::WaveTrack* dt = (MusECore::WaveTrack*) dtrack;
            dt->setChannels(st->channels());
        }
        emit tracklistChanged();
    }
    else
    {      
        dtrack = tracks->index(ntrack);
        if (dtrack->type() != type) {
            QMessageBox::critical(this, QString("MusE"),
                                  tr("Cannot copy/move/clone to different Track-Type"));
                                  return false;
        }
    }
    
    
    
    if(t == MOVE_MOVE)
    {
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::ModifyPartTick,spart,spart->tick(),dtick));
        
        new_partend=(spart->lenTick() + dtick);
    }  
    else
    {
        MusECore::Part* dpart;
        bool clone = (t == MOVE_CLONE || (t == MOVE_COPY && spart->hasClones()));
        
        // This increments aref count if cloned, and chains clones.
        // It also gives the new part a new serial number.
        if (clone)
            dpart = spart->createNewClone();
        else
            dpart = spart->duplicate();
        
        dpart->setTick(dtick);
        
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddPart,dpart));
        
        new_partend=(dpart->lenTick() + dpart->tick());
        
    }
    
    if (MusEGlobal::song->len() < new_partend) // FIXME this is buggy anyway.
            operations.push_back(  MusECore::UndoOp(MusECore::UndoOp::ModifySongLen, 
                                                    new_partend,
                                                    MusEGlobal::song->len() )  );
            
            
            
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
      x = AL::sigmap.raster(x, *_raster);
      if (x < 0)
            x = 0;
      return QPoint(x, y);
      }


void PartCanvas::songIsClearing()
{
  curItem=NULL;
  items.clearDelete();
}

//---------------------------------------------------------
//   partsChanged
//---------------------------------------------------------

void PartCanvas::partsChanged()
      {
      int sn = -1;
      if (curItem) sn=static_cast<NPart*>(curItem)->serial();
      curItem=NULL;     
      
      items.clearDelete();
      for (MusECore::ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
         if ((*t)->isVisible()) //ignore parts from hidden tracks
         {
            MusECore::PartList* pl = (*t)->parts();
            for (MusECore::ciPart i = pl->begin(); i != pl->end(); ++i) {
                  MusECore::Part* part = i->second;
                  NPart* np = new NPart(part);
                  items.add(np);
                  
                  if (np->serial() == sn)
                    curItem=np;
                  
                  if (i->second->selected())
                        selectItem(np, true);
                  
                  // Check for touching borders. p4.0.29
                  MusECore::Part* pp;
                  for(MusECore::ciPart ii = pl->begin(); ii != pl->end(); ++ii) 
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
      }
      redraw();
}

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void PartCanvas::updateSelection()
{
      Undo operations;
      bool changed=false;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            NPart* part = (NPart*)(i->second);
            operations.push_back(UndoOp(UndoOp::SelectPart, part->part(), i->second->isSelected(), part->part()->selected()));
            if (i->second->isSelected() != part->part()->selected())
                changed=true;
      }
      
      if (changed)
      {
            MusEGlobal::song->applyOperationGroup(operations);
            emit selectionChanged();
            redraw();
      }
}

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PartCanvas::resizeItem(CItem* i, bool noSnap, bool ctrl)
      {
      MusECore::Track* t = ((NPart*)(i))->track();
      MusECore::Part*  p = ((NPart*)(i))->part();

      int pos = p->tick() + i->width();
      int snappedpos = pos;
      if (!noSnap) {
            snappedpos = AL::sigmap.raster(pos, *_raster);
            }
      unsigned int newwidth = snappedpos - p->tick();
      if (newwidth == 0)
            newwidth = AL::sigmap.rasterStep(p->tick(), *_raster);

      MusEGlobal::song->cmdResizePart(t, p, newwidth, !ctrl);
      }

//---------------------------------------------------------
//   newItem
//    first create local Item
//---------------------------------------------------------

CItem* PartCanvas::newItem(const QPoint& pos, int key_modifiers)
      {
      int x = pos.x();
      if (x < 0)
            x = 0;
      if(!(key_modifiers & Qt::ShiftModifier))
        x = AL::sigmap.raster1(x, *_raster);
      int len   = pos.x() - x;
      if(len < 0)
        len = 0;
      unsigned trackIndex = y2pitch(pos.y());
      if (trackIndex >= tracks->size())
            return 0;
      MusECore::Track* track = tracks->index(trackIndex);
      if(!track)
        return 0;
        
      MusECore::Part* pa  = 0;
      NPart* np = 0;
      switch(track->type()) {
            case MusECore::Track::MIDI:
            case MusECore::Track::DRUM:
            case MusECore::Track::NEW_DRUM:
                  pa = new MusECore::MidiPart((MusECore::MidiTrack*)track);
                  pa->setTick(x);
                  pa->setLenTick(len);
                  break;
            case MusECore::Track::WAVE:
                  pa = new MusECore::WavePart((MusECore::WaveTrack*)track);
                  pa->setTick(x);
                  pa->setLenTick(len);
                  break;
            case MusECore::Track::AUDIO_OUTPUT:
            case MusECore::Track::AUDIO_INPUT:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_AUX:
            case MusECore::Track::AUDIO_SOFTSYNTH:
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
      if(!i)
        return;
      NPart* npart = (NPart*)(i);
      MusECore::Part* p = npart->part();
      if(!p)
        return;
      MusECore::Track* part_track = p->track();
      if(!part_track)
        return;

      int x = i->x();
      if (x < 0)
            x = 0;
      if(!noSnap)
        x = AL::sigmap.raster1(x, *_raster);
      p->setTick(x);
      
      unsigned trackIndex = y2pitch(i->y());
      unsigned int tsize = tracks->size();
      if (trackIndex >= tsize)
        trackIndex = (tsize > 0 ? tsize - 1 : 0);
      MusECore::Track* track = tracks->index(trackIndex);

      if(track != part_track)
      {
        if(track->type() == part_track->type())
        {
          p->setTrack(track);
          p->setName(track->name());
        }
        else
        {
          MusECore::Part* new_part = 0;
          switch(track->type())
          {
                case MusECore::Track::MIDI:
                case MusECore::Track::DRUM:
                case MusECore::Track::NEW_DRUM:
                      new_part = new MusECore::MidiPart((MusECore::MidiTrack*)track);
                      break;
                case MusECore::Track::WAVE:
                      new_part = new MusECore::WavePart((MusECore::WaveTrack*)track);
                      break;
                case MusECore::Track::AUDIO_OUTPUT:
                case MusECore::Track::AUDIO_INPUT:
                case MusECore::Track::AUDIO_GROUP:
                case MusECore::Track::AUDIO_AUX:
                case MusECore::Track::AUDIO_SOFTSYNTH:
                      break;
          }
          if(new_part)
          {
            new_part->setTick(p->tick());
            new_part->setName(track->name());
            new_part->setColorIndex(curColorIndex);
            delete p;
            npart->setPart(new_part);
            p = new_part;
          }
        }
      }
      
      int len = i->width();
      if (!noSnap)
            len = AL::sigmap.raster(len, *_raster);
      if (len == 0)
            len = AL::sigmap.rasterStep(p->tick(), *_raster);
      p->setLenTick(len);
      p->setSelected(true);
      MusEGlobal::audio->msgAddPart(p, true); //indicate undo
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PartCanvas::deleteItem(CItem* i)
      {
      MusECore::Part*  p = ((NPart*)(i))->part();
      MusEGlobal::audio->msgRemovePart(p, true); //Invokes songChanged which calls partsChanged which makes it difficult to delete them there
      return true;
      }

//---------------------------------------------------------
//   splitItem
//---------------------------------------------------------

void PartCanvas::splitItem(CItem* item, const QPoint& pt)
      {
      NPart* np = (NPart*) item;
      MusECore::Track* t = np->track();
      MusECore::Part*  p = np->part();
      int x = pt.x();
      if (x < 0)
            x = 0;
      MusEGlobal::song->cmdSplitPart(t, p, AL::sigmap.raster(x, *_raster));
      }

//---------------------------------------------------------
//   glueItem
//---------------------------------------------------------

void PartCanvas::glueItem(CItem* item)
      {
      NPart* np = (NPart*) item;
      MusECore::Track* t = np->track();
      MusECore::Part*  p = np->part();
      MusEGlobal::song->cmdGluePart(t, p);
      }

//---------------------------------------------------------
//   genItemPopup
//---------------------------------------------------------

QMenu* PartCanvas::genItemPopup(CItem* item)
      {
      NPart* npart = (NPart*) item;
      MusECore::Track::TrackType trackType = npart->track()->type();

      QMenu* partPopup = new QMenu(this);

      partPopup->addAction(new MenuTitleItem(tr("Part:"), partPopup));
      
      QAction *act_cut = partPopup->addAction(*editcutIconSet, tr("C&ut"));
      act_cut->setData(4);
      act_cut->setShortcut(Qt::CTRL+Qt::Key_X);

      QAction *act_copy = partPopup->addAction(*editcopyIconSet, tr("&Copy"));
      act_copy->setData(5);
      act_copy->setShortcut(Qt::CTRL+Qt::Key_C);

      partPopup->addSeparator();
      int rc = npart->part()->nClones();
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
            QAction *act_color = colorPopup->addAction(MusECore::colorRect(MusEGlobal::config.partColors[i], 80, 80), MusEGlobal::config.partColorNames[i]);
            act_color->setData(20+i);
            }

      QAction *act_delete = partPopup->addAction(QIcon(*deleteIcon), tr("delete")); // ddskrjo added QIcon to all
      act_delete->setData(1);
      QAction *act_split = partPopup->addAction(QIcon(*cutIcon), tr("split"));
      act_split->setData(2);
      QAction *act_glue = partPopup->addAction(QIcon(*glueIcon), tr("glue"));
      act_glue->setData(3);
      QAction *act_superglue = partPopup->addAction(QIcon(*glueIcon), tr("super glue (merge selection)"));
      act_superglue->setData(6);
      QAction *act_declone = partPopup->addAction(tr("de-clone"));
      act_declone->setData(15);

      partPopup->addSeparator();
      switch(trackType) {
            case MusECore::Track::MIDI: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startPianoEditAction);
                  partPopup->addMenu(MusEGlobal::muse->arranger()->parentWin()->scoreSubmenu);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startScoreEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startListEditAction);
                  QAction *act_mexport = partPopup->addAction(tr("save part to disk"));
                  act_mexport->setData(16);
                  }
                  break;
            case MusECore::Track::NEW_DRUM:
            case MusECore::Track::DRUM: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startDrumEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startListEditAction);
                  QAction *act_dexport = partPopup->addAction(tr("save part to disk"));
                  act_dexport->setData(16);
                  }
                  break;
            case MusECore::Track::WAVE: {
                  QAction *act_wedit = partPopup->addAction(QIcon(*edit_waveIcon), tr("wave edit"));
                  act_wedit->setData(14);
                  QAction *act_wexport = partPopup->addAction(tr("save part to disk"));
                  act_wexport->setData(16);
                  QAction *act_wfinfo = partPopup->addAction(tr("file info"));
                  act_wfinfo->setData(17);
                  }
                  break;
            case MusECore::Track::AUDIO_OUTPUT:
            case MusECore::Track::AUDIO_INPUT:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_AUX:
            case MusECore::Track::AUDIO_SOFTSYNTH:
                  break;
            }

      act_select->setEnabled( rc > 1);
      act_delete->setEnabled( true);
      act_cut->setEnabled( true);
      act_declone->setEnabled( rc > 1);

      genCanvasPopup(partPopup);
      return partPopup;
      }

//---------------------------------------------------------
//   itemPopup
//---------------------------------------------------------

void PartCanvas::itemPopup(CItem* item, int n, const QPoint& pt)
      {
      if(n >= TOOLS_ID_BASE)
      {
        canvasPopup(n);
        return;
      }
      
      MusECore::PartList* pl = new MusECore::PartList;
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
                  MusEGlobal::audio->msgRemovePart(npart->part());
                  break;
            case 5:
                  copy(pl);
                  break;
            case 6:
                  MusECore::merge_selected_parts();
                  break;

            case 14:    // wave edit
                    emit startEditor(pl, 4);
                  return;
            case 15:    // declone
                  {
                  MusECore::Part* spart  = npart->part();
                  MusECore::Part* dpart  = spart->duplicate(); // dpart will not be member of any clone chain!

                  Undo operations;
                  operations.push_back(UndoOp(UndoOp::DeletePart, spart));
                  operations.push_back(UndoOp(UndoOp::AddPart, dpart));
                  MusEGlobal::song->applyOperationGroup(operations);
                  break;
                  }
            case 16: // Export to file
                  {
                  const MusECore::Part* part = item->part();
                  bool popenFlag = false;
                  QString fn = getSaveFileName(QString(""), MusEGlobal::part_file_save_pattern, this, tr("MusE: save part"));
                  if (!fn.isEmpty()) {
                        FILE* fp = fileOpen(this, fn, ".mpt", "w", popenFlag, false, false);
                        if (fp) {
                              MusECore::Xml tmpXml = MusECore::Xml(fp);
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
                    MusECore::Part* p = item->part();
                    QString str = tr("Part name: %1\nFiles:").arg(p->name());
                    for (MusECore::ciEvent e = p->events().begin(); e != p->events().end(); ++e) 
                    {
                      MusECore::Event event = e->second;
                      MusECore::SndFileR f  = event.sndFile();
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
                    MusECore::Part* part = item->part();
                    
                    // Traverse and process the clone chain ring until we arrive at the same part again.
                    // The loop is a safety net.
                    MusECore::Part* p = part; 
                    
                    Undo operations;
                    if(part->hasClones())
                    {
                      operations.push_back(UndoOp(UndoOp::SelectPart, p, true, p->selected()));
                      for(MusECore::Part* it = p->nextClone(); it!=p; it=it->nextClone())
                        operations.push_back(UndoOp(UndoOp::SelectPart, it, true, it->selected()));

                      MusEGlobal::song->applyOperationGroup(operations);
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
                    
                    MusEGlobal::song->update(SC_PART_MODIFIED);
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

bool PartCanvas::mousePress(QMouseEvent* event)
      {
    if (event->modifiers() & Qt::ControlModifier) {
            return true;
            }
      QPoint pt = event->pos();
      Qt::MouseButton button = event->button();
      CItem* item = items.find(pt);

      switch (_tool) {
            default:
                  break; 
            case PointerTool:
            case PencilTool:
                  if (item && button == Qt::LeftButton)
                      emit trackChanged(item->part()->track());
                  break;
            case CutTool:
                  if (item && button == Qt::LeftButton) splitItem(item, pt);
                  break;
            case GlueTool:
                  if (item && button == Qt::LeftButton) glueItem(item);
                  break;
            case MuteTool:
                  {
                  if (item && button == Qt::LeftButton) {
                      NPart* np = (NPart*) item;
                      MusECore::Part*  p = np->part();
                      p->setMute(!p->mute());
                      redraw();
                      break;
                      }
                  }
                  break;
            case AutomationTool:
                  if (button == Qt::RightButton  ||
                      button == Qt::MidButton) {
                      
                      bool do_delete = false;
                      
                      if (button == Qt::MidButton) // mid-click
                        do_delete=true;
                      else // right-click
                      {
                        QMenu *automationMenu = new QMenu(this);
                        QAction* act;
                        automationMenu->addAction(new MenuTitleItem(tr("Automation:"), automationMenu));
                        act = automationMenu->addAction(tr("Remove selected"));
                        act->setData(0);
                        genCanvasPopup(automationMenu);
                        act = automationMenu->exec(event->globalPos());
                        if(act)
                        {
                          int n = act->data().toInt();
                          if(n == 0)
                            do_delete = true;
                          else
                          if(n >= TOOLS_ID_BASE)
                            canvasPopup(n);
                        }
                        delete automationMenu;
                      }
                      if (do_delete && automation.currentTrack) {
                          foreach(int frame, automation.currentCtrlFrameList)
                              MusEGlobal::audio->msgEraseACEvent((MusECore::AudioTrack*)automation.currentTrack,
                                       automation.currentCtrlList->id(), frame);
                          // User probably would like to hear results so make sure controller is enabled.
                          ((MusECore::AudioTrack*)automation.currentTrack)->enableController(automation.currentCtrlList->id(), true); 
                      }
                  }
                  else {
                      if (automation.controllerState != doNothing)
                          automation.moveController=true;
                  }
                  return false;
                  break;
            }
      return true;
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

      if (_tool == AutomationTool)
          processAutomationMovements(event->pos(), event->modifiers() & Qt::ShiftModifier);

      emit timeChanged(AL::sigmap.raster(x, *_raster));
      }

//---------------------------------------------------------
//   y2Track
//---------------------------------------------------------

MusECore::Track* PartCanvas::y2Track(int y) const
      {
      MusECore::TrackList* l = MusEGlobal::song->tracks();
      int ty = 0;
      for (MusECore::ciTrack it = l->begin(); it != l->end(); ++it) {
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
// DELETETHIS 10
//      if (_tool == AutomationTool) { // can't get the cursor pos to work right, skipping for now
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
            // this will probably never happen, as edit mode has been set
            // to "false" some usec ago by returnPressed, called by editingFinished.
            if ( key == Qt::Key_Return || key == Qt::Key_Enter ) 
                  {
                  return;
                  }
            // the below CAN indeed happen.
            else if ( key == Qt::Key_Escape )
                  {
                  lineEditor->hide();
                  editMode = false;
                  return;
                  }
            }
      // if returnPressed, called by editingFinished, was executed
      // a short time ago, ignore this keypress if it was enter or return
      if (editingFinishedTime.elapsed() < EDITING_FINISHED_TIMEOUT &&
          (key == Qt::Key_Return || key == Qt::Key_Enter) )
        return;

      if (event->modifiers() &  Qt::ShiftModifier)
            key +=  Qt::SHIFT;
      if (event->modifiers() &  Qt::AltModifier)
            key +=  Qt::ALT;
      if (event->modifiers() &  Qt::ControlModifier)
            key +=  Qt::CTRL;

      if (key == shortcuts[SHRT_DELETE].key) {
            if (getCurrentDrag())
                  return;
                
            MusEGlobal::song->msgRemoveParts();
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
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC].key) {
            int spos = AL::sigmap.raster2(pos[0] + 1, *_raster);    // Nudge by +1, then snap up with raster2.
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(0, p, true, true, true); 
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key) {
            int spos = pos[0] - AL::sigmap.rasterStep(pos[0], *_raster);
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(0, p, true, true, true);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key) {
            MusECore::Pos p(pos[0] + AL::sigmap.rasterStep(pos[0], *_raster), true);
            MusEGlobal::song->setPos(0, p, true, true, true);
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
      else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key) {
            emit setUsedTool(AutomationTool);
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
      else if (key == shortcuts[SHRT_TOOL_PAN].key) {
            emit setUsedTool(PanTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_ZOOM].key) {
            emit setUsedTool(ZoomTool);
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

      // Shortcuts that require selected parts from here
      if (!curItem) {
          if (items.size()==0) {
              event->ignore();  // give global accelerators a chance
              return;
          }
          for (iCItem i = items.begin(); i != items.end(); ++i) {
              NPart* part = (NPart*)(i->second);
              if (part->isSelected()) {
                curItem=part;
                break;
              }
          }
          if (!curItem)
            curItem = (NPart*)items.begin()->second; // just grab the first part
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
            MusECore::Pos p1(left_tick, true);
            MusECore::Pos p2(right_tick, true);
            MusEGlobal::song->setPos(1, p1);
            MusEGlobal::song->setPos(2, p2);
            return;
            }

      // Select part to the right
      else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
            if (key == shortcuts[SHRT_SEL_RIGHT_ADD].key)
                  add = true;

            MusECore::Part* part = curItem->part();
            MusECore::Track* track = part->track();
            unsigned int tick = part->tick();
            bool afterthis = false;
            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  NPart* npart = (NPart*)(i->second);
                  MusECore::Part* ipart = npart->part();
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

            MusECore::Part* part = curItem->part();
            MusECore::Track* track = part->track();
            unsigned int tick = part->tick();

            for (iCItem i = items.begin(); i != items.end(); ++i) {
                  NPart* npart = (NPart*)(i->second);
                  MusECore::Part* ipart = npart->part();

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
            MusECore::Track* track = curItem->part()->track();//top->part()->track();
            track = y2Track(track->y() - 1);

            //If we're at topmost (no track above), leave
            if (!track)
                  return;

            int middle = curItem->x() + curItem->part()->lenTick()/2;
            CItem *aboveL = 0, *aboveR = 0;
            //Upper limit: song end, lower limit: song start
            int ulimit  = MusEGlobal::song->len();
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
            MusECore::Track* track = curItem->part()->track();//bottom->part()->track();
            track = y2Track(track->y() + track->height() + 1 );
            int middle = curItem->x() + curItem->part()->lenTick()/2;
            //If we're at bottommost, leave
            if (!track)
                  return;

            CItem *belowL = 0, *belowR = 0;
            //Upper limit: song end , lower limit: song start
            int ulimit = MusEGlobal::song->len();
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
            MusECore::PartList* pl = new MusECore::PartList;
            NPart* npart = (NPart*)(curItem);
            MusECore::Track* track = npart->part()->track();
            pl->add(npart->part());
            int type = 0;

            //  Check if track is wave or drum,
            //  else track is midi

            switch (track->type()) {
                  case MusECore::Track::NEW_DRUM:
                  case MusECore::Track::DRUM:
                        type = 3;
                        break;

                  case MusECore::Track::WAVE:
                        type = 4;
                        break;

                  case MusECore::Track::MIDI:
                  case MusECore::Track::AUDIO_OUTPUT:
                  case MusECore::Track::AUDIO_INPUT:
                  case MusECore::Track::AUDIO_GROUP:
                  case MusECore::Track::AUDIO_AUX:
                  case MusECore::Track::AUDIO_SOFTSYNTH: //TODO
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

            //Check if we've hit the left, right, upper or lower boundaries of the window. If so, scroll to new position.
            if (newItem->x() < mapxDev(0)) {
                  emit horizontalScroll(rmapx(newItem->x() - xorg) - 10);  // Leave some room.
                  }
            else if (newItem->x() + newItem->width() > mapxDev(width())) {
                  int mx = rmapx(newItem->x());
                  int newx = mx + rmapx(newItem->width()) - width();
                  emit horizontalScroll( (newx > mx ? mx - 10 : newx + 10) - rmapx(xorg) );
                  }
                  
            if (newItem->y() < mapyDev(0)) {
                  int my = rmapy(newItem->y());
                  int newy = my + rmapy(newItem->height()) - height();
                  emit verticalScroll( (newy < my ? my - 10 : newy + 10) - rmapy(yorg) );
                  }
            else if (newItem->y() + newItem->height() > mapyDev(height())) {
                  emit verticalScroll( rmapy(newItem->y() + newItem->height() - yorg) - height() + 10);
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

      MusECore::Part* part = ((NPart*)item)->part();
      int pTick  = part->tick();
      from      -= pTick;
      to        -= pTick;
      if(from < 0)
        from = 0;
      if((unsigned int)to > part->lenTick())
        to = part->lenTick();  

      QBrush brush;
      
      QRect r    = item->bbox();
      // Compensation required for two pixel wide border. FIXME Prefer to do this after the map, but r is needed below.
      r.moveTop(r.y() + rmapyDev(1));  
      //QRect rr = p.transform().mapRect(r);    // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect rr = map(r);                        // Use our own map instead.                                
      
      QRect mr = map(rect);
      
      // Item bounding box x is in tick coordinates, same as rectangle.
      if((rr & mr).isNull())
        return;
      
      p.setWorldMatrixEnabled(false);
      
      // NOTE: Optimization: For each item, hasHiddenEvents() is called once in Canvas::draw(), and we use cachedHasHiddenEvents().
      // Not used for now.
      //int het = part->cachedHasHiddenEvents(); DELETETHIS or FIXME or whatever?
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
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            QLinearGradient gradient(rr.topLeft(), rr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else 
      if (part->selected()) 
      {
          QColor c(Qt::black);
          c.setAlpha(MusEGlobal::config.globalAlphaBlend);
          QLinearGradient gradient(rr.topLeft(), rr.bottomLeft());
          // Use a colour only about 20% lighter than black, rather than the 50% we use in MusECore::gGradientFromQColor
          //  and is used in darker()/lighter(), so that it is distinguished a bit better from grey non-part tracks.
          //c.setRgba(64, 64, 64, c.alpha());        
          gradient.setColorAt(0, QColor(51, 51, 51, MusEGlobal::config.globalAlphaBlend));
          gradient.setColorAt(1, c);
          brush = QBrush(gradient);
      }
      else
      if (part->mute()) 
      {
            QColor c(Qt::white);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            QLinearGradient gradient(rr.topLeft(), rr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else
      {
            QColor c(MusEGlobal::config.partColors[cidx]);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            brush = QBrush(MusECore::gGradientFromQColor(c, rr.topLeft(), rr.bottomLeft()));
      }  
      
      int h = rr.height();
      double s = double(h) / 4.0;
      int y0 = ys_0;
      int y2 = y0 + lrint(s * 2.0);
      int y4 = y0 + h;
      
      QPoint points[8];   
      int pts;
      
      // Fill the part rectangles, accounting for hidden events by using 'jagged' edges...
      
      p.setBrush(brush);
      p.setPen(Qt::NoPen);
      
      if(het)
      {
        // TODO: Make this part respect the requested drawing rectangle (rr & mr), for speed !
        
        pts = 0;
        if(het == (MusECore::Part::LeftEventsHidden | MusECore::Part::RightEventsHidden))
        {          
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_j, y2);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xs_0, y4);
          points[pts++] = QPoint(xs_j, y2);
          
          p.drawConvexPolygon(points, pts);   // Help says may be faster on some platforms (X11).
        }
        else
        if(het == MusECore::Part::LeftEventsHidden)
        {
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xs_0, y4);
          points[pts++] = QPoint(xs_j, y2);
          
          p.drawConvexPolygon(points, pts);   
        }
        else
        if(het == MusECore::Part::RightEventsHidden)
        {
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_j, y2);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xs_0, y4);
          
          p.drawConvexPolygon(points, pts);   
        }
        
        // Draw remaining 'hidden events' decorations with 'jagged' edges...
                      
        int part_r, part_g, part_b, brightness, color_brightness;
        MusEGlobal::config.partColors[cidx].getRgb(&part_r, &part_g, &part_b);
        brightness =  part_r*29 + part_g*59 + part_b*12;
        if ((brightness >= 12000 && !part->selected()))
          color_brightness=96; //0;    // too light: use dark color 
        else
          color_brightness=180; //255;   // too dark: use lighter color 
        QColor c(color_brightness,color_brightness,color_brightness, MusEGlobal::config.globalAlphaBlend);
        p.setBrush(QBrush(MusECore::gGradientFromQColor(c, rr.topLeft(), rr.bottomLeft())));
        if(het & MusECore::Part::RightEventsHidden)
        {
          pts = 0;
          points[pts++] = QPoint(xe_0, y0);
          points[pts++] = QPoint(xe_0, y4);
          points[pts++] = QPoint(xe_j, y2);
          
          p.drawConvexPolygon(points, pts);   
        }
        if(het & MusECore::Part::LeftEventsHidden)
        {
          pts = 0;
          points[pts++] = QPoint(xs_0, y0);
          points[pts++] = QPoint(xs_j, y2);
          points[pts++] = QPoint(xs_0, y4);
          
          p.drawConvexPolygon(points, pts);   
        }
      }  
      else
      {
        p.fillRect(rr & mr, brush);  // Respect the requested drawing rectangle. Gives speed boost!
      }
          
      // Draw a pattern brush on muted parts...
      if(part->mute())
      {
        p.setPen(Qt::NoPen);
        brush.setStyle(Qt::Dense7Pattern);
        
        p.fillRect(rr & mr, brush);   // Respect the requested drawing rectangle. Gives speed boost!                                                      
      }  
      
      p.setWorldMatrixEnabled(true);
      
      MusECore::MidiPart* mp = 0;
      MusECore::WavePart* wp = 0;
      MusECore::Track::TrackType type = part->track()->type();
      if (type == MusECore::Track::WAVE)
            wp =(MusECore::WavePart*)part;
      else
            mp = (MusECore::MidiPart*)part;

      if (wp)
          drawWavePart(p, rect, wp, r);
      else if (mp)
          drawMidiPart(p, rect, mp, r, from, to);

      p.setWorldMatrixEnabled(false);
        
        //
        // Now draw the borders, using custom segments...
        //
        
        p.setBrush(Qt::NoBrush);
        
        QColor pc((part->mute() || item->isMoving())? Qt::white : MusEGlobal::config.partColors[cidx]);
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
        if(part->hasClones())
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
        
        if(((NPart*)item)->rightBorderTouches)
          p.setPen(part->selected() ? penSelect1V : penNormal1V);
        else
          p.setPen(part->selected() ? penSelect2V : penNormal2V);
        
        if(rbx >= mrxs_0 && rbx <= mrxe_0)  // Respect the requested drawing rectangle. Gives speed boost!
        {
          QLine l2(rbx, ys_0, rbx, ye_0);            // Right
          p.drawLine(l2);        // Right line
        }

        if(((NPart*)item)->leftBorderTouches)
          p.setPen(part->selected() ? penSelect1V : penNormal1V);
        else
          p.setPen(part->selected() ? penSelect2V : penNormal2V);
        
        if(xs_0 >= mrxs_0 && xs_0 <= mrxe_0)
        {
          QLine l4(xs_0, ys_0, xs_0, ye_0);            // Left
          p.drawLine(l4);        //  Left line
        }
                
        p.setPen(part->selected() ? penSelect2H : penNormal2H);
        
        // Respect the requested drawing rectangle. Gives speed boost!
        QLine l1(lbx_c, ys_0, rbx_c, ys_0);
        p.drawLine(l1);  // Top line
        QLine l3(lbx_c, ye_0, rbx_c, ye_0);
        p.drawLine(l3);  // Bottom line
        
      if (MusEGlobal::config.canvasShowPartType & 1) {     // show names
            // draw name
            // FN: Set text color depending on part color (black / white)
            int part_r, part_g, part_b, brightness;
            // Since we'll draw the text on the bottom (to accommodate drum 'slivers'),
            //  get the lowest colour in the gradient used to draw the part.
            QRect tr = rr;
            tr.setX(tr.x() + 3);
            MusECore::gGradientFromQColor(MusEGlobal::config.partColors[cidx], tr.topLeft(), tr.bottomLeft()).stops().last().second.getRgb(&part_r, &part_g, &part_b);
            brightness =  part_r*29 + part_g*59 + part_b*12;
            //bool rev = (brightness < 12000 || part->selected()) && !part->mute() && !item->isMoving(); DELETETHIS
            bool rev = brightness >= 12000 && !part->selected();
            p.setFont(MusEGlobal::config.fonts[4]);
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
            
      p.setWorldMatrixEnabled(true);
      }

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void PartCanvas::drawMoving(QPainter& p, const CItem* item, const QRect&)
      {
        p.setPen( Qt::black);
        MusECore::Part* part = ((NPart*)item)->part();
        QColor c(part->mute() ? Qt::white : MusEGlobal::config.partColors[part->colorIndex()]);        
        c.setAlpha(128);  // Fix this regardless of config.globalAlphaBlend setting. Should be OK.
        p.setBrush(c);
        MusECore::TrackList* tl = MusEGlobal::song->tracks();
        int yy  = 0;
        int y = item->mp().y();
        int ih = item->height();
        for(MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it)
        {
          int h = (*it)->height();
          if(y < yy+h)
          {
            ih = h;
            break;
          }
          yy += h;
        }
        p.drawRect(item->mp().x(), item->mp().y(), item->width(), ih);
      }

//---------------------------------------------------------
//   drawMidiPart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void PartCanvas::drawMidiPart(QPainter& p, const QRect& rect, MusECore::MidiPart* midipart, const QRect& r, int from, int to)
{
	drawMidiPart(p, rect, midipart->events(), midipart->track(), midipart, r, midipart->tick(), from, to);
}

void PartCanvas::drawMidiPart(QPainter& p, const QRect&, const MusECore::EventList& events, MusECore::MidiTrack *mt, MusECore::MidiPart *pt, const QRect& r, int pTick, int from, int to)
{
  int color_brightness;
  QColor eventColor;
  
  if(pt) 
  {
    int part_r, part_g, part_b, brightness;
    MusEGlobal::config.partColors[pt->colorIndex()].getRgb(&part_r, &part_g, &part_b);
    brightness =  part_r*29 + part_g*59 + part_b*12;
    if (brightness >= 12000 && !pt->selected()) {
      eventColor=MusEGlobal::config.partMidiDarkEventColor;
      color_brightness=54; // 96;    // too bright: use dark color
    }
    else {
      eventColor=MusEGlobal::config.partMidiLightEventColor;
      color_brightness=200; //160;   // too dark: use lighter color
    }
  }
  else {
    eventColor=QColor(80,80,80);
    color_brightness=80;
  }
    
  if (MusEGlobal::config.canvasShowPartType & 2) {      // show events
            p.setPen(eventColor);
            // Do not allow this, causes segfault.
            if(from <= to)
            {
              MusECore::ciEvent ito(events.lower_bound(to));

              for (MusECore::ciEvent i = events.lower_bound(from); i != ito; ++i) {
                    MusECore::EventType type = i->second.type();
                    int a = i->second.dataA() | 0xff;
                    if (
                      ((MusEGlobal::config.canvasShowPartEvent & 1) && (type == MusECore::Note))
                      || ((MusEGlobal::config.canvasShowPartEvent & (2 | 4)) == (2 | 4) &&
                           type == MusECore::Controller && a == MusECore::CTRL_POLYAFTER)
                      || ((MusEGlobal::config.canvasShowPartEvent & 4) && (type == MusECore::Controller) &&
                          (a != MusECore::CTRL_POLYAFTER  || (MusEGlobal::config.canvasShowPartEvent & 2)) &&
                          (a != MusECore::CTRL_AFTERTOUCH || (MusEGlobal::config.canvasShowPartEvent & 16)))
                      || ((MusEGlobal::config.canvasShowPartEvent & (16 | 4)) == (16 | 4) &&
                          type == MusECore::Controller && a == MusECore::CTRL_AFTERTOUCH)
                      || ((MusEGlobal::config.canvasShowPartEvent & 64) && (type == MusECore::Sysex || type == MusECore::Meta))
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
      
      MusECore::ciEvent ito(events.lower_bound(to));
      bool isdrum = (mt->type() == MusECore::Track::DRUM  ||  mt->type() == MusECore::Track::NEW_DRUM);

      // draw controllers ------------------------------------------
      p.setPen(QColor(192,192,color_brightness/2));
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // PITCH BEND
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  int val=i->second.dataB();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == MusECore::CTRL_PITCH)
                    p.drawLine(t, hoffset + r.y() + th/2, t, hoffset + r.y() - val*th/8192/2 + th/2);
            }
      }

      p.setPen(QColor(192,color_brightness/2,color_brightness/2));
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // PAN
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  int val=i->second.dataB();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == 10)
                    p.drawLine(t, hoffset + r.y() + th - val*th/127, t, hoffset + r.y() + th);
            }
      }

      p.setPen(QColor(color_brightness/2,192,color_brightness/2));
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // VOLUME
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  int val=i->second.dataB();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == 7)
                    p.drawLine(t, hoffset + r.y() + th - val*th/127, t, hoffset + r.y() + th);
            }
      }

      p.setPen(QColor(0,0,255));
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // PROGRAM CHANGE
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  
                  int th = int(mt->height() * 0.75); // only draw on three quarters
                  int hoffset = (mt->height() - th ) / 2; // offset from bottom

                  if (ctrl_type == MusECore::CTRL_PROGRAM)
                    p.drawLine(t, hoffset + r.y(), t, hoffset + r.y() + th);
            }
      }





      // draw notes ------------------------------------------------

      int lowest_pitch=127;
      int highest_pitch=0;
      map<int,int> y_mapper;
      
      if (MusEGlobal::config.canvasShowPartType & 4) //y-stretch?
      {
        for (MusECore::ciEvent i = events.begin(); i != events.end(); ++i)
        {
          if (i->second.type()==MusECore::Note)
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
                printf("DEBUG: arranger: cakewalk enabled, y-stretching from %i to %i.\n",lowest_pitch, highest_pitch);
            else
            {
                printf("DEBUG: arranger: cakewalk enabled, y-stretching drums: ");;
                for (map<int,int>::iterator it=y_mapper.begin(); it!=y_mapper.end(); it++)
                    printf("%i ", it->first);
                printf("\n"); 
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

      p.setPen(eventColor);
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) {
            int t  = i->first + pTick;
            int te = t + i->second.lenTick();

            if (te < (from + pTick))
                  continue;

            if (te >= (to + pTick))
                  te = lrint(rmapxDev_f(rmapx_f(to + pTick) - 1.0));
            
            MusECore::EventType type = i->second.type();
            if (type == MusECore::Note) {
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
   const QRect& bb, MusECore::WavePart* wp, const QRect& _pr)
      {
      QRect rr = map(bb);                          // Use our own map instead.
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

      for (MusECore::ciEvent e = wp->events().begin(); e != wp->events().end(); ++e) {
            int cc = hh % 2 ? 0 : 1;
            MusECore::Event event = e->second;
            MusECore::SndFileR f  = event.sndFile();
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
            int postick = MusEGlobal::tempomap.frame2tick(wp->frame() + event.frame());
            int eventx = mapx(postick);
            int drawoffset;
            if((x1 - eventx) < 0)
              drawoffset = 0;
            else
              drawoffset = rmapxDev(x1 - eventx);
              postick += drawoffset;
            pos = event.spos() + MusEGlobal::tempomap.tick2frame(postick) - wp->frame() - event.frame();
            
            int i;
            if(x1 < eventx)
              i = eventx;
            else  
              i = x1;
            int ex = mapx(MusEGlobal::tempomap.frame2tick(wp->frame() + event.frame() + event.lenFrame()));
            if(ex > x2)
              ex = x2;
            if (h < 20) {
                  //    combine multi channels into one waveform
                  
                  for (; i < ex; i++) {
                        MusECore::SampleV sa[channels];
                        xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
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
                        p.setPen(MusEGlobal::config.partWaveColorPeak);
                        p.drawLine(i, y - peak - cc, i, y + peak);
                        p.setPen(MusEGlobal::config.partWaveColorRms);
                        p.drawLine(i, y - rms - cc, i, y + rms);
                        }
                  }
            else {
                  //  multi channel display
                  int hm = hh / (channels * 2);
                  int cc = hh % (channels * 2) ? 0 : 1;
                  for (; i < ex; i++) {
                        y  = pr.y() + hm;
                        MusECore::SampleV sa[channels];
                        xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
                        f.read(sa, xScale, pos);
                        postick += tickstep;
                        pos += xScale;
                        for (unsigned k = 0; k < channels; ++k) {
                              int peak = (sa[k].peak * (hm - 1)) >> 8;
                              int rms  = (sa[k].rms  * (hm - 1)) >> 8;
                              p.setPen(MusEGlobal::config.partWaveColorPeak);
                              p.drawLine(i, y - peak - cc, i, y + peak);
                              p.setPen(MusEGlobal::config.partWaveColorRms);
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
      MusECore::PartList pl;
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            if (!i->second->isSelected())
                  continue;
            NPart* npart = (NPart*)(i->second);
            pl.add(npart->part());
            }
      switch (cmd) {
            case CMD_CUT_PART:
            {
                  copy(&pl);
                  
                  MusECore::Undo operations;
                  
                  for (iCItem i = items.begin(); i != items.end(); ++i) {
                        if (i->second->isSelected()) {
                              NPart* p = (NPart*)(i->second);
                              MusECore::Part* part = p->part();
                              operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeletePart, part));
                              }
                      }
                  
                  MusEGlobal::song->applyOperationGroup(operations);
                  
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
            case CMD_PASTE_PART_TO_TRACK:
                  paste(false, PASTEMODE_MIX, true);
                  break;
            case CMD_PASTE_CLONE_PART_TO_TRACK:
                  paste(true, PASTEMODE_MIX, true);
                  break;
            case CMD_PASTE_DIALOG:
            {
                  unsigned temp_begin = AL::sigmap.raster1(MusEGlobal::song->vcpos(),0);
                  unsigned temp_end = AL::sigmap.raster2(temp_begin + MusECore::get_paste_len(), 0);
                  paste_dialog->raster = temp_end - temp_begin;
                  
                  if (paste_dialog->exec())
                  {
                    paste_mode_t paste_mode;
                    switch (paste_dialog->insert_method)
                    {
                      case 0: paste_mode=PASTEMODE_MIX; break;
                      case 1: paste_mode=PASTEMODE_MOVEALL; break;
                      case 2: paste_mode=PASTEMODE_MOVESOME; break;
                      default: paste_mode=PASTEMODE_MIX; // shall never be executed
                    }

                    paste(paste_dialog->clone, paste_mode, paste_dialog->all_in_one_track,
                          paste_dialog->number, paste_dialog->raster);
                  }
                  
                  break;
            }
            case CMD_INSERT_EMPTYMEAS:
                  int startPos=MusEGlobal::song->vcpos();
                  int oneMeas=AL::sigmap.ticksMeasure(startPos);
                  MusECore::Undo temp=MusECore::movePartsTotheRight(startPos,oneMeas);
                  MusEGlobal::song->applyOperationGroup(temp);
                  break;
            }
      }

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void PartCanvas::copy_in_range(MusECore::PartList* pl_)
{
  MusECore::PartList pl;
  MusECore::PartList result_pl;
  unsigned int lpos = MusEGlobal::song->lpos();
  unsigned int rpos = MusEGlobal::song->rpos();
  
  if (pl_->empty())
  {
    for (iCItem i = items.begin(); i != items.end(); ++i)
    {
      MusECore::Part* part=static_cast<NPart*>(i->second)->part();
      if ( (part->track()->isMidiTrack()) || (part->track()->type() == MusECore::Track::WAVE) )
        pl.add(part);
    }
  }
  else
  {
    for(MusECore::ciPart p = pl_->begin(); p != pl_->end(); ++p) 
      if ( (p->second->track()->isMidiTrack()) || (p->second->track()->type() == MusECore::Track::WAVE) )
        pl.add(p->second);
  }
  
  if (!pl.empty() && (rpos>lpos))
  {
    for(MusECore::ciPart p = pl.begin(); p != pl.end(); ++p) 
    {
      MusECore::Part* part=p->second;
      MusECore::Track* track=part->track();
      
      if ((part->tick() < rpos) && (part->endTick() > lpos)) //is the part in the range?
      {
        if ((lpos > part->tick()) && (lpos < part->endTick()))
        {
          MusECore::Part* p1;
          MusECore::Part* p2;
          
          part->splitPart(lpos, p1, p2);
          
          part=p2;
        }
        
        if ((rpos > part->tick()) && (rpos < part->endTick()))
        {
          MusECore::Part* p1;
          MusECore::Part* p2;
          
          part->splitPart(rpos, p1, p2);

          part=p1;
        }
        
        result_pl.add(part);
      }
    }
    
    copy(&result_pl);
  }
}

void PartCanvas::copy(MusECore::PartList* pl)
      {
      if (pl->empty())
            return;
      bool wave = false;
      bool midi = false;
      for(MusECore::ciPart p = pl->begin(); p != pl->end(); ++p) 
      {
        if(p->second->track()->isMidiTrack())
          midi = true;
        else
        if(p->second->track()->type() == MusECore::Track::WAVE)
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
      MusECore::Xml xml(tmp);

      // Clear the copy clone list.
      MusEGlobal::cloneList.clear();
      
      int level = 0;
      int tick = 0;
      for (MusECore::ciPart p = pl->begin(); p != pl->end(); ++p) {
            // Indicate this is a copy operation. Also force full wave paths.
            p->second->write(level, xml, true, true);
            
            int endTick = p->second->endTick();
            if (endTick > tick)
                  tick = endTick;
            }
      MusECore::Pos p(tick, true);
      MusEGlobal::song->setPos(0, p);
      QString mimeString = "text/x-muse-mixedpartlist";
      if (!midi)
          mimeString = "text/x-muse-wavepartlist";
      else if (!wave)
          mimeString = "text/x-muse-midipartlist";
      QMimeData *mimeData =  MusECore::file_to_mimedata(tmp, mimeString );
      QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
      fclose(tmp);
      }


MusECore::Undo PartCanvas::pasteAt(const QString& pt, MusECore::Track* track, unsigned int pos, bool clone, bool toTrack, int* finalPosPtr, set<MusECore::Track*>* affected_tracks)
      {
      MusECore::Undo operations;
      
      QByteArray ba = pt.toLatin1();
      const char* ptxt = ba.constData();
      MusECore::Xml xml(ptxt);
      bool firstPart=true;
      int  posOffset=0;
      unsigned int  finalPos = pos;
      int  notDone = 0;
      int  done = 0;
      bool end = false;

      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        end = true;
                        break;
                  case MusECore::Xml::TagStart:
                        if (tag == "part") {                              
                              // Read the part.
                              MusECore::Part* p = 0;
                              p = MusECore::Part::readFromXml(xml, track, clone, toTrack);

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
                              if (p->tick()+p->lenTick()>finalPos) {
                                finalPos=p->tick()+p->lenTick();
                              }
                              p->setSelected(true);
                              operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddPart,p));
                              if (affected_tracks)
                                affected_tracks->insert(p->track());
                              }
                        else
                              xml.unknown("PartCanvas::pasteAt");
                        break;
                  case MusECore::Xml::TagEnd:
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
      MusECore::Track* track = 0;
      
      // If we want to paste to a selected track...
      if (to_single_track)
      {  
        MusECore::TrackList* tl = MusEGlobal::song->tracks();
        for (MusECore::ciTrack i = tl->begin(); i != tl->end(); ++i) {
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
        if(to_single_track && track->type() != MusECore::Track::WAVE) 
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
        if(to_single_track && !track->isMidiTrack() && track->type() != MusECore::Track::WAVE) 
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
        unsigned int startPos=MusEGlobal::song->vcpos();
        set<MusECore::Track*> affected_tracks;

        deselectAll();
        
        MusECore::Undo operations;
        for (int i=0;i<amount;i++)
        {
          MusECore::Undo temp = pasteAt(txt, track, startPos + i*raster, clone, to_single_track, &endPos, &affected_tracks);
          operations.insert(operations.end(), temp.begin(), temp.end());
        }
        
        MusECore::Pos p(endPos, true);
        MusEGlobal::song->setPos(0, p);
        
        if (paste_mode != PASTEMODE_MIX)
        {
          int offset;
          if (amount==1) offset = endPos-startPos;
          else           offset = amount*raster;
            
          MusECore::Undo temp;
          if (paste_mode==PASTEMODE_MOVESOME)
            temp=MusECore::movePartsTotheRight(startPos, offset, false, &affected_tracks);
          else
            temp=MusECore::movePartsTotheRight(startPos, offset);
            
          operations.insert(operations.end(), temp.begin(), temp.end());
        }
        
        MusEGlobal::song->applyOperationGroup(operations);
      }

    }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PartCanvas::startDrag(CItem* item, DragType t)
      {
      NPart* p = (NPart*)(item);
      MusECore::Part* part = p->part();

      //---------------------------------------------------
      //    write part as XML into tmp file
      //---------------------------------------------------

      FILE* tmp = tmpfile();
      if (tmp == 0) {
            fprintf(stderr, "PartCanvas::startDrag() fopen failed: %s\n",
               strerror(errno));
            return;
            }
      MusECore::Xml xml(tmp);
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
      
      // "MusECore::Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object. 
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
      MusECore::CloneList copyCloneList = MusEGlobal::cloneList;
      // Clear the clone list to prevent any dangerous associations with
      //  current non-original parts.
      MusEGlobal::cloneList.clear();
      
      if (type == 1) 
      {
          printf("type1\n");
            text = QString(event->mimeData()->data("text/partlist"));
            
            int x = AL::sigmap.raster(event->pos().x(), *_raster);
            if (x < 0)
                  x = 0;
            unsigned trackNo = y2pitch(event->pos().y());
            MusECore::Track* track = 0;
            if (trackNo < tracks->size())
                  track = tracks->index(trackNo);
            if (track)
            {
                  deselectAll();
                  MusECore::Undo temp=pasteAt(text, track, x);
                  MusEGlobal::song->applyOperationGroup(temp);
            }
      }
      else if (type == 2) 
      {
          unsigned trackNo = y2pitch(event->pos().y());
          MusECore::Track* track = 0;
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
                text.endsWith(".flac",Qt::CaseInsensitive) || 
                text.endsWith(".mpt", Qt::CaseInsensitive) )
            {

                if (!track) { // we need to create a track for this drop
                    if (text.endsWith(".mpt", Qt::CaseInsensitive)) {
                        MusECore::Undo operations;
                        track = MusEGlobal::song->addTrack(operations, MusECore::Track::MIDI);    // Add at end of list.
                        MusEGlobal::song->applyOperationGroup(operations);
                    } else {
                        MusECore::Undo operations;
                        track = MusEGlobal::song->addTrack(operations, MusECore::Track::WAVE);    // Add at end of list.
                        MusEGlobal::song->applyOperationGroup(operations);
                    }
                }
                if (track->type() == MusECore::Track::WAVE &&
                        (text.endsWith(".wav", Qt::CaseInsensitive) || 
                          text.endsWith(".ogg", Qt::CaseInsensitive) ||
                          (text.endsWith(".flac", Qt::CaseInsensitive)) ))
                        {
                        unsigned tick = x;
                        MusEGlobal::muse->importWaveToTrack(text, tick, track);
                        }
                      // Changed by T356. Support mixed .mpt files.
                      else if ((track->isMidiTrack() || track->type() == MusECore::Track::WAVE) && text.endsWith(".mpt", Qt::CaseInsensitive))
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
      MusEGlobal::cloneList.clear();
      MusEGlobal::cloneList = copyCloneList;
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
      QColor baseColor(MusEGlobal::config.partCanvasBg.light(104));
      p.setPen(baseColor);

      //--------------------------------
      // vertical lines
      //-------------------------------
      if (MusEGlobal::config.canvasShowGrid) {
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
            if (*_raster == MusEGlobal::config.division *2)         // 1/2
                noDivisors=2;
            else if (*_raster== MusEGlobal::config.division)        // 1/4
                noDivisors=4;
            else if (*_raster==MusEGlobal::config.division/2)         // 1/8
                noDivisors=8;
            else if (*_raster==MusEGlobal::config.division/4)          // 1/16
                noDivisors=16;
            else if (*_raster==MusEGlobal::config.division/8)          // 1/16
                noDivisors=32;
            else if (*_raster==MusEGlobal::config.division/16)          // 1/16
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

      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      int yy = -rmapy(yorg) - ypos;
      int th;
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
            if (yy > my + mh)
                  break;
            MusECore::Track* track = *it;
            th = track->height();
            if (!th)
              continue;
            if (MusEGlobal::config.canvasShowGrid && (track->isMidiTrack() || track->type() == MusECore::Track::WAVE))   // Tim.
            {
              p.setPen(baseColor.dark(130));
              p.drawLine(mx, yy + th, mx + mw, yy + th);  
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
            {
              if (!track->isMidiTrack() && (track->type() != MusECore::Track::WAVE)) {
                    drawAudioTrack(p, mr, r, (MusECore::AudioTrack*)track);
              }              
            }
            yy += th;
      }
      
      p.restore();
}

//---------------------------------------------------------
//   drawLast
//---------------------------------------------------------
void PartCanvas::drawTopItem(QPainter& p, const QRect& rect)
{
    QRect mr = map(rect);
    
    int mx = mr.x();
    int my = mr.y();
    int mw = mr.width();
    int mh = mr.height();
    
    QColor baseColor(MusEGlobal::config.partCanvasBg.light(104));

    p.save();
    p.setWorldMatrixEnabled(false);
    
    MusECore::TrackList* tl = MusEGlobal::song->tracks();
    int yoff = -rmapy(yorg) - ypos;
    int yy = yoff;
    int th;
    for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
          if (yy > my + mh)
                break;
          MusECore::Track* track = *it;
          th = track->height();
          if (!th)
            continue;
          if (!track->isMidiTrack()) { // draw automation
                QRect r(mx, yy, mw, th);
                if(r.intersects(mr)) 
                {
                  drawAutomation(p, r, (MusECore::AudioTrack*)track);
                }  
          }
          yy += th;
          }

    unsigned int startPos = MusEGlobal::extSyncFlag.value() ? MusEGlobal::audio->getStartExternalRecTick() : MusEGlobal::audio->getStartRecordPos().tick(); 
    if (MusEGlobal::song->punchin())
      startPos=MusEGlobal::song->lpos();
    int startx = mapx(startPos);
    int width = mapx(MusEGlobal::song->cpos()) - mapx(startPos);

    if (MusEGlobal::song->cpos() < startPos) {
        p.restore();
        return; // no drawing if we are before punch out
    }
    if (MusEGlobal::song->punchout() && MusEGlobal::song->cpos() > MusEGlobal::song->rpos()) {
       p.restore();
       return; // no drawing if we are beyond punch out.
    }

    // write recording while it happens to get feedback
    // should be enhanced with solution that draws waveform also
    int yPos = yoff;
    if (MusEGlobal::song->record() && MusEGlobal::audio->isPlaying()) {
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
        MusECore::Track* track = *it;
        th = track->height();
        if (!th)
          continue;
        if (track->recordFlag()) {
            QPen pen(Qt::black, 0, Qt::SolidLine);
            p.setPen(pen);
            QColor c(MusEGlobal::config.partColors[0]);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
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

    // draw midi events on
    yPos=0;
    if (MusEGlobal::song->record() && MusEGlobal::audio->isPlaying()) {
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
           MusECore::Track* track = *it;

           if (track->isMidiTrack() && track->recordFlag()) {
               MusECore::MidiTrack *mt = (MusECore::MidiTrack*)track;
               QRect partRect(startPos,yPos, MusEGlobal::song->cpos()-startPos, track->height()); // probably the wrong rect
               MusECore::EventList myEventList;
               if (mt->mpevents.size()) {

                 for (MusECore::ciMPEvent i = mt->mpevents.begin(); i != mt->mpevents.end(); ++i) {
                    MusECore::MidiPlayEvent pe = *i;

                    if (pe.isNote() && !pe.isNoteOff()) {
                      MusECore::Event e(MusECore::Note);
                      e.setPitch(pe.dataA());
                      e.setTick(pe.time()-startPos);
                      e.setLenTick(MusEGlobal::song->cpos()-pe.time());
                      e.setC(1); // we abuse this value to determine that this note hasn't been concluded
                      myEventList.add(e);
                    }
                    else if (pe.isNoteOff()) {
                      for (MusECore::iEvent i = myEventList.begin(); i != myEventList.end(); ++i) {
                        MusECore::Event &e = i->second;
                        if (e.pitch() == pe.dataA() && e.dataC() == 1) {
                          e.setLenTick(pe.time() - e.tick()- startPos);
                          e.setC(0); // reset the variable we borrowed for state handling
                          continue;
                        }
                      }
                    }
                 }
               }
               drawMidiPart(p, rect, myEventList, mt, 0, partRect,startPos,0,MusEGlobal::song->cpos()-startPos);
           }
           yPos+=track->height();
      }
    }

}

//---------------------------------------------------------
//   drawAudioTrack
//---------------------------------------------------------
void PartCanvas::drawAudioTrack(QPainter& p, const QRect& r, const QRect& bbox, MusECore::AudioTrack* /*t*/)
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
      c.setAlpha(MusEGlobal::config.globalAlphaBlend);
      QLinearGradient gradient(mex + 1, mey + 1, mex + 1, mey + meh - 1);    // Inside the border
      gradient.setColorAt(0, c);
      gradient.setColorAt(1, c.darker());
      QBrush brush(gradient);
      p.fillRect(mr, brush);       // p4.0.30  ...
      
      // DELETETHIS 6
      //int xx = -rmapx(xorg) - xpos;           
      //printf("PartCanvas::drawAudioTrack x:%d y:%d w:%d h:%d th:%d xx:%d\n", r.x(), r.y(), r.width(), r.height(), t->height(), xx);  
      //if(r.x() <= xx)
      //  p.drawLine(r.x(), r.y(), r.x(), r.y() + r.height());                          // The left edge
      //p.drawLine(r.x(), r.y(), r.x() + r.width(), r.y());                             // The top edge
      //p.drawLine(r.x(), r.y() + r.height(), r.x() + r.width(), r.y() + r.height());   // The bottom edge
      
      if(mex >= mx && mex <= mx + mw)
        p.drawLine(mex, my, mex, my + mh - 1);                // The left edge
      //if(mex + mew >= mx && mex + mew <= mx + mw) DELETETHIS 2
      //  p.drawLine(mex + mew, my, mex + mew, my + mh - 1);  // The right edge. Not used - infinite to the right
      if(mey >= my && mey <= my + mh)
        p.drawLine(mx, mey, mx + mw - 1, mey);                // The top edge
      if(mey + meh >= my && mey + meh <= my + mh)
        p.drawLine(mx, mey + meh, mx + mw - 1, mey + meh);    // The bottom edge. Special for Audio track - draw one past bottom.
}

//---------------------------------------------------------
//   drawAutomation
//---------------------------------------------------------

void PartCanvas::drawAutomation(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
    //QRect rr = p.worldMatrix().mapRect(r);
    //p.save();
    //p.resetTransform();

    int bottom = rr.bottom() - 2;
    int height = bottom - rr.top() - 2; // limit height

    //printf("PartCanvas::drawAutomation x:%d y:%d w:%d h:%d height:%d\n", rr.x(), rr.y(), rr.width(), rr.height(), height); 
    
    p.setBrush(Qt::NoBrush);
    
    MusECore::CtrlListList* cll = t->controller();
    for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
    {
      MusECore::CtrlList *cl = icll->second;
      if (cl->dontShow() || !cl->isVisible())
        continue;
      MusECore::iCtrl ic=cl->begin();
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
      bool discrete = cl->mode() == MusECore::CtrlList::DISCRETE;  
      QPen pen1(cl->color(), 0);  
      QPen pen2(cl->color(), 2);  
      pen2.setCosmetic(true);

      // Store first value for later
      double yfirst;
      {
          if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
            yfirst = logToVal(cl->curVal(), min, max); // represent volume between 0 and 1
            if (yfirst < 0) yfirst = 0.0;
          }
          else {
            yfirst = (cl->curVal() - min)/(max-min);  // we need to set curVal between 0 and 1
          }
          yfirst = oldY = bottom - rmapy_f(yfirst) * height;
      }

      // Check that there IS automation, ic == cl->end means no automation
      if (ic == cl->end())
      {
          ypixel = yfirst;
      }
      else
      {
        for (; ic !=cl->end(); ++ic)
        {
            double y = ic->second.val; 
            if (cl->valueType() == MusECore::VAL_LOG ) {
              y = logToVal(y, min, max); // represent volume between 0 and 1
              if (y < 0) y = 0.0;
            }
            else 
              y = (y-min)/(max-min);  // we need to set curVal between 0 and 1
            
            ypixel = bottom - rmapy_f(y) * height;
            xpixel = mapx(MusEGlobal::tempomap.frame2tick(ic->second.frame));
            
            if (oldY==-1) oldY = ypixel;

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
            pen2.setColor((automation.currentCtrlValid && automation.currentCtrlList == cl && 
                           automation.currentCtrlFrameList.contains(ic->second.frame)) ?
                          Qt::white : cl->color());  
            
            p.setPen(pen2);
            p.drawRect(xpixel-2, ypixel-2, 5, 5);
            oldX = xpixel;
            oldY = ypixel;
            if (automation.currentCtrlValid && automation.currentCtrlList == cl &&
                 automation.currentCtrlFrameList.contains(ic->second.frame) &&
                 automation.currentCtrlFrameList.size() == 1) {
                    double val = ic->second.val;
                    QRect textRect = rr;
                    textRect.setX(xpixel + 20);
                    textRect.setY(ypixel);
                    if (cl->valueType() == MusECore::VAL_LOG) {
                        val = MusECore::fast_log10(ic->second.val) * 20.0;
                    }
                    p.drawText(textRect, QString("Param: %1, Value: %2").arg(cl->name()).arg(val));
            }
        }
      }

      p.setPen(pen1);
      //int xTextPos = mapx(0) > rmapx(0) ? mapx(0) + 5 : rmapx(0) + 5; // follow window..(doesn't work very well)
      int xTextPos = mapx(0) + 5;
      p.drawText(xTextPos,yfirst,100,height-2,0,cl->name());

      if (xpixel <= rr.right())
      {
        p.setPen(pen1);
        p.drawLine(xpixel, ypixel, rr.right(), ypixel);
      }  
    }
}

//---------------------------------------------------------
//  checkIfOnLine
//    check if our point is on the line defined by
//    by first/last X/Y
//---------------------------------------------------------

bool checkIfOnLine(double mouseX, double mouseY, double firstX, double lastX, double firstY, double lastY, int circumference)
{
  if (lastX==firstX)  
    return (ABS(mouseX-lastX) < circumference);
  else if (mouseX < firstX || mouseX > lastX+circumference) // (*)
    return false;
  else
  {
    double proportion = (mouseX-firstX)/(lastX-firstX); // a value between 0 and 1, where firstX->0 and lastX->1
    double calcY = (lastY-firstY)*proportion+firstY;    // where the drawn line's y-coord is at mouseX
    double slope = (lastY-firstY)/(lastX-firstX);
    
    return (ABS(calcY-mouseY) < (circumference * sqrt(1+slope*slope)));
    // this is equivalent to circumference / cos( atan(slope) ). to
    // verify, draw a sloped line (the graph), a 90°-line to it with
    // length "circumference". from the (unconnected) endpoint of that
    // line, draw a vertical line down to the sloped line.
    // use slope=tan(alpha) <==> alpha=atan(slope) and
    // cos(alpha) = adjacent side / hypothenuse (hypothenuse is what we
    // want, and adjacent = circumference).
    // to optimize: this looks similar to abs(slope)+1
    
    //return (ABS(calcY-mouseY) < circumference);
  }
  
  /* without the +circumference in the above if statement (*), moving
   * the mouse towards a control point from the right would result in
   * the line segment from the targeted point to the next to be con-
   * sidered, but not the segment from the previous to the targeted.
   * however, only points for which the line segment they _end_ is
   * under the cursor are considered, so we need to enlengthen this
   * a bit  (flo93)*/
}

//---------------------------------------------------------
//  checkIfNearPoint
//---------------------------------------------------------

bool checkIfNearPoint(int mouseX, int mouseY, int eventX, int eventY, int circumference)
{
  return (ABS(mouseX - eventX) < circumference &&  ABS(mouseY - eventY) < circumference);
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

void PartCanvas::checkAutomation(MusECore::Track * t, const QPoint &pointer, bool /*NOTaddNewCtrl*/)
{
    if (t->isMidiTrack())
      return;

    int mouseY;
    int trackY = t->y();
    int trackH = t->height();
    
    { int y = pointer.y();
      if(y < trackY || y >= (trackY + trackH))
        return; 
      mouseY =  mapy(y);  }
    
    int mouseX =  mapx(pointer.x());
    int circumference = 10;
    
    MusECore::CtrlListList* cll = ((MusECore::AudioTrack*) t)->controller();
    for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
      {
        MusECore::CtrlList *cl = icll->second;
        if (cl->dontShow() || !cl->isVisible()) {
          continue;
        }
        MusECore::iCtrl ic=cl->begin();

        int eventOldX = mapx(0);
        int eventX = eventOldX;
        int eventOldY = -1;
        int eventY = eventOldY;
        double min, max;
        cl->range(&min,&max);
        bool discrete = cl->mode() == MusECore::CtrlList::DISCRETE;  

        // First check that there IS automation for this controller, ic == cl->end means no automation
        if (ic == cl->end()) 
        {
          double y;   
          if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
            y = logToVal(cl->curVal(), min, max); // represent volume between 0 and 1
            if (y < 0) y = 0.0;
          }
          else 
            y = (cl->curVal() - min)/(max-min);  // we need to set curVal between 0 and 1
          eventY = eventOldY = mapy(trackY+trackH-1 - 2 - y * trackH);
        }
        else // we have automation, loop through it
        {  
          for (; ic!=cl->end(); ic++)
          {
             double y = ic->second.val;
             if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
               y = logToVal(y, min, max); // represent volume between 0 and 1
               if (y < 0) y = 0;
             }
             else 
               y = (y-min)/(max-min);  // we need to set curVal between 0 and 1
             
             eventY = mapy(trackY + trackH - 2 - y * trackH);
             eventX = mapx(MusEGlobal::tempomap.frame2tick(ic->second.frame));

             if (eventOldY==-1) eventOldY = eventY;
             
             //if (addNewCtrl) {
             bool onLine = checkIfOnLine(mouseX, mouseY, eventOldX,eventX, eventOldY, discrete? eventOldY:eventY, circumference);
             bool onPoint = false;
             if ( pointer.x() > 0 && pointer.y() > 0)
               onPoint = checkIfNearPoint(mouseX, mouseY, eventX, eventY, circumference);

             eventOldX = eventX;
             eventOldY = eventY;
             
             if (onLine) {
               if (!onPoint) {
                 QWidget::setCursor(Qt::CrossCursor);
                 automation.currentCtrlValid = false;
                 automation.controllerState = addNewController;
               }else {
                 QWidget::setCursor(Qt::OpenHandCursor);
                 automation.currentCtrlFrameList.clear();
                 automation.currentCtrlFrameList.append(ic->second.frame);
                 automation.currentCtrlValid = true;
                 automation.controllerState = movingController;
               }
               automation.currentCtrlList = cl;
               automation.currentTrack = t;
               update();
               return;
             }
          }
        } 

        // we are now after the last existing controller
        // check if we are reasonably close to a line, we only need to check Y
        // as the line is straight after the last controller
        //printf("post oldX:%d oldY:%d xpixel:%d ypixel:%d currX:%d currY:%d\n", oldX, oldY, xpixel, ypixel, currX, currY);
        if(mouseX >= eventX && ABS(mouseY-eventY) < circumference) {
          QWidget::setCursor(Qt::CrossCursor);
          automation.controllerState = addNewController;
          automation.currentCtrlList = cl;
          automation.currentTrack = t;
          automation.currentCtrlValid = false;
          return;
        }
      }
      // if there are no hits we default to clearing all the data
      automation.controllerState = doNothing;
      automation.currentCtrlValid = false;
      automation.currentCtrlList = 0;
      automation.currentTrack = 0;
      automation.currentCtrlFrameList.clear();
      setCursor();
}

void PartCanvas::controllerChanged(MusECore::Track* t, int)
{
  redraw((QRect(0, mapy(t->y()), width(), rmapy(t->height()))));  // TODO Check this - correct?
}

void PartCanvas::processAutomationMovements(QPoint pos, bool addPoint)
{

  if (_tool == AutomationTool) {

      if (!automation.moveController) { // currently nothing going lets's check for some action.
          MusECore::Track * t = y2Track(pos.y());
          if (t) {
             checkAutomation(t, pos, addPoint);
          }
          return;
      }

    // automation.moveController is set, lets rock.
    int prevFrame = 0;
    int nextFrame = -1;

    if (automation.controllerState == addNewController)
    {
       int frame = MusEGlobal::tempomap.tick2frame(pos.x());
       // FIXME Inefficient to add with wait here, then remove and add with wait again below. Tim.
       MusEGlobal::audio->msgAddACEvent((MusECore::AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), frame, 1.0 /*dummy value */);
       
       MusECore::iCtrl ic=automation.currentCtrlList->begin();
       for (; ic !=automation.currentCtrlList->end(); ++ic) {
         MusECore::CtrlVal &cv = ic->second;
         if (cv.frame == frame) {
           automation.currentCtrlFrameList.clear();
           automation.currentCtrlFrameList.append(cv.frame);
           automation.currentCtrlValid = true;
           automation.controllerState = movingController;
           break;
         }
       }
    }

    // get previous and next frame position to give x bounds for this event.
    MusECore::iCtrl ic=automation.currentCtrlList->begin();
    MusECore::iCtrl iprev = ic;
    for (; ic !=automation.currentCtrlList->end(); ++ic)
    {
       MusECore::CtrlVal &cv = ic->second;
       if (automation.currentCtrlFrameList.contains(cv.frame))
       {
         //currFrame = cv.frame;
         break;
       }  
       prevFrame = cv.frame;
       iprev = ic;
    }
    
    MusECore::iCtrl icc = ic;

    if ( ++ic != automation.currentCtrlList->end()) {
      MusECore::CtrlVal &cv = ic->second;
      nextFrame = cv.frame;
    }
    
    // A perfectly straight vertical line (two points with same frame) is impossible:
    //  there is only one value at t, and the next value at t+1, and so on.
    // Also these are maps, not multimaps.           p4.0.32 Tim.
    int newFrame = MusEGlobal::tempomap.tick2frame(pos.x());

    if (newFrame <= prevFrame) 
      newFrame=prevFrame + (icc == automation.currentCtrlList->begin() ? 0: 1);  // Only first item is allowed to go to zero x.
    if (nextFrame!=-1 && newFrame >= nextFrame) newFrame=nextFrame-1;
    
    int posy=mapy(pos.y());
    int tracky = mapy(automation.currentTrack->y());
    int trackHeight = automation.currentTrack->height();

    int mouseY = trackHeight - (posy - tracky)-2;
    double yfraction = ((double)mouseY)/automation.currentTrack->height();

    double min, max;
    automation.currentCtrlList->range(&min,&max);
    double cvval;    
    if (automation.currentCtrlList->valueType() == MusECore::VAL_LOG  ) { // use db scale for volume
       //printf("log conversion val=%f min=%f max=%f\n", yfraction, min, max);
       cvval = valToLog(yfraction, min, max);
       if (cvval< min) cvval=min;
       if (cvval>max) cvval=max;
    }
    else {
      // we need to set val between 0 and 1 (unless integer)
      cvval = yfraction * (max-min) + min;
      // 'Snap' to integer or boolean
      if (automation.currentCtrlList->mode() == MusECore::CtrlList::DISCRETE)
        cvval = rint(cvval + 0.1); // LADSPA docs say add a slight bias to avoid rounding errors. Try this.
      if (cvval< min) cvval=min;
      if (cvval>max) cvval=max;
    }
    
    automation.currentCtrlFrameList.clear();
    automation.currentCtrlFrameList.append(newFrame);
    automation.currentCtrlValid = true;
    
    if(icc != automation.currentCtrlList->end())
      MusEGlobal::audio->msgChangeACEvent((MusECore::AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), icc->second.frame, newFrame, cvval);
    else
      MusEGlobal::audio->msgAddACEvent((MusECore::AudioTrack*)automation.currentTrack, automation.currentCtrlList->id(), newFrame, cvval);

    // User probably would like to hear results so make sure controller is enabled.
    ((MusECore::AudioTrack*)automation.currentTrack)->enableController(automation.currentCtrlList->id(), true); 
  }

}

//---------------------------------------------------------
//
//  logToVal
//   - represent logarithmic value on linear scale from 0 to 1
//
//---------------------------------------------------------
double PartCanvas::logToVal(double inLog, double min, double max)
{
    if (inLog < min) inLog = min;
    if (inLog > max) inLog = max;
    double linMin = 20.0*MusECore::fast_log10(min);
    double linMax = 20.0*MusECore::fast_log10(max);
    double linVal = 20.0*MusECore::fast_log10(inLog);

    double outVal = (linVal-linMin) / (linMax - linMin);

    return outVal;
}

//---------------------------------------------------------
//
//  valToLog
//   - represent value from 0 to 1 as logarithmic value between min and max
//
//---------------------------------------------------------
double PartCanvas::valToLog(double inV, double min, double max)
{
    double linMin = 20.0*MusECore::fast_log10(min);
    double linMax = 20.0*MusECore::fast_log10(max);

    double linVal = (inV * (linMax - linMin)) + linMin;
    double outVal = exp10((linVal)/20.0);

    //printf("::valToLog inV %f outVal %f linVal %f min %f max %f\n", inV, outVal, linVal, min, max);
    if (outVal > max) outVal = max;
    if (outVal < min) outVal = min;
    return outVal;
}

//---------------------------------------------------------
//   endMoveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void PartCanvas::endMoveItems(const QPoint& pos, DragType dragtype, int dir, bool rasterize)
      {
      int dp = y2pitch(pos.y()) - y2pitch(start.y());
      int dx = pos.x() - start.x();

      if (dir == 1)
            dp = 0;
      else if (dir == 2)
            dx = 0;

      moveCanvasItems(moving, dp, dx, dragtype, rasterize);
      
      moving.clear();
      updateSelection();
      redraw();
      }

} // namespace MusEGui
