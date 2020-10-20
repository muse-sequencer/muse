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
#ifdef _WIN32
#include "mman.h"
#else
#include <sys/mman.h>
#endif
#include <errno.h>
#include <limits.h>
#include <map>
#include <assert.h>

#include <QClipboard>
#include <QMessageBox>
#include <QUrl>
#include <QPoint>
#include <QIcon>
#include <QMimeData>
#include <QDrag>
#include <QStringList>

#include "muse_math.h"
#include "fastlog.h"
#include "components/tools.h"
#include "arranger.h"
#include "arrangerview.h"
#include "structure.h"
#include "pcanvas.h"
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
#include "midi_consts.h"
#include "midictrl.h"
#include "utils.h"
#include "dialogs.h"
#include "components/pastedialog.h"
#include "undo.h"
#include "tracks_duplicate.h"
#include "name_factory.h"
#include "song.h"

// Forwards from header:
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QDragEnterEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include "ctrl.h"
#include "xml.h"
#include "part.h"
#include "citem.h"

using MusECore::Undo;
using MusECore::UndoOp;

#define ABS(x) (abs(x))

#define FABS(x) (fabs(x))

#define EDITING_FINISHED_TIMEOUT 50 /* in milliseconds */

using std::set;

namespace MusEGui {

const int PartCanvas::_automationPointDetectDist = 4;
const int PartCanvas::_automationPointWidthUnsel = 2;
const int PartCanvas::_automationPointWidthSel = 3;

//---------------------------------------------------------
//   NPart
//---------------------------------------------------------

NPart::NPart(MusECore::Part* p) : PItem(p)
      {
      leftBorderTouches = false;
      rightBorderTouches = false;

      _serial=_part->sn();

      int y  = track()->y();
      setPos(QPoint(_part->tick(), y));
      setBBox(QRect(_part->tick(), y, _part->lenTick(), track()->height()));
      }

//---------------------------------------------------------
//   PartCanvas
//---------------------------------------------------------

PartCanvas::PartCanvas(int* r, QWidget* parent, int sx, int sy)
   : Canvas(parent, sx, sy)
      {
      setObjectName("PartCanvas");
      setAcceptDrops(true);
      _raster = r;

      setFocusPolicy(Qt::StrongFocus);
      // Defaults:
      lineEditor = nullptr;
      editMode   = false;

      supportsResizeToTheLeft = true;
      setStatusTip(tr("Part canvas: Use Pencil tool to draw parts. Double-click to create a new MIDI/drum part between the range markers (set with MMB + RMB). Press F1 for more."));

      tracks = MusEGlobal::song->tracks();
      setMouseTracking(true);
      drag          = DRAG_OFF;
      curColorIndex = 0;
      automation.currentCtrlValid = false;
      automation.controllerState = doNothing;
      automation.moveController = false;
      automation.breakUndoCombo = false;
      updateItems();
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
      MusECore::ciTrack it;
      for (it = tl->begin(); it != tl->end(); ++it, ++idx) {
            int h = (*it)->height();
            if (y < yy+h)
                  break;
            yy += h;
            }
      if(it == tl->end()) {
            while(y >= yy + MusEGlobal::config.trackHeight) {
                  ++idx;
                  yy += MusEGlobal::config.trackHeight;
                  }
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
      MusECore::ciTrack it;
      for (it = tl->begin(); it != tl->end(); ++it, ++idx) {
            if (idx == p)
                  break;
            yy += (*it)->height();
            }
      if(it == tl->end()) {
            yy += (p - idx) * MusEGlobal::config.trackHeight;
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
      return MusEGlobal::config.trackHeight;
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
          //this check is necessary, because it returnPressed may be called
          //twice. the second call would cause a crash, however!
          MusECore::Part* part = editPart->part();
          // Indicate do undo, and do port controller values but not clone parts.

          Undo operations;
          operations.push_back(UndoOp(UndoOp::ModifyPartName,part, part->name(), lineEditor->text()));
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
      bool alt = event->modifiers() & Qt::AltModifier;
      if (curItem) {
          if ((event->button() == Qt::LeftButton) && ctrl && alt) {
              deselectAll();
              selectItem(curItem, true);
              emit dclickPart(((NPart*)(curItem))->track());
          }
          else if ((event->button() == Qt::LeftButton) && ctrl) {
                  editPart = (NPart*)curItem;
                  QRect r = map(curItem->bbox());
                  if (lineEditor == nullptr) {
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
                              {
                              MusECore::MidiPart* part = new MusECore::MidiPart((MusECore::MidiTrack*)track);
                              part->setTick(pos[1]);
                              part->setLenTick(pos[2]-pos[1]);
                              part->setName(track->name());
                              NPart* np = new NPart(part);
                              items.add(np);
                              deselectAll();
                              part->setSelected(true);
                              np->setSelected(true);
                              MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddPart, part));
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
//   moveCanvasItems
//---------------------------------------------------------

void PartCanvas::moveCanvasItems(CItemMap& items, int dp, int dx, DragType dtype, bool rasterize)
{
  MusECore::Undo operations;

  // We need a map of canvas items sorted by Y position.
  typedef std::multimap<int /* y */, CItem*> y_map_t;
  y_map_t y_map;
  for(ciCItem ici = items.cbegin(); ici != items.cend(); ++ici)
  {
    CItem* ci = ici->second;
    y_map.insert(std::pair<int, CItem*>(ci->bbox().y(), ci));
  }
  
  int cur_new_track_idx = -1;

  // Find out how many new track types we need, for the options dialog.
  int audio_found = 0;
  int midi_found = 0;
  int new_drum_found = 0;
  for(y_map_t::const_iterator ici = y_map.cbegin(); ici != y_map.cend(); ++ici)
  {
    CItem* ci = ici->second;
    ci->setMoving(false);

    const int x = ci->pos().x();
    const int y = ci->pos().y();
    const int nx = x + dx;
    const int ny = pitch2y(y2pitch(y) + dp);
    QPoint newpos = QPoint(nx, ny);
    if(rasterize)
      newpos = raster(newpos);

    const NPart* npart    = (NPart*) ci;
    const MusECore::Part* spart     = npart->part();
    const MusECore::Track* track    = npart->track();
    const unsigned dtick  = newpos.x(); // FIXME TODO make subtick-compatible!
    const int ntrack = y2pitch(ci->mp().y());
    const MusECore::Track::TrackType type = track->type();
    if (tracks->index(track) == ntrack && (dtick == spart->tick())) {
        continue;
    }

    if (ntrack >= (int)tracks->size()) {
        // Would create a new track, but only at the first part found on a track.
        if(ntrack != cur_new_track_idx)
        {
          cur_new_track_idx = ntrack;
          
          if(type == MusECore::Track::DRUM)
            ++new_drum_found;
          else if(type == MusECore::Track::MIDI)
            ++midi_found;
          else
            ++audio_found;
        }
    }
  }
  
  int flags = MusECore::Track::ASSIGN_PROPERTIES;
  if(audio_found != 0 || midi_found != 0 || new_drum_found != 0)
  {  
    MusEGui::DuplicateTracksDialog* dlg = new MusEGui::DuplicateTracksDialog(
        audio_found, midi_found, new_drum_found,
        nullptr, // parent
        false,   // copies
        true,    // allRoutes
        true,    // defaultRoutes
        false,   // noParts
        false,   // duplicateParts
        false,   // copyParts
        false    // cloneParts
      );

    int rv = dlg->exec();
    if(rv == QDialog::Rejected)
    {
      delete dlg;
      return;
    }
    
    if(dlg->copyStdCtrls())
      flags |= MusECore::Track::ASSIGN_STD_CTRLS;
    if(dlg->copyPlugins())
      flags |= MusECore::Track::ASSIGN_PLUGINS;
    if(dlg->copyPluginCtrls())
      flags |= MusECore::Track::ASSIGN_PLUGIN_CTRLS;
    if(dlg->allRoutes())
      flags |= MusECore::Track::ASSIGN_ROUTES;
    if(dlg->defaultRoutes())
      flags |= MusECore::Track::ASSIGN_DEFAULT_ROUTES;

    if(dlg->copyDrumlist())
      flags |= MusECore::Track::ASSIGN_DRUMLIST;
    
    delete dlg;
  }
  
  // Reset.
  cur_new_track_idx = -1;
  int num_incompatible = 0;
  MusECore::Track* dtrack = nullptr;
  MusECore::TrackNameFactory track_names;
  
  for(y_map_t::iterator ici = y_map.begin(); ici != y_map.end(); ++ici)
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

    NPart* npart    = (NPart*) ci;
    MusECore::Part* spart     = npart->part();
    MusECore::Track* track    = npart->track();
    unsigned dtick  = newpos.x(); // FIXME TODO make subtick-compatible!
    int ntrack = y2pitch(ci->mp().y());
    MusECore::Track::TrackType type = track->type();
    if (tracks->index(track) == ntrack && (dtick == spart->tick())) {
        continue;
    }

    if (ntrack >= (int)tracks->size()) {
        // Create a new track, but only at the first part found on a track.
        if(ntrack != cur_new_track_idx)
        {
          cur_new_track_idx = ntrack;

          // TODO Make sure the list redraws without this !
          //         emit tracklistChanged();
          
          if(!track_names.genUniqueNames(type, track->name(), 1))
            continue;
          
#if 1
          // FIXME Works but height is copied with PROPERTIES - we don't want that but it
          //        can be corrected below, BUT some other things are copied that we may not want.
          //       Do we really want copies, or a fresh blank track? Maybe pop up a dialog and ask ?
          dtrack = track->clone(flags);
#else
          dtrack = MusEGlobal::song->createTrack(type);
#endif
          
          if(!dtrack)
            continue;
          
#if 1
          // For the line above, if used. Fix the height instead of using the original track height.
          dtrack->setHeight(MusEGlobal::config.trackHeight);
#endif

          dtrack->setName(track_names.first());

          if (type == MusECore::Track::WAVE) {
              MusECore::WaveTrack* st = (MusECore::WaveTrack*) track;
              MusECore::WaveTrack* dt = (MusECore::WaveTrack*) dtrack;
              dt->setChannels(st->channels());
          }

          // Add at end of list.
          operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddTrack, -1, dtrack));
        }
    }
    else
    {
        dtrack = tracks->index(ntrack);
        if (dtrack->type() != type) {
            ++num_incompatible;
            continue;
        }
    }

    if(!dtrack)
      continue;

    if(dtype == MOVE_MOVE)
        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::MovePart, spart, spart->posValue(), dtick, MusECore::Pos::TICKS, track, dtrack));
    else
    {
        MusECore::Part* dpart;
        bool clone = (dtype == MOVE_CLONE || (dtype == MOVE_COPY && spart->hasClones()));

        // Gives the new part a new serial number.
        if (clone)
            dpart = spart->createNewClone();
        else
            dpart = spart->duplicate();

        dpart->setTick(dtick);
        dpart->setTrack(dtrack);

        spart->setSelected(false);
        dpart->setSelected(true);

        operations.push_back(MusECore::UndoOp(MusECore::UndoOp::AddPart,dpart));
    }
  }

  if(num_incompatible > 0)
  {
    QMessageBox::critical(this, QString("MusE"),
                          tr("Cannot copy/move/clone to different Track-Type"));
  }

  MusEGlobal::song->applyOperationGroup(operations);
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
      x = MusEGlobal::sigmap.raster(x, *_raster);
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
//   updateItems
//---------------------------------------------------------

void PartCanvas::updateItems()
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

                  // Check for touching borders.
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
//   itemSelectionsChanged
//---------------------------------------------------------

bool PartCanvas::itemSelectionsChanged(MusECore::Undo* operations, bool deselectAll)
{
      // Whether to deselect all events when clicking on parts.
      // This is normally undesirable but in the future we may need that ability...
      const bool deselect_events = false;
      const bool do_deselect_all_events = deselectAll && deselect_events;
      
      MusECore::Undo ops;
      MusECore::Undo* opsp = operations ? operations : &ops;
  
      //Undo operations;
      bool item_selected;
      bool obj_selected;
      bool changed=false;
      
      // If we are deselecting all, globally deselect all events,
      //  and don't bother individually deselecting objects, below.
      if(do_deselect_all_events)
      {
        //opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0, false));
        opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0));
        changed = true;
      }
      
      for (iCItem i = items.begin(); i != items.end(); ++i) {
            CItem* item = i->second;
            item_selected = item->isSelected();
            obj_selected = item->objectIsSelected();
                
            // Don't bother deselecting objects if we have already deselected all, above.
            if((item_selected || !do_deselect_all_events) &&
                ((item_selected != obj_selected) ||
                // Need to force this because after the 'deselect all events' command executes,
                //  if the item is selected another select needs to be executed even though it
                //  appears nothing changed here.
                (item_selected && do_deselect_all_events)))
              
            {
              opsp->push_back(UndoOp(UndoOp::SelectPart, item->part(), item_selected, obj_selected));
              // Here we have a choice of whether to allow undoing of selections.
              // Disabled for now, it's too tedious in use. Possibly make the choice user settable.
              // Operation set as not undoable.
              //operations.push_back(UndoOp(UndoOp::SelectPart, item->part(), item_selected, obj_selected, false));
              
              changed=true;
            }
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
//               fprintf(stderr, "PartCanvas::updateSelection: Applied SelectPart operations, redrawing\n");
      }

      return changed;
}

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PartCanvas::resizeItem(CItem* i, bool noSnap, bool ctrl)
{
   MusECore::Track* t = ((NPart*)(i))->track();
   MusECore::Part*  p = ((NPart*)(i))->part();

   unsigned int pos = p->tick() + i->width();
   int snappedpos = pos;
   if (!noSnap) {
      snappedpos = MusEGlobal::sigmap.raster(pos, *_raster);
   }
   unsigned int newTickWidth = snappedpos - p->tick();
   if (newTickWidth == 0) {
      newTickWidth = MusEGlobal::sigmap.rasterStep(p->tick(), *_raster);
   }
   unsigned int newTickPos = 0;
   if((i->mp() != i->pos()) && (resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT))
   {
      if(i->mp().x() < 0)
      {
        newTickPos = 0;
      }
      else
      {
        newTickPos  = i->mp().x();
      }
   }
   MusECore::resize_part(t, p, newTickWidth, resizeDirection, newTickPos, !ctrl);
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
        x = MusEGlobal::sigmap.raster1(x, *_raster);
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
        x = MusEGlobal::sigmap.raster1(x, *_raster);
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
            len = MusEGlobal::sigmap.raster(len, *_raster);
      if (len == 0)
            len = MusEGlobal::sigmap.rasterStep(p->tick(), *_raster);
      p->setLenTick(len);
      p->setSelected(true);
      i->setSelected(true);
      MusEGlobal::song->applyOperation(MusECore::UndoOp(MusECore::UndoOp::AddPart, p));
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PartCanvas::deleteItem(CItem* i)
      {
      MusECore::Part*  p = ((NPart*)(i))->part();
      // Invokes songChanged which calls partsChanged which makes it difficult to delete them there
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeletePart, p));
      return true;
      }

//---------------------------------------------------------
//   splitItem
//---------------------------------------------------------

void PartCanvas::splitItem(CItem* item, const QPoint& pt)
      {
      NPart* np = (NPart*) item;
      MusECore::Part*  p = np->part();
      int x = pt.x();
      if (x < 0)
            x = 0;
      split_part(p,MusEGlobal::sigmap.raster(x, *_raster));
      }

//---------------------------------------------------------
//   glueItem
//---------------------------------------------------------

void PartCanvas::glueItem(CItem* item)
      {
      NPart* np = (NPart*) item;
      merge_with_next_part(np->part());
      }

//---------------------------------------------------------
//   genItemPopup
//---------------------------------------------------------

QMenu* PartCanvas::genItemPopup(CItem* item)
      {
      NPart* npart = (NPart*) item;
      MusECore::Track::TrackType trackType = npart->track()->type();

      QMenu* partPopup = new QMenu(this);

      partPopup->addAction(new MenuTitleItem(tr("Part"), partPopup));

      QAction *act_cut = partPopup->addAction(*editcutIconSet, tr("C&ut"));
      act_cut->setData(OP_CUT);
      act_cut->setShortcut(Qt::CTRL+Qt::Key_X);

      QAction *act_copy = partPopup->addAction(*editcopyIconSet, tr("&Copy"));
      act_copy->setData(OP_COPY);
      act_copy->setShortcut(Qt::CTRL+Qt::Key_C);

      partPopup->addSeparator();
      int rc = npart->part()->nClones();
      QString st = QString(tr("S&elect "));
      if(rc > 1)
        st += (QString().setNum(rc) + QString(" "));
      st += QString(tr("clones"));
      QAction *act_select = partPopup->addAction(st);
      act_select->setData(OP_SELECT_CLONES);

      partPopup->addSeparator();
      QAction *act_rename = partPopup->addAction(tr("Rename"));
      act_rename->setData(OP_RENAME);

      QMenu* colorPopup = partPopup->addMenu(tr("Color"));

      // part color selection
      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
            QAction *act_color = colorPopup->addAction(MusECore::colorRect(MusEGlobal::config.partColors[i], 80, 80), MusEGlobal::config.partColorNames[i]);
            act_color->setData(OP_PARTCOLORBASE+i);
            }

      QAction *act_delete = partPopup->addAction(*deleteIconSVG, tr("Delete"));
      act_delete->setData(OP_DELETE);
      QAction *act_split = partPopup->addAction(*cutterIconSVG, tr("Split"));
      act_split->setData(OP_SPLIT);
      QAction *act_glue = partPopup->addAction(*glueIconSVG, tr("Glue"));
      act_glue->setData(OP_GLUE);
      QAction *act_superglue = partPopup->addAction(*glueIconSVG, tr("Super glue (merge selection)"));
      act_superglue->setData(OP_GLUESELECTION);
      QAction *act_declone = partPopup->addAction(tr("De-clone"));
      act_declone->setData(OP_DECLONE);

      partPopup->addSeparator();
      switch(trackType) {
            case MusECore::Track::MIDI: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startPianoEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startListEditAction);
                  partPopup->addMenu(MusEGlobal::muse->arranger()->parentWin()->scoreSubmenu);
                  QAction *act_mexport = partPopup->addAction(tr("Save part to disk..."));
                  act_mexport->setData(OP_SAVEPARTTODISK);
                  }
                  break;
            case MusECore::Track::DRUM: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startDrumEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startListEditAction);
                  QAction *act_dexport = partPopup->addAction(tr("Save part to disk..."));
                  act_dexport->setData(OP_SAVEPARTTODISK);
                  }
                  break;
            case MusECore::Track::WAVE: {
                  QAction *act_wedit = partPopup->addAction(*waveeditorSVGIcon, tr("Wave edit..."));
                  act_wedit->setData(OP_WAVEEDIT);
                  QAction *act_wexport = partPopup->addAction(tr("Save part to disk..."));
                  act_wexport->setData(OP_SAVEPARTTODISK);
                  QAction *act_wfinfo = partPopup->addAction(tr("File info..."));
                  act_wfinfo->setData(OP_FILEINFO);
                  QAction *act_wfnorm = partPopup->addAction(tr("Normalize"));
                  act_wfnorm->setData(OP_NORMALIZE);
                  act_wfnorm->setShortcut(Qt::CTRL+Qt::Key_N);
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

void PartCanvas::renameItem(CItem *item)
{
  editPart = (NPart*)(item);
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
   case OP_RENAME:     // rename
     renameItem(item);
      break;
   case OP_DELETE:     // delete
      deleteItem(item);
      break;
   case OP_SPLIT:     // split
      splitItem(item, pt);
      break;
   case OP_GLUE:     // glue
      glueItem(item);
      break;
   case OP_CUT:
      copy(pl);
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeletePart, npart->part()));
      break;
   case OP_COPY:
      copy(pl);
      break;
   case OP_GLUESELECTION:
      MusECore::merge_selected_parts();
      break;

   case OP_WAVEEDIT:    // wave edit
      emit startEditor(pl, 4);
      return;
   case OP_DECLONE:    // declone
   {
      MusECore::Part* spart  = npart->part();
      MusECore::Part* dpart  = spart->duplicate(); // dpart will not be member of any clone chain!

      Undo operations;
      operations.push_back(UndoOp(UndoOp::DeletePart, spart));
      operations.push_back(UndoOp(UndoOp::AddPart, dpart));
      MusEGlobal::song->applyOperationGroup(operations);
      break;
   }
   case OP_SAVEPARTTODISK: // Export to file
   {
      const MusECore::Part* part = item->part();
      bool popenFlag = false;
      QString fn = getSaveFileName(QString(""), MusEGlobal::part_file_save_pattern, this, tr("MusE: Save part"));
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

   case OP_FILEINFO: // File info
   {
      MusECore::Part* p = item->part();
      QString str = tr("Part name: %1\nFiles:").arg(p->name());
      for (MusECore::ciEvent e = p->events().begin(); e != p->events().end(); ++e)
      {
         MusECore::Event event = e->second;
         if(event.empty())
           continue;
         MusECore::SndFileR f  = event.sndFile();
         if (f.isNull())
            continue;
         str.append(QString("\n@") + QString().setNum(event.tick()) + QString(" len:") +
                    QString().setNum(event.lenTick()) + QString(" ") + f.path());
      }
      QMessageBox::information(this, "File info", str, "Ok", 0);
      break;
   }
   case OP_SELECT_CLONES: // Select clones
   {
      MusECore::Part* part = item->part();

      // Traverse and process the clone chain ring until we arrive at the same part again.
      // The loop is a safety net.
      MusECore::Part* p = part;

      Undo operations;
      if(part->hasClones())
      {
         // Here we have a choice of whether to allow undoing of selections.
         // Disabled for now, it's too tedious in use. Possibly make the choice user settable.
         operations.push_back(UndoOp(UndoOp::SelectPart, p, true, p->selected(), false));
         for(MusECore::Part* it = p->nextClone(); it!=p; it=it->nextClone())
            // Operation set as not undoable.
            operations.push_back(UndoOp(UndoOp::SelectPart, it, true, it->selected(), false));

         MusEGlobal::song->applyOperationGroup(operations);
      }

      break;
   }
   case OP_NORMALIZE: // Normalize
   {
      MusEGlobal::song->normalizeWaveParts(item->part());
      break;
   }
   case OP_PARTCOLORBASE ... NUM_PARTCOLORS+20:
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
                        automationMenu->addAction(new MenuTitleItem(tr("Automation"), automationMenu));
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
                          Undo operations;
                          foreach(int frame, automation.currentCtrlFrameList)
                              operations.push_back(UndoOp(UndoOp::DeleteAudioCtrlVal, automation.currentTrack, automation.currentCtrlList->id(), frame));
                          if(!operations.empty())
                          {
                            MusEGlobal::song->applyOperationGroup(operations);
                            // User probably would like to hear results so make sure controller is enabled.
                            ((MusECore::AudioTrack*)automation.currentTrack)->enableController(automation.currentCtrlList->id(), true);
                          }
                      }
                  }
                  else {
                      if (automation.controllerState != doNothing)
                      {
                          automation.moveController = true;
                          // Upon any next operations list execution, break any undo combining.
                          automation.breakUndoCombo = true;
                          newAutomationVertex(pt);
                      }
                  }
                  return false;
//                  break;
            }
      return true;
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void PartCanvas::mouseRelease(QMouseEvent*)
      {
          // clear all the automation parameters
          automation.moveController=false;
          automation.controllerState = doNothing;
          //automation.currentCtrl=0;
          automation.currentCtrlValid = false;
          automation.currentTrack=nullptr;
          automation.currentCtrlList=nullptr;
          //automation.breakUndoCombo = false; // Don't touch this here.
      }

//---------------------------------------------------------
//   viewMousevent
//---------------------------------------------------------

void PartCanvas::mouseMove(QMouseEvent* event)
      {
      int x = event->pos().x();
      if (x < 0)
            x = 0;

      if (_tool == AutomationTool) {
        event->accept();
        bool slowMotion = event->modifiers() & Qt::ShiftModifier;
        processAutomationMovements(event->pos(), slowMotion);
        emit timeChanged(MusEGlobal::sigmap.raster(x, *_raster));
        return;
      }

      event->ignore();
      emit timeChanged(MusEGlobal::sigmap.raster(x, *_raster));

      showStatusTip(event);
}

void PartCanvas::showStatusTip(QMouseEvent* event) {

    static CItem* hoverItem = nullptr;
    static Tool localTool;

    CItem* item;
        item = findCurrentItem(event->pos());

    if (item) {
        if (hoverItem == item && localTool == _tool)
            return;

        hoverItem = item;
        localTool = _tool;

        QString s;
        if (_tool & (MusEGui::PointerTool ))
            s = tr("LMB: Select/Move/Dblclick to edit | CTRL+LMB: Multi select/Move&Copy | CTRL+ALT+LMB: Dblclick to edit in new window | SHIFT+LMB: Select track | MMB: Delete");
        else if (_tool & (MusEGui::PencilTool))
            s = tr("LMB: Draw to resize | MMB: Delete | CTRL+RMB: Trim length");
        else if (_tool & (MusEGui::RubberTool))
            s = tr("LMB: Delete | CTRL+RMB: Trim length");
        else if (_tool & (MusEGui::CutTool))
            s = tr("LMB: Cut part in two");
        else if (_tool & (MusEGui::GlueTool))
            s = tr("LMB: Merge with following part");
        else if (_tool & (MusEGui::MuteTool))
            s = tr("LMB: Mute selected part");
        else if (_tool & (MusEGui::AutomationTool))
            s = tr("LMB: Edit automation events in audio parts");

        if (!s.isEmpty())
            MusEGlobal::muse->setStatusBarText(s);
    } else {
        if (hoverItem != nullptr) {
            MusEGlobal::muse->clearStatusBarText();
            hoverItem = nullptr;
        }
    }
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
      return nullptr;
      }

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void PartCanvas::keyPress(QKeyEvent* event)
{
      event->accept();

      int key = event->key();

      if (editMode)
      {
            // this will probably never happen, as edit mode has been set
            // to "false" some usec ago by returnPressed, called by editingFinished.
            if ( key == Qt::Key_Return || key == Qt::Key_Enter )
            {
                event->ignore();
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
              (key == Qt::Key_Return || key == Qt::Key_Enter) ) {
          event->ignore();
          return;
      }

      key = key | event->modifiers();

      if (key == shortcuts[SHRT_DELETE].key) {
            if (getCurrentDrag())
                  return;

            MusECore::delete_selected_parts();
            return;
      }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            int spos = pos[0];
            if(spos > 0)
            {
              spos -= 1;     // Nudge by -1, then snap down with raster1.
              spos = MusEGlobal::sigmap.raster1(spos, *_raster);
            }
            if(spos < 0)
              spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
      }
      else if (key == shortcuts[SHRT_POS_INC].key) {
            int spos = MusEGlobal::sigmap.raster2(pos[0] + 1, *_raster);    // Nudge by +1, then snap up with raster2.
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
      }
      else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key) {
            int spos = pos[0] - MusEGlobal::sigmap.rasterStep(pos[0], *_raster);
            if(spos < 0)
                spos = 0;
            MusECore::Pos p(spos,true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
            return;
      }
      else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key) {
            MusECore::Pos p(pos[0] + MusEGlobal::sigmap.rasterStep(pos[0], *_raster), true);
            MusEGlobal::song->setPos(MusECore::Song::CPOS, p, true, true, true);
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
      else if (key == shortcuts[SHRT_EDIT_TRACK_NAME].key) {
            emit editTrackNameSig();
            return;
      }
      else if (key == shortcuts[SHRT_MUTE_CURRENT_TRACKS].key) {
            emit muteSelectedTracks();
            return;
      }
      else if (key == shortcuts[SHRT_SOLO_CURRENT_TRACKS].key) {
            emit soloSelectedTracks();
            return;
      }
      if (key ==  shortcuts[SHRT_MIXER_STRIP_VOL_UP].key)
      {
          emit volumeSelectedTracks(1);
          return;
      }
      else if (key ==  shortcuts[SHRT_MIXER_STRIP_VOL_DOWN].key)
      {
          emit volumeSelectedTracks(-1);
          return;
      }
      if (key ==  shortcuts[SHRT_MIXER_STRIP_VOL_UP_PAGE].key)
      {
          emit volumeSelectedTracks(5);
          return;
      }
      else if (key ==  shortcuts[SHRT_MIXER_STRIP_VOL_DOWN_PAGE].key)
      {
          emit volumeSelectedTracks(-5);
          return;
      }
      else if (key ==  shortcuts[SHRT_MIXER_STRIP_PAN_LEFT].key)
      {
          emit panSelectedTracks(-1);
          return;
      }
      else if (key ==  shortcuts[SHRT_MIXER_STRIP_PAN_RIGHT].key)
      {
          emit panSelectedTracks(1);
          return;
      }
      else if (key ==  shortcuts[SHRT_MIXER_STRIP_PAN_LEFT_PAGE].key)
      {
          emit panSelectedTracks(-5);
          return;
      }
      else if (key ==  shortcuts[SHRT_MIXER_STRIP_PAN_RIGHT_PAGE].key)
      {
          emit panSelectedTracks(5);
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

            if(leftmost && rightmost)
            {
              int left_tick = leftmost->part()->tick();
              int right_tick = rightmost->part()->tick() + rightmost->part()->lenTick();
              MusECore::Pos p1(left_tick, true);
              MusECore::Pos p2(right_tick, true);
              MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
              MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
            }
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
            updateSelectedItem(newItem, add, singleSelection);
            return;
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
            updateSelectedItem(newItem, add, singleSelection);
            return;
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
            updateSelectedItem(newItem, add, singleSelection);
            return;
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
            updateSelectedItem(newItem, add, singleSelection);
            return;
      }
      else if (key == shortcuts[SHRT_RENAME_PART].key && curItem) {
        if (singleSelection) {
          renameItem(curItem);
        }
        return;
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
            return;
      }
      else {
            event->ignore();  // give global accelerators a chance
            return;
      }

      // if no return has caught the event we ignore it, we should never get here!
      fprintf(stderr, "End of PartCanvas::keyPress - we should never get here!\n");
      event->ignore();
}

void PartCanvas::updateSelectedItem(CItem* newItem, bool add, bool singleSelection)
{
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
//   keyRelease
//---------------------------------------------------------

void PartCanvas::keyRelease(QKeyEvent* event)
{
      const int key = event->key();
      
      // We do not want auto-repeat events.
      // It does press and release repeatedly. Wait till the last release comes.
      if(!event->isAutoRepeat())
      {
// For testing...
//         fprintf(stderr, "PartCanvas::keyRelease not isAutoRepeat\n");
      
        //event->accept();
      
        // Select part to the right
        if(key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key ||
        // Select part to the left
          key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key ||
        // Select nearest part on track above
          key == shortcuts[SHRT_SEL_ABOVE].key || key == shortcuts[SHRT_SEL_ABOVE_ADD].key ||
        // Select nearest part on track below
          key == shortcuts[SHRT_SEL_BELOW].key || key == shortcuts[SHRT_SEL_BELOW_ADD].key)
        {
            itemSelectionsChanged();
        }
        return;
      }
      
  Canvas::keyRelease(event);
}

//---------------------------------------------------------
//   drawItem
//    draws an item
//---------------------------------------------------------

void PartCanvas::drawItem(QPainter& p, const CItem* item, const QRect& mr, const QRegion&)
      {
      const QRect ur = mapDev(mr);
      
      int vfrom   = ur.x();
      int vto     = vfrom + ur.width();

      MusECore::Part* part = ((NPart*)item)->part();

      int pTick  = part->tick();
      vfrom      -= pTick;
      vto        -= pTick;
      if(vfrom < 0)
        vfrom = 0;
      if((unsigned int)vto > part->lenTick())
        vto = part->lenTick();

      const int uw1 = rmapxDev(1);
      const int uh1 = rmapyDev(1);
      const int uh2 = rmapyDev(2);
      
      QBrush brush;

      // This is the actual item drawing rectangle, ignoring any borders we place on it.
      QRect ubbr    = item->bbox();
      // Compensation required for our two-pixel wide border. This is due to how Qt draws borders with width > 1.
      // For example if our top is at y=10, Qt will draw the top border starting at y=9.
      // Thus we need to make our top y=11 so that it draws starting at y=10. Same with bottom. Just move rectangle down.
      // The left and right edges do NOT need adjustment here. // FIXME Prefer to do this after the map, but vbbr is needed below.
      ubbr.moveTop(ubbr.y() + uh1);
      
      //QRect rr = p.transform().mapRect(r);    // Gives inconsistent positions. Source shows wrong operation for our needs.
      QRect mbbr = map(ubbr);                        // Use our own map instead.

      // This is the update comparison rectangle. This would normally be the same as the item's bounding rectangle
      //  but in this case we have a two-pixel wide border. To accommodate for our border, expand the left edge
      //  left by one, the right edge right by one, and the bottom edge down by TWO. This way we catch the full
      //  necessary drawing rectangle when checking the requested update rectangle.
      QRect ubbr_exp = item->bbox().adjusted(rmapxDev(-1), 0, uw1, uh2);
      

// For testing...
//       fprintf(stderr, "PartCanvas::drawItem: vr x:%d y:%d w:%d h:%d vbbr_exp x:%d y:%d w:%d h:%d"
//                        " mr x:%d y:%d w:%d h:%d mbbr x:%d y:%d w:%d h:%d vfrom:%d vto:%d\n",
//               vr.x(), vr.y(), vr.width(), vr.height(),
//               vbbr_exp.x(), vbbr_exp.y(), vbbr_exp.width(), vbbr_exp.height(),
//               mr.x(), mr.y(), mr.width(), mr.height(),
//               mbbr.x(), mbbr.y(), mbbr.width(), mbbr.height(), 
//               vfrom, vto);
      
      // Now check intersection of the expanded comparison rectangle and the requested update rectangle.
      // Item bounding box x is in tick coordinates, same as rectangle.
      if((ubbr_exp & ur).isEmpty())
      {
// For testing...
//         fprintf(stderr, "...vbbr & vr is empty. Returning.\n");
        return;
      }

      const bool item_selected = item->isSelected();
      p.setWorldMatrixEnabled(false);

      // NOTE: Optimization: For each item, hasHiddenEvents() is called once in Canvas::draw(), and we use cachedHasHiddenEvents().
      // Not used for now.
      //int het = part->cachedHasHiddenEvents(); DELETETHIS or FIXME or whatever?
      int het = part->hasHiddenEvents();

      int xs_0 = mbbr.x();
      int xe_0 = xs_0 + mbbr.width();
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

      int ys_0 = mbbr.y();
      int ye_0 = ys_0 + mbbr.height();
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
            QLinearGradient gradient(mbbr.topLeft(), mbbr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else
      if (item_selected)
      {
          QColor c(Qt::black);
          c.setAlpha(MusEGlobal::config.globalAlphaBlend);
          QLinearGradient gradient(mbbr.topLeft(), mbbr.bottomLeft());
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
            QLinearGradient gradient(mbbr.topLeft(), mbbr.bottomLeft());
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, c.darker());
            brush = QBrush(gradient);
      }
      else
      {
            QColor c(MusEGlobal::config.partColors[cidx]);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            brush = QBrush(MusECore::gGradientFromQColor(c, mbbr.topLeft(), mbbr.bottomLeft()));
      }

      int h = mbbr.height();
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
        if ((brightness >= 12000 && !item_selected))
          color_brightness=96; //0;    // too light: use dark color
        else
          color_brightness=180; //255;   // too dark: use lighter color
        QColor c(color_brightness,color_brightness,color_brightness, MusEGlobal::config.globalAlphaBlend);
        p.setBrush(QBrush(MusECore::gGradientFromQColor(c, mbbr.topLeft(), mbbr.bottomLeft())));
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
        p.fillRect(mbbr & mr, brush);  // Respect the requested drawing rectangle. Gives speed boost!
      }

      // Draw a pattern brush on muted parts...
      if(part->mute())
      {
        p.setPen(Qt::NoPen);
        brush.setStyle(Qt::Dense7Pattern);

        p.fillRect(mbbr & mr, brush);   // Respect the requested drawing rectangle. Gives speed boost!
      }

      p.setWorldMatrixEnabled(true);

      MusECore::Track::TrackType type = part->track()->type();
      if (type == MusECore::Track::WAVE) {
        MusECore::WavePart* wp =(MusECore::WavePart*)part;
        drawWavePart(p, ur, wp, ubbr);
      } else {
        MusECore::MidiPart* mp = (MusECore::MidiPart*)part;
        drawMidiPart(p, ur, mp, ubbr, vfrom, vto, item_selected);
      }

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
          p.setPen(item_selected ? penSelect1V : penNormal1V);
        else
          p.setPen(item_selected ? penSelect2V : penNormal2V);

        if(rbx >= mrxs_0 && rbx <= mrxe_0)  // Respect the requested drawing rectangle. Gives speed boost!
        {
          QLine l2(rbx, ys_0, rbx, ye_0);            // Right
          p.drawLine(l2);        // Right line
        }

        if(((NPart*)item)->leftBorderTouches)
          p.setPen(item_selected ? penSelect1V : penNormal1V);
        else
          p.setPen(item_selected ? penSelect2V : penNormal2V);

        if(xs_0 >= mrxs_0 && xs_0 <= mrxe_0)
        {
          QLine l4(xs_0, ys_0, xs_0, ye_0);            // Left
          p.drawLine(l4);        //  Left line
        }

        p.setPen(item_selected ? penSelect2H : penNormal2H);

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
            QRect tr = mbbr;
            tr.setX(tr.x() + 3);
            MusECore::gGradientFromQColor(MusEGlobal::config.partColors[cidx], tr.topLeft(), tr.bottomLeft()).stops().last().second.getRgb(&part_r, &part_g, &part_b);
            brightness =  part_r*29 + part_g*59 + part_b*12;
            bool rev = brightness >= 12000 && !item_selected;
            p.setFont(MusEGlobal::config.fonts[2]);
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

void PartCanvas::drawMoving(QPainter& p, const CItem* item, const QRect&, const QRegion&)
      {
        QPen pen;
        pen.setCosmetic(true);
        pen.setColor( Qt::black);
        p.setPen(pen);
        
        MusECore::Part* part = ((NPart*)item)->part();
        QColor c(part->mute() ? Qt::white : MusEGlobal::config.partColors[part->colorIndex()]);
        c.setAlpha(128);  // Fix this regardless of config.globalAlphaBlend setting. Should be OK.
        p.setBrush(c);
        MusECore::TrackList* tl = MusEGlobal::song->tracks();
        int yy  = 0;
        int y = item->mp().y();
        int ih = item->height();
        MusECore::ciTrack it;
        for(it = tl->begin(); it != tl->end(); ++it)
        {
          int h = (*it)->height();
          if(y < yy+h)
          {
            ih = h;
            break;
          }
          yy += h;
        }
        if(it == tl->end())
          ih = MusEGlobal::config.trackHeight;
        p.drawRect(item->mp().x(), item->mp().y(), item->width(), ih);
      }

//---------------------------------------------------------
//   drawMidiPart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void PartCanvas::drawMidiPart(QPainter& p, const QRect& rect, MusECore::MidiPart* midipart,
                              const QRect& r, int from, int to, bool selected)
{
    drawMidiPart(p, rect, midipart->events(), midipart->track(), midipart, r, midipart->tick(), from, to, selected);
}

void PartCanvas::drawMidiPart(QPainter& p, const QRect&, const MusECore::EventList& events,
                              MusECore::MidiTrack *mt, MusECore::MidiPart* pt, const QRect& r,
                              int pTick, int from, int to, bool selected)
{
  int color_brightness;
  QColor eventColor;
  QPen pen;
  pen.setCosmetic(true);

  if(pt)
  {
    int part_r, part_g, part_b, brightness;
    MusEGlobal::config.partColors[pt->colorIndex()].getRgb(&part_r, &part_g, &part_b);
    brightness =  part_r*29 + part_g*59 + part_b*12;
    if (brightness >= 12000 && !selected) {
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
            pen.setColor(eventColor);
            p.setPen(pen);
            
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
      bool isdrum = mt->isDrumTrack();

      // draw controllers ------------------------------------------
      pen.setColor(QColor(192,192,color_brightness/2));
      p.setPen(pen);
            
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

      pen.setColor(QColor(192,color_brightness/2,color_brightness/2));
      p.setPen(pen);
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

      pen.setColor(QColor(color_brightness/2,192,color_brightness/2));
      p.setPen(pen);
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

      pen.setColor(QColor(0,0,255));
      p.setPen(pen);
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
          for (int cnt=0;cnt<128;cnt++)
            y_mapper[cnt]=cnt;

        if (MusEGlobal::heavyDebugMsg) printf("DEBUG: arranger: cakewalk enabled, y-stretch disabled\n");
      }

      pen.setColor(eventColor);
      p.setPen(pen);
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

void PartCanvas::drawWaveSndFile(QPainter &p, MusECore::SndFileR &f, int samplePos, unsigned rootFrame, unsigned startFrame, unsigned lengthFrames, int startY, int startX, int endX, int rectHeight)
{
   int h = rectHeight >> 1;
   int x1 = startX;
   int x2 = endX;
   if (f.isNull())
         return;
   unsigned channels = f.channels();
   if (channels == 0) {
         printf("drawWavePart: channels==0! %s\n", f.name().toLatin1().constData());
         return;
         }

   int xScale;
   int pos;
   int tickstep = rmapxDev(1);
   int postick = MusEGlobal::tempomap.frame2tick(rootFrame + startFrame);
   int eventx = mapx(postick);
   int drawoffset;
   if((x1 - eventx) < 0) {
     drawoffset = 0;
   }
   else {
     drawoffset = rmapxDev(x1 - eventx);
   }
   postick += drawoffset;
   pos = MusEGlobal::tempomap.tick2frame(postick) - rootFrame - startFrame;

   QPen pen;
   pen.setCosmetic(true);
   
   int i;
   if(x1 < eventx)
     i = eventx;
   else
     i = x1;
   int ex = mapx(MusEGlobal::tempomap.frame2tick(rootFrame + startFrame + lengthFrames));
   if(ex > x2)
     ex = x2;
   bool isfirst = true;
   const sf_count_t smps = f.samples();
   if (h < 20) {
         //    combine multi channels into one waveform
         int y = startY + h;
         int cc = rectHeight % 2 ? 0 : 1;
         for (; i < ex; i++) {
               MusECore::SampleV sa[channels];
               xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
               if((samplePos + f.convertPosition(pos)) > smps)
                 break;
               // Seek the file only once, not with every read!
               if(isfirst)
               {
                 isfirst = false;
                 if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, samplePos) == -1)
                   break;
               }
               f.readConverted(sa, xScale, pos, samplePos, true, false);

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
               peak = (peak * (rectHeight-2)) >> 9;
               rms  = (rms  * (rectHeight-2)) >> 9;
               int outer = peak;
               int inner = peak -1; //-1 < 0 ? 0 : peak -1;
               pen.setColor(MusEGlobal::config.partWaveColorPeak);
               p.setPen(pen);
               p.drawLine(i, y - outer - cc, i, y + outer);
               pen.setColor(MusEGlobal::config.partWaveColorRms);
               p.setPen(pen);
               if (MusEGlobal::config.waveDrawing == MusEGlobal::WaveRmsPeak)
                 p.drawLine(i, y - rms - cc, i, y + rms);
               else // WaveOutLine
                 p.drawLine(i, y - inner - cc, i, y + inner);
               }
         }
   else {
         //  multi channel display
         int hm = rectHeight / (channels * 2);
         int cc = rectHeight % (channels * 2) ? 0 : 1;
         for (; i < ex; i++) {
               int y  = startY + hm;
               MusECore::SampleV sa[channels];
               xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
               if((samplePos + f.convertPosition(pos)) > smps)
                 break;
               // Seek the file only once, not with every read!
               if(isfirst)
               {
                 isfirst = false;
                 if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, samplePos) == -1)
                   break;
               }
               f.readConverted(sa, xScale, pos, samplePos, true, false);

               postick += tickstep;
               pos += xScale;
               for (unsigned k = 0; k < channels; ++k) {
                     int peak = (sa[k].peak * (hm - 1)) >> 8;
                     int rms  = (sa[k].rms  * (hm - 1)) >> 8;
                     int outer = peak;
                     int inner = peak -1; //-1 < 0 ? 0 : peak -1;
                     pen.setColor(MusEGlobal::config.partWaveColorPeak);
                     p.setPen(pen);
                     p.drawLine(i, y - outer - cc , i, y + outer);
                     pen.setColor(MusEGlobal::config.partWaveColorRms);
                     p.setPen(pen);
                     if (MusEGlobal::config.waveDrawing == MusEGlobal::WaveRmsPeak)
                       p.drawLine(i, y - rms - cc, i, y + rms);
                     else // WaveOutLine
                       p.drawLine(i, y - inner - cc, i, y + inner);

                     y  += 2 * hm;
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
      //int h  = hh/2;
      int startY  = pr.y();

      for (auto reverseIterator = wp->events().rbegin(); reverseIterator != wp->events().rend(); ++reverseIterator)
      {

          MusECore::Event event = reverseIterator->second;
          auto f = event.sndFile();
          if (drag == DRAG_RESIZE && resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT && reverseIterator == wp->events().rbegin())
          {
              // the last event is subject to live extending

              // _pr is in tick resolution
              auto endFrame = MusEGlobal::tempomap.tick2frame(_pr.width());

              // we're at the last event, extend the wave drawing so it is displayed past the old end of the part
              drawWaveSndFile(p, f, event.spos(), wp->frame(), event.frame(), endFrame, startY, x1, x2, hh);
          }
          else if (drag == DRAG_RESIZE && resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
          {
//              auto startFrame = MusEGlobal::tempomap.tick2frame(_pr.)
              drawWaveSndFile(p, f, event.spos(), wp->frame(), event.frame(), event.lenFrame(), startY, x1, x2, hh);
          }
          else
          {
              drawWaveSndFile(p, f, event.spos(), wp->frame(), event.frame(), event.lenFrame(), startY, x1, x2, hh);
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
                  unsigned temp_begin = MusEGlobal::sigmap.raster1(MusEGlobal::song->vcpos(),0);
                  unsigned temp_end = MusEGlobal::sigmap.raster2(temp_begin + MusECore::get_paste_len(), 0);
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
                  unsigned int startPos=MusEGlobal::song->vcpos();
                  int oneMeas=MusEGlobal::sigmap.ticksMeasure(startPos);
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
      MusEGlobal::song->setPos(MusECore::Song::CPOS, p);
      QString mimeString = "text/x-muse-mixedpartlist";
      if (!midi)
          mimeString = "text/x-muse-wavepartlist";
      else if (!wave)
          mimeString = "text/x-muse-midipartlist";
      QMimeData *mimeData =  MusECore::file_to_mimedata(tmp, mimeString );
      QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
      fclose(tmp);
      }


MusECore::Undo PartCanvas::pasteAt(const QString& pt, MusECore::Track* track, unsigned int pos,
                                   bool clone, bool toTrack, unsigned int* finalPosPtr, set<MusECore::Track*>* affected_tracks)
      {
      MusECore::Undo operations;

      QByteArray ba = pt.toLatin1();
      const char* ptxt = ba.constData();
      MusECore::Xml xml(ptxt);
      bool firstPart=true;
      unsigned int  posOffset=0;
      bool fwdOffset = true;
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
                                    if(pos >= p->tick())
                                    {
                                      posOffset = pos - p->tick();
                                      fwdOffset = true;
                                    }
                                    else
                                    {
                                      posOffset = p->tick() - pos;
                                      fwdOffset = false;
                                    }
                                      
//                                     posOffset=pos-p->tick();
                                    }
                              if(fwdOffset)
                              {
                                p->setTick(p->tick()+posOffset);
                              }
                              else
                              {
                                if(p->tick() >= posOffset)
                                  p->setTick(p->tick()-posOffset);
                                else
                                  p->setTick(0);
                              }
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
        unsigned int endPos=0;
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
        MusEGlobal::song->setPos(MusECore::Song::CPOS, p);

        if (paste_mode != PASTEMODE_MIX)
        {
          unsigned int offset;
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

            int x = event->pos().x();
            if (x < 0)
                  x = 0;
            x = MusEGlobal::sigmap.raster(x, *_raster);
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

          int x = event->pos().x();
          if (x < 0)
                x = 0;
          x = MusEGlobal::sigmap.raster(x, *_raster);

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
                        track = MusEGlobal::song->addTrack(MusECore::Track::MIDI);    // Add at end of list.
                    } else {
                        track = MusEGlobal::song->addTrack(MusECore::Track::WAVE);    // Add at end of list.
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

void PartCanvas::drawCanvas(QPainter& p, const QRect& mr, const QRegion& mrg)
{
      p.save();
      p.setWorldMatrixEnabled(false);

      const ViewRect vr(mr, true);
      const ViewXCoordinate& vx = vr._x;
      const ViewWCoordinate& vw = vr._width;
      const ViewXCoordinate vx_2 = mathXCoordinates(vx, vw, MathAdd);
      
      int mx = mr.x();
      int my = mr.y();
      int mw = mr.width();
      int mh = mr.height();
      int mx_2 = mx + mw;
      int my_2 = my + mh;

      // Since the bounding box is open-ended (extends infintely to the right),
      //  form an end-point from screen coordinates.
      const ViewXCoordinate vbbx(0, false);
      const ViewXCoordinate vbbx_2(x() + width(), true);
      const ViewXCoordinate vdbbw(mathXCoordinates(vbbx_2, vbbx, MathSubtract));
      // Turn vdbbw into a width coordinate.
      const ViewWCoordinate vbbw(vdbbw._value, vdbbw.isMapped());
      const int mbbx = asIntMapped(vbbx);
      int mx0_lim = mbbx;
      if(mx0_lim < mx)
        mx0_lim = mx;
      if(mx0_lim < 0)
        mx0_lim = 0;

      //////////
      // GRID //
      //////////

      QPen pen;
      pen.setCosmetic(true);

      //--------------------------------
      // vertical lines
      //-------------------------------
      if (MusEGlobal::config.canvasShowGrid) {
        int rast = *_raster;
        if(rast == 0) // Special for arranger 'bar' snap.
        {
          rast = MusEGlobal::sigmap.ticks_beat(1);
        }
        
        drawTickRaster(p, mr, mrg, rast,
                         false, false, false,
                       MusEGlobal::config.partCanvasFineRasterColor,
                       MusEGlobal::config.partCanvasFineRasterColor,
                       MusEGlobal::config.partCanvasFineRasterColor,
                       MusEGlobal::config.partCanvasCoarseRasterColor);
      }

      //--------------------------------
      // horizontal lines
      //--------------------------------

      MusECore::TrackList* tl = MusEGlobal::song->tracks();
      int myy = -rmapy(yorg) - ypos;
      int mth;
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
        
            if (myy > my_2)
                  break;
            
            MusECore::Track* track = *it;
            mth = track->height();
            if(!mth)
              continue;
            
            const int myy_2 = myy + mth;
            
            const ViewYCoordinate vbby(myy, true);
            const ViewHCoordinate vbbh(mth, true);
            const ViewRect vbbox(vbbx, vbby, vbbw, vbbh);
            
// For testing...
//             vbbox.dump("PartCanvas::drawCanvas vbbox");
            
            if(MusEGlobal::config.canvasShowGrid && (track->isMidiTrack() || track->type() == MusECore::Track::WAVE))
            {
              if(compareXCoordinates(vx_2, vbbx, CompareGreaterEqual) && 
                (myy_2 >= my && myy_2 < my_2))
              {
// For testing...
//                 fprintf(stderr, "... bottom edge in range. Drawing bottom edge at mx0_lim:%d myy_2:%d mx_2:%d myy_2:%d\n",
//                         mx0_lim, myy_2, mx_2, myy_2);

                pen.setColor(MusEGlobal::config.partCanvasCoarseRasterColor);
                p.setPen(pen);
                p.drawLine(mx0_lim, myy_2, mx_2, myy_2);
              }
            }
            
            if (!track->isMidiTrack() && (track->type() != MusECore::Track::WAVE)) {
                  drawAudioTrack(p, mr, mrg, vbbox, (MusECore::AudioTrack*)track);
            }
            myy += mth;
      }

      p.restore();
}


//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------

void PartCanvas::drawTopItem(QPainter& p, const QRect& mr, const QRegion&)
{
    int mx = mr.x();
    int my = mr.y();
    int mw = mr.width();
    int mh = mr.height();

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
                  drawAutomationPoints(p, r, (MusECore::AudioTrack*)track);
                  drawAutomationText(p, r, (MusECore::AudioTrack*)track);
                }
          }
          yy += th;
          }

    unsigned int startPos = MusEGlobal::extSyncFlag ? MusEGlobal::audio->getStartExternalRecTick() : MusEGlobal::audio->getStartRecordPos().tick();
    if (MusEGlobal::song->punchin())
      startPos=MusEGlobal::song->lpos();
    int startx = mapx(startPos);
    if(startx < 0)
       startx = 0;
    int width = mapx(MusEGlobal::song->cpos()) - startx;



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
    // ^^ done
    int yPos = yoff;
    if (MusEGlobal::song->record() && MusEGlobal::audio->isPlaying()) {
      QPen pen(Qt::black, 0, Qt::SolidLine);
      pen.setCosmetic(true);
      for (MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it) {
        MusECore::Track* track = *it;
        th = track->height();
        if (!th)
          continue;
        if (track->recordFlag()) {
           p.setPen(pen);
           QColor c(MusEGlobal::config.partColors[0]);
           c.setAlpha(MusEGlobal::config.globalAlphaBlend);
           QLinearGradient gradient(QPoint(startx,yPos), QPoint(startx,yPos+th));
           gradient.setColorAt(0, c);
           gradient.setColorAt(1, c.darker());
           QBrush cc(gradient);
           p.setBrush(cc);

           //fprintf(stderr, "startx = %d\n", startx);
           p.drawRect(startx,yPos, width, th);

           if(track->type() == MusECore::Track::WAVE){
              if(MusEGlobal::config.liveWaveUpdate){
                 MusECore::SndFileR fp = ((MusECore::AudioTrack *)track)->recFile();
                 if(!fp.isNull()){
                    unsigned int _startFrame = MusEGlobal::tempomap.tick2frame(startPos);
                    unsigned int _endFrame = MusEGlobal::song->cPos().frame();
                    unsigned int _lengthFrame = _endFrame - _startFrame;
                    if(_startFrame <= _endFrame)
                    {
                       drawWaveSndFile(p, fp, 0, _startFrame, 0, _lengthFrame, yPos, startx, startx + width, th);
                    }
                 }
              }

           }
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
               drawMidiPart(p, mr, myEventList, mt, 0, partRect,startPos,0,MusEGlobal::song->cpos()-startPos, false);
           }
           yPos+=track->height();
      }
    }

}

//---------------------------------------------------------
//   drawAudioTrack
//---------------------------------------------------------

void PartCanvas::drawAudioTrack(QPainter& p, const QRect& mr, const QRegion& /*mrg*/,
                                const ViewRect& vbbox, MusECore::AudioTrack* /*t*/)
{
      const ViewRect vr(mr, true);
      const ViewXCoordinate& vx = vr._x;
      const ViewWCoordinate& vw = vr._width;
      const ViewXCoordinate vx_2 = mathXCoordinates(vx, vw, MathAdd);
      const ViewYCoordinate& vy = vr._y;
      const ViewHCoordinate& vh = vr._height;
      const ViewYCoordinate vy_2 = mathYCoordinates(vy, vh, MathAdd);
      
      // The gradient is to be on the inside of the border.
      const ViewRect vbb_gr = adjustedRect(
        vbbox,
        ViewWCoordinate(1, true),
        ViewHCoordinate(1, true),
        ViewWCoordinate(0, true),
        // Usually this would be -2 but here we want the bottom extended by 1.
        ViewHCoordinate(-1, true));
      
      const ViewRect vbr_gr = intersected(vr, vbb_gr);
      
      const QRect mbb    = asQRectMapped(vbbox);
      const QRect mbb_gr = asQRectMapped(vbb_gr);
      const QRect mbr_gr = asQRectMapped(vbr_gr);
      
      const int mx = mr.x();
      const int mw = mr.width();
      const int mx_2 = mx + mw;
      
      const ViewXCoordinate& vbbx = vbbox._x;
      const ViewYCoordinate& vbby = vbbox._y;
      const ViewHCoordinate& vbbh = vbbox._height;
      
      const ViewYCoordinate vbby_2 = mathYCoordinates(vbby, vbbh, MathAdd);
      
      const int mbbx = mbb.x();
      const int mbby = mbb.y();
      const int mbbh = mbb.height();
      
      const int mbby_2 = mbby + mbbh;
      
      QPen pen;
      pen.setCosmetic(true);
      pen.setColor(Qt::black);
      p.setPen(pen);
      
// For testing...
//       fprintf(stderr, "\nPartCanvas::drawAudioTrack: mbbox:\nx:%8d\t\ty:%8d\t\tw:%8d\t\th:%8d\n\n",
//               mbb.x(), mbb.y(), mbb.width(), mbb.height());
//       vbbox.dump("vbbox:");
//       vr.dump("vr:");
//       vbr.dump("vbr:");
//       vbr_gr.dump("vbr_gr:");

      if(!isViewRectEmpty(vbr_gr))
      {
// For testing...
//             fprintf(stderr, "\n...gradient in range. Drawing gradient at:\nmbr_gr.x:%8d\t\tmbr_gr.y:%8d\t\tmbr_gr.w:%8d\t\tmbr_gr.h:%8d\n\n",
//                     mbr_gr.x(), mbr_gr.y(), mbr_gr.width(), mbr_gr.height());
          
        QColor c(MusEGlobal::config.dummyPartColor);
        c.setAlpha(MusEGlobal::config.globalAlphaBlend);
        QLinearGradient gradient(mbb_gr.x(), mbb_gr.y(), mbb_gr.x(), mbb_gr.y() + mbb_gr.height());    // Inside the border
        gradient.setColorAt(0, c);
        gradient.setColorAt(1, c.darker());
        QBrush brush(gradient);
        p.fillRect(mbr_gr, brush);
      }

      int mx0_lim = mbbx;
      if(mx0_lim < mx)
        mx0_lim = mx;
      if(mx0_lim < 0)
        mx0_lim = 0;

      const ViewYCoordinate vbby_is = 
        compareYCoordinates(vy, vbby, CompareLess) ? vbby : vy;
      
      const ViewYCoordinate vbby_2is = 
        compareYCoordinates(vbby_2, vy_2, CompareLess) ? vbby_2 : vy_2;
      
      if(isXInRange(vbbx, vx, vx_2) &&
         compareYCoordinates(vbby_2is, vbby_is, CompareGreaterEqual))
      {
// For testing...
//         fprintf(stderr, "...left edge in range. Drawing left edge at mbbx:%d asMapped(vbby_is):%d mbbx:%d asMapped(vbby_2is):%d\n",
//                 mbbx, asMapped(vbby_is)._value, mbbx, asMapped(vbby_2is)._value);
        
        p.drawLine(mbbx, asMapped(vbby_is)._value, mbbx, asMapped(vbby_2is)._value); // The left edge
      }
      
      if(compareXCoordinates(vx_2, vbbx, CompareGreaterEqual))
      {
        if(isYInRange(vbby, vy, vy_2))
        {
// For testing...
//           fprintf(stderr, "...top edge in range. Drawing top edge at mx0_lim:%d mbby:%d mx_2:%d mbby:%d\n",
//                   mx0_lim, mbby, mx_2, mbby);
          
          p.drawLine(mx0_lim, mbby, mx_2, mbby); // The top edge
        }
        
        if(isYInRange(vbby_2, vy, vy_2))
        {
// For testing...
//           fprintf(stderr, "...bottom edge in range. Drawing bottom edge at mx0_lim:%d mbby_2:%d mx_2:%d mbby_2:%d\n",
//                   mx0_lim, mbby_2, mx_2, mbby_2);
          
          p.drawLine(mx0_lim, mbby_2, mx_2, mbby_2);    // The bottom edge. Special for Audio track - draw one past bottom.
        }
      }
}

//---------------------------------------------------------
//   drawAutomation
//---------------------------------------------------------

void PartCanvas::drawAutomation(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
    const int bottom = rr.bottom() - 2;
    const int height = bottom - rr.top() - 2; // limit height

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
      QColor line_color(cl->color());
      line_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
      QPen pen1(line_color);
      pen1.setCosmetic(true);
      QString txt;

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

            // Ideally we would like to cut the lines right at the rectangle boundaries.
            // But they might not be drawn exactly the same as the full line would.
            // So we'll also accept anything that started outside the boundaries.
            // A small acceptable speed hit relatively speaking - but far far better than drawing all.
            if(oldX <= rr.right() && xpixel >= rr.left() && oldY <= rr.bottom() && ypixel >= rr.top())
            {
              p.setPen(pen1);
              if(discrete)
              {
                p.drawLine(oldX, oldY, xpixel, oldY);
                p.drawLine(xpixel, oldY, xpixel, ypixel);
              }
              else
                p.drawLine(oldX, oldY, xpixel, ypixel);
            }

            if (xpixel > rr.right())
              break;

            oldX = xpixel;
            oldY = ypixel;
        }
      }

      if (xpixel <= rr.right())
      {
        p.setPen(pen1);
        p.drawLine(xpixel, ypixel, rr.right(), ypixel);
      }
    }
}

//---------------------------------------------------------
//   drawAutomationPoints
//---------------------------------------------------------

void PartCanvas::drawAutomationPoints(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
  const int bottom = rr.bottom() - 2;
  const int height = bottom - rr.top() - 2; // limit height
  const int mx0 = mapx(0);

  const int pw2  = _automationPointWidthUnsel / 2;
  const int pws2 = _automationPointWidthSel / 2;
  MusECore::CtrlListList* cll = t->controller();

  // Draw unselected vertices first.
  for(MusECore::ciCtrlList icll = cll->begin(); icll != cll->end(); ++icll)
  {
    MusECore::CtrlList *cl = icll->second;
    if(cl->dontShow() || !cl->isVisible())
      continue;
    if(rr.right() < mx0)
    {
      //p.restore();
      return;
    }

    double min, max;
    cl->range(&min,&max);
    const QColor line_col(cl->color());
    const QColor vtx_col1(line_col.red() ^ 255, line_col.green() ^ 255, line_col.blue() ^ 255);
    QColor vtx_col2(cl->color());
    vtx_col2.setAlpha(160);
    // If we happen to be using 1 pixel, use an inverted colour. Else use the line colour but slightly transparent.
    const QColor& vtx_col = (_automationPointWidthUnsel == 1) ? vtx_col1 : vtx_col2;
    QPen pen(vtx_col);
    pen.setCosmetic(true);
    p.setPen(pen);

    for(MusECore::ciCtrl ic = cl->begin(); ic != cl->end(); ++ic)
    {
      const int frame = ic->second.frame;
      if(automation.currentCtrlValid && automation.currentCtrlList == cl && automation.currentCtrlFrameList.contains(frame))
        continue;
      const int xpixel = mapx(MusEGlobal::tempomap.frame2tick(frame));
      if(xpixel > rr.right())
        break;

      double y = ic->second.val;
      if (cl->valueType() == MusECore::VAL_LOG ) {
        y = logToVal(y, min, max); // represent volume between 0 and 1
        if(y < 0) y = 0.0;
      }
      else
        y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

      const int ypixel = bottom - rmapy_f(y) * height;
      if(((xpixel + pw2 >= rr.left()) && (xpixel - pw2 <= rr.right())) &&
         ((ypixel + pw2 >= rr.top())  && (ypixel - pw2 <= rr.bottom())))
        //p.fillRect(xpixel - pw2, ypixel - pw2, _automationPointWidthUnsel, _automationPointWidthUnsel, vtx_col);
        // For some reason this is drawing one extra pixel width and height. ???
        p.drawRect(xpixel - pw2, ypixel - pw2, _automationPointWidthUnsel, _automationPointWidthUnsel);
    }
  }

  // Now draw selected vertices, so that they always appear on top.
  for(MusECore::ciCtrlList icll = cll->begin(); icll != cll->end(); ++icll)
  {
    MusECore::CtrlList *cl = icll->second;
    if(cl->dontShow() || !cl->isVisible())
      continue;
    if(rr.right() < mx0)
    {
      //p.restore();
      return;
    }

    double min, max;
    cl->range(&min,&max);
    const QColor line_col(cl->color());
    const QColor vtx_col(line_col.red() ^ 255, line_col.green() ^ 255, line_col.blue() ^ 255);

    for(MusECore::ciCtrl ic = cl->begin(); ic != cl->end(); ++ic)
    {
      const int frame = ic->second.frame;
      if(!automation.currentCtrlValid || automation.currentCtrlList != cl || !automation.currentCtrlFrameList.contains(frame))
        continue;
      const int xpixel = mapx(MusEGlobal::tempomap.frame2tick(frame));
      if(xpixel > rr.right())
        break;

      double y = ic->second.val;
      if (cl->valueType() == MusECore::VAL_LOG ) {
        y = logToVal(y, min, max); // represent volume between 0 and 1
        if (y < 0) y = 0.0;
      }
      else
        y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

      const int ypixel = bottom - rmapy_f(y) * height;
      if(((xpixel + pws2 >= rr.left()) && (xpixel - pws2 <= rr.right())) &&
         ((ypixel + pws2 >= rr.top())  && (ypixel - pws2 <= rr.bottom())))
        p.fillRect(xpixel - pws2, ypixel - pws2, _automationPointWidthSel, _automationPointWidthSel, Qt::white);
    }
  }
}

//---------------------------------------------------------
//   drawAutomationText
//---------------------------------------------------------

void PartCanvas::drawAutomationText(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
    const int bottom = rr.bottom() - 2;
    const int height = bottom - rr.top() - 2; // limit height

    p.setBrush(Qt::NoBrush);
    p.setFont(font());

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

      int xpixel = 0;
      int ypixel = 0;
      double min, max;
      cl->range(&min,&max);
      QPen pen1(cl->color());
      pen1.setCosmetic(true);
      const QColor line_col = cl->color();
      QColor txt_bk((line_col.red() + line_col.green() + line_col.blue()) / 3 >= 128 ? Qt::black : Qt::white);
      txt_bk.setAlpha(150);
      QString txt;

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
          yfirst = bottom - rmapy_f(yfirst) * height;
      }

      p.setPen(pen1);

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

          if (xpixel > rr.right())
            break;

          if(xpixel + 20 <= rr.right() && ypixel <= rr.bottom())
          //if(!automation.currentTextRect.isNull() &&
          //   automation.currentTextRect.left() <= rr.right() &&
          //   automation.currentTextRect.top() <= rr.bottom())
          {
            if (automation.currentCtrlValid && automation.currentCtrlList == cl &&
                automation.currentCtrlFrameList.contains(ic->second.frame) &&
                automation.currentCtrlFrameList.size() == 1) {
                    QRect textRect = p.fontMetrics().boundingRect(automation.currentText).adjusted(-4, -2, 4, 2);
                    textRect.moveLeft(xpixel + 20);
                    textRect.moveTop(ypixel);
                    if(textRect.right() >= rr.left() && textRect.bottom() >= rr.top())
                    {
                      p.fillRect(textRect, txt_bk);
                      p.drawText(textRect, Qt::AlignCenter, automation.currentText);
                    }
            }
          }
      }

      //const int xTextPos = mapx(0) > rmapx(0) ? mapx(0) + 5 : rmapx(0) + 5; // follow window..(doesn't work very well)
      const int xTextPos = mapx(0) + 5;
      if(xTextPos <= rr.right() && yfirst <= rr.bottom())
      {
        QRect textRect = fontMetrics().boundingRect(cl->name());
        textRect.moveLeft(xTextPos);
        textRect.moveTop(yfirst);
        if(textRect.right() >= rr.left() && textRect.bottom() >= rr.top())
          p.drawText(textRect, cl->name());
      }
    }
}

//---------------------------------------------------------
// distanceSqToSegment
// Returns the distance, squared, of a point to a line segment.
//---------------------------------------------------------

int distanceSqToSegment(double pointX, double pointY, double x1, double y1, double x2, double y2)
{
    double diffX = x2 - x1;
    double diffY = y2 - y1;

    if((diffX == 0) && (diffY == 0))
    {
      diffX = pointX - x1;
      diffY = pointY - y1;
      return diffX * diffX + diffY * diffY;
    }

    const double t = ((pointX - x1) * diffX + (pointY - y1) * diffY) / (diffX * diffX + diffY * diffY);

    if (t < 0.0)
    {
        //point is nearest to the first point i.e x1 and y1
        diffX = pointX - x1;
        diffY = pointY - y1;
    }
    else if (t > 1.0)
    {
        //point is nearest to the end point i.e x2 and y2
        diffX = pointX - x2;
        diffY = pointY - y2;
    }
    else
    {
        //if perpendicular line intersect the line segment.
        diffX = pointX - (x1 + t * diffX);
        diffY = pointY - (y1 + t * diffY);
    }

    return diffX * diffX + diffY * diffY;
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
  const int trackY = t->y();
  const int trackH = t->height();

  { int y = pointer.y();
    if(y < trackY || y >= (trackY + trackH))
      return;
    mouseY =  mapy(y);  }

  const int mouseX =  mapx(pointer.x());

  int closest_point_radius2 = PartCanvas::_automationPointDetectDist * PartCanvas::_automationPointDetectDist;
  int closest_point_frame = 0;
  double closest_point_value = 0.0;
  //int closest_point_x = 0;
  //int closest_point_y = 0;
  MusECore::CtrlList* closest_point_cl = NULL;

  int closest_line_dist2 = PartCanvas::_automationPointDetectDist * PartCanvas::_automationPointDetectDist;
  MusECore::CtrlList* closest_line_cl = NULL;

  MusECore::CtrlListList* cll = ((MusECore::AudioTrack*) t)->controller();
  for(MusECore::ciCtrlList icll = cll->begin(); icll != cll->end(); ++icll)
  {
    MusECore::CtrlList *cl = icll->second;
    if(cl->dontShow() || !cl->isVisible())
      continue;
    MusECore::ciCtrl ic=cl->begin();

    int eventOldX = mapx(0);
    int eventX = eventOldX;
    int eventOldY = -1;
    int eventY = eventOldY;
    double min, max;
    cl->range(&min,&max);
    bool discrete = cl->mode() == MusECore::CtrlList::DISCRETE;

    // First check that there IS automation for this controller, ic == cl->end means no automation
    if(ic == cl->end())
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
      for (; ic!=cl->end(); ++ic)
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

        if(pointer.x() > 0 && pointer.y() > 0)
        {
          const int dx = mouseX - eventX;
          const int dy = mouseY - eventY;
          const int r2 = dx * dx + dy * dy;
          if(r2 < closest_point_radius2)
          {
            closest_point_radius2 = r2;
            closest_point_frame = ic->second.frame;
            closest_point_value = ic->second.val;
            //closest_point_x = eventX;
            //closest_point_y = eventY;
            closest_point_cl = cl;
          }
        }

        const int ldist2 = distanceSqToSegment(mouseX, mouseY, eventOldX, eventOldY, eventX, discrete ? eventOldY : eventY);
        if(ldist2 < closest_line_dist2)
        {
          closest_line_dist2 = ldist2;
          closest_line_cl = cl;
        }

        eventOldX = eventX;
        eventOldY = eventY;
      }
    }

    if(mouseX >= eventX)
    {
      const int d2 = (mouseY-eventY) * (mouseY-eventY);
      if(d2 < closest_line_dist2)
      {
        closest_line_dist2 = d2;
        closest_line_cl = cl;
      }
    }
  }

  // Is the mouse close to a vertex? Since we currently don't use the addNewCtrl accel key, vertices take priority over lines.
  if(closest_point_cl)
  {
    QWidget::setCursor(Qt::PointingHandCursor);
    automation.currentCtrlFrameList.clear();
    automation.currentCtrlFrameList.append(closest_point_frame);
    automation.currentCtrlValid = true;
    automation.controllerState = movingController;
    automation.currentCtrlList = closest_point_cl;
    automation.currentTrack = t;

    // Store the text.
    if(closest_point_cl->valueType() == MusECore::VAL_LOG)
      //closest_point_value = MusECore::fast_log10(closest_point_value) * 20.0;
      closest_point_value = muse_val2dbr(closest_point_value); // Here we can use the slower but accurate function.
    automation.currentText = QString("Param:%1 Value:%2").arg(closest_point_cl->name()).arg(closest_point_value);

// FIXME These are attempts to update only the necessary rectangles. No time for it ATM, not much choice but to do full update.
#if 0
    // Be sure to erase (redraw) the old rectangles.
    int erase_ypixel = bottom - rmapy_f(y) * height;
    int erase_xpixel = mapx(MusEGlobal::tempomap.frame2tick(ic->second.frame));
    if(!automation.currentTextRect.isNull())
      update(automation.currentTextRect);
    if(!automation.currentVertexRect.isNull())
      update(automation.currentVertexRect);
    // Store the text and its rectangle.
    double value = closest_point_value;
    double min, max;
    closest_point_cl->range(&min,&max);
    if(closest_point_cl->valueType() == MusECore::VAL_LOG)
      //closest_point_value = MusECore::fast_log10(closest_point_value) * 20.0;
      value = muse_val2dbr(value); // Here we can use the slower but accurate function.
    automation.currentText = QString("Param:%1 Frame:%2 Value:%3").arg(closest_point_cl->name()).arg(closest_point_frame).arg(value);
//     automation.currentTextRect = fontMetrics().boundingRect(automation.currentText).adjusted(-4, -2, 4, 2);
    automation.currentTextRect = fontMetrics().boundingRect(automation.currentText).adjusted(0, 0, 4, 4);
//     automation.currentTextRect.moveLeft(closest_point_x + 20);
//     automation.currentTextRect.moveTop(closest_point_y);
//     if (inLog < min) inLog = min; // Should not happen
//     if (inLog > max) inLog = max;
//     double linMin = 20.0*MusECore::fast_log10(min);
//     double linMax = 20.0*MusECore::fast_log10(max);
//     double linVal = 20.0*MusECore::fast_log10(inLog);
    const double linMin = muse_val2dbr(min);
    const double linMax = muse_val2dbr(max);
    //const double n_value = (value - linMin) / (linMax - linMin); // Normalize
    automation.currentTick = MusEGlobal::tempomap.frame2tick(closest_point_frame);
    automation.currentYNorm = (value - linMin) / (linMax - linMin); // Normalize
    // Store the selected vertex rectangle. Use the selected size, which can be different than the unselected size.
    automation.currentVertexRect = QRect(closest_point_x - PartCanvas::_automationPointWidthSel / 2,
                                         closest_point_y - PartCanvas::_automationPointWidthSel / 2,
                                         PartCanvas::_automationPointWidthSel,
                                         PartCanvas::_automationPointWidthSel);
    // Now fill the text's new rectangle.
    update(automation.currentTextRect);
    // And fill the vertex's new rectangle.
    update(automation.currentVertexRect);
#else
    //update();
    controllerChanged(automation.currentTrack, automation.currentCtrlList->id());
#endif
    return;
  }

  // Is the mouse close to a line?
  if(closest_line_cl)
  {
    QWidget::setCursor(Qt::CrossCursor);
    automation.currentCtrlValid = false;
    automation.controllerState = addNewController;
    automation.currentCtrlList = closest_line_cl;
    automation.currentTrack = t;
#if 0
    // Be sure to erase (refill) the old rectangles.
    if(!automation.currentTextRect.isNull())
      update(automation.currentTextRect);
    if(!automation.currentVertexRect.isNull())
      update(automation.currentVertexRect);
#else
    //update();
    controllerChanged(automation.currentTrack, automation.currentCtrlList->id());
#endif
    return;
  }

  if(automation.currentCtrlValid && automation.currentTrack && automation.currentCtrlList)
    controllerChanged(automation.currentTrack, automation.currentCtrlList->id());

  // if there are no hits we default to clearing all the data
  automation.controllerState = doNothing;
  automation.currentCtrlValid = false;
  automation.currentCtrlList = 0;
  automation.currentTrack = 0;
  automation.currentCtrlFrameList.clear();
#if 0
  // Be sure to erase (refill) the old rectangles.
  if(!automation.currentTextRect.isNull())
    update(automation.currentTextRect);
  automation.currentTextRect = QRect();
  if(!automation.currentVertexRect.isNull())
    update(automation.currentVertexRect);
  automation.currentVertexRect = QRect();
#else
  //update();
#endif
  setCursor();
}

void PartCanvas::controllerChanged(MusECore::Track* t, int)
{
  redraw((QRect(0, mapy(t->y()), width(), rmapy(t->height()))));  // TODO Check this - correct?
}

void PartCanvas::processAutomationMovements(QPoint inPos, bool slowMotion)
{

  if (_tool != AutomationTool)
    return;

  if (!automation.moveController) { // currently nothing going lets's check for some action.
      MusECore::Track * t = y2Track(inPos.y());
      if (t) {
          checkAutomation(t, inPos, false);
      }
      automation.startMovePoint = inPos;
      return;
  }

  if(automation.controllerState != movingController)
  {
    automation.startMovePoint = inPos;
    return;
  }

  Undo operations;

  int deltaX = inPos.x() - automation.startMovePoint.x();
  int deltaY = inPos.y() - automation.startMovePoint.y();
  if (slowMotion)
  {
    deltaX /= 3;
    deltaY /= 3;
  }
  const QPoint pos(automation.startMovePoint.x() + deltaX, automation.startMovePoint.y() + deltaY);

  const int posy=mapy(pos.y());
  const int tracky = mapy(automation.currentTrack->y());
  const int trackHeight = automation.currentTrack->height();

  const int mouseY = trackHeight - (posy - tracky)-2;
  const double yfraction = ((double)mouseY)/automation.currentTrack->height();

  double min, max;
  automation.currentCtrlList->range(&min,&max);
  double cvval;
  if (automation.currentCtrlList->valueType() == MusECore::VAL_LOG  ) { // use db scale for volume
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

  // Store the text.
  automation.currentText = QString("Param:%1 Value:%2").arg(automation.currentCtrlList->name()).arg(cvval);

  const int fl_sz = automation.currentCtrlFrameList.size();
  for(int i = 0; i < fl_sz; ++i)
  {
    const int old_frame = automation.currentCtrlFrameList.at(i);
    const int old_tick = MusEGlobal::tempomap.frame2tick(old_frame);
    const int new_tick = old_tick + deltaX;
    const int delta_frame = MusEGlobal::tempomap.deltaTick2frame(old_tick, new_tick);

    MusECore::ciCtrl iold = automation.currentCtrlList->find(old_frame);
    if(iold != automation.currentCtrlList->end())
    {
      const double old_value = iold->second.val;

      // The minimum frame that this selected frame can be moved to is the previous
      //  UNSELECTED vertex frame, PLUS the number of items from here to that vertex...
      int min_prev_frame = 0;
      MusECore::ciCtrl iprev = iold;
      int prev_frame_offset = 0;
      while(iprev != automation.currentCtrlList->begin())
      {
        --iprev;
        ++prev_frame_offset;
        // Stop when we find the previous unselected frame.
        if(!automation.currentCtrlFrameList.contains(iprev->second.frame))
        {
          min_prev_frame = iprev->second.frame + prev_frame_offset;
          break;
        }
      }

      // The maximum frame that this selected frame can be moved to is the next
      //  UNSELECTED vertex frame, MINUS the number of items from here to that vertex...
      int max_next_frame = -1;
      MusECore::ciCtrl inext = iold;
      ++inext;
      int next_frame_offset = 1; // Yes, that's 1.
      while(inext != automation.currentCtrlList->end())
      {
        // Stop when we find the next unselected frame.
        if(!automation.currentCtrlFrameList.contains(inext->second.frame))
        {
          max_next_frame = inext->second.frame - next_frame_offset;
          break;
        }
        ++inext;
        ++next_frame_offset;
      }

      int new_frame = old_frame + delta_frame;
      if(new_frame < min_prev_frame)
        new_frame = min_prev_frame;
      if(max_next_frame != -1 && new_frame > max_next_frame)
        new_frame = max_next_frame;

      //if(old_frame != new_frame)
      //{
        automation.currentCtrlFrameList.replace(i, new_frame);
        operations.push_back(UndoOp(UndoOp::ModifyAudioCtrlVal, automation.currentTrack, automation.currentCtrlList->id(), old_frame, new_frame, old_value, cvval));
      //}
    }
  }

  automation.startMovePoint = inPos;
  if(!operations.empty())
  {
    operations.combobreaker = automation.breakUndoCombo;
    automation.breakUndoCombo = false; // Reset.

    MusEGlobal::song->applyOperationGroup(operations);
    // User probably would like to hear results so make sure controller is enabled.
    ((MusECore::AudioTrack*)automation.currentTrack)->enableController(automation.currentCtrlList->id(), true);
    controllerChanged(automation.currentTrack, automation.currentCtrlList->id());
  }
}

void PartCanvas::newAutomationVertex(QPoint pos)
{
  if (_tool != AutomationTool || automation.controllerState != addNewController)
    return;

  Undo operations;

  const int posy=mapy(pos.y());
  const int tracky = mapy(automation.currentTrack->y());
  const int trackHeight = automation.currentTrack->height();

  const int mouseY = trackHeight - (posy - tracky)-2;
  const double yfraction = ((double)mouseY)/automation.currentTrack->height();

  double min, max;
  automation.currentCtrlList->range(&min,&max);
  double cvval;
  if (automation.currentCtrlList->valueType() == MusECore::VAL_LOG  ) { // use db scale for volume
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

  // Store the text.
  automation.currentText = QString("Param:%1 Value:%2").arg(automation.currentCtrlList->name()).arg(cvval);

  const int frame = MusEGlobal::tempomap.tick2frame(pos.x());
  operations.push_back(UndoOp(UndoOp::AddAudioCtrlVal, automation.currentTrack, automation.currentCtrlList->id(), frame, cvval));
  automation.currentCtrlFrameList.clear();
  automation.currentCtrlFrameList.append(frame);
  automation.currentCtrlValid = true;
  automation.controllerState = movingController;

  automation.startMovePoint = pos;

  if(!operations.empty())
  {
    operations.combobreaker = automation.breakUndoCombo;
    automation.breakUndoCombo = false; // Reset.

    MusEGlobal::song->applyOperationGroup(operations);
    // User probably would like to hear results so make sure controller is enabled.
    ((MusECore::AudioTrack*)automation.currentTrack)->enableController(automation.currentCtrlList->id(), true);
    controllerChanged(automation.currentTrack, automation.currentCtrlList->id());
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
      itemSelectionsChanged();
      redraw();
      }

} // namespace MusEGui
