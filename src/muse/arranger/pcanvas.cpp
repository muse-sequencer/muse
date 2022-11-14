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
#include <stdint.h>

#include <QClipboard>
#include <QMessageBox>
#include <QUrl>
#include <QPoint>
#include <QIcon>
#include <QMimeData>
#include <QDrag>
#include <QStringList>
#include <QInputDialog>
#include <QVector>
#include <QCursor>
#include <QAction>
#include <QActionGroup>

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
#include "tracks_duplicate.h"
#include "name_factory.h"
#include "song.h"
#include "helper.h"

// Forwards from header:
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QDragEnterEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>

#include "xml.h"
#include "part.h"
#include "xml_statistics.h"

#include <QDebug>

using MusECore::Undo;
using MusECore::UndoOp;

#define ABS(x) (abs(x))

#define FABS(x) (fabs(x))

#define EDITING_FINISHED_TIMEOUT 50 /* in milliseconds */

using std::set;

namespace MusEGui {

//---------------------------------------------------------
//   NPart
//---------------------------------------------------------

NPart::NPart(MusECore::Part* p) : PItem(p)
      {
      leftBorderTouches = false;
      rightBorderTouches = false;

      _serial=_part->uuid();

      int y  = track()->y();
      setPos(QPoint(_part->tick(), y));
      setBBox(QRect(_part->tick(), y, _part->lenTick(), track()->height()));
      }

AutomationObject::AutomationObject()
{
  clear();
}

void AutomationObject::clear()
{
  currentCtrlFrameList.clear();
  currentCtrlList = nullptr;
  currentCtrlValid = false;
  currentTrack = nullptr;
  breakUndoCombo = false;
  currentFrame = 0;
  currentWorkingFrame = 0;
  currentVal = 0;
  controllerState = doNothing;
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
      setStatusTip(tr("Part canvas: Use Pencil tool to draw parts, or double-click to create a new MIDI/drum part between the range markers. Press F1 for help."));

      tracks = MusEGlobal::song->tracks();
      setMouseTracking(true);
      drag          = DRAG_OFF;
      curColorIndex = 0;

      // Set some point variable defaults. They will change when config changes.
      setAutomationPointRadius(2);

      updateItems();
      updateAudioAutomation();
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
                              part->setColorIndex(curColorIndex);
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

void PartCanvas::moveCanvasItems(CItemMap& items, int dp, int dx, DragType dtype, MusECore::Undo& operations, bool rasterize)
{
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
  curItem=nullptr;
  items.clearDelete();
}

//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void PartCanvas::updateItems()
      {
      QUuid sn;
      if (curItem) sn=static_cast<NPart*>(curItem)->serial();
      curItem=nullptr;

      items.clearDelete();
      for (MusECore::ciTrack t = tracks->begin(); t != tracks->end(); ++t) {
         if ((*t)->isVisible()) //ignore parts from hidden tracks
         {
            MusECore::PartList* pl = (*t)->parts();
            for (MusECore::ciPart i = pl->begin(); i != pl->end(); ++i) {
                  MusECore::Part* part = i->second;
                  NPart* np = new NPart(part);
                  items.add(np);

                  if (!sn.isNull() && np->serial() == sn)
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
//   updateAudioAutomation
//---------------------------------------------------------

void PartCanvas::updateAudioAutomation()
{
  automation.currentCtrlFrameList.clear();

  const bool isdrag = drag == DRAG_NEW || drag == DRAG_MOVE_START || drag == DRAG_COPY_START ||
    drag == DRAG_MOVE || drag == DRAGX_MOVE || drag == DRAGY_MOVE ||
    drag == DRAG_COPY || drag == DRAGX_COPY || drag == DRAGY_COPY;

  bool itemFound = false;
  for (MusECore::ciTrack it = tracks->cbegin(); it != tracks->cend(); ++it) {
    if((*it)->isMidiTrack())
      continue;
    const MusECore::AudioTrack* track = static_cast<const MusECore::AudioTrack*>(*it);
      // Do not include hidden tracks.
    if (!track->isVisible())
      continue;
    const MusECore::CtrlListList* cll = track->controller();
    for(MusECore::ciCtrlList icll = cll->cbegin(); icll != cll->cend(); ++icll)
    {
      const MusECore::CtrlList* cl = icll->second;
      // Do not include hidden controller lists.
      if(!cl->isVisible())
        continue;
      for(MusECore::ciCtrl ic = cl->cbegin(); ic != cl->cend(); ++ic)
      {
        const MusECore::CtrlVal& cv = ic->second;
        // Include only selected controller values.
        if(!cv.selected())
          continue;

        // If we are in audio controller move mode, we take the group end flag directly from
        //  the controller list item, to preserve the flag when moving controller points.
        // Otherwise we determine the flag ourselves by checking whether this item is the
        //  end of a group ie. the next item is unselected.
        bool isGroupEnd;
        if(MusEGlobal::song->audioCtrlMoveModeBegun())
          isGroupEnd = cv.groupEnd();
        else
        {
          MusECore::ciCtrl ic_next = ic;
          ++ic_next;
          isGroupEnd = (ic_next == cl->cend()) || (!ic_next->second.selected());
        }

        // If we were moving, check that the track, cl, and frame are still found.
        if(!itemFound && isdrag && automation.currentCtrlValid &&
           track == automation.currentTrack && cl == automation.currentCtrlList && ic->first == automation.currentWorkingFrame)
        {
          itemFound = true;
          // Update the current frame now that it has changed, so that it can still be highlighted.
          // This may be a moot 'point' (no pun intended), because as soon as the mouse is moved
          //  after this, it may pick a different closer point to highlight.
          // But at least this gives an opportunity to stay highlighted on this point for the moment,
          //  even though there may be closer points to highlight.
          automation.currentFrame = automation.currentWorkingFrame;
        }

        automation.currentCtrlFrameList.addSelected(
          track, cl->id(), ic->first, MusECore::AudioAutomationItem(
            ic->first, cv.value(), isGroupEnd, ic->second.discrete()));
      }
    }
  }

  // If we were not moving, or if we were but the item is not found, clear the automation state,
  //  otherwise leave it in the moving state with valid track, cl, and frame.
  // Or, if we are not in automation tool mode, clear the state since there shouldn't be a state right now.
  if(_tool != AutomationTool || !isdrag || !itemFound)
  {
    automation.controllerState = doNothing;
    automation.currentCtrlValid = false;
    automation.currentTrack = nullptr;
    automation.currentCtrlList = nullptr;
    automation.currentFrame = 0;
    automation.currentWorkingFrame = 0;
    automation.currentVal = 0;
    // Only do this in Automation tool mode, since the Canvas might legitimately
    //  be in an appropriate drag or cursor mode.
    // Allow lasso mode since it might first unselect some vertices causing this routine
    //  to be called, and we want it to continue in LASSO mode.
    if(_tool == AutomationTool && drag != DRAG_LASSO && drag != DRAG_LASSO_START)
    {
      drag = DRAG_OFF;
      setCursor();
      setMouseGrab(false);
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
        // This is a one-time operation (it has no 'undo' section).
        opsp->push_back(MusECore::UndoOp(MusECore::UndoOp::GlobalSelectAllEvents, false, 0, 0, true));
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

   unsigned int newPosOrLen = 0;
   if(resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
   {
      if(i->x() < 0)
      {
        newPosOrLen = 0;
      }
      else
      {
        newPosOrLen = i->x();
        if(!noSnap)
          newPosOrLen = MusEGlobal::sigmap.raster(newPosOrLen, *_raster);
      }
   }
   else
   {
      unsigned int endPos = p->tick() + i->width();
      if (!noSnap) {
            endPos = MusEGlobal::sigmap.raster(endPos, *_raster);
      }
      newPosOrLen = endPos - p->tick();
      if (newPosOrLen == 0) {
          newPosOrLen = MusEGlobal::sigmap.rasterStep(p->tick(), *_raster);
      }
   }
     
   // Do not force all clones to be done (false) until we have a key we can use for it.
   MusECore::resize_part(t, p, newPosOrLen, resizeDirection, false, ctrl);
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
            return nullptr;
      MusECore::Track* track = tracks->index(trackIndex);
      if(!track)
        return nullptr;

      MusECore::Part* pa = nullptr;
      NPart* np = nullptr;
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
                  return nullptr;
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
          MusECore::Part* new_part = nullptr;
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

      QAction *act_cut = partPopup->addAction(*cutSVGIcon, tr("C&ut"));
      act_cut->setData(OP_CUT);
      act_cut->setShortcut(shortcuts[SHRT_CUT].key);

      QAction *act_copy = partPopup->addAction(*copySVGIcon, tr("&Copy"));
      act_copy->setData(OP_COPY);
	  act_copy->setShortcut(shortcuts[SHRT_COPY].key);

      partPopup->addSeparator();
      int rc = npart->part()->nClones();
      QString st = QString(tr("S&elect "));
      if(rc > 1)
        st += (QString().setNum(rc) + QString(" "));
      st += QString(tr("clones"));
      QAction *act_select = partPopup->addAction(st);
      act_select->setData(OP_SELECT_CLONES);

      QAction *act_declone = partPopup->addAction(tr("De-clone"));
      act_declone->setData(OP_DECLONE);

      partPopup->addSeparator();

      bool multi = npart->isSelected() && countSelectedParts() > 1;
      QAction *act_rename = partPopup->addAction(multi ? tr("Rename selected") : tr("Rename"));
      act_rename->setData(OP_RENAME);

      QMenu* colorPopup = partPopup->addMenu(multi ? tr("Color selected") : tr("Color"));

      // part color selection
      for (int i = 0; i < NUM_PARTCOLORS; ++i) {
          QAction *act_color = nullptr;
          if (i == 0 && MusEGlobal::config.useTrackColorForParts)
              act_color = colorPopup->addAction(*tracktypeSVGIcon, tr("Track Color"));
          else
              act_color = colorPopup->addAction(MusECore::colorRect(MusEGlobal::config.partColors[i], 80, 80), MusEGlobal::config.partColorNames[i]);
          act_color->setData(OP_PARTCOLORBASE+i);

          if (i == 0)
              colorPopup->addSeparator();
      }

      partPopup->addSeparator();
      QAction *act_delete = partPopup->addAction(*deleteIconSVG, tr("Delete"));
      act_delete->setData(OP_DELETE);
      QAction *act_split = partPopup->addAction(*cutterIconSVG, tr("Split"));
      act_split->setData(OP_SPLIT);
      QAction *act_glue = partPopup->addAction(*glueIconSVG, tr("Glue"));
      act_glue->setData(OP_GLUE);
      QAction *act_superglue = partPopup->addAction(tr("Super Glue (Merge Selection)"));
      act_superglue->setData(OP_GLUESELECTION);

      partPopup->addSeparator();
      switch(trackType) {
            case MusECore::Track::MIDI: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startPianoEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startListEditAction);
                  partPopup->addMenu(MusEGlobal::muse->arranger()->parentWin()->scoreSubmenu);
                  partPopup->addSeparator();
                  QAction *act_mexport = partPopup->addAction(tr("Save Part to Disk..."));
                  act_mexport->setData(OP_SAVEPARTTODISK);
                  }
                  break;
            case MusECore::Track::DRUM: {
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startDrumEditAction);
                  partPopup->addAction(MusEGlobal::muse->arranger()->parentWin()->startListEditAction);
                  partPopup->addSeparator();
                  QAction *act_dexport = partPopup->addAction(tr("Save Part to Disk..."));
                  act_dexport->setData(OP_SAVEPARTTODISK);
                  }
                  break;
            case MusECore::Track::WAVE: {
                  QAction *act_wedit = partPopup->addAction(*waveeditorSVGIcon, tr("Wave Edit..."));
                  act_wedit->setData(OP_WAVEEDIT);
                  QAction *act_wexport = partPopup->addAction(tr("Save Part to Disk..."));
                  act_wexport->setData(OP_SAVEPARTTODISK);
                  QAction *act_wfinfo = partPopup->addAction(tr("File Info..."));
                  act_wfinfo->setData(OP_FILEINFO);
                  QAction *act_wfnorm = partPopup->addAction(tr("Normalize"));
                  act_wfnorm->setData(OP_NORMALIZE);
                  act_wfnorm->setShortcut(shortcuts[SHRT_PART_NORMALIZE].key);
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

      return partPopup;
      }

void PartCanvas::renameItem(CItem *item)
{
    NPart* p = static_cast<NPart*>(item);
    if (p->isSelected()) {
        const QString oldname = p->name();

        QInputDialog dlg(this);
        dlg.setWindowTitle(tr("Part Name"));
        dlg.setLabelText(tr("Enter part name:"));
        dlg.setTextValue(oldname);

        const int rc = dlg.exec();
        if(rc == QDialog::Rejected)
            return;

        const QString newname = dlg.textValue();

        if(newname == oldname)
            return;

        for (const auto& it : *MusEGlobal::song->tracks()) {
            for (const auto& ip : *it->parts()) {
                if (ip.second->selected())
                    ip.second->setName(newname);
            }
        }
    } else {
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
}

//---------------------------------------------------------
//   genAutomationPopup
//---------------------------------------------------------

QMenu* PartCanvas::genAutomationPopup(QMenu* menu)
      {
      QMenu* automationTopMenu = menu;
      if(!automationTopMenu)
        automationTopMenu = new QMenu(this);

      automationTopMenu->addAction(new MenuTitleItem(tr("Automation"), automationTopMenu));

      QMenu* automationMenu = automationTopMenu->addMenu(tr("Graphs"));

      QAction* act;

      act = automationMenu->addAction(tr("Remove selected"));
      act->setData(AUTO_OP_REMOVE);
      act->setEnabled(!automation.currentCtrlFrameList.empty());

      act = automationMenu->addAction(tr("Align selected to point"));
      act->setData(AUTO_OP_ALIGN_TO_SELECTED);
      act->setCheckable(false);
      act->setEnabled(automation.currentCtrlValid && !automation.currentCtrlFrameList.empty());

      {
        bool canDiscrete, canInterpolate;
        haveSelectedAutomationMode(&canDiscrete, &canInterpolate);

        act = automationMenu->addAction(tr("Set selected to discrete"));
        act->setData(AUTO_OP_SET_DISCRETE);
        act->setCheckable(false);
        act->setEnabled(canDiscrete);

        act = automationMenu->addAction(tr("Set selected to interpolated"));
        act->setData(AUTO_OP_SET_INTERPOLATED);
        act->setCheckable(false);
        act->setEnabled(canInterpolate);
      }

      automationMenu->addAction(new MenuTitleItem(tr("Paste/drop mode"), automationMenu));

      QActionGroup* ag = new QActionGroup(automationMenu);

      act = ag->addAction(tr("No erase"));
      act->setData(AUTO_OP_NO_ERASE_MODE);
      act->setCheckable(true);
      if(MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteNoErase)
        act->setChecked(true);

      act = ag->addAction(tr("Erase"));
      act->setData(AUTO_OP_ERASE_MODE);
      act->setCheckable(true);
      if(MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteErase)
        act->setChecked(true);

      act = ag->addAction(tr("Erase range"));
      act->setData(AUTO_OP_ERASE_RANGE_MODE);
      act->setCheckable(true);
      if(MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteEraseRange)
        act->setChecked(true);

      automationMenu->addActions(ag->actions());

      automationMenu->addSeparator();

      act = automationMenu->addAction(tr("End paste/drop mode"));
      act->setData(AUTO_OP_END_MOVE_MODE);
      act->setEnabled(MusEGlobal::song->audioCtrlMoveModeBegun() == true);

      return automationTopMenu;
      }

//---------------------------------------------------------
//   automationPopup
//---------------------------------------------------------

void PartCanvas::automationPopup(int n)
{
  if(n >= TOOLS_ID_BASE && n < AUTO_OP_BASE_ENUM)
  {
     canvasPopup(n);
     return;
  }

  Undo operations;
  if(n == AUTO_OP_REMOVE)
    deleteSelectedAutomation(operations);
  else if(n == AUTO_OP_ALIGN_TO_SELECTED)
    alignSelectedAutomation(operations);
  else if(n == AUTO_OP_SET_DISCRETE)
    setSelectedAutomationMode(operations, MusECore::CtrlList::DISCRETE);
  else if(n == AUTO_OP_SET_INTERPOLATED)
    setSelectedAutomationMode(operations, MusECore::CtrlList::INTERPOLATE);

  else if(n >= AUTO_OP_NO_ERASE_MODE && n <= AUTO_OP_ERASE_RANGE_MODE)
  {
    MusECore::CtrlList::PasteEraseOptions opts = MusECore::CtrlList::PasteNoErase;
    if(n == AUTO_OP_NO_ERASE_MODE)
      opts = MusECore::CtrlList::PasteNoErase;
    else if(n == AUTO_OP_ERASE_MODE)
      opts = MusECore::CtrlList::PasteErase;
    else if(n == AUTO_OP_ERASE_RANGE_MODE)
      opts = MusECore::CtrlList::PasteEraseRange;

    operations.push_back(UndoOp(UndoOp::SetAudioCtrlPasteEraseMode, opts));
    // Are we currently in a state of move/paste/drop?
    if(MusEGlobal::song->audioCtrlMoveModeBegun() == true)
      // Here we pass what the erase options WILL BECOME, not what they are now.
      MusEGlobal::song->collectAudioCtrlPasteModeOps(automation.currentCtrlFrameList, operations, opts, true);
    //else
      // Although we can do this, we have to use an undo operation every time (above) to be consistent,
      //  even if nothing was moved or dropped etc.
      //MusEGlobal::config.audioCtrlGraphPasteEraseOptions = opts;
  }
  else if(n == AUTO_OP_END_MOVE_MODE)
  {
    //unselectAllAutomation(operations);
    MusEGlobal::song->endAudioCtrlMoveMode(operations);
  }
  else
  {
    fprintf(stderr, "unknown automation action %d\n", n);
    return;
  }

  MusEGlobal::song->applyOperationGroup(operations);
}

//---------------------------------------------------------
//   itemPopup
//---------------------------------------------------------
void PartCanvas::itemPopup(CItem* item, int n, const QPoint& pt)
{
   if(n >= TOOLS_ID_BASE && n < AUTO_OP_BASE_ENUM)
   {
      canvasPopup(n);
      return;
   }

   if(n >= AUTO_OP_BASE_ENUM && n <= AUTO_OP_END_ENUM)
   {
      automationPopup(n);
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
         operations.push_back(UndoOp(UndoOp::SelectPart, p, true, p->selected(), MusEGlobal::config.selectionsUndoable));
         for(MusECore::Part* it = p->nextClone(); it!=p; it=it->nextClone())
            operations.push_back(UndoOp(UndoOp::SelectPart, it, true, it->selected(), MusEGlobal::config.selectionsUndoable));
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

      if (item->isSelected()) {
          //Loop through all parts and set color on selected:
          for (const auto& it : items) {
              if (it.second->isSelected())
                  it.second->part()->setColorIndex(curColorIndex);
          }
      } else {
          item->part()->setColorIndex(curColorIndex);
      }

      emit curPartColorIndexChanged(curColorIndex);
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

void PartCanvas::setPartColor(int idx)
{
    curColorIndex = idx;

    //Loop through all parts and set color on selected:
    for (const auto& it : items) {
        if (it.second->isSelected())
            it.second->part()->setColorIndex(curColorIndex);
    }

    MusEGlobal::song->update(SC_PART_MODIFIED);
    redraw();
}

void PartCanvas::setCurrentColorIndex(int idx)
{
    curColorIndex = idx;
}

//---------------------------------------------------------
//   mousePress
//---------------------------------------------------------

bool PartCanvas::mousePress(QMouseEvent* event)
      {
      const bool ctrlkey = keyState & Qt::ControlModifier;
      const bool shiftkey = keyState & Qt::ShiftModifier;
      const bool altkey = keyState & Qt::AltModifier;
      CItem* item = items.find( ev_pos );
      Undo operations;
      bool ret = true;

      if (!ctrlkey && (button == Qt::RightButton))
      {
        QMenu* menu = nullptr;
        if (curItem) {
              if (ctrlkey && virt() && (_tool == PointerTool || _tool == PencilTool || _tool == RubberTool)) {
                    // Let ancestor handle this specific case.
                    return true;
                    }
              else {
                    menu = genItemPopup(curItem);
                    if (!menu)
                      return false;
                    event->accept();
                    genAutomationPopup(menu);
                    genCanvasPopup(menu);
                    }
              }
        else {
              menu = genAutomationPopup();
              if (!menu)
                return false;
              event->accept();
              genCanvasPopup(menu);
              }

        QAction* act;

        act = menu->exec(event->globalPos());
        if(act)
        {
          const int n = act->data().toInt();
          if(n < TOOLS_ID_BASE)
            itemPopup(curItem, n, ev_pos);
          else if(n >= TOOLS_ID_BASE && n < AUTO_OP_BASE_ENUM)
            canvasPopup(n);
          else if(n >= AUTO_OP_BASE_ENUM && n <= AUTO_OP_END_ENUM)
            automationPopup(n);
        }
        delete menu;
        ret = false;
      }
      else switch (_tool) {
            default:
                  break;
            case PointerTool:
            case PencilTool:
                  if (!ctrlkey && item && button == Qt::LeftButton)
                      emit trackChanged(item->part()->track());
                  break;
            case CutTool:
                  if (!ctrlkey && item && button == Qt::LeftButton) splitItem(item, ev_pos );
                  break;
            case GlueTool:
                  if (!ctrlkey && item && button == Qt::LeftButton) glueItem(item);
                  break;
            case MuteTool:
                  {
                  if (!ctrlkey && item && button == Qt::LeftButton) {
                      NPart* np = (NPart*) item;
                      MusECore::Part*  p = np->part();
                      p->setMute(!p->mute());
                      redraw();
                      break;
                      }
                  }
                  break;
            case AutomationTool:
                  {
                  if (!ctrlkey && button == Qt::MiddleButton) {
                        deleteSelectedAutomation(operations);
                  }
                  else {
                      if (drag == DRAG_OFF && automation.controllerState == addNewController)
                      {
                        // If hovering over a line.
                        if(automation.currentCtrlList)
                        {
                          // Ctrl key not pressed? Unselect all other vertices.
                          if(!ctrlkey)
                            unselectAllAutomation(operations);
                          if(newAutomationVertex( ev_pos, operations, shiftkey))
                          {
                            // Reset.
                            automation.controllerState = doNothing;
                            drag = DRAG_NEW;
                            setCursor();
                            setMouseGrab(true); // CAUTION
                          }
                        }
                      }
                      else if (drag == DRAG_OFF && automation.controllerState == doNothing)
                      {
                        // If hovering over a line and vertex.
                        if(automation.currentCtrlList && automation.currentCtrlValid)
                        {
                          // We need to find the controller item.
                          MusECore::iCtrl ic = automation.currentCtrlList->find(automation.currentWorkingFrame);
                          if(ic != automation.currentCtrlList->end())
                          {
                            // Alt alone is usually reserved for moving a window in X11. Ignore shift + alt.
                            if (ctrlkey && !altkey)
                                  drag = DRAG_COPY_START;
                            else if (ctrlkey && altkey)
                                  drag = DRAG_CLONE_START;
                            else if (!ctrlkey && !altkey)
                                  drag = DRAG_MOVE_START;
                          }
                        }
                        // Not hovering over a vertex.
                        else
                        {
                          // Start the lasso...
                          drag = DRAG_LASSO_START;
                        }
                        setCursor();
                        setMouseGrab(true); // CAUTION
                      }
                  }

                  event->accept();

                  ret = false;
                  break;
                  }
            }

            if(!operations.empty())
            {
              // User probably would like to hear results so make sure controllers are internally enabled,
              //  in case the internal enables were off for some reason (they shouldn't be right now).
              // TODO: Refine this like it was before. It called AudioTrack::enableController().
              //       Make a new per-controller enable undo operation.
              // This is a non-undoable 'one-time' operation, removed after execution.
              operations.push_back(UndoOp(UndoOp::EnableAllAudioControllers, true));
              operations.combobreaker = true;
              MusEGlobal::song->applyOperationGroup(operations);
              automation.breakUndoCombo = false;
            }

      return ret;
      }

//---------------------------------------------------------
//   mouseRelease
//---------------------------------------------------------

void PartCanvas::mouseRelease(QMouseEvent* event)
{
  const bool ctrlkey = event->modifiers() & Qt::ControlModifier;
  const bool shiftkey = event->modifiers() & Qt::ShiftModifier;
  const bool altkey = event->modifiers() & Qt::AltModifier;
  Undo operations;
  bool redrawFlag = false;

  switch (drag) {
        case DRAG_MOVE_START:
        case DRAG_COPY_START:
        case DRAG_CLONE_START:

              if(_tool == AutomationTool)
              {
                if (altkey || !ctrlkey)
                  unselectAllAutomation(operations);
                // If hovering over a line and vertex.
                if(automation.currentCtrlList && automation.currentCtrlValid)
                {
                  // We need to find the controller item.
                  MusECore::iCtrl ic = automation.currentCtrlList->find(automation.currentWorkingFrame);
                  if(ic != automation.currentCtrlList->end())
                  {
                    const bool selected = ic->second.selected();
                    if (!shiftkey)
                    {
                      // Select or deselect only the clicked item.
                      operations.push_back(UndoOp(UndoOp::SelectAudioCtrlVal,
                        automation.currentCtrlList, automation.currentWorkingFrame,
                        selected, !(ctrlkey && selected), !MusEGlobal::config.selectionsUndoable));
                    }
                  }
                }
                redrawFlag = true;
              }
              break;

        default:
              break;
  }

  automation.controllerState = doNothing;
  // Direction argument doesn't matter, just pass zero.
  processAutomationMovements(event->pos(), 0, false);

  MusEGlobal::song->applyOperationGroup(operations);

  if (redrawFlag)
    redraw();
}

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void PartCanvas::mouseMove(QMouseEvent* event)
      {
      int x = event->pos().x();
      if (x < 0)
            x = 0;

      if (_tool == AutomationTool) {
        event->accept();
        QPoint dist  = ev_pos - start;
        int ax       = ABS(rmapx(dist.x()));
        int ay       = ABS(rmapy(dist.y()));
        bool isMoving  = (ax >= 2) || (ay > 2);
        Undo operations;

        switch (drag) {
                case DRAG_MOVE_START:
                case DRAG_COPY_START:
                case DRAG_CLONE_START:
                      {
                        if (!isMoving)
                              break;
                        int dir = 0;
                        if (keyState & Qt::ShiftModifier) {
                              if (ax > ay) {
                                    if (drag == DRAG_MOVE_START)
                                          drag = DRAGX_MOVE;
                                    else if (drag == DRAG_COPY_START)
                                          drag = DRAGX_COPY;
                                    else
                                          drag = DRAGX_CLONE;
                                    dir = 1;
                                    }
                              else {
                                    if (drag == DRAG_MOVE_START)
                                          drag = DRAGY_MOVE;
                                    else if (drag == DRAG_COPY_START)
                                          drag = DRAGY_COPY;
                                    else
                                          drag = DRAGY_CLONE;
                                    dir = 2;
                                    }
                              }
                        else {
                              if (drag == DRAG_MOVE_START)
                                    drag = DRAG_MOVE;
                              else if (drag == DRAG_COPY_START)
                                    drag = DRAG_COPY;
                              else
                                    drag = DRAG_CLONE;
                              }
                        setCursor();

                        // If hovering over a line and vertex.
                        if(automation.currentCtrlList && automation.currentCtrlValid)
                        {
                          // We need to find the controller item.
                          MusECore::iCtrl ic = automation.currentCtrlList->find(automation.currentWorkingFrame);
                          if(ic != automation.currentCtrlList->end())
                          {
                            const bool selected = ic->second.selected();
                            if (!selected)
                            {
                              if (drag == DRAG_MOVE)
                                unselectAllAutomation(operations);
                              operations.push_back(UndoOp(UndoOp::SelectAudioCtrlVal,
                                automation.currentCtrlList, automation.currentWorkingFrame,
                                selected, true, !MusEGlobal::config.selectionsUndoable));
                            }
                          }
                        }

                        DragType dt;
                        if (drag == DRAG_MOVE)
                              dt = MOVE_MOVE;
                        else if (drag == DRAG_COPY)
                              dt = MOVE_COPY;
                        else
                              dt = MOVE_CLONE;

                        startMoving(ev_pos, dir, dt, !(keyState & Qt::ShiftModifier));

                        //redraw();
                      }
                      break;

                case DRAG_NEW:
                      {
                        if (!isMoving)
                              break;
                        int dir = 0;
                        if (keyState & Qt::ShiftModifier) {
                              if (ax > ay) {
                                    drag = DRAGX_MOVE;
                                    dir = 1;
                                    }
                              else {
                                    drag = DRAGY_MOVE;
                                    dir = 2;
                                    }
                              }
                        else {
                              drag = DRAG_MOVE;
                              }
                        setCursor();

                        DragType dt;
                        if (drag == DRAG_MOVE)
                              dt = MOVE_MOVE;
                        else if (drag == DRAG_COPY)
                              dt = MOVE_COPY;
                        else
                              dt = MOVE_CLONE;

                        startMoving(ev_pos, dir, dt, !(keyState & Qt::ShiftModifier));

                        //redraw();
                      }
                      break;

                default:
                      processAutomationMovements(ev_pos, 0, /*slowMotion*/ false);
                break;
        }

        MusEGlobal::song->applyOperationGroup(operations);
      }
      else
      {
        event->ignore();
      }

      emit timeChanged(MusEGlobal::sigmap.raster(x, *_raster));
      //fprintf(stderr, "PartCanvas::mouseMove: x:%d *_raster:%d sigmap.raster(x, *_raster):%d\n",
      //        x, *_raster, MusEGlobal::sigmap.raster(x, *_raster));

      showStatusTip(event);
}

//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

bool PartCanvas::selectLasso(bool toggle, MusECore::Undo* undo)
      {
      if(_tool != AutomationTool)
        return Canvas::selectLasso(toggle);

      if(!undo)
        return false;

      const unsigned int lasso_SFrame = MusEGlobal::tempomap.tick2frame(lasso.x());
      const unsigned int lasso_EFrame = MusEGlobal::tempomap.tick2frame(lasso.x() + lasso.width());
      const int lasso_SY = lasso.y();
      const int lasso_EY = lasso.y() + lasso.height();

      bool changed = false;

      for (MusECore::ciTrack it = tracks->cbegin(); it != tracks->cend(); ++it) {
        if((*it)->isMidiTrack())
          continue;
        MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*it);
          // Do not include hidden tracks.
        if (!track->isVisible())
          continue;
        const int trackY = track->y();
        const int trackH = track->height();
        const MusECore::CtrlListList* cll = track->controller();
        for(MusECore::ciCtrlList icll = cll->cbegin(); icll != cll->cend(); ++icll)
        {
          MusECore::CtrlList* cl = icll->second;
          // Do not include hidden controller lists.
          if(!cl->isVisible())
            continue;

          if(lasso_SFrame >= lasso_EFrame)
            continue;

          MusECore::ciCtrl s_ic = cl->lower_bound(lasso_SFrame);
          if(s_ic == cl->cend())
            continue;
          MusECore::ciCtrl e_ic = cl->upper_bound(lasso_EFrame);
          if(e_ic == cl->cbegin())
            continue;

          double min, max;
          cl->range(&min,&max);

          for(MusECore::ciCtrl ic = s_ic; ic != e_ic; ++ ic)
          {
            const MusECore::CtrlVal& cv = ic->second;

            double y = cv.value();
            if (cl->valueType() == MusECore::VAL_LOG ) {
              y = logToVal(y, min, max); // represent volume between 0 and 1
              if(y < 0) y = 0.0;
            }
            else
              y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

            const int eventY = /*mapy*/(trackY + trackH - 2 - y * trackH);

            if(eventY < lasso_SY || eventY >= lasso_EY)
              continue;


            const bool sel = toggle ? !(toggle && cv.selected()) : true;
            // If toggle (ctrl) is false, all points will have been scheduled for deselection first,
            //  so just go ahead and select the point.
            if(!toggle || sel != cv.selected())
            {
              undo->push_back(UndoOp(UndoOp::SelectAudioCtrlVal,
                cl, ic->first, cv.selected(), sel, !MusEGlobal::config.selectionsUndoable));
              changed = true;
            }
          }
        }
      }

      return changed;
      }

void PartCanvas::deselectAll(MusECore::Undo* undo)
{
  if(_tool == AutomationTool)
  {
    if(undo)
      unselectAllAutomation(*undo);
  }
  else
  {
    Canvas::deselectAll(undo);
  }
}

void PartCanvas::showStatusTip(QMouseEvent* event) const {

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

void PartCanvas::setCursor()
{
    if(_tool != AutomationTool)
    {
      // Let the Canvas handle it, and call showCursor().
      Canvas::setCursor();
      return;
    }

    // Avoid duplication, just do it below.
    //showCursor();

    switch (drag) {

    case DRAGX_MOVE:
    case DRAGX_COPY:
    case DRAGX_CLONE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMoveHorizCursor);
        break;

    case DRAGY_MOVE:
    case DRAGY_COPY:
    case DRAGY_CLONE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMoveVertCursor);
        break;

    case DRAG_MOVE:
    case DRAG_COPY:
    case DRAG_CLONE:
    case DRAG_MOVE_START:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMove4WayCursor);
        break;

    case DRAG_RESIZE:
        // Make sure to do this.
        showCursor();
        QWidget::setCursor(*pencilMoveHorizCursor);
        break;

    case DRAG_OFF:
        // Make sure to do this.
        showCursor();
        if(automation.controllerState == addNewController)
          QWidget::setCursor(Qt::CrossCursor);
        else if(automation.controllerState == doNothing && automation.currentTrack && automation.currentCtrlList && automation.currentCtrlValid)
          QWidget::setCursor(Qt::PointingHandCursor);
        else
          // Let the Canvas handle it, and call showCursor().
          Canvas::setCursor();
        break;

    default:
        // Let the Canvas handle it, and call showCursor().
        Canvas::setCursor();
        break;
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

            MusECore::Undo operations;
            MusECore::delete_selected_parts(operations);
            MusECore::delete_selected_audio_automation(operations);
            MusEGlobal::song->applyOperationGroup(operations);
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

      CItem* newItem = nullptr;
      bool singleSelection = isSingleSelection();
      bool add = false;

      // Select part to the right
      if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
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
            MusECore::Track* track = curItem->part()->track();
            track = y2Track(track->y() - 1);

            //If we're at topmost (no track above), leave
            if (!track)
                  return;

            int middle = curItem->x() + curItem->part()->lenTick()/2;
            CItem *aboveL = nullptr, *aboveR = nullptr;
            //Upper limit: song end, lower limit: song start
            int ulimit  = MusEGlobal::song->len();
            int llimit = 0;

            while (newItem == nullptr) {
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
                        CItem* above  = nullptr;
                        above = (aboveL != nullptr) ? aboveL : aboveR;
                        newItem = above;
                  }
                  else { //We didn't hit anything. Move to track above, if there is one
                        track = y2Track(track->y() - 1);
                        if (track == nullptr)
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

            CItem *belowL = nullptr, *belowR = nullptr;
            //Upper limit: song end , lower limit: song start
            int ulimit = MusEGlobal::song->len();
            int llimit = 0;
            while (newItem == nullptr) {
                  int y = track->y() + 1;
                  int xoffset = 0;
                  int xleft   = middle - xoffset;
                  int xright  = middle + xoffset;
                  while ((xleft > llimit || xright < ulimit)  && (belowL == nullptr) && (belowR == nullptr)) {
                        xoffset += stepsize;
                        xleft  = middle - xoffset;
                        xright = middle + xoffset;
                        if (xleft >= 0)
                              belowL = items.find(QPoint(xleft,y));
                        if (xright <= ulimit)
                              belowR = items.find(QPoint(xright,y));
                  }

                  if ((belowL || belowR) != 0) { //We've hit something
                        CItem* below = nullptr;
                        below = (belowL != nullptr) ? belowL : belowR;
                        newItem = below;
                  }
                  else {
                        //Get next track below, or abort if this is the lowest
                        track = y2Track(track->y() + track->height() + 1 );
                        if (track == nullptr)
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
//      int xs_2 = xs_0 + 2;
//      if(xs_2 > xe_0)
//        xs_2 = xe_0;
      int xs_j = xs_0 + 8;
      if(xs_j > xe_0)
        xs_j = xe_0;

      int xe_1 = xe_0 - 1;
      if(xe_1 < xs_0)
        xe_1 = xs_0;
//      int xe_2 = xe_0 - 2;
//      if(xe_2 < xs_0)
//        xe_2 = xs_0;
      int xe_j = xe_0 - 8;
      if(xe_j < xs_0)
        xe_j = xs_0;

      int ys_0 = mbbr.y();
      int ye_0 = ys_0 + mbbr.height();
//      int ys_1 = ys_0 + 1;
//      if(ys_1 > ye_0)
//        ys_1 = ye_0;
//      int ys_2 = ys_0 + 2;
//      if(ys_2 > ye_0)
//        ys_2 = ye_0;
//      int ys_3 = ys_0 + 3;
//      if(ys_3 > ye_0)
//        ys_3 = ye_0;

//      int ye_1 = ye_0 - 1;
//      if(ye_1 < ys_0)
//        ye_1 = ys_0;
//      int ye_2 = ye_0 - 2;
//      if(ye_2 < ys_0)
//        ye_2 = ys_0;

      int mrxs_0 = mr.x();
      int mrxe_0 = mrxs_0 + mr.width();
      bool lbt = ((NPart*)item)->leftBorderTouches;
      bool rbt = ((NPart*)item)->rightBorderTouches;
      int lbx = lbt?xs_1:xs_0;
      int rbx = rbt?xe_1:xe_0;
      int lbx_c = lbx < mrxs_0 ? mrxs_0 : lbx;
      int rbx_c = rbx > mrxe_0 ? mrxe_0 : rbx;

      QColor partColor;
      if (part->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
          partColor = part->track()->color();
      else
          partColor = MusEGlobal::config.partColors[part->colorIndex()];

      int gradS = qBound(0, MusEGlobal::config.partGradientStrength, 200);

      if (item->isMoving())
      {
            QColor c(Qt::gray);
            c.setAlpha(MusEGlobal::config.globalAlphaBlend);
            brush = MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS);
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
            brush = MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS);
      }
      else
      {
          QColor c = partColor;
          c.setAlpha(MusEGlobal::config.globalAlphaBlend);
          brush = MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS);
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

        {
            // Draw remaining 'hidden events' decorations with 'jagged' edges...
            int color_brightness;
            if (MusECore::isColorBright(partColor) && !item_selected)
                color_brightness=96; //0;    // too light: use dark color
            else
                color_brightness=180; //255;   // too dark: use lighter color
            QColor c(color_brightness,color_brightness,color_brightness, MusEGlobal::config.globalAlphaBlend);
            // p.setBrush(QBrush(MusECore::gGradientFromQColor(c, mbbr.topLeft(), mbbr.bottomLeft())));
            p.setBrush(MusECore::getGradientFromColor(c, mbbr.topLeft(), mbbr.bottomLeft(), gradS));
        }

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
        drawWavePart(p, ur, wp, ubbr, item_selected);
      } else {
        MusECore::MidiPart* mp = (MusECore::MidiPart*)part;
        drawMidiPart(p, ur, mp, ubbr, vfrom, vto, item_selected);
      }

      p.setWorldMatrixEnabled(false);

        //
        // Now draw the borders, using custom segments...
        //

        p.setBrush(Qt::NoBrush);

        QColor pc((part->mute() || item->isMoving())? Qt::white : partColor);
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
            doff = (xdiff / 2) % 5;
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
            QRect tr = mbbr;
            tr.setX(tr.x() + 3);
            p.setFont(MusEGlobal::config.fonts[2]);
            p.setPen(Qt::black);
            p.drawText(tr.translated(1, 1), Qt::AlignBottom|Qt::AlignLeft, part->name());
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
        QColor partColor;
        if (part->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
            partColor = part->track()->color();
        else
            partColor = MusEGlobal::config.partColors[part->colorIndex()];
        QColor c(part->mute() ? Qt::white : partColor);
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
  unsigned int pt_len = 0;
  
  {
    int part_r, part_g, part_b, brightness;

    if(pt)
    {
      pt_len = pt->lenTick();
      if (pt->colorIndex() == 0 && MusEGlobal::config.useTrackColorForParts)
          pt->track()->color().getRgb(&part_r, &part_g, &part_b);
      else
          MusEGlobal::config.partColors[pt->colorIndex()].getRgb(&part_r, &part_g, &part_b);
    }
    else {
      if(curColorIndex == 0 && MusEGlobal::config.useTrackColorForParts)
        mt->color().getRgb(&part_r, &part_g, &part_b);
      else
        MusEGlobal::config.partColors[curColorIndex].getRgb(&part_r, &part_g, &part_b);
    }

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
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
                          if((int)i->first < 0 /*|| (int)i->first < from*/)
                            continue;
                          if((pt && (int)i->first >= (int)pt_len) /*|| (int)i->first >= to*/)
                            break;
#endif
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
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
            if((int)i->first < 0 /*|| (int)i->first < from*/)
              continue;
            if((pt && (int)i->first >= (int)pt_len) /*|| (int)i->first >= to*/)
              break;
#endif
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  if (ctrl_type == MusECore::CTRL_PITCH)
                  {
                    int val=i->second.dataB();

                    int th = int(mt->height() * 0.75); // only draw on three quarters
                    int hoffset = (mt->height() - th ) / 2; // offset from bottom

                    p.drawLine(t, hoffset + r.y() + th/2, t, hoffset + r.y() - val*th/8192/2 + th/2);
                  }
            }
      }

      pen.setColor(QColor(192,color_brightness/2,color_brightness/2));
      p.setPen(pen);
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // PAN
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
            if((int)i->first < 0 /*|| (int)i->first < from*/)
              continue;
            if((pt && (int)i->first >= (int)pt_len) /*|| (int)i->first >= to*/)
              break;
#endif
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  if (ctrl_type == 10)
                  {
                    int val=i->second.dataB();

                    int th = int(mt->height() * 0.75); // only draw on three quarters
                    int hoffset = (mt->height() - th ) / 2; // offset from bottom

                    p.drawLine(t, hoffset + r.y() + th - val*th/127, t, hoffset + r.y() + th);
                  }
            }
      }

      pen.setColor(QColor(color_brightness/2,192,color_brightness/2));
      p.setPen(pen);
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // VOLUME
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
            if((int)i->first < 0 /*|| (int)i->first < from*/)
              continue;
            if((pt && (int)i->first >= (int)pt_len) /*|| (int)i->first >= to*/)
              break;
#endif
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  if (ctrl_type == 7)
                  {
                    int val=i->second.dataB();

                    int th = int(mt->height() * 0.75); // only draw on three quarters
                    int hoffset = (mt->height() - th ) / 2; // offset from bottom

                    p.drawLine(t, hoffset + r.y() + th - val*th/127, t, hoffset + r.y() + th);
                  }
            }
      }

      pen.setColor(QColor(0,0,255));
      p.setPen(pen);
      for (MusECore::ciEvent i = events.begin(); i != ito; ++i) { // PROGRAM CHANGE
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
            if((int)i->first < 0 /*|| (int)i->first < from*/)
              continue;
            if((pt && (int)i->first >= (int)pt_len) /*|| (int)i->first >= to*/)
              break;
#endif
            int t  = i->first + pTick;

            MusECore::EventType type = i->second.type();
            if (type == MusECore::Controller) {
                  int ctrl_type=i->second.dataA();
                  if (ctrl_type == MusECore::CTRL_PROGRAM)
                  {
                    int th = int(mt->height() * 0.75); // only draw on three quarters
                    int hoffset = (mt->height() - th ) / 2; // offset from bottom

                    p.drawLine(t, hoffset + r.y(), t, hoffset + r.y() + th);
                  }                  
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
#ifdef ALLOW_LEFT_HIDDEN_EVENTS
            if((int)i->first < 0 /*|| (int)i->first < from*/)
              continue;
            if((pt && (int)i->first >= (int)pt_len) /*|| (int)i->first >= to*/)
              break;
#endif
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

void PartCanvas::drawWaveSndFile(QPainter &p, MusECore::SndFileR &f, int samplePos, unsigned rootFrame, unsigned startFrame, unsigned lengthFrames, int startY, int startX, int endX, int rectHeight, bool selected)
{
   const int endY = startY + rectHeight;
   const int h = rectHeight >> 1;
   const int x1 = startX;
   const int x2 = endX;
   if((int)startFrame + (int)lengthFrames < 0)
     return;
   
   int hiddenOffset = 0;
   int evAbsFrameLim = rootFrame + startFrame;
   if(evAbsFrameLim < 0)
   {
     hiddenOffset = -evAbsFrameLim;
     evAbsFrameLim = 0;
   }
   
   int postick = MusEGlobal::tempomap.frame2tick(evAbsFrameLim);
   
   const int event_x = mapx(postick);
   const int event_ex = mapx(MusEGlobal::tempomap.frame2tick(rootFrame + startFrame + lengthFrames));
   if(event_x >= x2 || event_ex < x1)
     return;
   int sx = event_x;
   int ex = event_ex;
   if(sx < x1)
     sx = x1;
   if(ex > x2)
     ex = x2;

   // Whether space permits displaying all channels or combining them into one.
   const bool multichan_disp = h >= 20;
   const int center = startY + rectHeight / 2;
   const QColor left_ch_color(0, 170, 255);
   const QColor right_ch_color(Qt::red);
   const QColor combo_ch_color(220, 120, 255);

  int xScale;
  int pos;
  int tickstep = rmapxDev(1);
  int drawoffset;
  if((x1 - event_x) < 0) {
    drawoffset = 0;
  }
  else {
    drawoffset = rmapxDev(x1 - event_x);
  }
  postick += drawoffset;
  pos = MusEGlobal::tempomap.tick2frame(postick) - rootFrame - startFrame;

  QPen pen;
  pen.setCosmetic(true);
  
  unsigned channels = 0;
  int wav_sx = 0;
  int wav_ex = 0;
  int wsx = 0;
  int wex = 0;
  bool wave_visible = false;

  if(!f.isNull())
  {
    channels = f.channels();
    if(channels > 0)
    {
      const sf_count_t smps = f.samples();

      if(-samplePos < smps && samplePos <= smps)
      {
        wave_visible = true;
        wav_sx = -samplePos;
        wav_ex = smps - samplePos - hiddenOffset;
        if(wav_sx < 0)
          wav_sx = 0;
        wav_sx += evAbsFrameLim;
        wav_sx = MusEGlobal::tempomap.frame2tick(wav_sx);

        wav_ex = f.unConvertPosition(wav_ex);
        if(wav_ex >= (int)lengthFrames - hiddenOffset)
        {
          wav_ex = lengthFrames - hiddenOffset;
          if(wav_ex > 0)
            --wav_ex;
        }
        wav_ex += evAbsFrameLim;
        wav_ex = MusEGlobal::tempomap.frame2tick(wav_ex);

        wav_sx = mapx(wav_sx);
        wav_ex = mapx(wav_ex);
        wsx = wav_sx < x1 ? x1 : wav_sx;
        wex = wav_ex > x2 ? x2 : wav_ex;
      }

      if (!multichan_disp) {
            //    combine multi channels into one waveform
            int y = startY + h;
            int cc = rectHeight % 2 ? 0 : 1;
            for (int i = sx; i < ex; ++i) {
                  MusECore::SampleV sa[channels];
                  xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
                  if((samplePos + f.convertPosition(pos)) > smps)
                    break;
                  // Seek the file only once, not with every read!
                  if(i == sx)
                  {
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

                  // Only if there's something to draw.
                  if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
                  {  
                    // If two (or more) channels combine blue (cyan) and red into say, magenta.
                    pen.setColor(QColor(channels > 1 ? combo_ch_color : left_ch_color));
                    p.setPen(pen);
                    p.drawLine(wsx, center, wex, center);
                  }
            }
      else {
            //  multi channel display
            int hm = rectHeight / (channels * 2);
            int cc = rectHeight % (channels * 2) ? 0 : 1;
            for (int i = sx; i < ex; ++i) {
                  int y  = startY + hm;
                  MusECore::SampleV sa[channels];
                  xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
                  if((samplePos + f.convertPosition(pos)) > smps)
                    break;
                  // Seek the file only once, not with every read!
                  if(i == sx)
                  {
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

                  // Only if there's something to draw.
                  if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
                  {  
                    const int hn = rectHeight / channels;
                    const int hhn = hn / 2;
                    for (unsigned int i = 0; i < channels; ++i) {
                          const int h2     = hn * i;
                          const int finY = startY + hhn + h2;
                          if(finY >= startY && finY < endY)
                          {
                            pen.setColor(QColor(i & 1 ? right_ch_color : left_ch_color));
                            p.setPen(pen);
                            p.drawLine(wsx, finY, wex, finY);
                          }
                        }
                  }
            }
    }
   }

  pen.setColor(selected ? Qt::white : Qt::black);
  QVector<qreal> customDashPattern;
  customDashPattern << 1.0 << 2.0;
  pen.setDashPattern(customDashPattern);
  const int pix_per_seg = 21;

  //fprintf(stderr, "x1:%d x2:%d startY:%d endY:%d rectHeight:%d y:%d h:%d event_x:%d event_ex:%d"
  //                " rootFrame:%d startFrame:%d lengthFrames:%d sx:%d ex:%d wav_sx:%d wav_ex:%d wsx:%d wex:%d\n",
  //        x1, x2, startY, endY, rectHeight, y(), height(), event_x, event_ex,
  //        rootFrame, startFrame, lengthFrames, sx, ex, wav_sx, wav_ex, wsx, wex);

  // Draw the complete line only if there are an even number of channels (space for the line in the middle).
  // Ensure a complete line is drawn even if there is no sound file or channels.
  if((channels & 1) == 0 && multichan_disp)
  {
    if(sx > event_x)
    {
      //fprintf(stderr, "Single line. Setting dash offset:%d\n", (sx - event_x) % 3);
      pen.setDashOffset((sx - event_x) % 3);
    }
    p.setPen(pen);
    MusECore::drawSegmentedHLine(&p, sx, ex, center, pix_per_seg);
  }
  else
  {
    // Draw only the required two segments of the line.
    if(wave_visible)
    {
      if(sx < wsx)
      {
        if(sx > event_x)
        {
          //fprintf(stderr, "Line segment 1. Setting dash offset:%d\n", (sx - event_x) % 3);
          pen.setDashOffset((sx - event_x) % 3);
        }
        p.setPen(pen);
        MusECore::drawSegmentedHLine(&p, sx, wsx - 1, center, pix_per_seg);
      }
      if(wex < ex)
      {
        if(sx > wex + 1)
        {
          //fprintf(stderr, "Line segment 2. Setting dash offset:%d\n", (sx - wex + 1) % 3);
          pen.setDashOffset((sx - wex + 1) % 3);
        }
        p.setPen(pen);
        MusECore::drawSegmentedHLine(&p, wex + 1, ex, center, pix_per_seg);
      }
    }
  }

  //
  // Draw custom dashed borders around the wave event
  //

  if(endY > startY)
  {
    // Reset offset back to zero.
    pen.setDashOffset(0);
    p.setPen(pen);
    // Left line:
    if(event_x >= x1 && event_x <= x2)
      MusECore::drawSegmentedVLine(&p, event_x, startY, endY - 1, pix_per_seg);
    // Right line:
    if(event_ex >= x1 && event_ex <= x2)
      MusECore::drawSegmentedVLine(&p, event_ex, startY, endY - 1, pix_per_seg);
  }
}

// TODO REMOVE Tim. wave. An attempt to re-write this method, in progress...
// void PartCanvas::drawWaveSndFile(QPainter &p, MusECore::SndFileR &f, int samplePos, unsigned rootFrame, unsigned startFrame, unsigned lengthFrames, int startY, int startX, int endX, int rectHeight, bool selected)
// {
//    const int endY = startY + rectHeight;
//    const int h = rectHeight >> 1;
//    const int x1 = startX;
//    const int x2 = endX;
//    const int md_x1 = mapxDev(startX);
//    const int md_x2 = mapxDev(endX);
//    const int md_x1_fr = MusEGlobal::tempomap.tick2frame(md_x1);
//    const int md_x2_fr = MusEGlobal::tempomap.tick2frame(md_x2);
//    const int event_abs_fr = rootFrame + startFrame;
//    const int event_abs_e_fr = event_abs_fr + lengthFrames;
//    if(event_abs_fr >= md_x2_fr || event_abs_e_fr < md_x1_fr)
//      return;
//    
//    int postick = MusEGlobal::tempomap.frame2tick(rootFrame + startFrame);
//    const int event_x = mapx(postick);
//    const int event_ex = mapx(MusEGlobal::tempomap.frame2tick(rootFrame + startFrame + lengthFrames));
//    if(event_x >= x2 || event_ex < x1)
//      return;
// //    int sx = event_x;
// //    int ex = event_ex;
// //    if(sx < x1)
// //      sx = x1;
// //    if(ex > x2)
// //      ex = x2;
// 
//    int ev_s_fr = event_abs_fr;
//    int ev_e_fr = event_abs_e_fr;
//    if(ev_s_fr < md_x1_fr)
//      ev_s_fr = md_x1_fr;
//    if(ev_e_fr > md_x2_fr)
//      ev_e_fr = md_x2_fr;
// 
//    int sx = mapx(MusEGlobal::tempomap.frame2tick(ev_s_fr));
//    int ex = mapx(MusEGlobal::tempomap.frame2tick(ev_e_fr));
// 
//    
//    // Whether space permits displaying all channels or combining them into one.
//    const bool multichan_disp = h >= 20;
//    const int center = startY + rectHeight / 2;
//    const QColor left_ch_color(0, 170, 255);
//    const QColor right_ch_color(Qt::red);
//    const QColor combo_ch_color(220, 120, 255);
// 
//   int xScale;
//   int pos;
//   int tickstep = rmapxDev(1);
//   int drawoffset;
//   if((x1 - event_x) < 0) {
//     drawoffset = 0;
//   }
//   else {
//     drawoffset = rmapxDev(x1 - event_x);
//   }
//   postick += drawoffset;
//   pos = MusEGlobal::tempomap.tick2frame(postick) - rootFrame - startFrame;
// 
//   QPen pen;
//   pen.setCosmetic(true);
//   
//   unsigned channels = 0;
//   int wav_sx = 0;
//   int wav_ex = 0;
//   int wsx = 0;
//   int wex = 0;
//   bool wave_visible = false;
// 
//   if(!f.isNull())
//   {
//     channels = f.channels();
//     if(channels > 0)
//     {
//       const sf_count_t smps = f.samples();
// 
//       if(-samplePos < smps && samplePos <= smps)
//       {
//         wave_visible = true;
//         wav_sx = -samplePos;
//         wav_ex = smps - samplePos;
//         if(wav_sx < 0)
//           wav_sx = 0;
//         wav_sx += startFrame + rootFrame;
//         wav_sx = MusEGlobal::tempomap.frame2tick(wav_sx);
// 
//         wav_ex = f.unConvertPosition(wav_ex);
//         if(wav_ex >= (int)lengthFrames)
//         {
//           wav_ex = lengthFrames;
//           if(wav_ex > 0)
//             --wav_ex;
//         }
//         wav_ex += startFrame + rootFrame;
//         wav_ex = MusEGlobal::tempomap.frame2tick(wav_ex);
// 
//         wav_sx = mapx(wav_sx);
//         wav_ex = mapx(wav_ex);
//         wsx = wav_sx < x1 ? x1 : wav_sx;
//         wex = wav_ex > x2 ? x2 : wav_ex;
//       }
// 
//       if (!multichan_disp) {
//             //    combine multi channels into one waveform
//             int y = startY + h;
//             int cc = rectHeight % 2 ? 0 : 1;
//             for (int i = sx; i < ex; ++i) {
//                   MusECore::SampleV sa[channels];
//                   xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
//                   if((samplePos + f.convertPosition(pos)) > smps)
//                     break;
//                   // Seek the file only once, not with every read!
//                   if(i == sx)
//                   {
//                     if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, samplePos) == -1)
//                       break;
//                   }
//                   f.readConverted(sa, xScale, pos, samplePos, true, false);
// 
//                   postick += tickstep;
//                   pos += xScale;
//                   int peak = 0;
//                   int rms  = 0;
//                   for (unsigned k = 0; k < channels; ++k) {
//                         if (sa[k].peak > peak)
//                               peak = sa[k].peak;
//                         rms += sa[k].rms;
//                         }
//                   rms /= channels;
//                   peak = (peak * (rectHeight-2)) >> 9;
//                   rms  = (rms  * (rectHeight-2)) >> 9;
//                   int outer = peak;
//                   int inner = peak -1; //-1 < 0 ? 0 : peak -1;
//                   pen.setColor(MusEGlobal::config.partWaveColorPeak);
//                   p.setPen(pen);
//                   p.drawLine(i, y - outer - cc, i, y + outer);
//                   pen.setColor(MusEGlobal::config.partWaveColorRms);
//                   p.setPen(pen);
//                   if (MusEGlobal::config.waveDrawing == MusEGlobal::WaveRmsPeak)
//                     p.drawLine(i, y - rms - cc, i, y + rms);
//                   else // WaveOutLine
//                     p.drawLine(i, y - inner - cc, i, y + inner);
//                   }
// 
//                   // Only if there's something to draw.
//                   if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
//                   {  
//                     // If two (or more) channels combine blue (cyan) and red into say, magenta.
//                     pen.setColor(QColor(channels > 1 ? combo_ch_color : left_ch_color));
//                     p.setPen(pen);
//                     p.drawLine(wsx, center, wex, center);
//                   }
//             }
//       else {
//             //  multi channel display
//             int hm = rectHeight / (channels * 2);
//             int cc = rectHeight % (channels * 2) ? 0 : 1;
//             for (int i = sx; i < ex; ++i) {
//                   int y  = startY + hm;
//                   MusECore::SampleV sa[channels];
//                   xScale = MusEGlobal::tempomap.deltaTick2frame(postick, postick + tickstep);
//                   if((samplePos + f.convertPosition(pos)) > smps)
//                     break;
//                   // Seek the file only once, not with every read!
//                   if(i == sx)
//                   {
//                     if(f.seekUIConverted(pos, SEEK_SET | SFM_READ, samplePos) == -1)
//                       break;
//                   }
//                   f.readConverted(sa, xScale, pos, samplePos, true, false);
// 
//                   postick += tickstep;
//                   pos += xScale;
//                   for (unsigned k = 0; k < channels; ++k) {
//                         int peak = (sa[k].peak * (hm - 1)) >> 8;
//                         int rms  = (sa[k].rms  * (hm - 1)) >> 8;
//                         int outer = peak;
//                         int inner = peak -1; //-1 < 0 ? 0 : peak -1;
//                         pen.setColor(MusEGlobal::config.partWaveColorPeak);
//                         p.setPen(pen);
//                         p.drawLine(i, y - outer - cc , i, y + outer);
//                         pen.setColor(MusEGlobal::config.partWaveColorRms);
//                         p.setPen(pen);
//                         if (MusEGlobal::config.waveDrawing == MusEGlobal::WaveRmsPeak)
//                           p.drawLine(i, y - rms - cc, i, y + rms);
//                         else // WaveOutLine
//                           p.drawLine(i, y - inner - cc, i, y + inner);
//                         y  += 2 * hm;
//                         }
//                   }
// 
//                   // Only if there's something to draw.
//                   if(wave_visible && wsx <= wex && wsx < x2 && wex >= x1)
//                   {  
//                     const int hn = rectHeight / channels;
//                     const int hhn = hn / 2;
//                     for (unsigned int i = 0; i < channels; ++i) {
//                           const int h2     = hn * i;
//                           const int finY = startY + hhn + h2;
//                           if(finY >= startY && finY < endY)
//                           {
//                             pen.setColor(QColor(i & 1 ? right_ch_color : left_ch_color));
//                             p.setPen(pen);
//                             p.drawLine(wsx, finY, wex, finY);
//                           }
//                         }
//                   }
//             }
//     }
//    }
// 
//   pen.setColor(selected ? Qt::white : Qt::black);
//   QVector<qreal> customDashPattern;
//   customDashPattern << 1.0 << 2.0;
//   pen.setDashPattern(customDashPattern);
//   const int pix_per_seg = 21;
// 
//   //fprintf(stderr, "x1:%d x2:%d startY:%d endY:%d rectHeight:%d y:%d h:%d event_x:%d event_ex:%d"
//   //                " rootFrame:%d startFrame:%d lengthFrames:%d sx:%d ex:%d wav_sx:%d wav_ex:%d wsx:%d wex:%d\n",
//   //        x1, x2, startY, endY, rectHeight, y(), height(), event_x, event_ex,
//   //        rootFrame, startFrame, lengthFrames, sx, ex, wav_sx, wav_ex, wsx, wex);
// 
//   // Draw the complete line only if there are an even number of channels (space for the line in the middle).
//   // Ensure a complete line is drawn even if there is no sound file or channels.
//   if((channels & 1) == 0 && multichan_disp)
//   {
//     if(sx > event_x)
//     {
//       //fprintf(stderr, "Single line. Setting dash offset:%d\n", (sx - event_x) % 3);
//       pen.setDashOffset((sx - event_x) % 3);
//     }
//     p.setPen(pen);
//     MusECore::drawSegmentedHLine(&p, sx, ex, center, pix_per_seg);
//   }
//   else
//   {
//     // Draw only the required two segments of the line.
//     if(wave_visible)
//     {
//       if(sx < wsx)
//       {
//         if(sx > event_x)
//         {
//           //fprintf(stderr, "Line segment 1. Setting dash offset:%d\n", (sx - event_x) % 3);
//           pen.setDashOffset((sx - event_x) % 3);
//         }
//         p.setPen(pen);
//         MusECore::drawSegmentedHLine(&p, sx, wsx - 1, center, pix_per_seg);
//       }
//       if(wex < ex)
//       {
//         if(sx > wex + 1)
//         {
//           //fprintf(stderr, "Line segment 2. Setting dash offset:%d\n", (sx - wex + 1) % 3);
//           pen.setDashOffset((sx - wex + 1) % 3);
//         }
//         p.setPen(pen);
//         MusECore::drawSegmentedHLine(&p, wex + 1, ex, center, pix_per_seg);
//       }
//     }
//   }
// 
//   //
//   // Draw custom dashed borders around the wave event
//   //
// 
//   if(endY > startY)
//   {
//     // Reset offset back to zero.
//     pen.setDashOffset(0);
//     p.setPen(pen);
//     // Left line:
//     if(event_x >= x1 && event_x <= x2)
//       MusECore::drawSegmentedVLine(&p, event_x, startY, endY - 1, pix_per_seg);
//     // Right line:
//     if(event_ex >= x1 && event_ex <= x2)
//       MusECore::drawSegmentedVLine(&p, event_ex, startY, endY - 1, pix_per_seg);
//   }
// }

//---------------------------------------------------------
//   drawWavePart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void PartCanvas::drawWavePart(QPainter& p,
   const QRect& bb, MusECore::WavePart* wp, const QRect& _pr, bool selected)
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
//           if (drag == DRAG_RESIZE && resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_RIGHT && reverseIterator == wp->events().rbegin())
//           {
//               // the last event is subject to live extending
// 
//               // _pr is in tick resolution
//               auto endFrame = MusEGlobal::tempomap.tick2frame(_pr.width());
// 
//               // we're at the last event, extend the wave drawing so it is displayed past the old end of the part
//               drawWaveSndFile(p, f, event.spos(), wp->frame(), event.frame(), endFrame, startY, x1, x2, hh, selected);
//           }
//           else
          if (drag == DRAG_RESIZE && resizeDirection == MusECore::ResizeDirection::RESIZE_TO_THE_LEFT)
          {
//              auto startFrame = MusEGlobal::tempomap.tick2frame(_pr.)
              drawWaveSndFile(p, f, event.spos(), wp->frame(), event.frame(), event.lenFrame(), startY, x1, x2, hh, selected);
          }
          else
          {
              drawWaveSndFile(p, f, event.spos(), wp->frame(), event.frame(), event.lenFrame(), startY, x1, x2, hh, selected);
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
            // Use only selected parts, except that the copy_in_range() method
            //  wants ALL of the parts so that it can decide what to include.
            if (!i->second->isSelected() && (cmd != CMD_COPY_PART_IN_RANGE))
                  continue;
            NPart* npart = (NPart*)(i->second);
            pl.add(npart->part());
            }
      switch (cmd) {
            case CMD_DELETE:
            {
                  MusECore::Undo operations;
                  for (iCItem i = items.begin(); i != items.end(); ++i) {
                        if (i->second->isSelected()) {
                              NPart* p = (NPart*)(i->second);
                              MusECore::Part* part = p->part();
                              operations.push_back(MusECore::UndoOp(MusECore::UndoOp::DeletePart, part));
                              }
                      }
                  MusECore::delete_selected_audio_automation(operations);
                  MusEGlobal::song->applyOperationGroup(operations);
                  break;
            }
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
                  MusECore::delete_selected_audio_automation(operations);
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
//   copy_in_range
//---------------------------------------------------------

void PartCanvas::copy_in_range(MusECore::PartList* pl_)
{
  //---------------------------------------------------
  //    write parts as XML into tmp file
  //---------------------------------------------------

  FILE* tmp = tmpfile();
  if (tmp == 0) {
        fprintf(stderr, "PartCanvas::copy() fopen failed: %s\n",
            strerror(errno));
        return;
        }

  MusECore::PartList pl;
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

  MusECore::Xml xml(tmp);

  bool havePartData = false;

  MusECore::XmlWriteStatistics stats;
  int level = 0;
  int tick = 0;

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

        if(!havePartData)
          havePartData = true;

        // Indicate this is a copy operation. Also force full wave paths.
        part->write(level, xml, true, true, &stats);

        int endTick = part->endTick();
        if (endTick > tick)
              tick = endTick;
      }
    }
  }

  // Copy automation between range, on all tracks.
  bool haveACData = copyAudioAutomation(
        level, xml,
        // useAllTracks
        true,
        // useRange
        true,
        // specificTrack
        nullptr,
        // Range position p0
        MusEGlobal::song->lPos(),
        // Range position p1
        MusEGlobal::song->rPos());

  if(havePartData)
  {
    MusECore::Pos p(tick, true);
    MusEGlobal::song->setPos(MusECore::Song::CPOS, p);
  }

  if(havePartData || haveACData)
  {
    QString mimeString = "text/x-muse-mixedpartlist";
    QMimeData *mimeData =  MusECore::file_to_mimedata(tmp, mimeString );
    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
  }

  fclose(tmp);
}

//---------------------------------------------------------
//   copy
//---------------------------------------------------------

void PartCanvas::copy(MusECore::PartList* pl)
      {
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

      bool havePartData = false;

      MusECore::XmlWriteStatistics stats;
      int level = 0;
      int tick = 0;
      // Copy the given parts.
      for (MusECore::ciPart p = pl->begin(); p != pl->end(); ++p) {
            if(!havePartData)
            {
              havePartData = true;
            }
            const MusECore::Part* part = p->second;
            // Indicate this is a copy operation. Also force full wave paths.
            part->write(level, xml, true, true, &stats);

            int endTick = part->endTick();
            if (endTick > tick)
                  tick = endTick;
            }

      // Copy selected automation on all tracks.
      bool haveACData = copyAudioAutomation(
            level, xml,
            // useAllTracks
            true,
            // useRange
            false);

      if(havePartData)
      {
        MusECore::Pos p(tick, true);
        MusEGlobal::song->setPos(MusECore::Song::CPOS, p);
      }

      if(havePartData || haveACData)
      {
        QString mimeString = "text/x-muse-mixedpartlist";
        QMimeData *mimeData =  MusECore::file_to_mimedata(tmp, mimeString );
        QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
      }
      fclose(tmp);
      }

//---------------------------------------------------------
//   copyAudioAutomation
//---------------------------------------------------------

bool PartCanvas::copyAudioAutomation(
  int level, MusECore::Xml& xml,
  bool useAllTracks,
  bool useRange,
  const MusECore::Track* specificTrack,
  const MusECore::Pos& p0, const MusECore::Pos& p1)
{
  bool haveData = false;
  for (MusECore::ciTrack it = tracks->cbegin(); it != tracks->cend(); ++it) {
    if((*it)->isMidiTrack())
      continue;
    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*it);
    // Do not include hidden tracks.
    if (!track->isVisible() || (!useAllTracks && ((specificTrack && track != specificTrack) || (!specificTrack && !track->selected()))))
      continue;
    const MusECore::CtrlListList* cll = track->controller();
    bool clFound = false;
    for(MusECore::ciCtrlList icll = cll->cbegin(); icll != cll->cend(); ++icll)
    {
      MusECore::CtrlList* cl = icll->second;
      // Do not include hidden controller lists.
      if(!cl->isVisible())
        continue;
      int i = 0;
      bool itemFound = false;
      for(MusECore::ciCtrl ic = cl->cbegin(); ic != cl->cend(); ++ ic)
      {
        const unsigned int frame = ic->first;
        const MusECore::CtrlVal& cv = ic->second;
        // Include only selected controller values, or controller items within positional range.
        if((useRange && (frame < p0.frame() || frame >= p1.frame())) || (!useRange && !cv.selected()))
          continue;

        if(!clFound)
        {
          // We want to be able to paste to the original tracks, yet have the flexibility to
          //  allow pasting to different tracks or even different controller numbers, since
          //  the tracks may have moved around before pasting, or we may be loading the xml
          //  contents from a file that was saved (via clipboard) from a completely different song.
          // For now just do the very least that is required: Remember the track index so that
          //  the paste side can have some kind of indexing and ordering to work with.
          // This way at least basic HORIZONTAL drag and drop or copy and paste should work.
          const QString s= QString("audioTrackAutomation trackUuid=\"%1\"").arg(track->uuid().toString());
          xml.tag(level++, s.toLatin1().constData());
          clFound = true;
        }
        if(!itemFound)
        {
          // Store some information about the controller so that the paste side can have some
          //  idea of how to match up the source and target.
          // The types probably need to match but at least we could use the range min and max
          //  to scale the contents on paste, although for now we'll likely just require them to match.
          //
          // We have no mechanism by which a user could paste to DIFFERENT controller numbers
          //  (but having similar properties).
          //
          // And consider what happens when copying the stuff to the clipboard, then say, moving a
          //  track or a rack plugin up or down, then attempting to paste.
          // What then? Where and how to paste?
          //
          // Ultimately, since allowing pasting from some completely different source or conditions
          //  makes it IMPOSSIBLE to guess where to paste, a special paste dialog could be shown
          //  where the target destinations could be chosen. (A sort of paste 'router'?)
          //
          // Store the current samplerate of these values so that the reader can convert.
          const QString s= QString("controller id=\"%1\" valueType=\"%2\" min=\"%3\" max=\"%4\" samplerate=\"%5\"")
              .arg(cl->id()).arg(cl->valueType()).arg(cl->minVal()).arg(cl->maxVal()).arg(MusEGlobal::sampleRate);
          xml.tag(level++, s.toLatin1().constData());

          itemFound = true;
        }

        // Write the item's frame and value.
        QString s = QString("%1 %2").arg(ic->first).arg(ic->second.value());
        MusECore::CtrlVal::CtrlValueFlags flags = ic->second.flags();
        // Strip out the group flag because we're determining it here.
        flags &= ~MusECore::CtrlVal::VAL_NON_GROUP_END;

        // Get a look-ahead iterator to the next item.
        MusECore::ciCtrl icla = ic;
        ++icla;

        // Look-ahead is at end? Force a group end.
        // Else if the next item will NOT be used, ie. not selected or within positional range,
        //  close out the group with a group end. Note the inverted logic (NON) of the actual flag.
        if(icla != cl->cend() &&
           (!useRange || (icla->first >= p0.frame() && icla->first < p1.frame())) &&
           (useRange || icla->second.selected()))
          flags |= MusECore::CtrlVal::VAL_NON_GROUP_END;
        if(flags != MusECore::CtrlVal::VAL_NOFLAGS)
          s += QString(" %1").arg(flags);

        s += QString(", ");

        xml.nput(level, s.toLatin1().constData());
        ++i;
        if (i >= 4) {
          xml.put(level, "");
          i = 0;
        }
        haveData = true;
      }

      if(itemFound)
      {
        if(i)
          xml.put(level, "");
        xml.etag(level--, "controller");
      }

    }
    if(clFound)
      xml.etag(level--, "audioTrackAutomation");
  }

  return haveData;
}

void PartCanvas::pasteAt(MusECore::Undo& operations, const QString& pt, MusECore::Track* track, unsigned int pos,
                                   bool clone, bool toTrack, unsigned int* finalPosPtr, set<MusECore::Track*>* affected_tracks)
      {
      MusECore::XmlReadStatistics stats;
      MusECore::PasteCtrlTrackMap pctm;
      std::set<MusECore::Part*> partList;

      unsigned int minPos = 0;
      bool minPosValid = false;
      parseArrangerPasteXml(pt, track, clone, toTrack, affected_tracks, &partList, &stats, &pctm, &minPos, &minPosValid);
      if(minPosValid)
      {
        processArrangerPasteObjects(operations, pos, finalPosPtr, &partList, &pctm, minPos);
      }
      else
      {
        for(std::set<MusECore::Part*>::const_iterator ip = partList.cbegin(); ip != partList.cend(); ++ip)
          delete *ip;
      }
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

        MusECore::Undo operations;
        deselectAll(&operations);

        for (int i=0;i<amount;i++)
          pasteAt(operations, txt, track, startPos + i*raster, clone, to_single_track, &endPos, &affected_tracks);

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
                  MusECore::Undo temp;
                  deselectAll(&temp);
                  pasteAt(temp, text, track, x);
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
        
        drawTickRaster(p, mr, mrg, rast,
                         false, false, false,
                       MusEGlobal::config.partCanvasBeatRasterColor,
                       MusEGlobal::config.partCanvasBeatRasterColor,
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
            
            if((MusEGlobal::config.canvasShowGrid || MusEGlobal::config.canvasShowGridHorizontalAlways) &&
               (track->isMidiTrack() || track->type() == MusECore::Track::WAVE))
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
                  drawAutomationFills(p, r, (MusECore::AudioTrack*)track);
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
           QColor c;
           if(curColorIndex == 0 && MusEGlobal::config.useTrackColorForParts)
             c = track->color();
           else
             c = MusEGlobal::config.partColors[curColorIndex];
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
                       drawWaveSndFile(p, fp, 0, _startFrame, 0, _lengthFrame, yPos, startx, startx + width, th, false);
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
          p.fillRect(mbr_gr, MusECore::getGradientFromColor(c, mbb_gr.topLeft(), mbb_gr.bottomLeft(),
                                        qBound(0, MusEGlobal::config.partGradientStrength, 200)));
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
//   drawAutomationFills
//---------------------------------------------------------

void PartCanvas::drawAutomationFills(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
  const bool moveModeBegun = MusEGlobal::song->audioCtrlMoveModeBegun();
  const bool isdrag = drag == DRAG_MOVE || drag == DRAGX_MOVE || drag == DRAGY_MOVE ||
    drag == DRAG_COPY || drag == DRAGX_COPY || drag == DRAGY_COPY;

  // Nothing to do?
  if(!moveModeBegun && !isdrag)
    return;

  // Note that QRect::bottom() is y() + height() - 1.
  const int bottom = rr.bottom() - _automationBottomMargin;
  const int top = rr.top() + _automationTopMargin;
  const int height = bottom - top;

  p.setBrush(Qt::NoBrush);

  const bool pasteNoErase    = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteNoErase;
  const bool pasteEraseRange = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteEraseRange;

  const MusECore::CtrlListList* cll = t->controller();
  const MusECore::CtrlListList* e_cll = t->erasedController();

  const MusECore::AudioAutomationItemMap* aaim = nullptr;
  if(isdrag)
  {
    MusECore::ciAudioAutomationItemTrackMap iaitm = automation.currentCtrlFrameList.find(t);
    if(iaitm != automation.currentCtrlFrameList.cend())
      aaim = &iaitm->second;
  }

  for(MusECore::CtrlListList::const_iterator icll =cll->cbegin();icll!=cll->cend();++icll)
  {
    int oldX = mapx(0);
    if(oldX > rr.right())
    {
      //p.restore();
      return;
    }

    MusECore::CtrlList *cl = icll->second;
    if (cl->dontShow() || !cl->isVisible())
      continue;

    int newX = oldX;
    int oldY = -1;

    double min, max;
    cl->range(&min,&max);
    //const bool discrete = cl->mode() == MusECore::CtrlList::DISCRETE;

    // Start the oldY off with the current value.
    {
      double startVal = cl->curVal();
      if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
        startVal = logToVal(startVal, min, max); // represent volume between 0 and 1
        if (startVal < 0) startVal = 0.0;
      }
      else {
        startVal = (startVal - min)/(max-min);  // we need to set curVal between 0 and 1
      }
      oldY = bottom - rmapy_f(startVal) * height;
    }
    int newY = oldY;

    p.setPen(Qt::NoPen);
    QColor c(cl->color());
    // Make the alpha highly transparent, almost invisible.
    c.setAlpha(20);
    p.setBrush(c);

    const MusECore::AudioAutomationItemList* aail = nullptr;
    if(isdrag && aaim)
    {
      MusECore::ciAudioAutomationItemMap iaaim = aaim->find(cl->id());
      if(iaaim != aaim->cend())
        aail = &iaaim->second._selectedList;
    }

    const bool have_aail = aail && !aail->empty();
    MusECore::ciAudioAutomationItemList iaail;
    if(have_aail)
      iaail = aail->cbegin();

    MusECore::ciCtrl ic = cl->cbegin();

    const MusECore::CtrlList *e_cl = nullptr;
    MusECore::ciCtrl ic_e;
    if(isdrag)
    {
      MusECore::ciCtrlList ie_cll = e_cll->find(cl->id());
      if(ie_cll != e_cll->cend())
      {
        e_cl = ie_cll->second;
        ic_e = e_cl->begin();
      }
    }

    bool isFirstPoint = true;
    bool in_group = false;

    while(true)
    {
      // Nothing (left) to do?
      if((!have_aail || iaail == aail->cend()) &&
        ic == cl->cend() &&
        (!e_cl || ic_e == e_cl->cend()))
        break;

      unsigned int frame;
      double value;
      bool new_in_group = in_group;
      bool doFills = false;

      // Is the local structure frame equal to or lower than either
      //  the controller or erased controller frame?
      // Then it gets priority.
      if(have_aail && iaail != aail->cend() &&
        (ic == cl->cend() || iaail->second._wrkFrame <= ic->first) &&
        (!e_cl || ic_e == e_cl->cend() || iaail->second._wrkFrame <= ic_e->first))
      {
        if(pasteNoErase)
            new_in_group = false;
        else if(pasteEraseRange)
            new_in_group = true;
        else //if(pasteErase)
            new_in_group = !iaail->second._groupEnd;

        frame = iaail->second._wrkFrame;
        value = iaail->second._wrkVal;
        doFills = true;

        ++iaail;

        // In paste erase range mode if this is the last selected point
        //  reset the in group flag.
        if(pasteEraseRange && iaail == aail->cend())
          new_in_group = false;

        // If there's also a controller value at this location, skip it.
        if(ic != cl->cend() && ic->first == frame)
          ++ic;
        // If there's also an erased controller value at this location, skip it.
        if(e_cl && ic_e != e_cl->cend() && ic_e->first == frame)
          ++ic_e;
      }

      // Is the erased controller frame equal to or lower than the controller frame?
      // Then it gets priority.
      else if(e_cl && ic_e != e_cl->cend() && ic != cl->cend() && ic_e->first <= ic->first)
      {
        // If in pasteErase or pasteEraseRange mode and we're in a group, skip this point.
        if(isdrag && !pasteNoErase && in_group)
        {
//             // If there's also a controller value at this location, use it.
//             if(!in_group && ic != cl->cend() && ic->first == ic_e->first)
//             // If there's a controller value, use it.
//             if(!in_group && ic != cl->cend() && ic->first != ic_e->first)
//             {
//               frame = ic->first;
//               value = ic->second.value();
//               ++ic;
//             }

          ++ic_e;
          continue;
        }

        frame = ic_e->first;
        value = ic_e->second.value();
        ++ic_e;
        // If there's also a controller value at this location, skip it.
        if(ic != cl->cend() && ic->first == ic_e->first)
          ++ic;
      }

      // Otherwise the controller frame gets priority.
      else if(ic != cl->cend())
      {
        // Skip this point if it's not selected, or in pasteErase or pasteEraseRange mode and we're in a group,
        //  or if the point is an original moving point.
        if(!ic->second.selected() || (isdrag && ((!pasteNoErase && in_group) || (have_aail && aail->find(ic->first) != aail->cend()))))
        {
          ++ic;
          continue;
        }

        if(!isdrag && moveModeBegun /*&& ic->second.selected()*/)
        {
            doFills = true;
            if(pasteNoErase)
                new_in_group = false;
            else if(pasteEraseRange)
                new_in_group = true;
            else //if(pasteErase)
                new_in_group = !ic->second.groupEnd();
        }

        frame = ic->first;
        value = ic->second.value();

        ++ic;
        // In paste erase range mode if this is the last point
        //  reset the in group flag.
        if(pasteEraseRange && ic == cl->cend())
          new_in_group = false;
      }
      else
      {
        continue;
      }


      double y = value;
      if (cl->valueType() == MusECore::VAL_LOG ) {
        y = logToVal(y, min, max); // represent volume between 0 and 1
        if (y < 0) y = 0.0;
      }
      else
        y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

      newY = bottom - rmapy_f(y) * height;
      newX = mapx(MusEGlobal::tempomap.frame2tick(frame));

      if(isFirstPoint)
      {
        isFirstPoint = false;
        oldY = newY;
      }

      // Ideally we would like to cut the lines right at the rectangle boundaries.
      // But they might not be drawn exactly the same as the full line would.
      // So we'll also accept anything that started outside the boundaries.
      // A small acceptable speed hit relatively speaking - but far far better than drawing all.
      if(oldX <= rr.right() && newX >= rr.left() && oldY <= bottom && newY >= top)
      {
        if(doFills && in_group && (isdrag || MusEGlobal::song->audioCtrlMoveModeBegun()))
        {
          QPoint ar[4];
          //ar[0] = QPoint(oldX, oldY);
          ar[0] = QPoint(oldX, top);
          ar[1] = QPoint(newX, top);
          ar[2] = QPoint(newX, bottom);
          ar[3] = QPoint(oldX, bottom);
          //if(discrete)
          //  ar[1] = QPoint(newX, oldY);
          //else
          //  ar[1] = QPoint(newX, newY);
          p.drawConvexPolygon(ar, 4);
        }
      }

      oldX = newX;
      oldY = newY;
      in_group = new_in_group;

      if (oldX > rr.right())
        break;
    }
  }
}

void PartCanvas::drawAutomation(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
  // Note that QRect::bottom() is y() + height() - 1.
  const int bottom = rr.bottom() - _automationBottomMargin;
  const int top = rr.top() + _automationTopMargin;
  const int height = bottom - top;

  p.setBrush(Qt::NoBrush);

  const bool moveModeBegun = MusEGlobal::song->audioCtrlMoveModeBegun();
  const bool isdrag = drag == DRAG_MOVE || drag == DRAGX_MOVE || drag == DRAGY_MOVE ||
    drag == DRAG_COPY || drag == DRAGX_COPY || drag == DRAGY_COPY;

  // Whether we are copying while moving.
  const bool isCopying = drag == DRAG_COPY || drag == DRAGX_COPY || drag == DRAGY_COPY;

  const bool pasteNoErase    = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteNoErase;
  const bool pasteEraseRange = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteEraseRange;

  const MusECore::CtrlListList* cll = t->controller();
  const MusECore::CtrlListList* e_cll = t->erasedController();

  const MusECore::AudioAutomationItemMap* aaim = nullptr;
  if(isdrag)
  {
    MusECore::ciAudioAutomationItemTrackMap iaitm = automation.currentCtrlFrameList.find(t);
    if(iaitm != automation.currentCtrlFrameList.cend())
      aaim = &iaitm->second;
  }

  for(MusECore::CtrlListList::const_iterator icll =cll->cbegin();icll!=cll->cend();++icll)
  {
    int oldX = mapx(0);
    if(oldX > rr.right())
    {
      //p.restore();
      return;
    }

    MusECore::CtrlList *cl = icll->second;
    if (cl->dontShow() || !cl->isVisible())
      continue;

    int newX = oldX;
    int oldY = -1;

    double min, max;
    cl->range(&min,&max);
    const bool discrete = cl->mode() == MusECore::CtrlList::DISCRETE;

    // Start the oldY off with the current value.
    {
      double startVal = cl->curVal();
      if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
        startVal = logToVal(startVal, min, max); // represent volume between 0 and 1
        if (startVal < 0) startVal = 0.0;
      }
      else {
        startVal = (startVal - min)/(max-min);  // we need to set curVal between 0 and 1
      }
      oldY = bottom - rmapy_f(startVal) * height;
    }
    int newY = oldY;

    QColor line_color(cl->color());
    line_color.setAlpha(MusEGlobal::config.globalAlphaBlend);
    QPen pen1(line_color);
    pen1.setCosmetic(true);
    p.setPen(pen1);

    const MusECore::AudioAutomationItemList* aail = nullptr;
    if(isdrag && aaim)
    {
      MusECore::ciAudioAutomationItemMap iaaim = aaim->find(cl->id());
      if(iaaim != aaim->cend())
        aail = &iaaim->second._selectedList;
    }

    const bool have_aail = aail && !aail->empty();
    MusECore::ciAudioAutomationItemList iaail;
    if(have_aail)
      iaail = aail->cbegin();

    MusECore::ciCtrl ic = cl->cbegin();

    const MusECore::CtrlList *e_cl = nullptr;
    MusECore::ciCtrl ic_e;
    if(isdrag)
    {
      MusECore::ciCtrlList ie_cll = e_cll->find(cl->id());
      if(ie_cll != e_cll->cend())
      {
        e_cl = ie_cll->second;
        ic_e = e_cl->begin();
      }
    }

    bool isFirstPoint = true;
    bool in_group = false;
    bool cvDiscrete = true;

    while(true)
    {
      // Nothing (left) to do?
      if((!have_aail || iaail == aail->cend()) &&
        ic == cl->cend() &&
        (!e_cl || ic_e == e_cl->cend()))
        break;

      unsigned int frame;
      double value;
      bool new_in_group = in_group;
      bool newCvDiscrete = cvDiscrete;

      // Is the local structure frame equal to or lower than either
      //  the controller or erased controller frame?
      // Then it gets priority.
      if(have_aail && iaail != aail->cend() &&
        (ic == cl->cend() || iaail->second._wrkFrame <= ic->first) &&
        (!e_cl || ic_e == e_cl->cend() || iaail->second._wrkFrame <= ic_e->first))
      {
        if(pasteNoErase)
            new_in_group = false;
        else if(pasteEraseRange)
            new_in_group = true;
        else //if(pasteErase)
            new_in_group = !iaail->second._groupEnd;

        frame = iaail->second._wrkFrame;
        value = iaail->second._wrkVal;
        newCvDiscrete = iaail->second._discrete;

        ++iaail;

        // In paste erase range mode if this is the last selected point
        //  reset the in group flag.
        if(pasteEraseRange && iaail == aail->cend())
          new_in_group = false;

        // If there's also a controller value at this location, skip it.
        if(ic != cl->cend() && ic->first == frame)
          ++ic;
        // If there's also an erased controller value at this location, skip it.
        if(e_cl && ic_e != e_cl->cend() && ic_e->first == frame)
          ++ic_e;
      }

      // Is the erased controller frame equal to or lower than the controller frame?
      // Then it gets priority.
      else if(e_cl && ic_e != e_cl->cend() && ic != cl->cend() && ic_e->first <= ic->first)
      {
        // If in pasteErase or pasteEraseRange mode and we're in a group, skip this point.
        if(isdrag && !pasteNoErase && in_group)
        {
          ++ic_e;
          continue;
        }

        frame = ic_e->first;
        value = ic_e->second.value();
        newCvDiscrete = ic_e->second.discrete();
        ++ic_e;
        // If there's also a controller value at this location, skip it.
        if(ic != cl->cend() && ic->first == ic_e->first)
          ++ic;
      }

      // Otherwise the controller frame gets priority.
      else if(ic != cl->cend())
      {
        // Skip this point if in pasteErase or pasteEraseRange mode and we're in a group,
        //  or if the point is an original moving point except when copying.
        if(isdrag)
        {
          const bool found = have_aail && aail->find(ic->first) != aail->cend();
          if((!found && !pasteNoErase && in_group) || (found && !isCopying))
          {
            ++ic;
            continue;
          }
        }

        if(!isdrag && moveModeBegun && ic->second.selected())
        {
            if(pasteNoErase)
                new_in_group = false;
            else if(pasteEraseRange)
                new_in_group = true;
            else //if(pasteErase)
                new_in_group = !ic->second.groupEnd();
        }

        frame = ic->first;
        value = ic->second.value();
        newCvDiscrete = ic->second.discrete();

        ++ic;
        // In paste erase range mode if this is the last point
        //  reset the in group flag.
        if(pasteEraseRange && ic == cl->cend())
          new_in_group = false;
      }
      else
      {
        continue;
      }


      double y = value;
      if (cl->valueType() == MusECore::VAL_LOG ) {
        y = logToVal(y, min, max); // represent volume between 0 and 1
        if (y < 0) y = 0.0;
      }
      else
        y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

      newY = bottom - rmapy_f(y) * height;
      newX = mapx(MusEGlobal::tempomap.frame2tick(frame));

      if(isFirstPoint)
      {
        isFirstPoint = false;
        oldY = newY;
      }

      // Ideally we would like to cut the lines right at the rectangle boundaries.
      // But they might not be drawn exactly the same as the full line would.
      // So we'll also accept anything that started outside the boundaries.
      // A small acceptable speed hit relatively speaking - but far far better than drawing all.
      if(oldX <= rr.right() && newX >= rr.left() && oldY <= bottom && newY >= top)
      {
        // For now we do not allow interpolation of integer or enum controllers.
        // TODO: It would require custom line drawing and corresponding hit detection.
        if(discrete || cvDiscrete)
        {
          p.drawLine(oldX, oldY, newX, oldY);
          p.drawLine(newX, oldY, newX, newY);
        }
        else
          p.drawLine(oldX, oldY, newX, newY);
      }

      oldX = newX;
      oldY = newY;
      in_group = new_in_group;
      cvDiscrete = newCvDiscrete;

      if (oldX > rr.right())
        break;
    }

    // Draw the remaining line to the end. If there were points this will draw
    //  the last point's value, otherwise the controller's current value.
    if((!have_aail || iaail == aail->cend()) &&
        ic == cl->cend() &&
        (!e_cl || ic_e == e_cl->cend()) &&
        oldY <= bottom && oldY >= top)
    {
      // Limit start x to requested rectangle x.
      if(oldX < rr.left())
        oldX = rr.left();
      if(oldX <= rr.right())
        p.drawLine(oldX, oldY, rr.right(), oldY);
    }
  }
}

//---------------------------------------------------------
//   drawAutomationPoints
//---------------------------------------------------------

void PartCanvas::drawAutomationPoints(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
  const int mx0 = mapx(0);
  if(rr.right() < mx0)
  {
    //p.restore();
    return;
  }

  const MusECore::AudioTrack* ct = t;

  const bool pasteNoErase    = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteNoErase;
  const bool pasteErase      = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteErase;
  const bool pasteEraseRange = MusEGlobal::config.audioCtrlGraphPasteEraseOptions == MusECore::CtrlList::PasteEraseRange;

  const bool isMoving = drag == DRAG_MOVE || drag == DRAGX_MOVE || drag == DRAGY_MOVE ||
    drag == DRAG_COPY || drag == DRAGX_COPY || drag == DRAGY_COPY;

  const MusECore::CtrlListList* cll = ct->controller();
  const MusECore::CtrlListList* e_cll = ct->erasedController();

  const QColor hoverColor(Qt::yellow);
  QPen hoverPen(hoverColor);
  hoverPen.setCosmetic(true);

  const MusECore::AudioAutomationItemMap* aaim = nullptr;
  {
    const MusECore::ciAudioAutomationItemTrackMap iatm = automation.currentCtrlFrameList.find(ct);
    if(iatm != automation.currentCtrlFrameList.cend())
      aaim = &iatm->second;
  }

  //-------------------------------------------------------------------
  // Draw unselected vertices first.
  //-------------------------------------------------------------------
  for(MusECore::ciCtrlList icll = cll->begin(); icll != cll->end(); ++icll)
  {
    MusECore::CtrlList *cl = icll->second;
    if(cl->dontShow() || !cl->isVisible())
      continue;

    const QColor line_col(cl->color());
    const QColor vtx_col1(line_col.red() ^ 255, line_col.green() ^ 255, line_col.blue() ^ 255);
    QColor vtx_col2(cl->color());
    vtx_col2.setAlpha(160);
    // If we happen to be using 1 pixel, use an inverted colour. Else use the line colour but slightly transparent.
    const QColor& vtx_col = (_automationPointRadius == 0) ? vtx_col1 : vtx_col2;
    QPen pen(vtx_col);
    pen.setCosmetic(true);

    const MusECore::AudioAutomationItemList* aail = nullptr;
    if(isMoving && aaim)
    {
      MusECore::ciAudioAutomationItemMap iaaim = aaim->find(cl->id());
      if(iaaim != aaim->cend())
        aail = &iaaim->second._selectedList;
    }

    const bool have_aail = aail && !aail->empty();
    MusECore::ciAudioAutomationItemList iaail;
    if(have_aail)
      iaail = aail->cbegin();

    MusECore::ciCtrl ic = cl->cbegin();

    const MusECore::CtrlList *e_cl = nullptr;
    MusECore::ciCtrl ic_e;
    if(isMoving)
    {
      MusECore::ciCtrlList ie_cll = e_cll->find(cl->id());
      if(ie_cll != e_cll->cend())
      {
        e_cl = ie_cll->second;
        ic_e = e_cl->begin();
      }
    }

    bool in_group = false;

    while(true)
    {
      //-------------------------------------------------------------------
      //  Here we draw unselected vertices, but we ignore them if they are
      //   now being covered up by a moving group of vertices.
      //  Exclude the currently highlighted point which is done below.
      //-------------------------------------------------------------------
      for( ; ic != cl->end(); ++ic)
      {
        if(ic->second.selected())
          continue;

        if(have_aail)
        {
          // If a selected point is at this point's frame, ignore this point and move on
          //  to both the next point and the next selected point.
          if(iaail != aail->cend() && ic->first == iaail->second._wrkFrame)
          {
            ++ic;
            break;
          }
          // Be sure to include processing all the vertices after the LAST selected vertex...
          else if(iaail != aail->cend() && ic->first >= iaail->second._wrkFrame)
            break;
          //  ...and here too.
          else if(((pasteErase || pasteEraseRange) && in_group && (iaail == aail->cend() || ic->first < iaail->second._wrkFrame)))
            continue;
        }

        if(!automation.currentCtrlValid ||
           automation.currentTrack != ct ||
           automation.currentCtrlList != cl ||
           automation.currentFrame != ic->first)
        {
          if(!drawAutomationPoint(p, rr, hoverPen, pen, _automationPointRadius,
            ct, cl, ic->first, ic->first, ic->second.value(), ic->second.discrete(),
            MusEGlobal::config.audioAutomationShowBoxes))
          {
            // Force the iterator to the end.
            ic = cl->end();
            break;
          }
        }
      }

      //-------------------------------------------------------------------
      //  And here we draw vertices that WERE covered up and erased,
      //   if they are now being exposed again by moving a group of vertices.
      //-------------------------------------------------------------------
      if(e_cl)
      {
        for( ; ic_e != e_cl->end(); ++ic_e)
        {
          if(have_aail)
          {
            // If a selected point is at this point's frame, ignore this point and move on to
            //  both the next point and the next selected point.
            if(iaail != aail->cend() && ic_e->first == iaail->second._wrkFrame)
            {
              ++ic_e;
              break;
            }
            // Be sure to include processing all the vertices after the LAST selected vertex...
            else if(iaail != aail->cend() && ic_e->first >= iaail->second._wrkFrame)
              break;
            //  ...and here too.
            if(((pasteErase || pasteEraseRange) && in_group && (iaail == aail->cend() || ic_e->first < iaail->second._wrkFrame)))
              continue;
          }

          // Just in case they happen to be selected.
          if(ic_e->second.selected())
          {
            if(!fillAutomationPoint(p, rr, hoverColor, Qt::white, _automationPointRadius + _automationPointExtraRadius,
              ct, cl, ic_e->first, ic_e->first, ic_e->second.value(), ic_e->second.discrete(),
              MusEGlobal::config.audioAutomationShowBoxes))  // Yes that's cl.
            {
              // Force the iterator to the end.
              ic_e = e_cl->end();
              break;
            }
          }
          else
          {
            if(!drawAutomationPoint(p, rr, hoverPen, pen, _automationPointRadius,
              ct, cl, ic_e->first, ic_e->first, ic_e->second.value(), ic_e->second.discrete(),
              MusEGlobal::config.audioAutomationShowBoxes))     // Yes that's cl.
            {
              // Force the iterator to the end.
              ic_e = e_cl->end();
              break;
            }
          }
        }
      }

      // Nothing left to do?
      if((ic == cl->end() && (!e_cl || ic_e == e_cl->end())) || !have_aail || iaail == aail->cend())
        break;

      if(pasteNoErase)
          in_group = false;
      else if(pasteEraseRange)
          in_group = true;
      else //if(pasteErase)
          in_group = have_aail && !iaail->second._groupEnd;

      if(have_aail)
      {
        ++iaail;
        // Force the last selected vertex to be group end, in case it's not marked so.
        if(iaail == aail->cend())
          in_group = false;
      }
    }
  }

  //-------------------------------------------------------------------
  // Now draw selected vertices, so that they always appear on top.
  //-------------------------------------------------------------------

  if(aaim)
  {
    for(MusECore::ciAudioAutomationItemMap iaim = aaim->cbegin(); iaim != aaim->cend(); ++iaim)
    {
      const int ctrlId = iaim->first;
      MusECore::ciCtrlList icl = ct->controller()->find(ctrlId);
      if(icl == ct->controller()->cend())
        continue;
      const MusECore::CtrlList* cl = icl->second;
      const MusECore::AudioAutomationItemList& ail = iaim->second._selectedList;

      //---------------------------------------------------
      // If moving, draw all the original points in gray.
      //---------------------------------------------------
      if(isMoving)
      {
        for(MusECore::ciAudioAutomationItemList iail = ail.cbegin(); iail != ail.cend(); ++iail)
        {
          if(!fillAutomationPoint(p, rr, Qt::lightGray, Qt::gray,
             _automationPointRadius,
             ct, cl, iail->second._wrkFrame, iail->first, iail->second._value, iail->second._discrete,
             MusEGlobal::config.audioAutomationShowBoxes))
            break;
        }
      }

      //---------------------------------------------------------------------------------------------
      // Draw all the working points - excluding the currently highlighted point which is done below.
      //---------------------------------------------------------------------------------------------
      for(MusECore::ciAudioAutomationItemList iail = ail.cbegin(); iail != ail.cend(); ++iail)
      {
        if(!automation.currentCtrlValid ||
           automation.currentTrack != ct ||
           automation.currentCtrlList != cl ||
           automation.currentWorkingFrame != iail->second._wrkFrame)
        {
          if(!fillAutomationPoint(p, rr, hoverColor, Qt::white,
            _automationPointRadius + _automationPointExtraRadius,
            ct, cl, iail->second._wrkFrame, iail->second._wrkFrame, iail->second._wrkVal, iail->second._discrete,
            MusEGlobal::config.audioAutomationShowBoxes))
            break;
        }
      }
    }
  }

  //------------------------------------------
  // Now draw the currently highlighted point.
  //------------------------------------------
  if(automation.currentCtrlValid && automation.currentTrack == ct)
  {
    MusECore::ciCtrl ic_cur = automation.currentCtrlList->find(automation.currentFrame);
    if(ic_cur != automation.currentCtrlList->cend())
    {
      if(ic_cur->second.selected())
      {
        if(aaim)
        {
          MusECore::ciAudioAutomationItemMap iaim_cur = aaim->find(automation.currentCtrlList->id());
          if(iaim_cur != aaim->cend())
          {
            const MusECore::AudioAutomationItemList& ail_cur = iaim_cur->second._selectedList;
            // We need more info than is available in the 'automation' object. Find the exact point.
            // Here we look for the original frame (currentFrame), which is how the map is indexed.
            MusECore::ciAudioAutomationItemList iail_cur = ail_cur.find(automation.currentFrame);
            if(iail_cur != ail_cur.cend())
            {
              fillAutomationPoint(p, rr, hoverColor, Qt::white,
                _automationPointRadius + _automationPointExtraRadius,
                ct, automation.currentCtrlList, iail_cur->second._wrkFrame, iail_cur->second._wrkFrame,
                iail_cur->second._wrkVal, iail_cur->second._discrete,
                MusEGlobal::config.audioAutomationShowBoxes);
            }
          }
        }
      }
      else
      {
        const QColor line_col(automation.currentCtrlList->color());
        const QColor vtx_col1(line_col.red() ^ 255, line_col.green() ^ 255, line_col.blue() ^ 255);
        QColor vtx_col2(automation.currentCtrlList->color());
        vtx_col2.setAlpha(160);
        // If we happen to be using 1 pixel, use an inverted colour. Else use the line colour but slightly transparent.
        const QColor& vtx_col = (_automationPointRadius == 0) ? vtx_col1 : vtx_col2;
        QPen pen(vtx_col);
        pen.setCosmetic(true);

        drawAutomationPoint(p, rr, hoverPen, pen, _automationPointRadius,
          ct, automation.currentCtrlList, ic_cur->first, ic_cur->first, ic_cur->second.value(), ic_cur->second.discrete(),
          MusEGlobal::config.audioAutomationShowBoxes);
      }
    }
  }
}

bool PartCanvas::drawAutomationPoint(
  QPainter& p, const QRect& rr, const QPen& currentPen, const QPen& nonCurrentPen, int pointRadius,
  const MusECore::AudioTrack* t, const MusECore::CtrlList* cl, unsigned int currentFrame, unsigned int newFrame,
  double value, bool discrete, bool fullSize)
{
  const int pdia  = 2 * pointRadius /*+ 1*/;
  const int xpixel = mapx(MusEGlobal::tempomap.frame2tick(newFrame));
  if((fullSize && xpixel - pointRadius > rr.right()) || (!fullSize && xpixel > rr.right()))
    return false;

  // Note that QRect::bottom() is y() + height() - 1.
  const int bottom = rr.bottom() - _automationBottomMargin;
  const int top = rr.top() + _automationTopMargin;
  const int height = bottom - top;
  double min, max;
  cl->range(&min,&max);

  const bool isCurrent =
    automation.currentTrack == t &&
    automation.currentCtrlValid &&
    automation.currentCtrlList == cl &&
    automation.currentWorkingFrame == currentFrame;

  double y = value;
  if (cl->valueType() == MusECore::VAL_LOG ) {
    y = logToVal(y, min, max); // represent volume between 0 and 1
    if(y < 0) y = 0.0;
  }
  else
    y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

  if(isCurrent)
    p.setPen(currentPen);
  else
    p.setPen(nonCurrentPen);
  p.setBrush(QBrush());

  const int ypixel = bottom - rmapy_f(y) * height;
  if(fullSize)
  {
    if(((xpixel + pointRadius >= rr.left()) && (xpixel - pointRadius <= rr.right())) &&
      ((ypixel + pointRadius >= top)  && (ypixel - pointRadius <= bottom)))
    {
      if(discrete)
        p.drawRect(xpixel - pointRadius, ypixel - pointRadius, pdia, pdia);
      else
        p.drawEllipse(xpixel - pointRadius, ypixel - pointRadius, pdia, pdia);
    }
  }
  else
  {
    if(((xpixel >= rr.left()) && (xpixel <= rr.right())) &&
      ((ypixel >= top)  && (ypixel <= bottom)))
    {
      p.drawPoint(xpixel, ypixel);
    }
  }

  return true;
}

bool PartCanvas::fillAutomationPoint(
  QPainter& p, const QRect& rr, const QColor& currentColor, const QColor& nonCurrentColor, int pointRadius,
  const MusECore::AudioTrack* t, const MusECore::CtrlList* cl, unsigned int currentFrame, unsigned int newFrame,
  double value, bool discrete, bool fullSize)
{
  const int pdia  = 2 * pointRadius /*+ 1*/;
  const int xpixel = mapx(MusEGlobal::tempomap.frame2tick(newFrame));
  if((fullSize && xpixel - pointRadius > rr.right()) || (!fullSize && xpixel > rr.right()))
    return false;

  // Note that QRect::bottom() is y() + height() - 1.
  const int bottom = rr.bottom() - _automationBottomMargin;
  const int top = rr.top() + _automationTopMargin;
  const int height = bottom - top;
  double min, max;
  cl->range(&min,&max);

  const bool isCurrent =
    automation.currentTrack == t &&
    automation.currentCtrlValid &&
    automation.currentCtrlList == cl &&
    automation.currentWorkingFrame == currentFrame;

  double y = value;
  if (cl->valueType() == MusECore::VAL_LOG ) {
    y = logToVal(y, min, max); // represent volume between 0 and 1
    if(y < 0) y = 0.0;
  }
  else
    y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

  const int ypixel = bottom - rmapy_f(y) * height;
  if(fullSize)
  {
    if(((xpixel + pointRadius >= rr.left()) && (xpixel - pointRadius <= rr.right())) &&
      ((ypixel + pointRadius >= top) && (ypixel - pointRadius <= bottom)))
    {
      if(discrete)
      {
        // Note the + 1 required here for fill width and height.
        p.fillRect(xpixel - pointRadius, ypixel - pointRadius,
          pdia + 1, pdia + 1, isCurrent ? currentColor : nonCurrentColor);
      }
      else
      {
        p.setPen(isCurrent ? currentColor : nonCurrentColor);
        p.setBrush(isCurrent ? currentColor : nonCurrentColor);
        p.drawEllipse(xpixel - pointRadius, ypixel - pointRadius, pdia, pdia);
      }
    }
  }
  else
  {
    if(((xpixel >= rr.left()) && (xpixel <= rr.right())) &&
      ((ypixel >= top)  && (ypixel <= bottom)))
    {
      QColor c(isCurrent ? currentColor : nonCurrentColor);
      // Force the point to be as bright and visible as possible.
      c.setAlpha(255);
      p.setPen(c);
      p.drawPoint(xpixel, ypixel);
    }
  }

  return true;
}

//---------------------------------------------------------
//   drawAutomationText
//---------------------------------------------------------

void PartCanvas::drawAutomationText(QPainter& p, const QRect& rr, MusECore::AudioTrack *t)
{
    if(rr.right() < mapx(0))
    {
      //p.restore();
      return;
    }
  // Note that QRect::bottom() is y() + height() - 1.
  const int bottom = rr.bottom() - _automationBottomMargin;
  const int top = rr.top() + _automationTopMargin;
  const int height = bottom - top;

    p.setBrush(Qt::NoBrush);
    p.setFont(font());

    MusECore::CtrlListList* cll = t->controller();
    for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
    {
      MusECore::CtrlList *cl = icll->second;
      if (cl->dontShow() || !cl->isVisible())
        continue;

      int xpixel = 0;
      int ypixel = 0;
      double min, max;
      cl->range(&min,&max);
      QPen pen1(cl->color());
      pen1.setCosmetic(true);
      const QColor line_col = cl->color();
      QColor txt_bk((line_col.red() + line_col.green() + line_col.blue()) / 3 >= 128 ? Qt::black : Qt::white);
      txt_bk.setAlpha(150);

      p.setPen(pen1);

      // Draw the current automation value text.
      if(automation.currentTrack == t && automation.currentCtrlValid && automation.currentCtrlList == cl)
      {
        double y = automation.currentVal;
        if (cl->valueType() == MusECore::VAL_LOG ) {
          y = logToVal(y, min, max); // represent volume between 0 and 1
          if (y < 0) y = 0.0;
        }
        else
          y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

        ypixel = bottom - rmapy_f(y) * height;
        xpixel = mapx(MusEGlobal::tempomap.frame2tick(automation.currentWorkingFrame));

        if(xpixel + 20 <= rr.right() && ypixel <= bottom)
        {
          QRect textRect = p.fontMetrics().boundingRect(automation.currentText).adjusted(-4, -2, 4, 2);
          textRect.moveLeft(xpixel + 20);
          textRect.moveTop(ypixel);
          if(textRect.right() >= rr.left() && textRect.bottom() >= top)
          {
            p.fillRect(textRect, txt_bk);
            p.drawText(textRect, Qt::AlignCenter, automation.currentText);
          }
        }
      }

// Name text has been moved to the track list automation column. Keep in case improved later?
#if 0
      // Draw the controller name text.
      double yfirst;
      if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
        yfirst = logToVal(cl->curVal(), min, max); // represent volume between 0 and 1
        if (yfirst < 0) yfirst = 0.0;
      }
      else {
        yfirst = (cl->curVal() - min)/(max-min);  // we need to set curVal between 0 and 1
      }
      yfirst = bottom - rmapy_f(yfirst) * height;
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
#endif
    }
}

//---------------------------------------------------------
// distanceSqToSegment
// Returns the distance, squared, of a point to a line segment.
//---------------------------------------------------------

std::int64_t distanceSqToSegment(double pointX, double pointY, double x1, double y1, double x2, double y2)
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
//---------------------------------------------------------

void PartCanvas::checkAutomation(const QPoint &pointer)
{
  MusECore::Track * t = y2Track(pointer.y());
  if (t && !t->isMidiTrack()) {

    int mouseY;
    const int top = t->y() + _automationTopMargin;
    const int bottom = t->y() + t->height() - 1 - _automationBottomMargin;
    const int height = bottom - top;

    const int pointy = pointer.y();
    // Accept y values from full top to bottom, even if the drawing area has a y margin.
    if(pointy >= t->y() && pointy < (t->y() + t->height()))
    {
      mouseY =  mapy(pointy);
      const int mouseX =  mapx(pointer.x());

      const std::int64_t detectRadius = _automationPointRadius;
      const std::int64_t detectRadius2 = detectRadius * detectRadius;
      const std::int64_t detectRadiusSel = _automationPointRadius + _automationPointExtraRadius;
      const std::int64_t detectRadius2Sel = detectRadiusSel * detectRadiusSel;

      std::int64_t closest_point_radius2 = 0;
      int closest_point_frame = 0;
      double closest_point_value = 0.0;
      MusECore::CtrlList* closest_point_cl = nullptr;

      std::int64_t closest_point_radius2_sel = 0;
      int closest_point_frame_sel = 0;
      double closest_point_value_sel = 0.0;
      MusECore::CtrlList* closest_point_cl_sel = nullptr;

      std::int64_t closest_line_dist2 = detectRadius2;
      MusECore::CtrlList* closest_line_cl = nullptr;

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
        bool cvDiscrete = true;
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
          eventY = eventOldY = mapy(bottom - y * height);
        }
        else // we have automation, loop through it
        {
          for (; ic!=cl->end(); ++ic)
          {
            double y = ic->second.value();
            if (cl->valueType() == MusECore::VAL_LOG ) { // use db scale for volume
              y = logToVal(y, min, max); // represent volume between 0 and 1
              if (y < 0) y = 0;
            }
            else
              y = (y-min)/(max-min);  // we need to set curVal between 0 and 1

            eventY = mapy(bottom - y * height);
            eventX = mapx(MusEGlobal::tempomap.frame2tick(ic->first));

            if (eventOldY==-1) eventOldY = eventY;

            if(pointer.x() >= 0 && pointer.y() >= 0)
            {
              const std::int64_t dx = mouseX - eventX;
              const std::int64_t dy = mouseY - eventY;
              const std::int64_t dx2 = dx * dx;
              const std::int64_t dy2 = dy * dy;
              const std::int64_t r2 = dx2 + dy2;

              const bool disc = ic->second.discrete();
              if(ic->second.selected())
              {
                if(((disc && dx2 <= detectRadius2Sel && dy2 <= detectRadius2Sel) || (!disc && r2 <= detectRadius2Sel)) &&
                  (!closest_point_cl_sel || r2 < closest_point_radius2_sel))
                {
                  closest_point_radius2_sel = r2;
                  closest_point_frame_sel = ic->first;
                  closest_point_value_sel = ic->second.value();
                  closest_point_cl_sel = cl;
                }
              }
              else
              {
                // Is the point close enough, and this is the first iteration or is the point
                //  closer than any point so far?
                // Unlike the selected points above, here we check which unselected point hit is the closest.
                if(((disc && dx2 <= detectRadius2 && dy2 <= detectRadius2) || (!disc && r2 <= detectRadius2)) &&
                  (!closest_point_cl || r2 < closest_point_radius2))
                {
                  closest_point_radius2 = r2;
                  closest_point_frame = ic->first;
                  closest_point_value = ic->second.value();
                  closest_point_cl = cl;
                }
              }
            }

            // For now we do not allow interpolation of integer or enum controllers.
            // TODO: It would require custom line drawing and corresponding hit detection.
            const bool isDiscrete = discrete || cvDiscrete;

            const std::int64_t ldist2 = distanceSqToSegment(mouseX, mouseY, eventOldX, eventOldY, eventX, isDiscrete ? eventOldY : eventY);
            if(ldist2 < closest_line_dist2)
            {
              closest_line_dist2 = ldist2;
              closest_line_cl = cl;
            }

            eventOldX = eventX;
            eventOldY = eventY;
            cvDiscrete = ic->second.discrete();
          }
        }

        if(mouseX >= eventX)
        {
          const std::int64_t dy = mouseY-eventY;
          const std::int64_t d2 = dy * dy;
          if(d2 < closest_line_dist2)
          {
            closest_line_dist2 = d2;
            closest_line_cl = cl;
          }
        }
      }

      // If there is a selected point anywhere nearby, it gets priority over unselected points,
      //  no matter how close they may be.
      if(closest_point_cl_sel)
      {
        // Re-use the non-selected variables, for ease below.
        closest_point_frame = closest_point_frame_sel;
        closest_point_value = closest_point_value_sel;
        closest_point_cl = closest_point_cl_sel;
      }

      // Is the mouse close to a vertex? Since we currently don't use the addNewCtrl accel key, vertices take priority over lines.
      if(closest_point_cl)
      {
        automation.currentCtrlValid = true;
        automation.controllerState = doNothing;
        automation.currentCtrlList = closest_point_cl;
        automation.currentTrack = t;
        automation.currentFrame = automation.currentWorkingFrame = closest_point_frame;
        // Store the value.
        automation.currentVal = closest_point_value;
        // Store the text.
        if(closest_point_cl->valueType() == MusECore::VAL_LOG)
          closest_point_value = muse_val2db(closest_point_value); // Here we can use the slower but accurate function.
        automation.currentText = QString("Param:%1 Value:%2").arg(closest_point_cl->name()).arg(closest_point_value, 0, 'g', 3);

        setCursor();

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
          value = muse_val2db(value); // Here we can use the slower but accurate function.
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
        const double linMin = muse_val2db(min);
        const double linMax = muse_val2db(max);
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
        automation.currentCtrlValid = false;
        automation.controllerState = addNewController;
        automation.currentCtrlList = closest_line_cl;
        automation.currentTrack = t;
        automation.currentFrame = 0;
        automation.currentWorkingFrame = 0;
        automation.currentVal = 0;
        setCursor();

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
    }
  }

  // If there is a current track, schedule a redraw of it.
  if(automation.currentTrack)
    controllerChanged(automation.currentTrack);

  // if there are no hits we default to clearing all the data

  automation.controllerState = doNothing;
  automation.currentCtrlValid = false;
  automation.currentCtrlList = nullptr;
  automation.currentTrack = nullptr;
  automation.currentFrame = 0;
  automation.currentWorkingFrame = 0;
  automation.currentVal = 0;
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

void PartCanvas::controllerChanged(
  const MusECore::Track* t, int ctrlId, unsigned int frame, MusECore::CtrlGUIMessage::Type type)
{
  //----------------------------------------------------------------------------
  // We may be receiving this message from another thread, delayed by IPC timing.
  // It is possible things may have changed by now. Be careful...
  // Check if the track still exists.
  //----------------------------------------------------------------------------
  const bool trackExists = MusEGlobal::song->trackExists(t);

  //--------------------------------------------------------------
  // Check if the controller exists in the model and is visible.
  //--------------------------------------------------------------
  const MusECore::AudioTrack* at = nullptr;
  const MusECore::CtrlList* track_cl = nullptr;
  if(type == MusECore::CtrlGUIMessage::ADDED && trackExists && !t->isMidiTrack())
  {
    at = static_cast<const MusECore::AudioTrack*>(t);
    MusECore::ciCtrlList icl = at->controller()->find(ctrlId);
    if(icl != at->controller()->cend())
    {
      if(icl->second->isVisible())
        track_cl = icl->second;
    }
  }

  //----------------------------------------------------
  // Check if the point exists in the local structures.
  //----------------------------------------------------

  MusECore::iAudioAutomationItemTrackMap iaatm;
  MusECore::iAudioAutomationItemMap iaim;
  MusECore::iAudioAutomationItemList iail;
  MusECore::AudioAutomationItemMap* aaim = nullptr;
  bool iailFound = false;
  if(type == MusECore::CtrlGUIMessage::ADDED || type == MusECore::CtrlGUIMessage::DELETED)
  {
    iaatm = automation.currentCtrlFrameList.find(t);
    if(iaatm != automation.currentCtrlFrameList.end())
    {
      aaim = &iaatm->second;
      iaim = aaim->find(ctrlId);
      if(iaim != aaim->end())
      {
        iail = iaim->second._selectedList.find(frame);
        if(iail != iaim->second._selectedList.end())
          iailFound = true;
      }
    }
  }

  bool found = false;

  //--------------------------------------------
  // Check if the point exists in the model.
  //--------------------------------------------
  if(track_cl)
  {
    MusECore::ciCtrl ic = track_cl->find(frame);
    if(ic != track_cl->cend() && (ic->second.flags() & MusECore::CtrlVal::VAL_SELECTED))
    {
      found = true;
      const MusECore::CtrlVal& cv = ic->second;
      //----------------------------------------------------------------------------------------------
      // If a local item exists for the point, update its members from the model's item. //----------------------------------------------------------------------------------------------
      if(iailFound)
      {
        // No choice but to update the working value as well.
        iail->second._wrkVal = iail->second._value = cv.value();
        iail->second._discrete = cv.discrete();
      }
      //---------------------------------------------------------------------
      // Otherwise go ahead and add a new local item based on the model item.
      //---------------------------------------------------------------------
      else
        automation.currentCtrlFrameList.addSelected(at, ctrlId, frame, MusECore::AudioAutomationItem(frame, cv));
    }
  }

  //--------------------------------------------------------------------
  // If adding and the point was not found in the model, or if deleting,
  //  delete the point in the local structures.
  //--------------------------------------------------------------------
  if((type == MusECore::CtrlGUIMessage::ADDED && !found) || type == MusECore::CtrlGUIMessage::DELETED)
  {
    if(iailFound)
    {
      //automation.currentCtrlFrameList.delSelected(t, ctrlId, frame);
      iaim->second._selectedList.erase(iail);
      if(iaim->second._selectedList.empty())
        aaim->erase(iaim);
      if(aaim->empty())
        automation.currentCtrlFrameList.erase(iaatm);
    }
  }

  //--------------------------------------------------------------------
  // Paint update.
  //--------------------------------------------------------------------
  if(trackExists &&
     (type == MusECore::CtrlGUIMessage::PAINT_UPDATE ||
      (track_cl && (type == MusECore::CtrlGUIMessage::ADDED || type == MusECore::CtrlGUIMessage::DELETED))))
    redraw((QRect(0, mapy(t->y()), width(), rmapy(t->height()))));  // TODO Check this - correct?
}

bool PartCanvas::getMovementRange(
  const MusECore::CtrlList* cl, unsigned int frame, double* value, unsigned int* minPrevFrame,
  unsigned int* maxNextFrame, bool* maxNextFrameValid) const
{
  MusECore::ciCtrl iold = cl->find(frame);
  if(iold == cl->cend())
    return false;

  if(value)
    *value = iold->second.value();

  if(minPrevFrame)
  {
    // The minimum frame that this selected frame can be moved to is the previous
    //  UNSELECTED vertex frame, PLUS the number of items from here to that vertex...
    unsigned int min_prev_frame = 0;
    MusECore::ciCtrl iprev = iold;
    unsigned int prev_frame_offset = 0;
    while(iprev != cl->cbegin())
    {
      --iprev;
      ++prev_frame_offset;
      // Stop when we find the previous unselected frame.
      if(!iprev->second.selected())
      {
        min_prev_frame = iprev->first + prev_frame_offset;
        break;
      }
    }
    *minPrevFrame = min_prev_frame;
  }

  if(maxNextFrame)
  {
    // The maximum frame that this selected frame can be moved to is the next
    //  UNSELECTED vertex frame, MINUS the number of items from here to that vertex...
    unsigned int max_next_frame = 0;
    bool max_next_frame_valid = false;
    MusECore::ciCtrl inext = iold;
    ++inext;
    unsigned int next_frame_offset = 1; // Yes, that's 1.
    while(inext != cl->cend())
    {
      // Stop when we find the next unselected frame.
      if(!inext->second.selected())
      {
        max_next_frame = inext->first - next_frame_offset;
        max_next_frame_valid = true;
        break;
      }
      ++inext;
      ++next_frame_offset;
    }
    *maxNextFrame = max_next_frame;
    if(maxNextFrameValid)
      *maxNextFrameValid = max_next_frame_valid;
  }

  return true;
}

void PartCanvas::unselectAllAutomation(MusECore::Undo& undo) const
{
  for (MusECore::ciTrack it = tracks->cbegin(); it != tracks->cend(); ++it) {
    if((*it)->isMidiTrack())
      continue;
    // Include all tracks.
    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*it);
    const MusECore::CtrlListList* cll = track->controller();
    for(MusECore::ciCtrlList icll = cll->cbegin(); icll != cll->cend(); ++icll)
    {
      // Include all controllers.
      MusECore::CtrlList* cl = icll->second;
      for(MusECore::iCtrl ic = cl->begin(); ic != cl->end(); ++ ic)
      {
        MusECore::CtrlVal& cv = ic->second;
        // Include only already selected controller values.
        if(!cv.selected())
          continue;
        undo.push_back(UndoOp(UndoOp::SelectAudioCtrlVal, cl, ic->first, cv.selected(), false, !MusEGlobal::config.selectionsUndoable));
      }
    }
  }
}

void PartCanvas::deleteSelectedAutomation(MusECore::Undo& undo) const
{
  for(MusECore::ciAudioAutomationItemTrackMap iatm = automation.currentCtrlFrameList.cbegin(); iatm != automation.currentCtrlFrameList.cend(); ++iatm)
  {
    const MusECore::Track* track = iatm->first;
    const MusECore::AudioAutomationItemMap& atm = iatm->second;
    for(MusECore::ciAudioAutomationItemMap iaim = atm.cbegin(); iaim != atm.cend(); ++iaim)
    {
      const int ctrlId = iaim->first;
      const MusECore::AudioAutomationItemList& ail = iaim->second._selectedList;
      for(MusECore::ciAudioAutomationItemList iail = ail.cbegin(); iail != ail.cend(); ++iail)
      {
        const unsigned int frame = iail->first;
        undo.push_back(UndoOp(UndoOp::DeleteAudioCtrlVal, track, ctrlId, frame, 0, 0, 0));
      }
    }
  }
}

void PartCanvas::alignSelectedAutomation(MusECore::Undo& undo) const
{
  if(!automation.currentCtrlValid)
    return;

  const MusECore::Track* curtrack = automation.currentTrack;
  const int curid = automation.currentCtrlList->id();
  const unsigned int curframe = automation.currentFrame;
  const double curval = automation.currentVal;
  double min, max, trackMin, trackMax;
  automation.currentCtrlList->range(&min, &max);

  for(MusECore::ciAudioAutomationItemTrackMap iatm = automation.currentCtrlFrameList.cbegin();
      iatm != automation.currentCtrlFrameList.cend(); ++iatm)
  {
    const MusECore::Track* track = iatm->first;
    if(track->isMidiTrack())
      continue;
    const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(track);
    const MusECore::AudioAutomationItemMap& atm = iatm->second;
    for(MusECore::ciAudioAutomationItemMap iaim = atm.cbegin(); iaim != atm.cend(); ++iaim)
    {
      const int ctrlId = iaim->first;
      MusECore::ciCtrlList icl = at->controller()->find(ctrlId);
      if(icl == at->controller()->cend())
        continue;
      const MusECore::CtrlList* track_cl = icl->second;
      track_cl->range(&trackMin, &trackMax);

      const MusECore::AudioAutomationItemList& ail = iaim->second._selectedList;
      for(MusECore::ciAudioAutomationItemList iail = ail.cbegin(); iail != ail.cend(); ++iail)
      {
        const unsigned int frame = iail->first;
        // Skip the given point.
        if(track == curtrack && ctrlId == curid && frame == curframe)
          continue;

        double newValue = curval;

        if(track_cl != automation.currentCtrlList)
        {
          if (automation.currentCtrlList->valueType() == MusECore::VAL_LOG ) {
            newValue = logToVal(newValue, min, max); // represent volume between 0 and 1
            if(newValue < 0) newValue = 0.0;
          }
          else
            newValue = (newValue-min)/(max-min);  // we need to set curVal between 0 and 1

          if (track_cl->valueType() == MusECore::VAL_LOG ) {
            newValue = valToLog(newValue, trackMin, trackMax);
            if (newValue< trackMin) newValue=trackMin;
            if (newValue>trackMax) newValue=trackMax;
          }
          else
            newValue = newValue * (trackMax-trackMin) + trackMin;
        }

        undo.push_back(UndoOp(UndoOp::ModifyAudioCtrlVal, track, ctrlId, frame, frame, iail->second._value, newValue));
      }
    }
  }
}

void PartCanvas::setSelectedAutomationMode(MusECore::Undo& undo, MusECore::CtrlList::Mode mode) const
{
  for(MusECore::ciAudioAutomationItemTrackMap iatm = automation.currentCtrlFrameList.cbegin();
      iatm != automation.currentCtrlFrameList.cend(); ++iatm)
  {
    const MusECore::Track* track = iatm->first;
    if(track->isMidiTrack())
      continue;
    const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(track);
    const MusECore::AudioAutomationItemMap& atm = iatm->second;
    for(MusECore::ciAudioAutomationItemMap iaim = atm.cbegin(); iaim != atm.cend(); ++iaim)
    {
      const int ctrlId = iaim->first;
      MusECore::ciCtrlList icl = at->controller()->find(ctrlId);
      if(icl == at->controller()->cend())
        continue;
      const MusECore::CtrlList* track_cl = icl->second;
      const bool ctrlIsDiscrete = track_cl->mode() == MusECore::CtrlList::DISCRETE;

      MusECore::CtrlList* addCtrlList = new MusECore::CtrlList(*track_cl, MusECore::CtrlList::ASSIGN_PROPERTIES);
      MusECore::CtrlList* eraseCtrlList = new MusECore::CtrlList(*track_cl, MusECore::CtrlList::ASSIGN_PROPERTIES);

      const MusECore::AudioAutomationItemList& ail = iaim->second._selectedList;
      for(MusECore::ciAudioAutomationItemList iail = ail.cbegin(); iail != ail.cend(); ++iail)
      {
        const unsigned int frame = iail->first;
        const MusECore::AudioAutomationItem& aai =  iail->second;

        // Don't bother if the point is already in the given mode or the mode can't be changed.
        // For now we do not allow interpolation of integer or enum controllers.
        // TODO: It would require custom line drawing and corresponding hit detection.
        if((mode == MusECore::CtrlList::DISCRETE && aai._discrete) ||
           (mode == MusECore::CtrlList::INTERPOLATE && (!aai._discrete || ctrlIsDiscrete)))
          continue;

        eraseCtrlList->add(frame, MusECore::CtrlVal(aai._value, /*selected*/true,
                           aai._discrete, aai._groupEnd));
        addCtrlList->add(frame, MusECore::CtrlVal(aai._value, /*selected*/true,
                           mode == MusECore::CtrlList::DISCRETE, aai._groupEnd));
      }

      // If nothing was changed, delete and ignore.
      if(eraseCtrlList->empty())
      {
        delete eraseCtrlList;
        eraseCtrlList = nullptr;
      }
      if(addCtrlList->empty())
      {
        delete addCtrlList;
        addCtrlList = nullptr;
      }
      if(eraseCtrlList || addCtrlList)
      {
        undo.push_back(MusECore::UndoOp(
          MusECore::UndoOp::ModifyAudioCtrlValList, track, ctrlId, eraseCtrlList, addCtrlList));
      }
    }
  }
}

void PartCanvas::haveSelectedAutomationMode(bool* canDiscrete, bool* canInterpolate) const
{
  if(canDiscrete)
    *canDiscrete = false;
  if(canInterpolate)
    *canInterpolate = false;
  for(MusECore::ciAudioAutomationItemTrackMap iatm = automation.currentCtrlFrameList.cbegin();
      iatm != automation.currentCtrlFrameList.cend(); ++iatm)
  {
    const MusECore::Track* track = iatm->first;
    if(track->isMidiTrack())
      continue;
    const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(track);
    const MusECore::AudioAutomationItemMap& atm = iatm->second;
    for(MusECore::ciAudioAutomationItemMap iaim = atm.cbegin(); iaim != atm.cend(); ++iaim)
    {
      const int ctrlId = iaim->first;
      MusECore::ciCtrlList icl = at->controller()->find(ctrlId);
      if(icl == at->controller()->cend())
        continue;
      const MusECore::CtrlList* track_cl = icl->second;
      const bool ctrlIsDiscrete = track_cl->mode() == MusECore::CtrlList::DISCRETE;

      const MusECore::AudioAutomationItemList& ail = iaim->second._selectedList;
      for(MusECore::ciAudioAutomationItemList iail = ail.cbegin(); iail != ail.cend(); ++iail)
      {
        const MusECore::AudioAutomationItem& aai =  iail->second;
        if(aai._discrete)
        {
          // For now we do not allow interpolation of integer or enum controllers.
          // TODO: It would require custom line drawing and corresponding hit detection.
          if(canInterpolate && !ctrlIsDiscrete)
            *canInterpolate = true;
        }
        else
        {
          if(canDiscrete)
            *canDiscrete = true;
        }
        // If both have been found, we're done.
        if((!canDiscrete || *canDiscrete) && (!canInterpolate || *canInterpolate))
          return;
      }
    }
  }
}

//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
void PartCanvas::processAutomationMovements(QPoint pos, int /*dir*/, bool /*rasterize*/)
{
  if (_tool != AutomationTool)
  {
    automation.controllerState = doNothing;
    return;
  }

  if (drag == DRAG_MOVE || drag == DRAGX_MOVE || drag == DRAGY_MOVE ||
    drag == DRAG_COPY || drag == DRAGX_COPY || drag == DRAGY_COPY || drag == DRAG_MOVE_START || drag == DRAG_COPY_START) {
    automation.controllerState = doNothing;
  }
  else {
      checkAutomation(pos);
  }
}

bool PartCanvas::commitAutomationChanges(MusECore::Undo& undo, bool isCopy)
{
  // Gather the necessary operations.
  const bool ret = MusEGlobal::song->collectAudioCtrlPasteModeOps(
    automation.currentCtrlFrameList, undo, MusEGlobal::config.audioCtrlGraphPasteEraseOptions, true, isCopy);
  // If necessary, upon any next operations list execution, break any undo combining.
  undo.combobreaker = automation.breakUndoCombo;
  // Reset.
  automation.breakUndoCombo = false;
  return ret;
}

bool PartCanvas::newAutomationVertex(QPoint pos, MusECore::Undo& undo, bool snap)
{
  if (_tool != AutomationTool || automation.controllerState != addNewController)
    return false;

  // get the current controller value at the frame pointed at
  unsigned int frame = MusEGlobal::tempomap.tick2frame(pos.x());

  MusECore::CtrlInterpolate ctrlInterpolate;
  automation.currentCtrlList->getInterpolation(frame, false, &ctrlInterpolate);

  if(snap)
  {
    // Take advantage of the ctrlInterpolate structure to get the the min/max frames.
    // First, try snapping to the closest frame.
    unsigned int rasterFrame = MusEGlobal::tempomap.tick2frame(MusEGlobal::sigmap.raster(pos.x(), *_raster));
    if(rasterFrame <= ctrlInterpolate.sFrame)
    {
      // The snapped frame is less than or equal to the minimum. Try snapping to the right.
      rasterFrame = MusEGlobal::tempomap.tick2frame(MusEGlobal::sigmap.raster2(pos.x(), *_raster));
      if(rasterFrame > ctrlInterpolate.sFrame && (!ctrlInterpolate.eFrameValid || rasterFrame < ctrlInterpolate.eFrame))
        // The snapped frame is within the limits. Snap to it.
        frame = rasterFrame;
    }
    else if(ctrlInterpolate.eFrameValid && rasterFrame >= ctrlInterpolate.eFrame)
    {
      // The snapped frame is greater than or equal to the maximum. Try snapping to the left.
      rasterFrame = MusEGlobal::tempomap.tick2frame(MusEGlobal::sigmap.raster1(pos.x(), *_raster));
      if(rasterFrame > ctrlInterpolate.sFrame && (!ctrlInterpolate.eFrameValid || rasterFrame < ctrlInterpolate.eFrame))
        // The snapped frame is within the limits. Snap to it.
        frame = rasterFrame;
    }
    else
    {
      // The snapped frame is within the limits. Snap to it.
      frame = rasterFrame;
    }
  }


  const double cvval =
    ctrlInterpolate.doInterp ? automation.currentCtrlList->interpolate(frame, ctrlInterpolate) : ctrlInterpolate.sVal;

  // Keep a string for easy displaying of automation values
  double displayCvVal =  cvval;
  if(automation.currentCtrlList->valueType() == MusECore::VAL_LOG)
    displayCvVal = muse_val2db(cvval);
  automation.currentText = QString("Param:%1 Value:%2").arg(automation.currentCtrlList->name()).arg(displayCvVal, 0, 'g', 3);

  // Now that we have a track, cl, and frame for a new vertex, set the automation object mode to moving.
  automation.currentFrame = automation.currentWorkingFrame = frame;
  automation.currentVal = cvval;
  automation.currentCtrlValid = true;
  automation.breakUndoCombo = true;

  undo.push_back(UndoOp(UndoOp::AddAudioCtrlVal, automation.currentTrack, automation.currentCtrlList->id(), frame,
      cvval, MusECore::CtrlVal::VAL_SELECTED |
        // The undo system automatically sets the VAL_DISCRETE flag if the controller mode is DISCRETE.
        (MusEGlobal::config.audioAutomationDrawDiscrete ? MusECore::CtrlVal::VAL_DISCRETE : MusECore::CtrlVal::VAL_NOFLAGS)));

  return true;
}

//---------------------------------------------------------
//
//  logToVal
//   - represent logarithmic value on linear scale from 0 to 1
//
//---------------------------------------------------------
double PartCanvas::logToVal(double inLog, double min, double max) const
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
double PartCanvas::valToLog(double inV, double min, double max) const
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
//
//  deltaValToLog
//
//---------------------------------------------------------
double PartCanvas::deltaValToLog(double inLog, double inLinDeltaNormalized, double min, double max) const
{
    if (inLog < min) inLog = min;
    if (inLog > max) inLog = max;
    //const double linMin = 20.0*MusECore::fast_log10(min);
    //const double linMax = 20.0*MusECore::fast_log10(max);
    //const double linVal = 20.0*MusECore::fast_log10(inLog);
    const double linMin = 20.0*log10(min);
    const double linMax = 20.0*log10(max);
    const double linVal = 20.0*log10(inLog);
    const double linDeltaVal = (inLinDeltaNormalized * (linMax - linMin)) /*+ linMin*/;

    double outVal = linVal + linDeltaVal;
    outVal = exp10((outVal)/20.0);
    if (outVal > max) outVal = max;
    if (outVal < min) outVal = min;

    return outVal;
}

//---------------------------------------------------------
//   startMoving
//    copy selection-List to moving-List
//---------------------------------------------------------

void PartCanvas::startMoving(const QPoint& pos, int dir, DragType dt, bool rasterize)
      {
        Canvas::startMoving(pos, dir, dt, rasterize);
      moveItems(pos, dir, rasterize);
      }

//---------------------------------------------------------
//   moveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void PartCanvas::moveItems(const QPoint& inPos, int dir, bool rasterize)
{
  // If we are in AutomationTool mode moving automation points, limit the movement
  //  of parts (or other object types) that are also moving, to the horizontal direction.
  if(_tool != AutomationTool || dir != 2)
    Canvas::moveItems(inPos, _tool == AutomationTool && dir == 0 ? 1 : dir, rasterize);

  // We cannot copy automation points to time positions where points already exist.
  // Only one point per time position is allowed.
  // Therefore copy + vertical dir mode makes no sense here.
  switch(drag)
  {
    case DRAG_COPY:
    case DRAGX_COPY:
    case DRAGY_COPY:
          if(dir == 2)
            return;
          break;
    default:
      break;
  }

  int deltaX = 0;
  int deltaY = 0;
  if(dir == 0 || dir == 1)
    deltaX = inPos.x() - start.x();
  if(dir == 0 || dir == 2)
    deltaY = inPos.y() - start.y();

  // If we want to rasterize and we're in automation tool mode and there's a current
  //  point being dragged, use it as the reference by which to snap to the grid.
  // Otherwise the only other thing left to do is use the first item
  //  as the reference, done below.
  if(rasterize && _tool == AutomationTool && automation.currentCtrlValid)
  {
    const unsigned int tck = MusEGlobal::tempomap.frame2tick(automation.currentFrame);
    unsigned int newTck = (deltaX < 0 && (unsigned int)-deltaX > tck) ? 0 : tck + deltaX;
    const unsigned int newTckSnap = MusEGlobal::sigmap.raster(newTck, *_raster);
    if(newTckSnap != newTck)
    {
      const int snapDiff = newTckSnap - newTck;
      deltaX += snapDiff;
    }
  }

  // If we are NOT in AutomationTool mode, limit the movement of audio
  //  automation vertices that are also moving, to the horizontal direction.
  const double curTrackDeltaYNormalized =
    (_tool == AutomationTool && automation.currentTrack) ? ((double)deltaY / (double)automation.currentTrack->height()) : 0.0;

  for(MusECore::iAudioAutomationItemTrackMap iatm = automation.currentCtrlFrameList.begin(); iatm != automation.currentCtrlFrameList.end(); ++iatm)
  {
    const MusECore::Track* track = iatm->first;
    if(track->isMidiTrack())
      continue;
    const MusECore::AudioTrack* at = static_cast<const MusECore::AudioTrack*>(track);
    MusECore::AudioAutomationItemMap& atm = iatm->second;
    for(MusECore::iAudioAutomationItemMap iaim = atm.begin(); iaim != atm.end(); ++iaim)
    {
      const int ctrlId = iaim->first;
      MusECore::ciCtrlList icl = at->controller()->find(ctrlId);
      if(icl == at->controller()->cend())
        continue;
      const MusECore::CtrlList* cl = icl->second;

      MusECore::AudioAutomationItemList& ail = iaim->second._selectedList;
      bool changed = false;
      int adjDeltaX = deltaX;
      for(MusECore::iAudioAutomationItemList iail = ail.begin(); iail != ail.end(); ++iail)
      {
        MusECore::AudioAutomationItem& aai = iail->second;
        const unsigned int oldFrame = iail->first;
        const unsigned int oldTick = MusEGlobal::tempomap.frame2tick(oldFrame);

        // At the first item, check if the movement is out of bounds
        //  and adjust the deltaX accordingly this once so that all the next
        //  items in this item list also have this adjustment in deltaX.
        // We CANNOT have automation points bunched up at an edge because that results
        //  in more than one point at a given time position. The movement can also
        //  cause an item's working frame to be BEFORE a later item's working frame
        //  which causes malaise because the drawing tries to draw backwards and
        //  the end movement routines can freeze etc.
        if(iail == ail.begin() && adjDeltaX < 0 && (unsigned int)-adjDeltaX > oldTick)
          adjDeltaX += -adjDeltaX - oldTick;

        unsigned int newTick = oldTick + adjDeltaX;
        // If we want to rasterize, and we're in automation tool mode but there is no current point being
        //  dragged OR we're not in automation tool mode, not much choice but to just use the first item
        //  as the reference by which to snap to the grid.
        if(rasterize && (_tool != AutomationTool || !automation.currentCtrlValid) && iail == ail.begin())
        {
          const unsigned int newTickSnap = MusEGlobal::sigmap.raster(newTick, *_raster);
          if(newTickSnap != newTick)
          {
            const int snapDiff = newTickSnap - newTick;
            adjDeltaX += snapDiff;
            newTick = newTickSnap;
          }
        }

        const int deltaFrame = MusEGlobal::tempomap.deltaTick2frame(oldTick, newTick);
        const unsigned int newFrame =
          (deltaFrame < 0 && ((unsigned int)-deltaFrame) > oldFrame) ? 0 : oldFrame + deltaFrame;

        const bool isCurrent =
          automation.currentCtrlValid &&
          automation.currentTrack == track &&
          automation.currentCtrlList == cl &&
          automation.currentWorkingFrame == aai._wrkFrame;

        {
          if(deltaY != 0)
          {
            double min, max;
            cl->range(&min,&max);
            double cvval;
            if (cl->valueType() == MusECore::VAL_LOG  ) { // use db scale for volume
                cvval = deltaValToLog(aai._value, -curTrackDeltaYNormalized, min, max);
            }
            else {
              // we need to set val between 0 and 1 (unless integer)
              cvval = -curTrackDeltaYNormalized * (max-min) + aai._value;
              // 'Snap' to integer or boolean
              if (cl->mode() == MusECore::CtrlList::DISCRETE)
                cvval = rint(cvval + 0.1); // LADSPA docs say add a slight bias to avoid rounding errors. Try this.
              if (cvval< min) cvval=min;
              if (cvval>max) cvval=max;
            }

            if(isCurrent)
            {
              // Store the text.
              double displayCvVal =  cvval;
              if(cl->valueType() == MusECore::VAL_LOG)
                displayCvVal = muse_val2db(cvval);
              automation.currentText = QString("Param:%1 Value:%2").arg(cl->name()).arg(displayCvVal, 0, 'g', 3);
              automation.currentVal = cvval;
            }

            // If the values are not exactly equal.
            if(aai._wrkVal != cvval)
            {
              changed = true;
              aai._wrkVal = cvval;
            }
          }

          //if(oldFrame != newFrame)
          if(aai._wrkFrame != newFrame)
          {
            changed = true;
            aai._wrkFrame = newFrame;
            if(isCurrent)
              automation.currentWorkingFrame = aai._wrkFrame;
          }
        }
      }
      if(changed)
        controllerChanged(track, cl->id());
    }
  }
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

      Undo operations;
      moveCanvasItems(moving, dp, dx, dragtype, operations, rasterize);

      const bool isCopy = dragtype == MOVE_COPY;

      if(commitAutomationChanges(operations, isCopy))
        MusEGlobal::song->beginAudioCtrlMoveMode(operations);

      automation.controllerState = doNothing;
      // Direction argument doesn't matter, just pass zero.
      processAutomationMovements(pos, 0, false);

      if(!operations.empty())
      {
        MusEGlobal::song->applyOperationGroup(operations);
        automation.breakUndoCombo = false;
      }

      moving.clear();
      itemSelectionsChanged();
      redraw();
      }

void PartCanvas::setRangeToSelection() {

    CItem *leftmost = nullptr, *rightmost = nullptr;

    for (const auto& i : items) {
        if (i.second->isSelected()) {
            if (!leftmost)
                leftmost = i.second;
            else if (leftmost->x() > i.second->x())
                leftmost = i.second;

            if (!rightmost)
                rightmost = i.second;
            else
                if (rightmost->x() < i.second->x())
                    rightmost = i.second;
        }
    }

    if (leftmost && rightmost)
    {
        int left_tick = leftmost->part()->tick();
        int right_tick = rightmost->part()->tick() + rightmost->part()->lenTick();
        MusECore::Pos p1(left_tick, true);
        MusECore::Pos p2(right_tick, true);

        if (p1 < MusEGlobal::song->lPos()) {
            MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
            MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
        } else {
            MusEGlobal::song->setPos(MusECore::Song::RPOS, p2);
            MusEGlobal::song->setPos(MusECore::Song::LPOS, p1);
        }
    }
}

int PartCanvas::currentPartColorIndex() const
{
  return curColorIndex;
}

//---------------------------------------------------------
//   isSingleAudioAutomationSelection
//---------------------------------------------------------

bool PartCanvas::isSingleAudioAutomationSelection() const
      {
      return audioAutomationSelectionSize() == 1;
      }

//---------------------------------------------------------
//   audioAutomationSelectionSize
//---------------------------------------------------------

int PartCanvas::audioAutomationSelectionSize() const
      {
      int n = 0;
      for(MusECore::ciAudioAutomationItemTrackMap iaaitm = automation.currentCtrlFrameList.cbegin();
          iaaitm != automation.currentCtrlFrameList.cend(); ++iaaitm)
      {
        const MusECore::AudioAutomationItemMap& aaim = iaaitm->second;
        for(MusECore::ciAudioAutomationItemMap iaaim = aaim.cbegin(); iaaim != aaim.cend(); ++iaaim)
        {
          const MusECore::AudioAutomationItemList& aail = iaaim->second._selectedList;
          n += aail.size();
        }
      }
      return n;
      }

//---------------------------------------------------------
//   audioAutomationItemsAreSelected
//---------------------------------------------------------

bool PartCanvas::audioAutomationItemsAreSelected() const
      {
      // Technically we really ought to iterate all tracks and controllers and items
      //  checking for selected, but that would likely be very time-consuming.
      // So we will have to just trust our own selection list.
      return automation.currentCtrlFrameList.itemsAreSelected();
      }

//---------------------------------------------------------
//   tagItems
//---------------------------------------------------------

void PartCanvas::tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const
{
  // Let the canvas handle any Event based items.
  Canvas::tagItems(tag_list, options);

  const bool tagSelected = options._flags & MusECore::TagSelected;
  const bool tagAllItems = options._flags & MusECore::TagAllItems;
  const bool range       = options._flags & MusECore::TagRange;
  const MusECore::Pos& p0 = options._p0;
  const MusECore::Pos& p1 = options._p1;
  const unsigned int p0frame = p0.frame();
  const unsigned int p1frame = p1.frame();

  for (MusECore::ciTrack it = tracks->cbegin(); it != tracks->cend(); ++it) {
    if((*it)->isMidiTrack())
      continue;
    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(*it);
      // Do not include hidden tracks.
    if (!track->isVisible())
      continue;
    const MusECore::CtrlListList* cll = track->controller();
    for(MusECore::ciCtrlList icll = cll->cbegin(); icll != cll->cend(); ++icll)
    {
      MusECore::CtrlList* cl = icll->second;
      // Do not include hidden controller lists.
      if(!cl->isVisible())
        continue;
      for(MusECore::ciCtrl ic = cl->cbegin(); ic != cl->cend(); ++ ic)
      {
        const unsigned int frame = ic->first;
        const MusECore::CtrlVal& cv = ic->second;

        if((tagAllItems
            || (tagSelected && cv.selected())
            /*|| (tagMoving && cv._isMoving)*/ )  // TODO ?
          && (!range || (frame >= p0frame && frame < p1frame)))
        {
          tag_list->add(track, cl, frame, cv.value());
        }
      }
    }
  }

}

int PartCanvas::automationPointRadius() const { return _automationPointRadius; }

void PartCanvas::setAutomationPointRadius(int r)
{
  _automationPointRadius = r;
  _automationPointExtraRadius = 1; // Fixed for now...
  _automationTopMargin = _automationPointRadius + 1;
  _automationBottomMargin = _automationPointRadius + 1;
}

} // namespace MusEGui
