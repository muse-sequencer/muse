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
#include "helper.h"
#include "icons.h"
#include "dlist.h"
#include "song.h"
#include "scrollscale.h"
#include "dcanvas.h"

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

      for (int i = 0; i < ourDrumMapSize; ++i) {
            int yy = i * TH;
            if (yy+TH < y)
                  continue;
            if (yy > y + h)
                  break;
            DrumMap* dm = &ourDrumMap[i];
            if (dm == currentlySelected)
                  p.fillRect(x, yy, w, TH, Qt::yellow);
//            else
//                  p.eraseRect(x, yy, w, TH);
            QHeaderView *h = header;
            p.save();
            p.setWorldMatrixEnabled(false);
            for (int k = 0; k < h->count(); ++k) {
                  int x   = h->sectionPosition(k);
                  int w   = h->sectionSize(k);
                  //QRect r = p.combinedTransform().mapRect(QRect(x+2, yy, w-4, TH));  // Gives inconsistent positions. Source shows wrong operation for our needs.
                  QRect r = map(QRect(x+2, yy, w-4, TH));                              // Use our own map instead.
                  QString s;
                  int align = Qt::AlignVCenter | Qt::AlignHCenter;

                  //p.save();
                  //p.setWorldMatrixEnabled(false);
                  switch (k) {
                        case COL_VOL:
                              s.setNum(dm->vol);
                              break;
                        case COL_QNT:
                              s.setNum(dm->quant);
                              break;
                        case COL_LEN:
                              s.setNum(dm->len);
                              break;
                        case COL_ANOTE:
                              s =  MusEUtil::pitch2string(dm->anote);
                              break;
                        case COL_ENOTE:
                              s =  MusEUtil::pitch2string(dm->enote);
                              break;
                        case COL_LV1:
                              s.setNum(dm->lv1);
                              break;
                        case COL_LV2:
                              s.setNum(dm->lv2);
                              break;
                        case COL_LV3:
                              s.setNum(dm->lv3);
                              break;
                        case COL_LV4:
                              s.setNum(dm->lv4);
                              break;
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
                        case COL_CHANNEL:
                              s.setNum(dm->channel+1);
                              break;
                        case COL_PORT:
                              s.sprintf("%d:%s", dm->port+1, midiPorts[dm->port].portname().toLatin1().constData());
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

void DList::devicesPopupMenu(DrumMap* t, int x, int y, bool changeAll)
      {
      if (!old_style_drummap_mode)
      {
        printf("THIS SHOULD NEVER HAPPEN: devicesPopupMenu() called in new style mode!\n");
        return;
      }
      
      QMenu* p = midiPortsPopup();
      QAction* act = p->exec(mapToGlobal(QPoint(x, y)), 0);
      bool doemit = false;
      if (act) {
            int n = act->data().toInt();
            if (!changeAll)
            {
                if(n != t->port)
                {
                  audio->msgIdle(true);
                  song->remapPortDrumCtrlEvents(getSelectedInstrument(), -1, -1, n);
                  audio->msgIdle(false);
                  t->port = n;
                  doemit = true;
                }  
            }      
            else {
                  audio->msgIdle(true);
                  // Delete all port controller events.
                  song->changeAllPortDrumCtrlEvents(false);
                  
                  for (int i = 0; i < ourDrumMapSize; i++)
                        ourDrumMap[i].port = n;
                  // Add all port controller events.
                  song->changeAllPortDrumCtrlEvents(true);
                  
                  audio->msgIdle(false);
                  doemit = true;
                  }
            }
      delete p;
      if(doemit)
      {
        int instr = getSelectedInstrument();
        if(instr != -1)
          //emit curDrumInstrumentChanged(instr);
          song->update(SC_DRUMMAP);
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
      DrumMap* dm = &ourDrumMap[instrument];
      DrumMap dm_old = *dm;

      setCurDrumInstrument(instrument);

      startY = y;
      sInstrument = instrument;
      drag   = START_DRAG;

      DCols col = DCols(x2col(x)); //FINDMICH update

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
            if (((editEntry && editEntry != dm)  || col != selectedColumn) && editEntry != 0) {
                  returnPressed();
                  }
            }

      switch (col) {
            case COL_NONE:
                  break;
            case COL_MUTE:
                  if (button == Qt::LeftButton)
                        dm->mute = !dm->mute;
                  break;
            case COL_PORT: // this column isn't visible in new style drum mode
                  if ((button == Qt::RightButton) || (button == Qt::LeftButton)) {
                        bool changeAll = ev->modifiers() & Qt::ControlModifier;
                        devicesPopupMenu(dm, mapx(x), mapy(instrument * TH), changeAll);
                        }
                  break;
            case COL_VOL:
                  val = dm->vol + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 999) //changed from 200 to 999 by flo93
                        val = 999;
                  dm->vol = (unsigned char)val;      
                  break;
            case COL_QNT:
                  dm->quant += incVal;
                  // ?? range
                  break;
            case COL_ENOTE:
                  val = dm->enote + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  
                  if (old_style_drummap_mode) //FINDMICH auch beim doppelklick!
                  {
                      //Check if there is any other drumMap with the same inmap value (there should be one (and only one):-)
                      //If so, switch the inmap between the instruments
                      for (int i=0; i<ourDrumMapSize; i++) {
                            if (ourDrumMap[i].enote == val && &ourDrumMap[i] != dm) {
                                  drumInmap[int(dm->enote)] = i;
                                  ourDrumMap[i].enote = dm->enote;
                                  break;
                                  }
                            }
                      //TODO: Set all the notes on the track with instrument=dm->enote to instrument=val
                      drumInmap[val] = instrument;
                  }
                  dm->enote = val;
                  break;
            case COL_LEN:
                  val = dm->len + incVal;
                  if (val < 0)
                        val = 0;
                  dm->len = val;
                  break;
            case COL_ANOTE:
                  if (old_style_drummap_mode) //only allow changing in old style mode FINDMICH auch beim doppelklick
                  {
                    val = dm->anote + incVal;
                    if (val < 0)
                          val = 0;
                    else if (val > 127)
                          val = 127;
                    if(val != dm->anote)
                    {
                      audio->msgIdle(true);
                      song->remapPortDrumCtrlEvents(instrument, val, -1, -1);
                      audio->msgIdle(false);
                      dm->anote = val;
                      song->update(SC_DRUMMAP);
                    }
                  }
                  
                  {
                  int velocity = 127 * float(ev->x()) / width();
                  emit keyPressed(instrument, velocity);//(dm->anote, shift);
                  }
                  break;
            case COL_CHANNEL: // this column isn't visible in new style drum mode
                  val = dm->channel + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  
                  if (ev->modifiers() & Qt::ControlModifier) {
                        audio->msgIdle(true);
                        // Delete all port controller events.
                        song->changeAllPortDrumCtrlEvents(false, true);
                        
                        for (int i = 0; i < ourDrumMapSize; i++)
                              ourDrumMap[i].channel = val;
                        // Add all port controller events.
                        song->changeAllPortDrumCtrlEvents(true, true);
                        audio->msgIdle(false);
                        song->update(SC_DRUMMAP);
                        }
                  else
                  {
                      if(val != dm->channel)
                      {
                        audio->msgIdle(true);
                        song->remapPortDrumCtrlEvents(instrument, -1, val, -1);
                        audio->msgIdle(false);
                        dm->channel = val;
                        song->update(SC_DRUMMAP);
                      }  
                  }      
                  break;
            case COL_LV1:
                  val = dm->lv1 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv1 = val;
                  break;
            case COL_LV2:
                  val = dm->lv2 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv2 = val;
                  break;
            case COL_LV3:
                  val = dm->lv3 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv3 = val;
                  break;
            case COL_LV4:
                  val = dm->lv4 + incVal;
                  if (val < 0)
                        val = 0;
                  else if (val > 127)
                        val = 127;
                  dm->lv4 = val;
                  break;
            case COL_NAME:
                  emit keyPressed(instrument, 100); //Mapping done on other side, send index
                  break;

            default:
                  break;
            }
      
      if (!old_style_drummap_mode && dm_old != *dm) //something changed and we're in new style mode?
      {
        //FINDMICHJETZT propagate that!
        dcanvas->propagate_drummap_change(dm-ourDrumMap);
      }
      
      redraw();
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

      if ((section == COL_NAME || section == COL_VOL || section == COL_LEN || section == COL_LV1 ||
         section == COL_LV2 || section == COL_LV3 || section == COL_LV4 || section == COL_QNT ||
         (section == COL_CHANNEL && old_style_drummap_mode) ) && (ev->button() == Qt::LeftButton))
         {
           lineEdit(instrument, section);
         }
      else if (((section == COL_ANOTE && old_style_drummap_mode) || section == COL_ENOTE) && (ev->button() == Qt::LeftButton))
        pitchEdit(instrument, section);
      else
            viewMousePressEvent(ev);
      }



//---------------------------------------------------------
//   lineEdit
//---------------------------------------------------------
void DList::lineEdit(int line, int section)
      {
            DrumMap* dm = &ourDrumMap[line];
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

                  case COL_VOL: {
                  editor->setText(QString::number(dm->vol));
                  break;
                  }
                  
                  case COL_LEN: {
                  editor->setText(QString::number(dm->len));
                  break;
                  }

                  case COL_LV1:
                  editor->setText(QString::number(dm->lv1));
                  break;

                  case COL_LV2:
                  editor->setText(QString::number(dm->lv2));
                  break;

                  case COL_LV3:
                  editor->setText(QString::number(dm->lv3));
                  break;

                  case COL_LV4:
                  editor->setText(QString::number(dm->lv4));
                  break;

                  case COL_QNT:
                  editor->setText(QString::number(dm->quant));
                  break;

                  case COL_CHANNEL:
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
            DrumMap* dm = &ourDrumMap[line];
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
                  case COL_ENOTE:
                  pitch_editor->setValue(dm->enote);
                  break;

                  case COL_ANOTE:
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
      DrumMap* dm = &ourDrumMap[instr];
      if (currentlySelected != dm) {
            currentlySelected = dm;
            emit curDrumInstrumentChanged(instr);
            song->update(SC_DRUMMAP); //FINDMICH necessary??
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
      int val = -1;
      if (selectedColumn != COL_NAME) 
      {
            val = atoi(editor->text().toAscii().constData());
            
            switch (selectedColumn)
            {
              case COL_VOL:
                  if (val > 999) //changed from 200 to 999 by flo93
                  val = 999;
                  if (val < 0)
                  val = 0;
                  break;
                  
              case COL_LV1:
              case COL_LV2:
              case COL_LV3:
              case COL_LV4:
                  if (val > 127) //Check bounds for lv1-lv4 values
                  val = 127;
                  if (val < 0)
                  val = 0;
                  break;
                  
              case COL_CHANNEL:
                  val--;
                  if (val >= 16)
                  val = 15;
                  if (val < 0)
                  val = 0;
                  break;
                  
              default: break;
            }  
      }     
      
      DrumMap editEntryOld = *editEntry;
      switch(selectedColumn) {
            case COL_NAME:
                  editEntry->name = editor->text();
                  break;

            case COL_LEN:
                  editEntry->len = atoi(editor->text().toAscii().constData());
                  break;

            case COL_VOL:
                  editEntry->vol = val;
                  break;

            case COL_LV1:
                  editEntry->lv1 = val;
                  break;

            case COL_LV2:
                  editEntry->lv2 = val;
                  break;

            case COL_LV3:
                  editEntry->lv3 = val;
                  break;

            case COL_LV4:
                  editEntry->lv4 = val;
                  break;

            case COL_QNT:
                  editEntry->quant = val;
                  break;

            case COL_CHANNEL:
                  editEntry->channel = val;
                  break;

            default:
                  printf("Return pressed in unknown column\n");
                  break;
            }
      
      if (editEntryOld != *editEntry)
      {
        //FINDMICHJETZT propagate!
        dcanvas->propagate_drummap_change(editEntry-ourDrumMap);
      }
      
      selectedColumn = -1;
      editor->hide();
      editEntry = 0;
      setFocus();
      redraw();
      }

//---------------------------------------------------------
//   pitchValueChanged
//---------------------------------------------------------

void DList::pitchEdited()
{
      int val=pitch_editor->value();
      int instrument=(editEntry-ourDrumMap);
      
      DrumMap editEntryOld=*editEntry;
      switch(selectedColumn) {
            case COL_ANOTE:
               if (old_style_drummap_mode) //should actually be always true, but to be sure...
               {
                    if(val != editEntry->anote)
                    {
                      audio->msgIdle(true);
                      song->remapPortDrumCtrlEvents(instrument, val, -1, -1);
                      audio->msgIdle(false);
                      editEntry->anote = val;
                      song->update(SC_DRUMMAP);
                    }
               }
               else
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: pitch edited of anote in new style mode!\n");
               break;

            case COL_ENOTE:
               if (old_style_drummap_mode)
               {
                  //Check if there is any other drumMap with the same inmap value (there should be one (and only one):-)
                  //If so, switch the inmap between the instruments
                  for (int i=0; i<ourDrumMapSize; i++) {
                        if (ourDrumMap[i].enote == val && &ourDrumMap[i] != editEntry) {
                              drumInmap[int(editEntry->enote)] = i;
                              ourDrumMap[i].enote = editEntry->enote;
                              break;
                              }
                        }
                  //TODO: Set all the notes on the track with instrument=dm->enote to instrument=val
                  drumInmap[val] = instrument;
                }
               editEntry->enote = val;
               break;
            default:
                  printf("ERROR: THIS SHOULD NEVER HAPPEN: Value changed in unknown column\n");
                  break;
            }
      
      if (editEntryOld != *editEntry)
      {
        //FINDMICHJETZT propagate
        dcanvas->propagate_drummap_change(editEntry-ourDrumMap);
      }
      
      selectedColumn = -1;
      pitch_editor->hide();
      editEntry = 0;
      setFocus();
      redraw();
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
   : MusEWidget::View(parent, 1, ymag)
      {
      setBg(Qt::white);
      
      dcanvas=dcanvas_;
      ourDrumMap=dcanvas->getOurDrumMap();
      ourDrumMapSize=dcanvas->getOurDrumMapSize();
      old_style_drummap_mode=oldstyle;
      
      if (!h){
      h = new QHeaderView(Qt::Horizontal, parent);}
      header = h;
      scroll = 0;
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
            unsigned dInstrument = y / TH;
            setCursor(QCursor(Qt::ArrowCursor));
            currentlySelected = &ourDrumMap[int(dInstrument)];
            emit curDrumInstrumentChanged(dInstrument);
            emit mapChanged(sInstrument, dInstrument); //Track instrument change done in canvas
            }
      drag = NORMAL;
//??      redraw();          //commented out NOT by flo93; was already commented out
//      if (editEntry)            //removed by flo93; seems to work without it
//            editor->setFocus(); //and causes segfaults after adding the pitchedits
      int x = ev->x();
      int y = ev->y();
      bool shift = ev->modifiers() & Qt::ShiftModifier;
      unsigned instrument = y / TH;

      DCols col = DCols(x2col(x));

      switch (col) {
            case COL_NAME:
                  emit keyReleased(instrument, shift);
                  break;
            case COL_ANOTE:
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
      return drumInmap[int(currentlySelected->enote)];
      }


