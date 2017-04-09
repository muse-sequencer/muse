//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dlist.cpp,v 1.9.2.7 2009/10/16 21:50:16 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QCursor>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>

#include <stdio.h>

#include "globals.h"
#include "globaldefs.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "pitchedit.h"
#include "midiport.h"
#include "drummap.h"
#include "drumedit.h"
#include "helper.h"
#include "icons.h"
#include "dlist.h"
#include "song.h"
#include "dcanvas.h"
#include "minstrument.h"

namespace MusEGui {

//---------------------------------------------------------
//   DLineEdit
//---------------------------------------------------------

DLineEdit::DLineEdit(QWidget* parent) : QLineEdit(parent)
{
  // Reset these since our parent will typically turn them on for speed.
  setAutoFillBackground(true);
  setAttribute(Qt::WA_NoSystemBackground, false);
  setAttribute(Qt::WA_StaticContents, false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);

  setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  setContentsMargins(0, 0, 0, 0);
}

bool DLineEdit::event(QEvent* e)
{
  switch(e->type())
  {
    case QEvent::KeyPress:
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(e);
      switch(ke->key())
      {
        // For return, we want to close the editor but don't want the
        //  parent to receive the event which will just open the box again.
        case Qt::Key_Return:
        case Qt::Key_Enter:
          e->accept();
          emit returnPressed();
          return true;
        break;

        case Qt::Key_Escape:
        {
          e->accept();
          emit escapePressed();
          return true;
        }
        break;

        default:
        break;
      }
    }
    break;

    case QEvent::NonClientAreaMouseButtonPress:
      // FIXME: Doesn't work.
      //fprintf(stderr, "DLineEdit::event NonClientAreaMouseButtonPress\n");
    case QEvent::FocusOut:
      e->accept();
      emit returnPressed();
      return true;
    break;

    default:
    break;
  }

  // Do not pass ANY events on to the parent.
  QLineEdit::event(e);
  e->accept();
  return true;
}

//---------------------------------------------------------
//   DrumListSpinBox
//---------------------------------------------------------

DrumListSpinBox::DrumListSpinBox(QWidget* parent) : QSpinBox(parent)
{
  // Reset these since our parent will typically turn them on for speed.
  setAutoFillBackground(true);
  setAttribute(Qt::WA_NoSystemBackground, false);
  setAttribute(Qt::WA_StaticContents, false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);

  setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  setContentsMargins(0, 0, 0, 0);
}

bool DrumListSpinBox::event(QEvent* e)
{
  switch(e->type())
  {
    case QEvent::KeyPress:
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(e);
      switch(ke->key())
      {
        // For return, we want to close the editor but don't want the
        //  parent to receive the event which will just open the box again.
        case Qt::Key_Return:
        case Qt::Key_Enter:
          e->accept();
          emit returnPressed();
          return true;
        break;

        case Qt::Key_Escape:
        {
          e->accept();
          emit escapePressed();
          return true;
        }
        break;

        default:
        break;
      }
    }
    break;

    case QEvent::NonClientAreaMouseButtonPress:
      // FIXME: Doesn't work.
      //fprintf(stderr, "DLineEdit::event NonClientAreaMouseButtonPress\n");
    case QEvent::FocusOut:
      e->accept();
      emit returnPressed();
      return true;
    break;

    default:
    break;
  }

  // Do not pass ANY events on to the parent.
  QSpinBox::event(e);
  e->accept();
  return true;
}

//---------------------------------------------------------
//   DPitchEdit
//---------------------------------------------------------

DPitchEdit::DPitchEdit(QWidget* parent) : PitchEdit(parent)
{
  // Reset these since our parent will typically turn them on for speed.
  setAutoFillBackground(true);
  setAttribute(Qt::WA_NoSystemBackground, false);
  setAttribute(Qt::WA_StaticContents, false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);

  setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  setContentsMargins(0, 0, 0, 0);
}

bool DPitchEdit::event(QEvent* e)
{
  switch(e->type())
  {
    case QEvent::KeyPress:
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(e);
      switch(ke->key())
      {
        // For return, we want to close the editor but don't want the
        //  parent to receive the event which will just open the box again.
        case Qt::Key_Return:
        case Qt::Key_Enter:
          e->accept();
          emit returnPressed();
          return true;
        break;

        case Qt::Key_Escape:
        {
          e->accept();
          emit escapePressed();
          return true;
        }
        break;

        default:
        break;
      }
    }
    break;

    case QEvent::NonClientAreaMouseButtonPress:
      // FIXME: Doesn't work.
      //fprintf(stderr, "DPitchEdit::event NonClientAreaMouseButtonPress\n");
    case QEvent::FocusOut:
      e->accept();
      emit returnPressed();
      return true;
    break;

    default:
    break;
  }

  // Do not pass ANY events on to the parent.
  Awl::PitchEdit::event(e);
  e->accept();
  return true;
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void DList::draw(QPainter& p, const QRect& rect)
      {
      using MusECore::WorkingDrumMapEntry;

      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();

      //---------------------------------------------------
      //    Tracks
      //---------------------------------------------------

      p.setPen(Qt::black);
      QColor override_col(Qt::gray);
      override_col.setAlpha(64);

      QFont fnt(p.font());

      for (int instrument = 0; instrument < ourDrumMapSize; ++instrument) {
            int yy = instrument * TH;
            if (yy+TH < y)
                  continue;
            if (yy > y + h)
                  break;
            MusECore::DrumMap* dm = &ourDrumMap[instrument];
            if (dm == currentlySelected)
                  p.fillRect(x, yy, w, TH, Qt::yellow);
//            else
//                  p.eraseRect(x, yy, w, TH); DELETETHIS?
            QHeaderView *h = header;
            p.save();
            p.setWorldMatrixEnabled(false);
            for (int k = 0; k < h->count(); ++k) {
                  if (h->isSectionHidden(k))
                      continue;

                  int isWorkingItem = MusECore::WorkingDrumMapEntry::NoOverride;

                  int x   = h->sectionPosition(k);
                  int w   = h->sectionSize(k);
                  //QRect r = p.combinedTransform().mapRect(QRect(x+2, yy, w-4, TH));  // Gives inconsistent positions. Source shows wrong operation for our needs.
                  QRect r = map(QRect(x+2, yy, w-4, TH));                              // Use our own map instead.
                  QString s;
                  int align = Qt::AlignVCenter | Qt::AlignHCenter;

                  bool doOverrideFill = true;

                  switch (k) {
                        case COL_VOLUME:
                              s.setNum(dm->vol);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::VolField);
                              break;
                        case COL_QUANT:
                              s.setNum(dm->quant);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::QuantField);
                              break;
                        case COL_NOTELENGTH:
                              s.setNum(dm->len);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::LenField);
                              break;
                        case COL_NOTE:
                              s =  MusECore::pitch2string(dm->anote);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::ANoteField);
                              break;
                        case COL_INPUTTRIGGER:
                              s =  MusECore::pitch2string(dm->enote);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::ENoteField);
                              break;
                        case COL_LEVEL1:
                              s.setNum(dm->lv1);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::Lv1Field);
                              break;
                        case COL_LEVEL2:
                              s.setNum(dm->lv2);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::Lv2Field);
                              break;
                        case COL_LEVEL3:
                              s.setNum(dm->lv3);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::Lv3Field);
                              break;
                        case COL_LEVEL4:
                              s.setNum(dm->lv4);
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::Lv4Field);
                              break;
                        case COL_HIDE:
                        {
                              //  isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, VolField);
                              doOverrideFill = false;
                              if(dcanvas)
                              {
                                bool hidden=false;
                                bool shown=false;
                                QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                                int pitch = dcanvas->get_instrument_map()[instrument].pitch;

                                for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end() && !(hidden&&shown); track++)
  //                                 if (dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch])
                                  if (dynamic_cast<MusECore::MidiTrack*>(*track)->drummap()[pitch].hide)
                                    hidden=true;
                                  else
                                    shown=true;

                                if (!hidden && !shown)
                                  printf("THIS SHOULD NEVER HAPPEN: in DList::draw(): instrument %i's track group is empty. strange...\n", instrument);

                                const QPixmap* pm = NULL;

                                if (shown && !hidden)
                                      pm = eyeIcon;
                                else if (!shown && hidden)
                                      pm = eyeCrossedIcon;
                                else if (shown && hidden)
                                      pm = eyeGrayIcon;
                                else //if (!shown && !hidden)
                                      pm = NULL;

                                if (pm)
                                {
                                // p.setPen(Qt::red);
                                  p.drawPixmap(
                                    r.x() + r.width()/2 - pm->width()/2,
                                    r.y() + r.height()/2 - pm->height()/2,
                                    *pm);
                                // p.setPen(Qt::black);
                                }
                              }
                              else
                              {
                                const QPixmap* pm = dm->hide ? eyeCrossedIcon : eyeIcon;
                                if (dm->hide)
                                  pm = eyeCrossedIcon;
                                else
                                  pm = eyeIcon;
                                // p.setPen(Qt::red);
                                p.drawPixmap(
                                  r.x() + r.width()/2 - pm->width()/2,
                                  r.y() + r.height()/2 - pm->height()/2,
                                  *pm);
                                // p.setPen(Qt::black);
                              }
                                    
                              break;
                        }
                        case COL_MUTE:
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::MuteField);
                              doOverrideFill = false;
                              if(isWorkingItem == MusECore::WorkingDrumMapEntry::NoOverride)
                                p.fillRect(r, override_col);
                              if (dm->mute) {
                                    p.setPen(Qt::red);
                                    const QPixmap& pm = *muteIcon;
                                    p.drawPixmap(
                                       r.x() + r.width()/2 - pm.width()/2,
                                       r.y() + r.height()/2 - pm.height()/2,
                                       pm);
                                    p.setPen(Qt::black);
                                    }
                              break;
                        case COL_NAME:
                              {
                                if(dcanvas)
                                  isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::NameField);
                                doOverrideFill = false;
                                if(isWorkingItem == MusECore::WorkingDrumMapEntry::NoOverride)
                                  p.fillRect(r, override_col);
                                if(dcanvas && dcanvas->part())
                                {
                                  MusECore::Part* cur_part = dcanvas->part();
                                  if(cur_part->track() && cur_part->track()->isMidiTrack())
                                  {
                                    MusECore::MidiTrack* cur_track = static_cast<MusECore::MidiTrack*>(cur_part->track());
                                    int cur_channel      = cur_track->outChannel();
                                    MusECore::MidiPort* cur_port   = &MusEGlobal::midiPorts[cur_track->outPort()];

                                    if(old_style_drummap_mode)
                                    {
                                      // Default to track port if -1 and track channel if -1.
                                      int channel = dm->channel;
                                      if(channel == -1)
                                        channel = cur_channel;
                                      int mport = dm->port;
                                      if(mport == -1)
                                        mport = cur_track->outPort();
                                      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mport];
                                      int instr_pitch = dm->anote;
                                      MusECore::MidiCtrlValListList* cll = mp->controller();
                                      const int min = channel << 24;
                                      const int max = min + 0x1000000;
                                      bool found = false;
                                      bool used = false;
                                      bool off = true;
                                      for(MusECore::iMidiCtrlValList imcvl = cll->lower_bound(min); imcvl != cll->lower_bound(max); ++imcvl)
                                      {
                                        MusECore::MidiCtrlValList* cl = imcvl->second;
                                        MusECore::MidiController* c   = mp->midiController(cl->num());
                                        if(!c->isPerNoteController())
                                          continue;
                                        int cnum = c->num();
                                        int num = cl->num();
                                        int pitch = num & 0x7f;
                                        if(pitch != instr_pitch)
                                          continue;

                                        found = true;
                                        for(MusECore::ciEvent ie = cur_part->events().begin(); ie != cur_part->events().end(); ++ie)
                                        {
                                          MusECore::Event e = ie->second;
                                          if(e.type() != MusECore::Controller)
                                            continue;
                                          int ctl_num = e.dataA();
                                          if((ctl_num | 0xff) == cnum && (ctl_num & 0x7f) == instrument)
                                          {
                                            used = true;
                                            break;
                                          }
                                        }
                                        off = cl->hwVal() == MusECore::CTRL_VAL_UNKNOWN;  // Does it have a value or is it 'off'?
                                        if(used && !off)
                                          break;  // We have all the info we need, done.
                                      }

                                      if(found)
                                      {
                                        int rx = r.x() + 1;
                                        int ry = r.y() + r.height()/2 - 3;
                                        int rw = 6;
                                        int rh = 6;
                                        if(used)
                                        {
                                          if(off)
                                            p.drawPixmap(rx, ry, rw, rh, *greendot12x12Icon);
                                          else
                                            p.drawPixmap(rx, ry, rw, rh, *orangedot12x12Icon);
                                        }
                                        else
                                        {
                                          if(off)
                                            p.drawPixmap(rx, ry, rw, rh, *graydot12x12Icon);
                                          else
                                            p.drawPixmap(rx, ry, rw, rh, *bluedot12x12Icon);
                                        }
                                      }
                                    }
                                    else
                                    {

                                      QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                                      bool found = false;
                                      bool used = false;
                                      bool off = true;

                                      // REMOVE Tim. Or FIXME. An attempt to light the dots while respecting grouping.
                                      //if(!group->empty())
                                      {
//                                         int group_size = group->size();
//                                         if(group_size == 1)
//                                         {
//                                           MusECore::Track* t = *group->begin();
//                                           MusECore::PartList* part_list = dcanvas->drumEdit()->parts();
//                                           for(MusECore::ciPart ip = part_list->cbegin(); ip != part_list->cend(); ++ip) {
//                                             if(ip->second->track() == t) {
//                                                }
//                                           }
//                                         }

                                        int instr_pitch = dcanvas->get_instrument_map()[instrument].pitch;

                                        //if(//dcanvas->drumEdit()->group_mode() == DrumEdit::DONT_GROUP ||
                                          //dcanvas->drumEdit()->group_mode() == DrumEdit::GROUP_MAX ||
                                          //dcanvas->drumEdit()->group_mode() == DrumEdit::GROUP_SAME_CHANNEL ||
                                          //group_size >= 2 && group->find(cur_track) != group->end())
                                        //for(QSet<MusECore::Track*>::iterator it = group->begin(); it != group->end(); ++it)
                                        if(group->find(cur_track) != group->end())
                                        {
                                          //if(!(*it)->isMidiTrack())
                                          //  continue;
                                          //MusECore::MidiTrack* track = static_cast<MusECore::MidiTrack*>(*it);
                                          //MusECore::MidiPort* mp = &MusEGlobal::midiPorts[track->outPort()];
                                          //MusECore::MidiCtrlValListList* cll = mp->controller();
                                          MusECore::MidiCtrlValListList* cll = cur_port->controller();
                                          //int channel = track->outChannel();
                                          //const int min = channel << 24;
                                          const int min = cur_channel << 24;
                                          const int max = min + 0x1000000;

                                          for(MusECore::iMidiCtrlValList imcvl = cll->lower_bound(min); imcvl != cll->lower_bound(max); ++imcvl)
                                          {
                                            MusECore::MidiCtrlValList* cl = imcvl->second;
                                            MusECore::MidiController* c   = cur_port->midiController(cl->num());
                                            if(!c->isPerNoteController())
                                              continue;
                                            int cnum = c->num();
                                            int num = cl->num();
                                            int pitch = num & 0x7f;
                                            if(pitch != instr_pitch)
                                              continue;

                                            found = true;
                                            const MusECore::EventList& el = cur_part->events();
                                            //MusECore::PartList* part_list = dcanvas->drumEdit()->parts();
                                            //for(MusECore::ciPart ip = part_list->cbegin(); ip != part_list->cend(); ++ip)
                                            {
                                              //MusECore::Part* part = ip->second;
                                              ///if(part->track() != 
                                              //const MusECore::EventList& el = part->events();
                                              for(MusECore::ciEvent ie = el.begin(); ie != el.end(); ++ie)
                                              {
                                                MusECore::Event e = ie->second;
                                                if(e.type() != MusECore::Controller)
                                                  continue;
                                                int ctl_num = e.dataA();
                                                if((ctl_num | 0xff) == cnum && (ctl_num & 0x7f) == pitch)
                                                {
                                                  used = true;
                                                  break;
                                                }
                                              }
                                            }
                                            off = cl->hwVal() == MusECore::CTRL_VAL_UNKNOWN;  // Does it have a value or is it 'off'?
                                            if(used && !off)
                                              break;  // We have all the info we need, done.
                                          }
                                        }

                                        if(found)
                                        {
                                          int rx = r.x() + 1;
                                          int ry = r.y() + r.height()/2 - 3;
                                          int rw = 6;
                                          int rh = 6;
                                          if(used)
                                          {
                                            if(off)
                                              p.drawPixmap(rx, ry, rw, rh, *greendot12x12Icon);
                                            else
                                              p.drawPixmap(rx, ry, rw, rh, *orangedot12x12Icon);
                                          }
                                          else
                                          {
                                            if(off)
                                              p.drawPixmap(rx, ry, rw, rh, *graydot12x12Icon);
                                            else
                                              p.drawPixmap(rx, ry, rw, rh, *bluedot12x12Icon);
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                                QString str = dm->name;
                                align = Qt::AlignVCenter | Qt::AlignLeft;
                                fnt.setItalic(false);
                                fnt.setBold(false);

#ifdef _USE_INSTRUMENT_OVERRIDES_
                                if(isWorkingItem &
                                  (MusECore::WorkingDrumMapEntry::TrackOverride | MusECore::WorkingDrumMapEntry::TrackDefaultOverride))
                                  fnt.setBold(true);
                                else if(isWorkingItem &
                                  (MusECore::WorkingDrumMapEntry::InstrumentOverride | MusECore::WorkingDrumMapEntry::InstrumentDefaultOverride))
                                  fnt.setItalic(true);
#else
                                if(isWorkingItem & (MusECore::WorkingDrumMapEntry::TrackOverride |
                                                    MusECore::WorkingDrumMapEntry::TrackDefaultOverride))
                                  fnt.setBold(true);
                                if( (isWorkingItem & MusECore::WorkingDrumMapEntry::TrackDefaultOverride) &&
                                   !(isWorkingItem & MusECore::WorkingDrumMapEntry::TrackOverride))
                                  fnt.setItalic(true);
#endif

                                p.setFont(fnt);
                                p.drawText(r.x() + 8, r.y(), r.width() - 8, r.height(), align, str);
                              }
                              break;
                              
                        // Default to track port if -1 and track channel if -1.
                        case COL_OUTCHANNEL:
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::ChanField);
                              if(dm->channel != -1)
                                s.setNum(dm->channel+1);
                              break;
                        case COL_OUTPORT:
                              if(dcanvas)
                                isWorkingItem = dcanvas->isWorkingMapInstrument(instrument, WorkingDrumMapEntry::PortField);
                              if(dm->port != -1)
                                s.sprintf("%d:%s", dm->port+1, MusEGlobal::midiPorts[dm->port].portname().toLatin1().constData());
                              align = Qt::AlignVCenter | Qt::AlignLeft;
                              break;
                        }

                  if(doOverrideFill && isWorkingItem == MusECore::WorkingDrumMapEntry::NoOverride)
                    p.fillRect(r, override_col);

                  if (!s.isEmpty())
                  {
                        fnt.setItalic(false);
                        fnt.setBold(false);

#ifdef _USE_INSTRUMENT_OVERRIDES_
                        if(isWorkingItem &
                           (MusECore::WorkingDrumMapEntry::TrackOverride | MusECore::WorkingDrumMapEntry::TrackDefaultOverride))
                          fnt.setBold(true);
                        else if(isWorkingItem &
                           (MusECore::WorkingDrumMapEntry::InstrumentOverride | MusECore::WorkingDrumMapEntry::InstrumentDefaultOverride))
                          fnt.setItalic(true);
#else
                        if(isWorkingItem & (MusECore::WorkingDrumMapEntry::TrackOverride |
                                            MusECore::WorkingDrumMapEntry::TrackDefaultOverride))
                          fnt.setBold(true);
                        if( (isWorkingItem & MusECore::WorkingDrumMapEntry::TrackDefaultOverride) &&
                           !(isWorkingItem & MusECore::WorkingDrumMapEntry::TrackOverride))
                          fnt.setItalic(true);
#endif

                        p.setFont(fnt);
                        p.drawText(r, align, s);
                  }
                  }
            p.restore();
            }

      //---------------------------------------------------
      //    horizontal lines
      //---------------------------------------------------

      p.setPen(Qt::gray);
      int yy  = (y / TH) * TH;
      for (; yy < y + h; yy += TH) {
            p.drawLine(x, yy, x + w, yy);
            }

      if (drag == DRAG) {
            int y  = (startY/TH) * TH;
            int dy = startY - y;
            int yy = curY - dy;
            p.setPen(Qt::green);
            p.drawLine(x, yy, x + w, yy);
            p.drawLine(x, yy+TH, x + w, yy+TH);
            p.setPen(Qt::gray);
            }

      //---------------------------------------------------
      //    vertical Lines
      //---------------------------------------------------

      p.setWorldMatrixEnabled(false);
      int n = header->count();
      x = 0;
      for (int i = 0; i < n; i++) {
            x += header->sectionSize(header->visualIndex(i));
            p.drawLine(x, 0, x, height());
            }
      p.setWorldMatrixEnabled(true);
      }

//---------------------------------------------------------
//   devicesPopupMenu
//---------------------------------------------------------

bool DList::devicesPopupMenu(MusECore::DrumMap* t, int x, int y)
      {
      QMenu* p = MusECore::midiPortsPopup(0, t->port, true);  // Include a "<Default>" entry. Do not pass parent! Causes accelerators to be returned in QAction::text() !
      QAction* act = p->exec(mapToGlobal(QPoint(x, y)), 0);
      if(!act)
      {
        delete p;
        return false;
      }

      int n = act->data().toInt();
      delete p;

      const int openConfigId = MIDI_PORTS;
      const int defaultId    = MIDI_PORTS + 1;

      if(n < 0 || n > defaultId)     // Invalid item.
        return false;

      if(n == openConfigId)    // Show port config dialog.
      {
        MusEGlobal::muse->configMidiPorts();
        return false;
      }

      if(n == defaultId)   // Means the <default> -1
        n = -1;

      bool changed = false;
      if(n != t->port)
      {
        t->port = n;      // -1 is allowed
        changed = true;
      }
      return changed;
    }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void DList::viewMousePressEvent(QMouseEvent* ev)
      {
      ev->accept();

      int x      = ev->x();
      int y      = ev->y();
      int button = ev->button();
      int instrument = y / TH;
      if (instrument >= ourDrumMapSize) instrument=ourDrumMapSize-1;
      if (instrument < 0) instrument=0;
      if (ourDrumMapSize==0) return;

      setCurDrumInstrument(instrument);

      MusECore::DrumMap* dm = &ourDrumMap[instrument];
      MusECore::DrumMap dm_old = *dm;

      startY = y;
      sInstrument = instrument;
      drag   = START_DRAG;

      DrumColumn col = DrumColumn(x2col(x));

      if(button == Qt::RightButton)
      {
        if(dcanvas && !old_style_drummap_mode)
        {
          enum MapPopupIDs { HideInstrumentID = 0, ShowInstrumentID,
                            ResetFieldID, ResetItemID, ResetColumnID, ResetMapID,
                            SetFieldID, SetItemID, SetColumnID, SetMapID,
                            SetDefaultFieldID, SetDefaultItemID, SetDefaultColumnID, SetDefaultMapID,
                            ResetAllPatchMapsID,
#ifdef _USE_INSTRUMENT_OVERRIDES_
                            ResetInstrumentFieldID, ResetInstrumentItemID, ResetInstrumentMapID,
                            SetInstrumentFieldID, SetInstrumentItemID, SetInstrumentMapID
#endif
          };

          const int field = col2Field(col);
          const int overrides = dcanvas->isWorkingMapInstrument(instrument, field);
          const bool has_overrides = dcanvas->hasOverrides(instrument);

          const bool track_override = overrides & MusECore::WorkingDrumMapEntry::TrackOverride;
          const bool track_def_override = overrides & MusECore::WorkingDrumMapEntry::TrackDefaultOverride;
#ifdef _USE_INSTRUMENT_OVERRIDES_
          const bool instr_override = overrides & MusECore::WorkingDrumMapEntry::InstrumentOverride;
          const bool instr_def_override = overrides & MusECore::WorkingDrumMapEntry::InstrumentDefaultOverride;
#endif

          const int all_fields_overrides = dcanvas->isWorkingMapInstrument(instrument, MusECore::WorkingDrumMapEntry::AllFields);
          const bool all_fields_track_override = all_fields_overrides & MusECore::WorkingDrumMapEntry::TrackOverride;
          const bool all_fields_track_def_override = all_fields_overrides & MusECore::WorkingDrumMapEntry::TrackDefaultOverride;
#ifdef _USE_INSTRUMENT_OVERRIDES_
          const bool all_fields_instr_override = all_fields_overrides & MusECore::WorkingDrumMapEntry::InstrumentOverride;
          const bool all_fields_instr_def_override = all_fields_overrides & MusECore::WorkingDrumMapEntry::InstrumentDefaultOverride;
#endif

          bool hidden=false;
          bool shown=false;
          QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
          int pitch = dcanvas->get_instrument_map()[instrument].pitch;

          for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end() && !(hidden&&shown); track++)
            if (dynamic_cast<MusECore::MidiTrack*>(*track)->drummap()[pitch].hide)
              hidden=true;
            else
              shown=true;

          QMenu* popup = new QMenu(NULL /* intendedly not "this" */);

          popup->setToolTipsVisible(true);

          QAction* act = popup->addAction(tr("Hide this instrument"));
          if(!shown)
            act->setEnabled(false);
          act->setData(HideInstrumentID);
          act->setToolTip(tr("This turns a blue eye into a crossed eye"));

          act = popup->addAction(tr("Show this instrument"));
          if(!hidden)
            act->setEnabled(false);
          act->setData(ShowInstrumentID);
          act->setToolTip(tr("This turns a crossed eye into a blue eye"));

          popup->addSeparator();

          act = popup->addAction(tr("Set field"));
          //act->setEnabled(field != MusECore::WorkingDrumMapEntry::ENoteField);
          act->setData(SetFieldID);
          act->setToolTip(tr("Sets a field"));

          act = popup->addAction(tr("Set row"));
          //act->setEnabled(field != MusECore::WorkingDrumMapEntry::ENoteField);
          act->setData(SetItemID);
          act->setToolTip(tr("Sets a row"));

          act = popup->addAction(tr("Set column"));
          act->setEnabled(field != MusECore::WorkingDrumMapEntry::ENoteField);
          act->setData(SetColumnID);
          act->setToolTip(tr("Sets a whole column to the field"));

          act = popup->addAction(tr("Set list"));
          //act->setEnabled(field != MusECore::WorkingDrumMapEntry::ENoteField);
          act->setData(SetMapID);
          act->setToolTip(tr("Sets the whole list"));

          popup->addSeparator();

          act = popup->addAction(tr("Reset field"));
          act->setEnabled(track_override || track_def_override);
          act->setData(ResetFieldID);
          act->setToolTip(tr("Resets a field in a row to default patch or instrument value"));

          act = popup->addAction(tr("Reset row"));
          act->setEnabled(all_fields_track_override || all_fields_track_def_override);
          act->setData(ResetItemID);
          act->setToolTip(tr("Resets a row to the instrument values"));

          act = popup->addAction(tr("Reset column"));
          //act->setEnabled(!all_fields_track_def_override);
          act->setData(ResetColumnID);
          act->setToolTip(tr("Resets a whole column to the the instrument values"));

          act = popup->addAction(tr("Reset list"));
          act->setEnabled(has_overrides);
          act->setData(ResetMapID);
          act->setToolTip(tr("Resets the whole list to the instrument values"));

          popup->addSeparator();
          
          act = popup->addAction(tr("Reset track's drum list"));
          act->setEnabled(dcanvas->hasOverrides(instrument));
          act->setData(ResetAllPatchMapsID);
          act->setToolTip(tr("Resets all lists on all patches to the instrument values"));

          popup->addSeparator();

          act = popup->addAction(tr("Promote field to default patch"));
          act->setData(SetDefaultFieldID);
          act->setToolTip(tr("Promotes a field in a row to the default patch"));

          act = popup->addAction(tr("Promote row to default patch"));
          act->setData(SetDefaultItemID);
          act->setToolTip(tr("Promotes a row to the default patch"));

          act = popup->addAction(tr("Promote column to default patch"));
          act->setData(SetDefaultColumnID);
          act->setToolTip(tr("Promotes a column to the default patch"));

          act = popup->addAction(tr("Promote list to default patch"));
          act->setData(SetDefaultMapID);
          act->setToolTip(tr("Promotes the whole list to the default patch"));

#ifdef _USE_INSTRUMENT_OVERRIDES_
          popup->addSeparator();

          act = popup->addAction(tr("Reset instrument field"));
          act->setEnabled(instr_override || instr_def_override);
          act->setData(ResetInstrumentFieldID);

          act = popup->addAction(tr("Reset instrument row"));
          act->setEnabled(all_fields_instr_override || all_fields_instr_def_override);
          act->setData(ResetInstrumentItemID);

          act = popup->addAction(tr("Reset instrument list"));
          act->setEnabled(all_fields_instr_override || all_fields_instr_def_override);
          act->setData(ResetInstrumentMapID);

          popup->addSeparator();

          act = popup->addAction(tr("Set instrument field"));
          act->setEnabled(track_override);
          act->setData(SetInstrumentFieldID);

          act = popup->addAction(tr("Set instrument row"));
          act->setEnabled(track_override);
          act->setData(SetInstrumentItemID);

          act = popup->addAction(tr("Set instrument list"));
          act->setEnabled(track_override);
          act->setData(SetInstrumentMapID);
#endif

          int id = -1;
          QAction* result = popup->exec(ev->globalPos());
          if(result)
            id = result->data().toInt();

          delete popup;

          switch(id)
          {
            case HideInstrumentID:
              dm->hide = true;
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::HideField, false, false, false, false);
            break;

            case ShowInstrumentID:
              dm->hide = false;
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::HideField, false, false, false, false);
            break;

            case ResetFieldID:
              // If there is a track override do not include defaults. This way a track override will be removed
              //  first, then if reset field is clicked again any track default override will removed.
              dcanvas->propagate_drummap_change(instrument, field, true, !track_override, false, false);
              //update();
            break;

            case ResetItemID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, true, true, false, false);
              //update();
            break;

            case ResetMapID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, true, true, false, true);
            break;

            case SetDefaultFieldID:
              dcanvas->propagate_drummap_change(instrument, field, false, true, false, false);
            break;

            case SetDefaultItemID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, false, true, false, false);
            break;

            case SetDefaultMapID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, false, true, false, true);
            break;

            case SetFieldID:
              dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
            break;

            case SetItemID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, false, false, false, false);
            break;

            case SetColumnID:
              dcanvas->propagate_drummap_change(instrument, field, false, false, false, true);
            break;

            case SetMapID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, false, false, false, true);
            break;

            case SetDefaultColumnID:
              dcanvas->propagate_drummap_change(instrument, field, false, true, false, true);
            break;

            case ResetColumnID:
              dcanvas->propagate_drummap_change(instrument, field, true, true, false, true);
            break;

            case ResetAllPatchMapsID:
              dcanvas->resetOverridesForAllPatches(instrument);
            break;

#ifdef _USE_INSTRUMENT_OVERRIDES_
            case ResetInstrumentFieldID:
              // If there is an instrument override do not include defaults. This way an instrument override will be removed
              //  first, then if reset instrument field is clicked again any instrument default override will removed.
              dcanvas->propagate_drummap_change(instrument, field, true, false, true, false);
              //update();
            break;

            case ResetInstrumentItemID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, true, true, true, false);
              //update();
            break;

            case ResetInstrumentMapID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, true, true, true, true);
            break;

            case SetInstrumentFieldID:
              dcanvas->propagate_drummap_change(instrument, field, false, false, true, false);
              //update();
            break;

            case SetInstrumentItemID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, false, false, true, false);
              //update();
            break;

            case SetInstrumentMapID:
              dcanvas->propagate_drummap_change(instrument, MusECore::WorkingDrumMapEntry::AllFields, false, false, true, true);
            break;
#endif

            default:
            return;
          }
        }
        return;
      }

      int field = MusECore::WorkingDrumMapEntry::NoField;
      switch (col) {
            case COL_NONE:
            case COL_VOLUME:
            case COL_QUANT:
            case COL_INPUTTRIGGER:
            case COL_NOTELENGTH:
            case COL_NOTE:
            case COL_OUTCHANNEL: // this column isn't visible in new style drum mode
            case COL_LEVEL1:
            case COL_LEVEL2:
            case COL_LEVEL3:
            case COL_LEVEL4:
                  break;
            case COL_HIDE:
                  if (button == Qt::LeftButton)
                  {
                    field = MusECore::WorkingDrumMapEntry::HideField;
                    dm->hide = !dm->hide;
                  }
                  break;
            case COL_MUTE:
                  field = MusECore::WorkingDrumMapEntry::MuteField;
                  if (button == Qt::LeftButton)
                        dm->mute = !dm->mute;
                  break;
            case COL_OUTPORT: // this column isn't visible in new style drum mode
                  field = MusECore::WorkingDrumMapEntry::PortField;
                  if ((button == Qt::RightButton) || (button == Qt::LeftButton)) {
                        devicesPopupMenu(dm, mapx(x), mapy(instrument * TH));
                        }
                  break;
            case COL_NAME:
                  field = MusECore::WorkingDrumMapEntry::NameField;
                  if (button == Qt::LeftButton)
                  {
                      int velo = 127 * (ev->x() - header->sectionPosition(COL_NAME)) / (header->sectionSize(COL_NAME) - 10);
                      if (velo <= 0) velo = 1;  // Zero note on vel is not allowed.
                      else if (velo > 127 ) velo = 127;
                      emit keyPressed(instrument, velo); //Mapping done on other side, send index
                  }
                  else if (button == Qt::MidButton && dcanvas) // hide that instrument
                  {
                    dm->hide = true;
                  }
                  break;
            default:
                  break;
            }

      update();
      if (!old_style_drummap_mode && dm_old != *dm && dcanvas) //something changed and we're in new style mode?
        dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);

      MusEGlobal::song->update(SC_DRUM_SELECTION);
      }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void DList::viewMouseDoubleClickEvent(QMouseEvent* ev)
      {
      int x = ev->x();
      int y = ev->y();
      unsigned instrument = y / TH;

      int section = header->logicalIndexAt(x);

      if (section == COL_NAME && (ev->button() == Qt::LeftButton))
         {
           lineEdit(instrument, section);
         }
      else if ((section == COL_VOLUME || section == COL_NOTELENGTH || section == COL_LEVEL1 ||
         section == COL_LEVEL2 || section == COL_LEVEL3 || section == COL_LEVEL4 || section == COL_QUANT ||
         section == COL_OUTCHANNEL ) && (ev->button() == Qt::LeftButton))
         {
           valEdit(instrument, section);
         }
      else if ((section == COL_NOTE || section == COL_INPUTTRIGGER) && (ev->button() == Qt::LeftButton))
        pitchEdit(instrument, section);
      else
            viewMousePressEvent(ev);
      }



//---------------------------------------------------------
//   lineEdit
//---------------------------------------------------------
void DList::lineEdit(int line, int section)
      {
            if (line >= ourDrumMapSize) line=ourDrumMapSize-1;
            if (line < 0) line=0;
            if (ourDrumMapSize==0) return;

            MusECore::DrumMap* dm = &ourDrumMap[line];
            editEntry = dm;
            if (editor == 0) {
                  editor = new DLineEdit(this);
                  connect(editor, SIGNAL(returnPressed()),
                     SLOT(returnPressed()));
                  connect(editor, SIGNAL(escapePressed()),
                     SLOT(escapePressed()));
                  editor->setFrame(true);
                  }
            int colx = mapx(header->sectionPosition(section));
            int colw = rmapx(header->sectionSize(section));
            int coly = mapy(line * TH);
            int colh = rmapy(TH);
            selectedColumn = section; //Store selected column to have an idea of which one was selected when return is pressed
            switch (section) {
                  case COL_NAME:
                  editor->setText(dm->name);
                  break;
            }

            editor->end(false);
            editor->setGeometry(colx, coly, colw, colh);
            editor->show();
            editor->setFocus();
     }

//---------------------------------------------------------
//   valEdit
//---------------------------------------------------------
void DList::valEdit(int line, int section)
      {
            if (line >= ourDrumMapSize) line=ourDrumMapSize-1;
            if (line < 0) line=0;
            if (ourDrumMapSize==0) return;

            MusECore::DrumMap* dm = &ourDrumMap[line];
            editEntry = dm;
            if (val_editor == 0) {
                  val_editor = new DrumListSpinBox(this);
                  connect(val_editor, SIGNAL(returnPressed()),
                     SLOT(valEdited()));
                  connect(val_editor, SIGNAL(escapePressed()),
                     SLOT(escapePressed()));
                  val_editor->setFrame(true);
                  }
            int colx = mapx(header->sectionPosition(section));
            int colw = rmapx(header->sectionSize(section));
            int coly = mapy(line * TH);
            int colh = rmapy(TH);
            selectedColumn = section; //Store selected column to have an idea of which one was selected when return is pressed
            switch (section) {
                  case COL_VOLUME:
                  val_editor->setRange(0, 250);
                  val_editor->setValue(dm->vol);
                  break;

                  case COL_NOTELENGTH:
                  val_editor->setRange(1, 1000000);
                  val_editor->setValue(dm->len);
                  break;

                  case COL_LEVEL1:
                  // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
                  val_editor->setRange(1, 127);
                  val_editor->setValue(dm->lv1);
                  break;

                  case COL_LEVEL2:
                  val_editor->setRange(1, 127);
                  val_editor->setValue(dm->lv2);
                  break;

                  case COL_LEVEL3:
                  val_editor->setRange(1, 127);
                  val_editor->setValue(dm->lv3);
                  break;

                  case COL_LEVEL4:
                  val_editor->setRange(1, 127);
                  val_editor->setValue(dm->lv4);
                  break;

                  case COL_QUANT:
                  val_editor->setRange(0, 1000000);
                  val_editor->setValue(dm->quant);
                  break;

                  case COL_OUTCHANNEL:
                  val_editor->setRange(0, MIDI_CHANNELS);
                  // Default to track port if -1 and track channel if -1.
                  if(dm->channel != -1)
                    val_editor->setValue(dm->channel+1);
                  break;
            }

            val_editor->setGeometry(colx, coly, colw, colh);
            val_editor->selectAll();
            val_editor->show();
            val_editor->setFocus();
     }

//---------------------------------------------------------
//   pitchEdit
//---------------------------------------------------------
void DList::pitchEdit(int line, int section)
      {
            if (line >= ourDrumMapSize) line=ourDrumMapSize-1;
            if (line < 0) line=0;
            if (ourDrumMapSize==0) return;

            MusECore::DrumMap* dm = &ourDrumMap[line];
            editEntry = dm;
            if (pitch_editor == 0) {
                  pitch_editor = new DPitchEdit(this);
                  connect(pitch_editor, SIGNAL(returnPressed()),
                     SLOT(pitchEdited()));
                  connect(pitch_editor, SIGNAL(escapePressed()),
                     SLOT(escapePressed()));
                  pitch_editor->setFrame(true);
                  }
            int colx = mapx(header->sectionPosition(section));
            int colw = rmapx(header->sectionSize(section));
            int coly = mapy(line * TH);
            int colh = rmapy(TH);
            selectedColumn = section; //Store selected column to have an idea of which one was selected when return is pressed
            switch (section) {
                  case COL_INPUTTRIGGER:
                  pitch_editor->setValue(dm->enote);
                  break;

                  case COL_NOTE:
                  pitch_editor->setValue(dm->anote);
                  break;
            }

            pitch_editor->setGeometry(colx, coly, colw, colh);
            pitch_editor->show();
            pitch_editor->setFocus();

     }


//---------------------------------------------------------
//   x2col
//---------------------------------------------------------

int DList::x2col(int x) const
      {
      int col = 0;
      int w = 0;
      for (; col < header->count(); col++) {
            w += header->sectionSize(col);
            if (x < w)
                  break;
            }
      if (col == header->count())
            return -1;
      return header->logicalIndex(col);
      }

int DList::col2Field(int col) const
{
  switch(col)
  {
    case COL_NONE:
      return MusECore::WorkingDrumMapEntry::NoField;
    case COL_HIDE:
      return MusECore::WorkingDrumMapEntry::HideField;
    case COL_MUTE:
      return MusECore::WorkingDrumMapEntry::MuteField;
    case COL_NAME:
      return MusECore::WorkingDrumMapEntry::NameField;
    case COL_VOLUME:
      return MusECore::WorkingDrumMapEntry::VolField;
    case COL_QUANT:
      return MusECore::WorkingDrumMapEntry::QuantField;
    case COL_INPUTTRIGGER:
      return MusECore::WorkingDrumMapEntry::ENoteField;
    case COL_NOTELENGTH:
      return MusECore::WorkingDrumMapEntry::LenField;
    case COL_NOTE:
      return MusECore::WorkingDrumMapEntry::ANoteField;
    case COL_OUTCHANNEL:
      return MusECore::WorkingDrumMapEntry::ChanField;
    case COL_OUTPORT:
      return MusECore::WorkingDrumMapEntry::PortField;
    case COL_LEVEL1:
      return MusECore::WorkingDrumMapEntry::Lv1Field;
    case COL_LEVEL2:
      return MusECore::WorkingDrumMapEntry::Lv2Field;
    case COL_LEVEL3:
      return MusECore::WorkingDrumMapEntry::Lv3Field;
    case COL_LEVEL4:
      return MusECore::WorkingDrumMapEntry::Lv4Field;
  }
  return MusECore::WorkingDrumMapEntry::NoField;
}

int DList::field2Col(int field) const
{
  switch(field)
  {
    case MusECore::WorkingDrumMapEntry::NoField:
      return COL_NONE;
    case MusECore::WorkingDrumMapEntry::HideField:
      return COL_HIDE;
    case MusECore::WorkingDrumMapEntry::MuteField:
      return COL_MUTE;
    case MusECore::WorkingDrumMapEntry::NameField:
      return COL_NAME;
    case MusECore::WorkingDrumMapEntry::VolField:
      return COL_VOLUME;
    case MusECore::WorkingDrumMapEntry::QuantField:
      return COL_QUANT;
    case MusECore::WorkingDrumMapEntry::ENoteField:
      return COL_INPUTTRIGGER;
    case MusECore::WorkingDrumMapEntry::LenField:
      return COL_NOTELENGTH;
    case MusECore::WorkingDrumMapEntry::ANoteField:
      return COL_NOTE;
    case MusECore::WorkingDrumMapEntry::ChanField:
      return COL_OUTCHANNEL;
    case MusECore::WorkingDrumMapEntry::PortField:
      return COL_OUTPORT;
    case MusECore::WorkingDrumMapEntry::Lv1Field:
      return COL_LEVEL1;
    case MusECore::WorkingDrumMapEntry::Lv2Field:
      return COL_LEVEL2;
    case MusECore::WorkingDrumMapEntry::Lv3Field:
      return COL_LEVEL3;
    case MusECore::WorkingDrumMapEntry::Lv4Field:
      return COL_LEVEL4;
  }
  return COL_NONE;
}


//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void DList::setCurDrumInstrument(int instr)
      {
      if (instr < 0 || instr >= ourDrumMapSize)
        return; // illegal instrument
      MusECore::DrumMap* dm = &ourDrumMap[instr];
      if (currentlySelected != dm) {
            currentlySelected = dm;
            emit curDrumInstrumentChanged(instr);
            //MusEGlobal::song->update(SC_DRUMMAP); //FINDMICHJETZT what for?? wtf?
            redraw(); // FINDMICHJETZT using redraw() instead of the above.
            }
      }

//---------------------------------------------------------
//   sizeChange
//---------------------------------------------------------

void DList::sizeChange(int section, int, int)
{
  redraw();

  if(editEntry==NULL)
    return;

  const int line = (editEntry-ourDrumMap);

  int colx = mapx(header->sectionPosition(section));
  int colw = rmapx(header->sectionSize(section));
  int coly = mapy(line * TH);
  int colh = rmapy(TH);

  if(editor && editor->isVisible())
    editor->setGeometry(colx, coly, colw, colh);

  if(val_editor && val_editor->isVisible())
    val_editor->setGeometry(colx, coly, colw, colh);

  if(pitch_editor && pitch_editor->isVisible())
    pitch_editor->setGeometry(colx, coly, colw, colh);
}

//---------------------------------------------------------
//   pitchValueChanged
//---------------------------------------------------------

void DList::escapePressed()
{
  selectedColumn = -1;
  if(editor)
  {
    editor->blockSignals(true);
    editor->hide();
    editor->blockSignals(false);
  }
  if(val_editor)
  {
    val_editor->blockSignals(true);
    val_editor->hide();
    val_editor->blockSignals(false);
  }
  if(pitch_editor)
  {
    pitch_editor->blockSignals(true);
    pitch_editor->hide();
    pitch_editor->blockSignals(false);
  }
  editEntry = 0;
  setFocus();
  update();
}

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void DList::returnPressed()
      {
      if (editor==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: editor is NULL in DList::returnPressed()!\n");
        return;
      }

      if (editEntry==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: editEntry is NULL in DList::returnPressed()!\n");
        selectedColumn = -1;
        editor->blockSignals(true);
        editor->hide();
        editor->blockSignals(false);
        setFocus();
        update();
        return;
      }

      const int instrument = (editEntry-ourDrumMap);

      int field = MusECore::WorkingDrumMapEntry::NoField;
      MusECore::DrumMap editEntryOld = *editEntry;
      switch(selectedColumn) {
            case COL_NAME:
                  editEntry->name = editor->text();
                  field = MusECore::WorkingDrumMapEntry::NameField;
                  break;

            default:
                  printf("Return pressed in unknown column\n");
                  break;
            }

      const bool do_prop = (editEntryOld != *editEntry && dcanvas);

      // Clear these before the operations.
      selectedColumn = -1;
      editor->blockSignals(true);
      editor->hide();
      editor->blockSignals(false);
      editEntry = 0;
      setFocus();
      update();

      if(do_prop)
        dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
      }

//---------------------------------------------------------
//   valEdited
//---------------------------------------------------------

void DList::valEdited()
      {
      if (val_editor==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: val_editor is NULL in DList::returnPressed()!\n");
        return;
      }

      if (editEntry==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: editEntry is NULL in DList::returnPressed()!\n");
        selectedColumn = -1;
        val_editor->blockSignals(true);
        val_editor->hide();
        val_editor->blockSignals(false);
        setFocus();
        update();
        return;
      }

      const int instrument = (editEntry-ourDrumMap);
      int val = val_editor->value();

      switch (selectedColumn)
      {
        case COL_VOLUME:
            if (val > 250)
            val = 250;
            if (val < 0)
            val = 0;
            break;

        case COL_LEVEL1:
        case COL_LEVEL2:
        case COL_LEVEL3:
        case COL_LEVEL4:
            if (val > 127) //Check bounds for lv1-lv4 values
            val = 127;
            // Zero note on vel is not allowed now.
            else if (val <= 0)
              val = 1;
            break;

        case COL_OUTCHANNEL:
            // Default to track port if -1 and track channel if -1.
            if(val <= 0)
              val = -1;
            else
              val--;
            if (val >= MIDI_CHANNELS)
              val = MIDI_CHANNELS - 1;
            break;

        default: break;
      }

      int field = MusECore::WorkingDrumMapEntry::NoField;
      MusECore::DrumMap editEntryOld = *editEntry;
      switch(selectedColumn) {
            case COL_NOTELENGTH:
                  editEntry->len = val;
                  field = MusECore::WorkingDrumMapEntry::LenField;
                  break;

            case COL_VOLUME:
                  editEntry->vol = val;
                  field = MusECore::WorkingDrumMapEntry::VolField;
                  break;

            case COL_LEVEL1:
                  editEntry->lv1 = val;
                  field = MusECore::WorkingDrumMapEntry::Lv1Field;
                  break;

            case COL_LEVEL2:
                  editEntry->lv2 = val;
                  field = MusECore::WorkingDrumMapEntry::Lv2Field;
                  break;

            case COL_LEVEL3:
                  editEntry->lv3 = val;
                  field = MusECore::WorkingDrumMapEntry::Lv3Field;
                  break;

            case COL_LEVEL4:
                  editEntry->lv4 = val;
                  field = MusECore::WorkingDrumMapEntry::Lv4Field;
                  break;

            case COL_QUANT:
                  editEntry->quant = val;
                  field = MusECore::WorkingDrumMapEntry::QuantField;
                  break;

            case COL_OUTCHANNEL:
                  editEntry->channel = val;
                  field = MusECore::WorkingDrumMapEntry::ChanField;
                  break;

            default:
                  printf("Value edited in unknown column\n");
                  break;
            }

      const bool do_prop = (editEntryOld != *editEntry && dcanvas);

      // Clear these before the operations.
      selectedColumn = -1;
      val_editor->blockSignals(true);
      val_editor->hide();
      val_editor->blockSignals(false);
      editEntry = 0;
      setFocus();
      update();

      if(do_prop)
        dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
      }

//---------------------------------------------------------
//   pitchEdited
//---------------------------------------------------------

void DList::pitchEdited()
{
      if (pitch_editor==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: pitch_editor is NULL in DList::pitchEdited()!\n");
        return;
      }

      if (editEntry==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: editEntry is NULL in DList::pitchEdited()!\n");
        selectedColumn = -1;
        pitch_editor->blockSignals(true);
        pitch_editor->hide();
        pitch_editor->blockSignals(false);
        setFocus();
        update();
        return;
      }

      const int val=pitch_editor->value();
      const int instrument=(editEntry-ourDrumMap);

      int field = MusECore::WorkingDrumMapEntry::NoField;
      MusECore::DrumMap editEntryOld=*editEntry;
      switch(selectedColumn) {
            case COL_NOTE:
               field = MusECore::WorkingDrumMapEntry::ANoteField;
               if(val != editEntry->anote)
                 editEntry->anote = val;
               break;

            case COL_INPUTTRIGGER:
               field = MusECore::WorkingDrumMapEntry::ENoteField;
               if (old_style_drummap_mode)
               {
                  //Check if there is any other MusEGlobal::drumMap with the same inmap value (there should be one (and only one):-)
                  //If so, switch the inmap between the instruments
                  for (int i=0; i<ourDrumMapSize; i++) {
                        if (ourDrumMap[i].enote == val && &ourDrumMap[i] != editEntry) {
                              MusEGlobal::drumInmap[int(editEntry->enote)] = i;
                              ourDrumMap[i].enote = editEntry->enote;
                              break;
                              }
                        }
                  //TODO: Set all the notes on the track with instrument=dm->enote to instrument=val
                  MusEGlobal::drumInmap[val] = instrument;
               }
               else
               {
                  if (dcanvas)
                  {
                      // Clear these before the operations.
                      selectedColumn = -1;
                      pitch_editor->blockSignals(true);
                      pitch_editor->hide();
                      pitch_editor->blockSignals(false);
                      setFocus();
                      update();

                      if(editEntry->enote != val)
                      {
                        editEntry->enote = val;
                        editEntry = 0;
                        dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
                      }
                      else
                        editEntry = 0;
                      return;
                  }
                  else
                  {
                      for (int i=0;i<128;i++)
                        if (ourDrumMap[i].enote==val)
                        {
                          ourDrumMap[i].enote=editEntry->enote;
                          break;
                        }
                  }
               }
               editEntry->enote = val;
               break;

            default:
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: Value changed in unknown column\n");
                  break;
            }


      const bool do_prop = (editEntryOld != *editEntry && dcanvas);

      // Clear these before the operations.
      selectedColumn = -1;
      pitch_editor->blockSignals(true);
      pitch_editor->hide();
      pitch_editor->blockSignals(false);
      editEntry = 0;
      setFocus();
      update();

      if(do_prop)
        dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
      }


//---------------------------------------------------------
//   moved
//---------------------------------------------------------

void DList::moved(int section, int, int)
{
  redraw();

  if(editEntry==NULL)
    return;

  const int line = (editEntry-ourDrumMap);

  int colx = mapx(header->sectionPosition(section));
  int colw = rmapx(header->sectionSize(section));
  int coly = mapy(line * TH);
  int colh = rmapy(TH);

  if(editor && editor->isVisible())
    editor->setGeometry(colx, coly, colw, colh);

  if(val_editor && val_editor->isVisible())
    val_editor->setGeometry(colx, coly, colw, colh);

  if(pitch_editor && pitch_editor->isVisible())
    pitch_editor->setGeometry(colx, coly, colw, colh);
}

//---------------------------------------------------------
//   tracklistChanged
//---------------------------------------------------------

void DList::tracklistChanged()
      {
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void DList::songChanged(MusECore::SongChangedFlags_t flags)
      {
      if (flags & SC_DRUMMAP) {
            redraw();
            }
      }

//---------------------------------------------------------
//   DList
//---------------------------------------------------------

void DList::init(QHeaderView* h, QWidget* parent)
{
      setBg(MusEGlobal::config.drumListBg);
      if (!h)
        h = new QHeaderView(Qt::Horizontal, parent);

      header = h;
      connect(header, SIGNAL(sectionResized(int,int,int)),
         SLOT(sizeChange(int,int,int)));
      connect(header, SIGNAL(sectionMoved(int, int,int)), SLOT(moved(int,int,int)));
      setFocusPolicy(Qt::StrongFocus);
      drag = NORMAL;
      editor = 0;
      val_editor = 0;
      pitch_editor = 0;
      editEntry = 0;

      if (ourDrumMapSize!=0)
      {
        // always select a drum instrument
        currentlySelected = &ourDrumMap[0];
      }
      else
      {
        currentlySelected = NULL;
      }
      
      selectedColumn = -1;

}

DList::DList(QHeaderView* h, QWidget* parent, int ymag, DrumCanvas* dcanvas_, bool oldstyle)
   : MusEGui::View(parent, 1, ymag)
      {
      dcanvas=dcanvas_;
      ourDrumMap=dcanvas->getOurDrumMap();
      ourDrumMapSize=dcanvas->getOurDrumMapSize();
      old_style_drummap_mode=oldstyle;
      connect(dcanvas, SIGNAL(ourDrumMapChanged(bool)), SLOT(ourDrumMapChanged(bool)));
      
      init(h, parent);
      }

DList::DList(QHeaderView* h, QWidget* parent, int ymag, MusECore::DrumMap* dm, int dmSize)
   : MusEGui::View(parent, 1, ymag)
      {
      dcanvas=NULL;
      ourDrumMap=dm;
      ourDrumMapSize=dmSize;
      old_style_drummap_mode=false;
      
      init(h, parent);
      }

//---------------------------------------------------------
//   ~DList
//---------------------------------------------------------

DList::~DList()
      {
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void DList::viewMouseMoveEvent(QMouseEvent* ev)
      {
      curY = ev->y();
      int delta = curY - startY;
      switch (drag) {
            case START_DRAG: // this cannot happen if ourDrumMapSize==0
                  if (delta < 0)
                        delta = -delta;
                  if (delta <= 2)
                        return;
                  drag = DRAG;
                  setCursor(QCursor(Qt::SizeVerCursor));
                  redraw();
                  break;
            case NORMAL:
                  break;
            case DRAG:
                  redraw();
                  break;
            }
      }

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void DList::viewMouseReleaseEvent(QMouseEvent* ev)
      {
      if (drag == DRAG) {
            int y = ev->y();
            int dInstrument;
            if (old_style_drummap_mode)
                  dInstrument = y / TH;
            else
                  dInstrument = (y+TH/2) / TH;
            
            if (dInstrument < 0) dInstrument=0;
            if (old_style_drummap_mode)
            {
              if (dInstrument >= ourDrumMapSize) dInstrument=ourDrumMapSize-1;
            }
            else
            {
              if (dInstrument > ourDrumMapSize) dInstrument=ourDrumMapSize; // allow moving something below the last element
            }
            
            int cur_sel = (!old_style_drummap_mode && dInstrument>sInstrument) ? dInstrument-1 : dInstrument;
            
            setCursor(QCursor(Qt::ArrowCursor));
            currentlySelected = &ourDrumMap[cur_sel];
            emit curDrumInstrumentChanged((unsigned)cur_sel);
            emit mapChanged(sInstrument, (unsigned)dInstrument); //Track instrument change done in canvas
            }
      drag = NORMAL;
      int x = ev->x();
      int y = ev->y();
      bool shift = ev->modifiers() & Qt::ShiftModifier;
      unsigned instrument = y / TH;

      DrumColumn col = DrumColumn(x2col(x));

      switch (col) {
            case COL_NAME:
                  emit keyReleased(instrument, shift);
                  break;
            case COL_NOTE:
                  emit keyReleased(instrument, shift);
                  break;
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void DList::wheelEvent(QWheelEvent* ev)
{
  ev->accept();

  Qt::MouseButtons buttons = ev->buttons();
  int keyState = ev->modifiers();
  //bool shift = keyState & Qt::ShiftModifier;
  bool ctrl = keyState & Qt::ControlModifier;
  int x = ev->x();
  int y = ev->y();

  DrumColumn col = DrumColumn(x2col(x));

  if(buttons != Qt::LeftButton || ourDrumMapSize == 0 || col == COL_NONE)
  {
    //ev->ignore();
    //View::wheelEvent(ev);
    //ev->accept();
    emit redirectWheelEvent(ev);
    return;
  }

  int instrument = y / TH;
  if (instrument >= ourDrumMapSize)
    instrument = ourDrumMapSize-1;
  if (instrument < 0)
    instrument = 0;

//   setCurDrumInstrument(instrument);

  MusECore::DrumMap* dm = &ourDrumMap[instrument];
  MusECore::DrumMap dm_old = *dm;

  //startY = y;
  //sInstrument = instrument;
  //drag   = START_DRAG;

  int val;
//   int incVal = 0;
//   if (button == Qt::RightButton)
//         incVal = 1;
//   else if (button == Qt::MidButton)
//         incVal = -1;


  const QPoint pixelDelta = ev->pixelDelta();
  const QPoint angleDegrees = ev->angleDelta() / 8;
  int delta = 0;
  if(!pixelDelta.isNull())
    delta = pixelDelta.y();
  else if(!angleDegrees.isNull())
    delta = angleDegrees.y() / 15;



  int field = MusECore::WorkingDrumMapEntry::NoField;
  switch (col)
  {
    case COL_NONE:
    case COL_HIDE:
    case COL_MUTE:
    case COL_NAME:
          break;

    case COL_OUTPORT: // this column isn't visible in new style drum mode
// TODO
//           field = MusECore::WorkingDrumMapEntry::PortField;
//           if ((button == Qt::RightButton) || (button == Qt::LeftButton)) {
//                 bool changeAll = ev->modifiers() & Qt::ControlModifier;
//                 devicesPopupMenu(dm, mapx(x), mapy(instrument * TH), changeAll);
//                 }
          break;
    case COL_VOLUME:
          field = MusECore::WorkingDrumMapEntry::VolField;
          val = dm->vol + delta;
          if (val < 0)
                val = 0;
          else if (val > 250)
                val = 250;
          dm->vol = (unsigned char)val;
          break;
    case COL_QUANT:
          field = MusECore::WorkingDrumMapEntry::QuantField;
          dm->quant += delta;
          // ?? range
          break;
    case COL_INPUTTRIGGER:
          field = MusECore::WorkingDrumMapEntry::ENoteField;
          val = dm->enote + delta;
          if (val < 0)
                val = 0;
          else if (val > 127)
                val = 127;

          if (old_style_drummap_mode)
          {
              //Check if there is any other drumMap with the same inmap value (there should be one (and only one):-)
              //If so, switch the inmap between the instruments
              for (int i=0; i<ourDrumMapSize; i++) {
                    if (ourDrumMap[i].enote == val && &ourDrumMap[i] != dm) {
                          MusEGlobal::drumInmap[int(dm->enote)] = i;
                          ourDrumMap[i].enote = dm->enote;
                          break;
                          }
                    }
              //TODO: Set all the notes on the track with instrument=dm->enote to instrument=val
              MusEGlobal::drumInmap[val] = instrument;
          }
          else
          {
            if (dcanvas)
            {
              if(dm->enote != val)
              {
                dm->enote = val;
                update();
                dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
              }
              return;

            }
            else
            {
              for (int i=0;i<128;i++)
                if (ourDrumMap[i].enote==val)
                {
                  ourDrumMap[i].enote=dm->enote;
                  break;
                }
            }
          }

          dm->enote = val;
          break;

    case COL_NOTELENGTH:
          field = MusECore::WorkingDrumMapEntry::LenField;
          val = dm->len + delta;
          if (val < 0)
                val = 0;
          dm->len = val;
          break;
    case COL_NOTE:
          field = MusECore::WorkingDrumMapEntry::ANoteField;
          if (old_style_drummap_mode) //only allow changing in old style mode
          {
            val = dm->anote + delta;
            if (val < 0)
                  val = 0;
            else if (val > 127)
                  val = 127;
            if(val != dm->anote)
            {
              MusEGlobal::audio->msgIdle(true);
              MusEGlobal::song->remapPortDrumCtrlEvents(instrument, val, -1, -1);
              MusEGlobal::audio->msgIdle(false);
              dm->anote = val;
              MusEGlobal::song->update(SC_DRUMMAP);
            }
          }

          emit keyPressed(instrument, 100);
          break;
    case COL_OUTCHANNEL: // this column isn't visible in new style drum mode
          field = MusECore::WorkingDrumMapEntry::ChanField;
          val = dm->channel + delta;
          // Default to track port if -1 and track channel if -1.
          if (val < -1)
                val = -1;
          else if (val > 127)
                val = 127;

          if (ctrl) {
                MusEGlobal::audio->msgIdle(true);
                // Delete all port controller events.
                MusEGlobal::song->changeAllPortDrumCtrlEvents(false, true);

                for (int i = 0; i < ourDrumMapSize; i++)
                      ourDrumMap[i].channel = val;
                // Add all port controller events.
                MusEGlobal::song->changeAllPortDrumCtrlEvents(true, true);
                MusEGlobal::audio->msgIdle(false);
                MusEGlobal::song->update(SC_DRUMMAP);
                }
          else
          {
              if(val != dm->channel)
              {
                MusEGlobal::audio->msgIdle(true);
                int mchan = val;
                if(mchan == -1 && dcanvas && dcanvas->part() && dcanvas->part()->track() && dcanvas->part()->track()->isMidiTrack())
                  mchan = static_cast<MusECore::MidiTrack*>(dcanvas->part()->track())->outChannel();
                if(val != -1)
                  MusEGlobal::song->remapPortDrumCtrlEvents(instrument, -1, val, -1);
                MusEGlobal::audio->msgIdle(false);
                dm->channel = val;
                MusEGlobal::song->update(SC_DRUMMAP);
              }
          }
          break;
    case COL_LEVEL1:
          field = MusECore::WorkingDrumMapEntry::Lv1Field;
          val = dm->lv1 + delta;
          // Zero note on vel is not allowed now.
          if (val <= 0)
                val = 1;
          else if (val > 127)
                val = 127;
          dm->lv1 = val;
          break;
    case COL_LEVEL2:
          field = MusECore::WorkingDrumMapEntry::Lv2Field;
          val = dm->lv2 + delta;
          // Zero note on vel is not allowed now.
          if (val <= 0)
                val = 1;
          else if (val > 127)
                val = 127;
          dm->lv2 = val;
          break;
    case COL_LEVEL3:
          field = MusECore::WorkingDrumMapEntry::Lv3Field;
          val = dm->lv3 + delta;
          // Zero note on vel is not allowed now.
          if (val <= 0)
                val = 1;
          else if (val > 127)
                val = 127;
          dm->lv3 = val;
          break;
    case COL_LEVEL4:
          field = MusECore::WorkingDrumMapEntry::Lv4Field;
          val = dm->lv4 + delta;
          // Zero note on vel is not allowed now.
          if (val <= 0)
                val = 1;
          else if (val > 127)
                val = 127;
          dm->lv4 = val;
          break;
    default:
          break;
  }

  update();
  if (!old_style_drummap_mode && dm_old != *dm && dcanvas) //something changed and we're in new style mode?
    dcanvas->propagate_drummap_change(instrument, field, false, false, false, false);
}
      
//---------------------------------------------------------
//   getSelectedInstrument
//---------------------------------------------------------

int DList::getSelectedInstrument()
      {
      if (currentlySelected == NULL)
            return -1;
      return currentlySelected - ourDrumMap;
      }


void DList::ourDrumMapChanged(bool instrMapChanged)
{
  int selIdx = currentlySelected ? (currentlySelected - ourDrumMap) : -1;
  int editIdx = editEntry ? (editEntry - ourDrumMap) : -1;
  
  ourDrumMap=dcanvas->getOurDrumMap();
  ourDrumMapSize=dcanvas->getOurDrumMapSize();
  
  if (instrMapChanged)
  {
    if (editEntry!=NULL)
    {
      printf("THIS SHOULD NEVER HAPPEN: DList::ourDrumMapChanged(true) caused editEntry to be\n"
             "                          invalidated. The current active editor will have no\n"
             "                          effect, expect potential breakage...\n");
      editEntry=NULL;
    }
  }
  else // that is: if (!instrMapChanged)
  {
    // if the instrumentMap has not changed, then its size and so
    // ourDrumMapSize cannot have changed as well.
    if (editIdx >= ourDrumMapSize)
    {
      printf("THIS SHOULD NEVER HAPPEN: editIdx got out of bounds although ourDrumMapSize\n"
             "                          cannot have changed (actually)\n");
      editIdx=-1;
    }
    editEntry=(editIdx>=0) ? &ourDrumMap[editIdx] : NULL;
  }
  
  if (selIdx >= ourDrumMapSize) selIdx=ourDrumMapSize-1;
  if (selIdx < 0) selIdx=0;
  currentlySelected = (ourDrumMapSize!=0) ? &ourDrumMap[selIdx] : NULL;
  
  if (ourDrumMapSize==0)
    drag = NORMAL;

  redraw();
}

} // namespace MusEGui
