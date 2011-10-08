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
//                  p.eraseRect(x, yy, w, TH);
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

                  //p.save();
                  //p.setWorldMatrixEnabled(false);
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
                              s = dm->name;
                              align = Qt::AlignVCenter | Qt::AlignLeft;
                              break;
                        case COL_OUTCHANNEL:
                              s.setNum(dm->channel+1);
                              break;
                        case COL_OUTPORT:
                              s.sprintf("%d:%s", dm->port+1, MusEGlobal::midiPorts[dm->port].portname().toLatin1().constData());
                              align = Qt::AlignVCenter | Qt::AlignLeft;
                              break;
                        }
                  if (!s.isEmpty())
                        p.drawText(r, align, s);
                  //p.restore();
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
      
      QMenu* p = MusECore::midiPortsPopup();
      QAction* act = p->exec(mapToGlobal(QPoint(x, y)), 0);
      bool doemit = false;
      if (act) {
            int n = act->data().toInt();
            if (!changeAll)
            {
                if(n != t->port)
                {
                  MusEGlobal::audio->msgIdle(true);
                  MusEGlobal::song->remapPortDrumCtrlEvents(getSelectedInstrument(), -1, -1, n);
                  MusEGlobal::audio->msgIdle(false);
                  t->port = n;
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
            }
      delete p;
      if(doemit)
      {
        int instr = getSelectedInstrument();
        if(instr != -1)
          //emit curDrumInstrumentChanged(instr);
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
      unsigned instrument = y / TH;
      MusECore::DrumMap* dm = &ourDrumMap[instrument];
      MusECore::DrumMap dm_old = *dm;

      setCurDrumInstrument(instrument);

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
                  if (val < 0)
                        val = 0;
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
                        MusEGlobal::song->remapPortDrumCtrlEvents(instrument, -1, val, -1);
                        MusEGlobal::audio->msgIdle(false);
                        dm->channel = val;
                        MusEGlobal::song->update(SC_DRUMMAP);
                      }  
                  }      
                  break;
            case COL_LEVEL1:
                  val = dm->lv1 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv1 = val;
                  break;
            case COL_LEVEL2:
                  val = dm->lv2 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv2 = val;
                  break;
            case COL_LEVEL3:
                  val = dm->lv3 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv3 = val;
                  break;
            case COL_LEVEL4:
                  val = dm->lv4 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv4 = val;
                  break;
            case COL_NAME:
                  if (button == Qt::LeftButton)
                  {
                      int velo = 127 * (ev->x() - header->sectionPosition(COL_NAME)) / (header->sectionSize(COL_NAME) - 10);
                      if (velo < 0) velo = 0;
                      if (velo > 127 ) velo = 127;
                      emit keyPressed(instrument, velo); //Mapping done on other side, send index
                  }
                  else if (button == Qt::MidButton) // hide that instrument
                  {
                    QSet<MusECore::Track*>* group = &dcanvas->get_instrument_map()[instrument].tracks;
                    int pitch = dcanvas->get_instrument_map()[instrument].pitch;
                    for (QSet<MusECore::Track*>::iterator track=group->begin(); track!=group->end(); track++)
                      dynamic_cast<MusECore::MidiTrack*>(*track)->drummap_hidden()[pitch] = true;                    
                  }
                  else if (button == Qt::RightButton)
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
      
      if (!old_style_drummap_mode && dm_old != *dm) //something changed and we're in new style mode?
        dcanvas->propagate_drummap_change(dm-ourDrumMap);
      
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
      if (instr < 0 || instr >= ourDrumMapSize -1)
        return; // illegal instrument
      MusECore::DrumMap* dm = &ourDrumMap[instr];
      if (currentlySelected != dm) {
            currentlySelected = dm;
            emit curDrumInstrumentChanged(instr);
            MusEGlobal::song->update(SC_DRUMMAP);
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
            val = atoi(editor->text().toAscii().constData());
            
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
                  if (val < 0)
                  val = 0;
                  break;
                  
              case COL_OUTCHANNEL:
                  val--;
                  if (val >= 16)
                  val = 15;
                  if (val < 0)
                  val = 0;
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
                  editEntry->len = atoi(editor->text().toAscii().constData());
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
      
      if (editEntryOld != *editEntry)
        dcanvas->propagate_drummap_change(editEntry-ourDrumMap);
      
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
               editEntry->enote = val;
               break;

            default:
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: Value changed in unknown column\n");
                  break;
            }
      
      if (editEntryOld != *editEntry)
        dcanvas->propagate_drummap_change(editEntry-ourDrumMap);
      
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

void DList::songChanged(int flags)
      {
      if (flags & SC_DRUMMAP) {
            redraw();
            }
      }

//---------------------------------------------------------
//   DList
//---------------------------------------------------------

DList::DList(QHeaderView* h, QWidget* parent, int ymag, DrumCanvas* dcanvas_, bool oldstyle)
   : MusEGui::View(parent, 1, ymag)
      {
      setBg(Qt::white);
      
      dcanvas=dcanvas_;
      ourDrumMap=dcanvas->getOurDrumMap();
      ourDrumMapSize=dcanvas->getOurDrumMapSize();
      old_style_drummap_mode=oldstyle;
      connect(dcanvas, SIGNAL(ourDrumMapChanged(bool)), SLOT(ourDrumMapChanged(bool)));
      
      if (!h){
      h = new QHeaderView(Qt::Horizontal, parent);}
      header = h;
      //ORCAN- CHECK if really needed: header->setTracking(true);
      connect(header, SIGNAL(sectionResized(int,int,int)),
         SLOT(sizeChange(int,int,int)));
      connect(header, SIGNAL(sectionMoved(int, int,int)), SLOT(moved(int,int,int)));
      setFocusPolicy(Qt::StrongFocus);
      drag = NORMAL;
      editor = 0;
      pitch_editor = 0;
      editEntry = 0;
      // always select a drum instrument
      currentlySelected = &ourDrumMap[0];
      selectedColumn = -1;
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
            case START_DRAG:
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
            if (dInstrument >= ourDrumMapSize) dInstrument=ourDrumMapSize-1;
            
            int cur_sel = (!old_style_drummap_mode && dInstrument>sInstrument) ? dInstrument-1 : dInstrument;
            
            setCursor(QCursor(Qt::ArrowCursor));
            currentlySelected = &ourDrumMap[cur_sel];
            emit curDrumInstrumentChanged((unsigned)cur_sel);
            emit mapChanged(sInstrument, (unsigned)dInstrument); //Track instrument change done in canvas
            }
      drag = NORMAL;
//??      redraw();          //commented out NOT by flo93; was already commented out
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
//   getSelectedInstrument
//---------------------------------------------------------

int DList::getSelectedInstrument()
      {
      if (currentlySelected == 0)
            return -1;
      return MusEGlobal::drumInmap[int(currentlySelected->enote)];
      }


void DList::ourDrumMapChanged(bool instrMapChanged)
{
  int selIdx = currentlySelected - ourDrumMap;
  
  ourDrumMap=dcanvas->getOurDrumMap();
  ourDrumMapSize=dcanvas->getOurDrumMapSize();
  
  if (instrMapChanged)
    editEntry=NULL;
  
  if (selIdx >= ourDrumMapSize) selIdx=ourDrumMapSize-1;
  currentlySelected = &ourDrumMap[selIdx];

  redraw();
}

} // namespace MusEGui
