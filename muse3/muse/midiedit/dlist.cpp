//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dlist.cpp,v 1.9.2.7 2009/10/16 21:50:16 terminator356 Exp $
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

namespace MusEGui {

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void DList::draw(QPainter& p, const QRect& rect)
      {
      int x = rect.x();
      int y = rect.y();
      int w = rect.width();
      int h = rect.height();

      //---------------------------------------------------
      //    Tracks
      //---------------------------------------------------

      p.setPen(Qt::black);

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
                  
                  
                  int x   = h->sectionPosition(k);
                  int w   = h->sectionSize(k);
                  //QRect r = p.combinedTransform().mapRect(QRect(x+2, yy, w-4, TH));  // Gives inconsistent positions. Source shows wrong operation for our needs.
                  QRect r = map(QRect(x+2, yy, w-4, TH));                              // Use our own map instead.
                  QString s;
                  int align = Qt::AlignVCenter | Qt::AlignHCenter;

                  switch (k) {
                        case COL_VOLUME:
                              s.setNum(dm->vol);
                              break;
                        case COL_QUANT:
                              s.setNum(dm->quant);
                              break;
                        case COL_NOTELENGTH:
                              s.setNum(dm->len);
                              break;
                        case COL_NOTE:
                              s =  MusECore::pitch2string(dm->anote);
                              break;
                        case COL_INPUTTRIGGER:
                              s =  MusECore::pitch2string(dm->enote);
                              break;
                        case COL_LEVEL1:
                              s.setNum(dm->lv1);
                              break;
                        case COL_LEVEL2:
                              s.setNum(dm->lv2);
                              break;
                        case COL_LEVEL3:
                              s.setNum(dm->lv3);
                              break;
                        case COL_LEVEL4:
                              s.setNum(dm->lv4);
                              break;
                        case COL_HIDE:
                        {
                              bool hidden=false;
                              bool shown=false;
                              QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                              int pitch = dcanvas->get_instrument_map()[instrument].pitch;
                              
                              for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end() && !(hidden&&shown); track++)
                                if (dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch])
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
                                    
                              break;
                        }
                        case COL_MUTE:
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
                                p.drawText(r.x() + 8, r.y(), r.width() - 8, r.height(), align, str);
                              }
                              break;
                              
                        // Default to track port if -1 and track channel if -1.
                        case COL_OUTCHANNEL:
                              if(dm->channel != -1)
                                s.setNum(dm->channel+1);
                              break;
                        case COL_OUTPORT:
                              if(dm->port != -1)
                                s.sprintf("%d:%s", dm->port+1, MusEGlobal::midiPorts[dm->port].portname().toLatin1().constData());
                              align = Qt::AlignVCenter | Qt::AlignLeft;
                              break;
                        }
                  if (!s.isEmpty())
                        p.drawText(r, align, s);
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

void DList::devicesPopupMenu(MusECore::DrumMap* t, int x, int y, bool changeAll)
      {
      if (!old_style_drummap_mode)
      {
        printf("THIS SHOULD NEVER HAPPEN: devicesPopupMenu() called in new style mode!\n");
        return;
      }
      
      QMenu* p = MusECore::midiPortsPopup(this, t->port, true);  // Include a "<Default>" entry.
      QAction* act = p->exec(mapToGlobal(QPoint(x, y)), 0);
      bool doemit = false;
      if(!act)
      {
        delete p;
        return;
      }  
      
      int n = act->data().toInt();
      delete p;

      const int openConfigId = MIDI_PORTS;
      const int defaultId    = MIDI_PORTS + 1;

      if(n < 0 || n > defaultId)     // Invalid item.
        return;

      if(n == openConfigId)    // Show port config dialog.
      {
        MusEGlobal::muse->configMidiPorts();
        return;
      }

      if(n == defaultId)   // Means the <default> -1
        n = -1;
      
      if (!changeAll)
      {
          if(n != t->port)
          {
            int mport = n;
            // Default to track port if -1 and track channel if -1.
            if(mport == -1)
            {
              if(!dcanvas || !dcanvas->part())
                return;
              MusECore::Part* cur_part = dcanvas->part();
              if(!cur_part->track() || !cur_part->track()->isMidiTrack())
                return;
              MusECore::MidiTrack* cur_track = static_cast<MusECore::MidiTrack*>(cur_part->track());
              mport = cur_track->outPort();
            }
            MusEGlobal::audio->msgIdle(true);
            MusEGlobal::song->remapPortDrumCtrlEvents(getSelectedInstrument(), -1, -1, mport);
            MusEGlobal::audio->msgIdle(false);
            t->port = n;      // -1 is allowed
            doemit = true;
          }  
      }      
      else {
            MusEGlobal::audio->msgIdle(true);
            // Delete all port controller events.
            MusEGlobal::song->changeAllPortDrumCtrlEvents(false);
            
            for (int i = 0; i < ourDrumMapSize; i++)
                  ourDrumMap[i].port = n;
            // Add all port controller events.
            MusEGlobal::song->changeAllPortDrumCtrlEvents(true);
            
            MusEGlobal::audio->msgIdle(false);
            doemit = true;
            }

      if(doemit)
      {
        int instr = getSelectedInstrument();
        if(instr != -1)
          MusEGlobal::song->update(SC_DRUMMAP);
      }            
    }

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void DList::viewMousePressEvent(QMouseEvent* ev)
      {
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

      int val;
      int incVal = 0;
      if (button == Qt::RightButton)
            incVal = 1;
      else if (button == Qt::MidButton)
            incVal = -1;

      // Check if we're already editing anything and have pressed the mouse
      // elsewhere
      // In that case, treat it as if a return was pressed

      if (button == Qt::LeftButton) {
            if (editEntry && (editEntry != dm  || col != selectedColumn)) {
                  returnPressed();
                  }
            }

      switch (col) {
            case COL_NONE:
                  break;
            case COL_HIDE:
                  if (button == Qt::LeftButton)
                  {
                    bool hidden=true;
                    QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                    int pitch = dcanvas->get_instrument_map()[instrument].pitch;
                    
                    for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end(); track++)
                      if (dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch] == false)
                      {
                        hidden=false;
                        break;
                      }
                    
                    for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end(); track++)
                      dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch] = !hidden;
                  }
                  break;
            case COL_MUTE:
                  if (button == Qt::LeftButton)
                        dm->mute = !dm->mute;
                  break;
            case COL_OUTPORT: // this column isn't visible in new style drum mode
                  if ((button == Qt::RightButton) || (button == Qt::LeftButton)) {
                        bool changeAll = ev->modifiers() & Qt::ControlModifier;
                        devicesPopupMenu(dm, mapx(x), mapy(instrument * TH), changeAll);
                        }
                  break;
            case COL_VOLUME:
                  val = dm->vol + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 999) //changed from 200 to 999 by flo93
                        val = 999;
                  dm->vol = (unsigned char)val;      
                  break;
            case COL_QUANT:
                  dm->quant += incVal;
                  // ?? range
                  break;
            case COL_INPUTTRIGGER:
                  val = dm->enote + incVal;
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
                      //Check if there is any other drumMap with the same inmap value (there should be one (and only one):-)
                      //If so, switch the inmap between the instruments
                      for (QSet<MusECore::Track*>::iterator it = dcanvas->get_instrument_map()[instrument].tracks.begin(); it!=dcanvas->get_instrument_map()[instrument].tracks.end(); it++)
                      {
                        MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(*it);
                        mt->drummap()[mt->map_drum_in(val)].enote=dm->enote;
                        mt->set_drummap_tied_to_patch(false);
                      }
                      // propagating this is unneccessary as it's already done.
                      // updating the drumInmap is unneccessary, as the propagate call below
                      // does this for us.
                      // updating ourDrumMap is unneccessary because the song->update(SC_DRUMMAP)
                      // does this for us.
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
                  val = dm->len + incVal;
                  if (val < 0)
                        val = 0;
                  dm->len = val;
                  break;
            case COL_NOTE:
                  if (old_style_drummap_mode) //only allow changing in old style mode
                  {
                    val = dm->anote + incVal;
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
                  val = dm->channel + incVal;
                  // Default to track port if -1 and track channel if -1.
                  if (val < -1)
                        val = -1;
                  else if (val > 127)
                        val = 127;
                  
                  if (ev->modifiers() & Qt::ControlModifier) {
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
                  val = dm->lv1 + incVal;
                  // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                   if (val < 0)
//                         val = 0;
                  if (val <= 0)
                        val = 1;
                  else if (val > 127)
                        val = 127;
                  dm->lv1 = val;
                  break;
            case COL_LEVEL2:
                  val = dm->lv2 + incVal;
                  // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                   if (val < 0)
//                         val = 0;
                  if (val <= 0)
                        val = 1;
                  else if (val > 127)
                        val = 127;
                  dm->lv2 = val;
                  break;
            case COL_LEVEL3:
                  val = dm->lv3 + incVal;
                  // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                   if (val < 0)
//                         val = 0;
                  if (val <= 0)
                        val = 1;
                  else if (val > 127)
                        val = 127;
                  dm->lv3 = val;
                  break;
            case COL_LEVEL4:
                  val = dm->lv4 + incVal;
                  // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                   if (val < 0)
//                         val = 0;
                  if (val <= 0)
                        val = 1;
                  else if (val > 127)
                        val = 127;
                  dm->lv4 = val;
                  break;
            case COL_NAME:
                  if (button == Qt::LeftButton)
                  {
                      int velo = 127 * (ev->x() - header->sectionPosition(COL_NAME)) / (header->sectionSize(COL_NAME) - 10);
                      // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                       if (velo < 0) velo = 0;
                      if (velo <= 0) velo = 1;  // Zero note on vel is not allowed.
                      else if (velo > 127 ) velo = 127;
                      emit keyPressed(instrument, velo); //Mapping done on other side, send index
                  }
                  else if (button == Qt::MidButton && dcanvas) // hide that instrument
                  {
                    QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                    int pitch = dcanvas->get_instrument_map()[instrument].pitch;
                    for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end(); track++)
                      dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch] = true;                    
                  }
                  else if (button == Qt::RightButton && dcanvas)
                  {
                    bool hidden=false;
                    bool shown=false;
                    QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                    int pitch = dcanvas->get_instrument_map()[instrument].pitch;
                    
                    for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end() && !(hidden&&shown); track++)
                      if (dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch])
                        hidden=true;
                      else
                        shown=true;

                    QMenu* popup = new QMenu(NULL /* intendedly not "this" */); 
                    QAction* hideAction = popup->addAction(tr("hide this instrument"));
                    QAction* showAction = popup->addAction(tr("show this instrument"));
                    showAction->setToolTip(tr("this turns a grayed out eye into a blue eye"));
                    
                    if (!hidden)
                      showAction->setEnabled(false);
                    if (!shown)
                      hideAction->setEnabled(false);
                    
                    QAction* result = popup->exec(ev->globalPos());
                    if (result==hideAction)
                      for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end(); track++)
                        dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch] = true;                    
                    else if (result==showAction)
                      for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end(); track++)
                        dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch] = false;       
                    
                    delete popup;
                  }
                  break;
            default:
                  break;
            }
      
      if (!old_style_drummap_mode && dm_old != *dm && dcanvas) //something changed and we're in new style mode?
        dcanvas->propagate_drummap_change(instrument, (dm_old.enote != dm->enote));
      
      MusEGlobal::song->update(SC_DRUMMAP);
      //redraw(); //this is done by the songChanged slot
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

      if ((section == COL_NAME || section == COL_VOLUME || section == COL_NOTELENGTH || section == COL_LEVEL1 ||
         section == COL_LEVEL2 || section == COL_LEVEL3 || section == COL_LEVEL4 || section == COL_QUANT ||
         (section == COL_OUTCHANNEL && old_style_drummap_mode) ) && (ev->button() == Qt::LeftButton))
         {
           lineEdit(instrument, section);
         }
      else if (((section == COL_NOTE && old_style_drummap_mode) || section == COL_INPUTTRIGGER) && (ev->button() == Qt::LeftButton))
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

                  case COL_VOLUME: {
                  editor->setText(QString::number(dm->vol));
                  break;
                  }
                  
                  case COL_NOTELENGTH: {
                  editor->setText(QString::number(dm->len));
                  break;
                  }

                  case COL_LEVEL1:
                  editor->setText(QString::number(dm->lv1));
                  break;

                  case COL_LEVEL2:
                  editor->setText(QString::number(dm->lv2));
                  break;

                  case COL_LEVEL3:
                  editor->setText(QString::number(dm->lv3));
                  break;

                  case COL_LEVEL4:
                  editor->setText(QString::number(dm->lv4));
                  break;

                  case COL_QUANT:
                  editor->setText(QString::number(dm->quant));
                  break;

                  case COL_OUTCHANNEL:
                  // Default to track port if -1 and track channel if -1.
                  if(dm->channel != -1)  
                    editor->setText(QString::number(dm->channel+1));
                  break;
            }

            editor->end(false);
            editor->setGeometry(colx, coly, colw, colh);
            // In all cases but the column name, select all text:
            if (section != COL_NAME)
                  editor->selectAll();
            editor->show();
            editor->setFocus();

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
                  connect(pitch_editor, SIGNAL(editingFinished()),
                     SLOT(pitchEdited()));
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

void DList::sizeChange(int, int, int)
      {
      redraw();
      }

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void DList::returnPressed()
      {
      if (editEntry==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: editEntry is NULL in DList::returnPressed()!\n");
        return;
      }
      
      int val = -1;
      if (selectedColumn != COL_NAME) 
      {
            val = atoi(editor->text().toLatin1().constData());
            
            switch (selectedColumn)
            {
              case COL_VOLUME:
                  if (val > 999) //changed from 200 to 999 by flo93
                  val = 999;
                  if (val < 0)
                  val = 0;
                  break;
                  
              case COL_LEVEL1:
              case COL_LEVEL2:
              case COL_LEVEL3:
              case COL_LEVEL4:
                  if (val > 127) //Check bounds for lv1-lv4 values
                  val = 127;
                  // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//                   if (val < 0)
//                   val = 0;
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
      }     
      
      MusECore::DrumMap editEntryOld = *editEntry;
      switch(selectedColumn) {
            case COL_NAME:
                  editEntry->name = editor->text();
                  break;

            case COL_NOTELENGTH:
                  editEntry->len = atoi(editor->text().toLatin1().constData());
                  break;

            case COL_VOLUME:
                  editEntry->vol = val;
                  break;

            case COL_LEVEL1:
                  editEntry->lv1 = val;
                  break;

            case COL_LEVEL2:
                  editEntry->lv2 = val;
                  break;

            case COL_LEVEL3:
                  editEntry->lv3 = val;
                  break;

            case COL_LEVEL4:
                  editEntry->lv4 = val;
                  break;

            case COL_QUANT:
                  editEntry->quant = val;
                  break;

            case COL_OUTCHANNEL:
                    editEntry->channel = val;
                  break;

            default:
                  printf("Return pressed in unknown column\n");
                  break;
            }
      
      if (editEntryOld != *editEntry && dcanvas)
        dcanvas->propagate_drummap_change(editEntry-ourDrumMap, false);
      
      selectedColumn = -1;
      editor->hide();
      editEntry = 0;
      setFocus();
      MusEGlobal::song->update(SC_DRUMMAP);
      //redraw(); //this is done by the songChanged slot
      }

//---------------------------------------------------------
//   pitchValueChanged
//---------------------------------------------------------

void DList::pitchEdited()
{
      if (editEntry==NULL)
      {
        printf("THIS SHOULD NEVER HAPPEN: editEntry is NULL in DList::pitchEdited()!\n");
        return;
      }

      int val=pitch_editor->value();
      int instrument=(editEntry-ourDrumMap);
      
      MusECore::DrumMap editEntryOld=*editEntry;
      switch(selectedColumn) {
            case COL_NOTE:
               if (old_style_drummap_mode) //should actually be always true, but to be sure...
               {
                    if(val != editEntry->anote)
                    {
                      MusEGlobal::audio->msgIdle(true);
                      MusEGlobal::song->remapPortDrumCtrlEvents(instrument, val, -1, -1);
                      MusEGlobal::audio->msgIdle(false);
                      editEntry->anote = val;
                      MusEGlobal::song->update(SC_DRUMMAP);
                    }
               }
               else
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: pitch edited of anote in new style mode!\n");
               break;

            case COL_INPUTTRIGGER:
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
                      //Check if there is any other drumMap with the same inmap value (there should be one (and only one):-)
                      //If so, switch the inmap between the instruments
                      for (QSet<MusECore::Track*>::iterator it = dcanvas->get_instrument_map()[instrument].tracks.begin(); it!=dcanvas->get_instrument_map()[instrument].tracks.end(); it++)
                      {
                        MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(*it);
                        mt->drummap()[mt->map_drum_in(val)].enote=editEntry->enote;
                        mt->set_drummap_tied_to_patch(false);
                      }
                      // propagating this is unneccessary as it's already done.
                      // updating the drumInmap is unneccessary, as the propagate call below
                      // does this for us.
                      // updating ourDrumMap is unneccessary because the song->update(SC_DRUMMAP)
                      // does this for us.
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
      
      if (editEntryOld != *editEntry && dcanvas)
        dcanvas->propagate_drummap_change(editEntry-ourDrumMap, (editEntryOld.enote!=editEntry->enote));
      
      selectedColumn = -1;
      pitch_editor->hide();
      editEntry = 0;
      setFocus();
      MusEGlobal::song->update(SC_DRUMMAP);
      //redraw(); //this is done by the songChanged slot
      }

//---------------------------------------------------------
//   moved
//---------------------------------------------------------

void DList::moved(int, int, int)
      {
      redraw();
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
      //ORCAN- CHECK if really needed: header->setTracking(true); DELETETHIS seems like it's unneeded ;)
      connect(header, SIGNAL(sectionResized(int,int,int)),
         SLOT(sizeChange(int,int,int)));
      connect(header, SIGNAL(sectionMoved(int, int,int)), SLOT(moved(int,int,int)));
      setFocusPolicy(Qt::StrongFocus);
      drag = NORMAL;
      editor = 0;
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
//??      redraw();          //commented out NOT by flo93; was already commented out. DELETETHIS? not the below, only this single line!
//      if (editEntry)            //removed by flo93; seems to work without it
//            editor->setFocus(); //and causes segfaults after adding the pitchedits
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
            emit redirectWheelEvent(ev);
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
