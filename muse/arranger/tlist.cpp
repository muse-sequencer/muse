//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tlist.cpp,v 1.31.2.31 2009/12/15 03:39:58 terminator356 Exp $
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

#include <cmath>

#include <QAction>
#include <QActionGroup>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QIcon>
#include <QSpinBox>
#include <QToolTip>
#include <QList>

#include "globals.h"
#include "icons.h"
#include "scrollscale.h"
#include "tlist.h"
#include "xml.h"
#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "midiseq.h"
#include "comment.h"
#include "track.h"
#include "song.h"
#include "header.h"
#include "node.h"
#include "audio.h"
#include "instruments/minstrument.h"
#include "app.h"
#include "helper.h"
#include "gconfig.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "synth.h"
#include "config.h"
#include "popupmenu.h"
#include "menutitleitem.h"
#include "arranger.h"
#include "undo.h"
#include "midi_audio_control.h"
#include "ctrl.h"

#ifdef DSSI_SUPPORT
#include "dssihost.h"
#endif

namespace MusEGui {

static const int MIN_TRACKHEIGHT = 20;
static const int WHEEL_DELTA = 120;
QColor collist[] = { Qt::red, Qt::yellow, Qt::blue , Qt::black, Qt::white, Qt::green };

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

TList::TList(Header* hdr, QWidget* parent, const char* name)
   : QWidget(parent) // Qt::WNoAutoErase | Qt::WResizeNoErase are no longer needed according to Qt4 doc
      {
      setBackgroundRole(QPalette::NoRole);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      // This is absolutely required for speed! Otherwise painfully slow because we get 
      //  full rect paint events even on small scrolls! See help on QPainter::scroll().
      setAttribute(Qt::WA_OpaquePaintEvent);
      
      setObjectName(name);
      ypos = 0;
      editMode = false;
      editJustFinished=false;
      setFocusPolicy(Qt::NoFocus);
      setMouseTracking(true);
      header    = hdr;

      _scroll    = 0;
      editTrack = 0;
      editor    = 0;
      chan_edit = NULL;
      ctrl_edit = NULL;
      mode      = NORMAL;

      //setBackgroundMode(Qt::NoBackground); // ORCAN - FIXME. DELETETHIS?
      //setAttribute(Qt::WA_OpaquePaintEvent);
      resizeFlag = false;

      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(redraw()));
      connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(maybeUpdateVolatileCustomColumns()));
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void TList::songChanged(int flags)
      {
      if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_TRACK_INSERTED
         | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_ROUTE | SC_CHANNELS | SC_MIDI_TRACK_PROP
         | SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED
         | SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED ))
            redraw();
      if (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED))
            adjustScrollbar();
      }

//---------------------------------------------------------
//   drawCenteredPixmap
//    small helper function for "draw()" below
//---------------------------------------------------------

static void drawCenteredPixmap(QPainter& p, const QPixmap* pm, const QRect& r)
      {
      p.drawPixmap(r.x() + (r.width() - pm->width())/2, r.y() + (r.height() - pm->height())/2, *pm);
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void TList::paintEvent(QPaintEvent* ev)
      {
      paint(ev->rect());
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void TList::redraw()
      {
      update();
      }

//---------------------------------------------------------
//   redraw
//---------------------------------------------------------

void TList::redraw(const QRect& r)
      {
      update(r);
      }


//---------------------------------------------------------
//   event
//---------------------------------------------------------

bool TList::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
         QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
         MusECore::TrackList* l = MusEGlobal::song->tracks();
         int idx = 0;
         int yy  = -ypos;
         for (MusECore::iTrack i = l->begin(); i != l->end(); ++idx, yy += (*i)->height(), ++i) {
               MusECore::Track* track = *i;
               MusECore::Track::TrackType type = track->type();
               int trackHeight = track->height();
               if (trackHeight==0) // not visible
                     continue;
               if (helpEvent->pos().y() > yy && helpEvent->pos().y() < yy + trackHeight) {
                   if (type == MusECore::Track::AUDIO_SOFTSYNTH) {
                     MusECore::SynthI *s = (MusECore::SynthI*)track;
                     QToolTip::showText(helpEvent->globalPos(),track->name() + " : " + s->synth()->description());
                   }
                   else
                      QToolTip::showText(helpEvent->globalPos(),track->name());
               }
         }
        return true;
    }
    return QWidget::event(event);
}

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void TList::paint(const QRect& r)
      {
      if (!isVisible())
            return;
      QRect rect(r);
      QPainter p(this);

      if (bgPixmap.isNull())
            p.fillRect(rect, MusEGlobal::config.trackBg);
      else
            p.drawTiledPixmap(rect, bgPixmap, QPoint(rect.x(), ypos + rect.y()));
      p.setClipRegion(rect);

      //printf("TList::paint hasClipping:%d\n", p.hasClipping());   // Tested true.
      
      int y  = rect.y();
      int w  = rect.width();
      int h  = rect.height();
      int x1 = rect.x();
      int x2 = rect.x() + w;

      //---------------------------------------------------
      //    Tracks
      //---------------------------------------------------                         

      QColor mask_edge = QColor(90, 90, 90, 45);
      QColor mask_center = QColor(240, 240, 240, 175);
      QLinearGradient mask;
      mask.setColorAt(0, mask_edge);
      mask.setColorAt(0.15, mask_center);
      mask.setColorAt(0.3, mask_center);
      mask.setColorAt(0.85, mask_edge);
      mask.setColorAt(1, mask_edge);


      MusECore::TrackList* l = MusEGlobal::song->tracks();
      int idx = 0;
      int yy  = -ypos;
      for (MusECore::iTrack i = l->begin(); i != l->end(); ++idx, yy += (*i)->height(), ++i) {
            MusECore::Track* track = *i;
            MusECore::Track::TrackType type = track->type();
            int trackHeight = track->height();
            if (trackHeight==0) // not visible
                  continue;
            if (yy >= (y + h))
                  break;
            if ((yy + trackHeight) < y)
                  continue;
            //
            // clear one row
            //
            QColor bg;
            if (track->selected()) {
                  bg = MusEGlobal::config.selectTrackBg;
                  p.setPen(MusEGlobal::config.selectTrackFg);
                  }
            else {	
                  switch(type) {
                        case MusECore::Track::MIDI:
                              bg = MusEGlobal::config.midiTrackBg;
                              break;
                        case MusECore::Track::DRUM:
                              bg = MusEGlobal::config.drumTrackBg;
                              break;
                        case MusECore::Track::WAVE:
                              bg = MusEGlobal::config.waveTrackBg;
                              break;
                        case MusECore::Track::AUDIO_OUTPUT:
                              bg = MusEGlobal::config.outputTrackBg;
                              break;
                        case MusECore::Track::AUDIO_INPUT:
                              bg = MusEGlobal::config.inputTrackBg;
                              break;
                        case MusECore::Track::AUDIO_GROUP:
                              bg = MusEGlobal::config.groupTrackBg;
                              break;
                        case MusECore::Track::AUDIO_AUX:
                              bg = MusEGlobal::config.auxTrackBg;
                              break;
                        case MusECore::Track::AUDIO_SOFTSYNTH:
                              bg = MusEGlobal::config.synthTrackBg;
                              break;
                        }

                  p.setPen(palette().color(QPalette::Active, QPalette::Text));
                  }
            p.fillRect(x1, yy, w, trackHeight, bg);

	    if (track->selected()) {
                  mask.setStart(QPointF(0, yy));
                  mask.setFinalStop(QPointF(0, yy + trackHeight));
                  p.fillRect(x1, yy, w, trackHeight, mask);
                  }

            int x = 0;
            for (int index = 0; index < header->count(); ++index) {
                  int section = header->logicalIndex(index);
                  int w   = header->sectionSize(section);
                  QRect r = p.combinedTransform().mapRect(QRect(x+2, yy, w-4, trackHeight)); 

                  switch (section) {
                        case COL_RECORD:
                              if (track->canRecord() && !header->isSectionHidden(COL_RECORD)) {
                                    //bool aa = p.testRenderHint(QPainter::SmoothPixmapTransform); // Antialiasing);  // The rec icon currently looks very jagged. AA should help.
                                    //p.setRenderHint(QPainter::SmoothPixmapTransform); //Antialiasing);  
                                    drawCenteredPixmap(p,
                                       track->recordFlag() ? record_on_Icon : record_off_Icon, r);
                                    //p.setRenderHint(QPainter::SmoothPixmapTransform, aa); //Antialiasing, aa);  
                                    }
                              break;
                        case COL_CLASS:
                              {
                              if (header->isSectionHidden(COL_CLASS))
                                break;
                              const QPixmap* pm = 0;
                              switch(type) {
                                    case MusECore::Track::MIDI:
                                          pm = addtrack_addmiditrackIcon;
                                          break;
                                    case MusECore::Track::DRUM:
                                          pm = addtrack_drumtrackIcon;
                                          break;
                                    case MusECore::Track::WAVE:
                                          pm = addtrack_wavetrackIcon;
                                          break;
                                    case MusECore::Track::AUDIO_OUTPUT:
                                          pm = addtrack_audiooutputIcon;
                                          break;
                                    case MusECore::Track::AUDIO_INPUT:
                                          pm = addtrack_audioinputIcon;
                                          break;
                                    case MusECore::Track::AUDIO_GROUP:
                                          pm = addtrack_audiogroupIcon;
                                          break;
                                    case MusECore::Track::AUDIO_AUX:
                                          pm = addtrack_auxsendIcon;
                                          break;
                                    case MusECore::Track::AUDIO_SOFTSYNTH:
                                          pm = synthIcon;
                                          break;
                                    }
                              drawCenteredPixmap(p, pm, r);
                              }
                              break;
                        case COL_MUTE:
                              if (track->off())
                                    drawCenteredPixmap(p, offIcon, r);
                              else if (track->mute())
                                    drawCenteredPixmap(p, editmuteSIcon, r);
                              break;
                        case COL_SOLO:
                              if(track->solo() && track->internalSolo())
                                    drawCenteredPixmap(p, blacksqcheckIcon, r);
                              else      
                              if(track->internalSolo())
                                    drawCenteredPixmap(p, blacksquareIcon, r);
                              else
                              if (track->solo())
                                    drawCenteredPixmap(p, bluedotIcon, r);
                              break;
                        case COL_TIMELOCK:
                              if (track->isMidiTrack()
                                 && track->locked()) {
                                    drawCenteredPixmap(p, lockIcon, r);
                                    }
                              break;
                        case COL_NAME:
                              p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, track->name());
                              break;
                        case COL_OCHANNEL:
                              {
                              QString s;
                              int n;
                              if (track->isMidiTrack() && track->type() == MusECore::Track::DRUM) {
                                    p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, "-");
                                    break;
                              }
                              else if (track->isMidiTrack()) {
                                    n = ((MusECore::MidiTrack*)track)->outChannel() + 1;
                                    }
                              else {
                                    // show number of ports
                                    n = ((MusECore::WaveTrack*)track)->channels();
                                    }
                              s.setNum(n);
                              p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, s);
                              }
                              break;
                        case COL_OPORT:
                              {
                              QString s;
                              if (track->isMidiTrack()) {
                                    int outport = ((MusECore::MidiTrack*)track)->outPort();
                                    s.sprintf("%d:%s", outport+1, MusEGlobal::midiPorts[outport].portname().toLatin1().constData());
                                    }
                              else
                              if(track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
                              {
                                MusECore::MidiDevice* md = dynamic_cast<MusECore::MidiDevice*>(track);  
                                if(md)
                                {
                                  int outport = md->midiPort();
                                  if((outport >= 0) && (outport < MIDI_PORTS))
                                    s.sprintf("%d:%s", outport+1, MusEGlobal::midiPorts[outport].portname().toLatin1().constData());
                                  else
                                    s = tr("<none>");
                                }  
                              }  
                              
                              p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, s);
                              }
                              break;
                        case COL_AUTOMATION:
                              {
                              QString s="-";

                              if (!track->isMidiTrack()) {
                                    MusECore::CtrlListList* cll = ((MusECore::AudioTrack*)track)->controller();
                                    int countAll=0, countVisible=0;
                                    for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
                                        MusECore::CtrlList *cl = icll->second;
                                        if (!cl->dontShow())
                                            countAll++;
                                        if (cl->isVisible())
                                            countVisible++;
                                    }
                                    s.sprintf(" %d(%d) %s",countVisible, countAll, tr("visible").toAscii().data());
                                    }


                              p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, s);
                              }
                              break;
                        case COL_CLEF:
                              if (track->isMidiTrack() && track->type() == MusECore::Track::MIDI) { // no drum tracks!
                                QString s = tr("no clef");
                                if (((MusECore::MidiTrack*)track)->getClef() == trebleClef)
                                  s=tr("Treble");
                                else if (((MusECore::MidiTrack*)track)->getClef() == bassClef)
                                  s=tr("Bass");
                                else if (((MusECore::MidiTrack*)track)->getClef() == grandStaff)
                                  s=tr("Grand");
                                p.drawText(r, Qt::AlignVCenter|Qt::AlignLeft, s);
                              }
                              break;
                        default:
                              if (section>=COL_CUSTOM_MIDICTRL_OFFSET)
                              {
                                if (track->isMidiTrack())
                                {
                                  int col_ctrl_no=Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;
                                  MusECore::MidiTrack* mt=dynamic_cast<MusECore::MidiTrack*>(track);
                                  MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                                  MusECore::MidiController* mctl = mp->midiController(col_ctrl_no);
                                  int val;
                                  if (Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos == 
                                      Arranger::custom_col_t::AFFECT_BEGIN)
                                    val=mt->getControllerChangeAtTick(0,col_ctrl_no,MusECore::CTRL_VAL_UNKNOWN);
                                  else
                                  {
                                    val=mp->hwCtrlState(mt->outChannel(), col_ctrl_no);
                                    old_ctrl_hw_states[mt][section]=val;
                                  }
                                    
                                  if (val!=MusECore::CTRL_VAL_UNKNOWN)
                                    val-=mctl->bias();
                                  
                                  if (col_ctrl_no!=MusECore::CTRL_PROGRAM)
                                  {
                                    p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, 
                                      (val!=MusECore::CTRL_VAL_UNKNOWN)?QString::number(val):tr("off"));
                                  }
                                  else
                                  {
                                    MusECore::MidiInstrument* instr = mp->instrument();
                                    QString name;
                                    if (val!=MusECore::CTRL_VAL_UNKNOWN)
                                      name = instr->getPatchName(mt->outChannel(), val, MusEGlobal::song->mtype(), mt->type()==MusECore::Track::DRUM);
                                    else
                                      name = tr("<unknown>");
                                      
                                    p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, name);
                                  }
                                }
                              }
                              break;
                        }
                  x += header->sectionSize(section);
                  }
            p.setPen(Qt::gray);
            p.drawLine(x1, yy, x2, yy);
            }
      p.drawLine(x1, yy, x2, yy);

      if (mode == DRAG) {
            int yy = curY - dragYoff;
            p.setPen(Qt::green);
            p.drawLine(x1, yy, x2, yy);
            p.drawLine(x1, yy + dragHeight, x2, yy+dragHeight);
            }

      //---------------------------------------------------
      //    draw vertical lines
      //---------------------------------------------------

      int n = header->count();
      int xpos = 0;
      p.setPen(Qt::gray);
      for (int index = 0; index < n; index++) {
            int section = header->logicalIndex(index);
            xpos += header->sectionSize(section);
            p.drawLine(xpos, 0, xpos, height());
            }
      }

void TList::maybeUpdateVolatileCustomColumns()
{
  MusECore::TrackList* l = MusEGlobal::song->tracks();
  int idx = 0;
  int yy  = -ypos;
  for (MusECore::iTrack i = l->begin(); i != l->end(); ++idx, yy += (*i)->height(), ++i)
  {
    MusECore::Track* track = *i;
    int trackHeight = track->height();
    if (trackHeight==0) // not visible
          continue;


    int x = 0;
    for (int index = 0; index < header->count(); ++index)
    {
      int section = header->logicalIndex(index);

      if (section>=COL_CUSTOM_MIDICTRL_OFFSET && track->isMidiTrack() &&
            (Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos == 
             Arranger::custom_col_t::AFFECT_CPOS) )
      {
        int w   = header->sectionSize(section);
        QRect r = QRect(x+2, yy, w-4, trackHeight); 

        int ctrl_no = Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;

        MusECore::MidiTrack* mt=(MusECore::MidiTrack*)track;
        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
        int new_val = mp->hwCtrlState(mt->outChannel(), ctrl_no);

        if (new_val != old_ctrl_hw_states[track][section])
          update(r);
      }

      x += header->sectionSize(section);
    }
  }
}

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void TList::returnPressed()
{
      if(editTrack)
      {  
        if(editor && editor->isVisible())
        {    
          //editor->hide();
          if (editor->text() != editTrack->name()) {
                MusECore::TrackList* tl = MusEGlobal::song->tracks();
                for (MusECore::iTrack i = tl->begin(); i != tl->end(); ++i) {
                      if ((*i)->name() == editor->text()) {
                            QMessageBox::critical(this,
                              tr("MusE: bad trackname"),
                              tr("please choose a unique track name"),
                              QMessageBox::Ok,
                              Qt::NoButton,
                              Qt::NoButton);
                            editTrack = 0;
                            editor->blockSignals(true);  
                            editor->hide();
                            editor->blockSignals(false); 
                            setFocus();
                            return;
                            }
                      }
                      
                MusEGlobal::song->startUndo();
                MusEGlobal::song->addUndo(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackName, 
                                                          editTrack, 
                                                          editTrack->name().toLatin1().constData(), 
                                                          editor->text().toLatin1().constData()));
                editTrack->setName(editor->text());
                //MusEGlobal::song->update(SC_TRACK_MODIFIED);   //DELETETHIS
                MusEGlobal::song->endUndo(-1);                   //uagh, why "-1", why no proper flags?
                }
        }    
        
        editTrack = 0;
      }  
      
      editMode = false;
      editJustFinished = true;
      if(editor && editor->isVisible())
      {  
        editor->blockSignals(true);  
        editor->hide();
        editor->blockSignals(false); 
      }  
      setFocus();
}

void TList::chanValueFinished()
{
  if(editTrack)
  { 
    if(editTrack->isMidiTrack()) 
    {
      MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editTrack);
      if (mt && mt->type() != MusECore::Track::DRUM)
      {
        int channel = chan_edit->value() - 1;
        if(channel >= MIDI_CHANNELS)
          channel = MIDI_CHANNELS - 1;
        if(channel < 0)
          channel = 0;
        if(channel != mt->outChannel()) 
        {
              MusEGlobal::song->startUndo();
              MusEGlobal::song->addUndo(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackChannel, 
                                                          editTrack, 
                                                          mt->outChannel(), 
                                                          channel));
              //mt->setOutChannel(channel); DELETETHIS 10 (only the comments of course)
              MusEGlobal::audio->msgIdle(true);
              //MusEGlobal::audio->msgSetTrackOutChannel(mt, channel);
              mt->setOutChanAndUpdate(channel);
              MusEGlobal::audio->msgIdle(false);
              //if (mt->type() == MusECore::MidiTrack::DRUM) {//Change channel on all drum instruments
              //      for (int i=0; i<DRUM_MAPSIZE; i++)
              //            MusEGlobal::drumMap[i].channel = channel;
              //      }
              MusEGlobal::audio->msgUpdateSoloStates();                   
              //MusEGlobal::song->endUndo(SC_CHANNELS);
              //MusEGlobal::song->endUndo(SC_MIDI_TRACK_PROP | SC_ROUTE);  
              MusEGlobal::song->endUndo(SC_MIDI_TRACK_PROP);
        }      
      }
    }
    else
    {
        if(editTrack->type() != MusECore::Track::AUDIO_SOFTSYNTH)
        {
          MusECore::AudioTrack* at = dynamic_cast<MusECore::AudioTrack*>(editTrack);
          if(at)
          {  
            int n = chan_edit->value();
            if(n > MAX_CHANNELS)
                  n = MAX_CHANNELS;
            else if (n < 1)
                  n = 1;
            if(n != at->channels()) 
            {
                  MusEGlobal::song->startUndo();
                  MusEGlobal::song->addUndo(MusECore::UndoOp(MusECore::UndoOp::ModifyTrackChannel, 
                                                              editTrack, 
                                                              at->channels(), 
                                                              n));
                  MusEGlobal::audio->msgSetChannels(at, n);
                  MusEGlobal::song->endUndo(SC_CHANNELS);
            }
          }       
        }         
    }      
  
    editTrack = 0;
  }

  editMode = false;
  editJustFinished=true;
  if(chan_edit->isVisible())
  {  
    chan_edit->blockSignals(true);  
    chan_edit->hide();
    chan_edit->blockSignals(false);
  }  
  setFocus();
}

void TList::ctrlValueFinished()
{
  if(editTrack && editTrack->isMidiTrack())
  { 
    MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editTrack);
    if (mt && mt->type() != MusECore::Track::DRUM)
    {
      int val = ctrl_edit->value();
      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
      MusECore::MidiController* mctl = mp->midiController(ctrl_num);

      if (val==ctrl_edit->minimum())
        val=MusECore::CTRL_VAL_UNKNOWN;
      else
        val+=mctl->bias();
        
      if (val!=MusECore::CTRL_VAL_UNKNOWN)
      {
        record_controller_change_and_maybe_send(ctrl_at_tick, ctrl_num, val, mt);
      }
      else
      {
        MusECore::Undo operations;
        for (MusECore::iPart p = mt->parts()->begin(); p!=mt->parts()->end(); p++)
        {
          if (p->second->tick()==0)
          {
            for (MusECore::iEvent ev=p->second->events()->begin(); ev!=p->second->events()->end(); ev++)
            {
              if (ev->second.tick()!=0) break;
              else if (ev->second.type()==MusECore::Controller && ev->second.dataA()==ctrl_num)
              {
                using MusECore::UndoOp;
                operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, p->second, false, false));
                break;
              }
            }
          }
        }
        MusEGlobal::song->applyOperationGroup(operations);
      }
    }

    editTrack = 0;
  }

  editMode = false;
  editJustFinished=true;
  if(ctrl_edit->isVisible())
  {  
    ctrl_edit->blockSignals(true);  
    ctrl_edit->hide();
    ctrl_edit->blockSignals(false);
  }  
  setFocus();
}

//---------------------------------------------------------
//   adjustScrollbar
//---------------------------------------------------------

void TList::adjustScrollbar()
      {
      int h = 0;
      MusECore::TrackList* l = MusEGlobal::song->tracks();
      for (MusECore::iTrack it = l->begin(); it != l->end(); ++it)
            h += (*it)->height();
      _scroll->setMaximum(h +30);
      redraw();
      }

//---------------------------------------------------------
//   y2Track
//---------------------------------------------------------

MusECore::Track* TList::y2Track(int y) const
      {
      MusECore::TrackList* l = MusEGlobal::song->tracks();
      int ty = 0;
      for (MusECore::iTrack it = l->begin(); it != l->end(); ++it) {
            int h = (*it)->height();
            if (y >= ty && y < ty + h)
                  return *it;
            ty += h;
            }
      return 0;
      }

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void TList::mouseDoubleClickEvent(QMouseEvent* ev)
      {
      int button  = ev->button();
      //bool ctrl  = ((QInputEvent*)ev)->modifiers() & Qt::ControlModifier;
      if(button != Qt::LeftButton) {   
        mousePressEvent(ev);
        return;
      }  

      int x       = ev->x();
      int section = header->logicalIndexAt(x);
      if (section == -1)
      {  
        mousePressEvent(ev);  
        return;
      }
      
      MusECore::Track* t = y2Track(ev->y() + ypos);

      if (t) {
            int colx = header->sectionPosition(section);
            int colw = header->sectionSize(section);
            int coly = t->y() - ypos;
            int colh = t->height();

            if (section == COL_NAME) {
                  editTrack = t;
                  if (editor == 0) {
                        editor = new QLineEdit(this);
                        editor->setFrame(false);
                        connect(editor, SIGNAL(editingFinished()), SLOT(returnPressed()));   
                        }
                  editor->setText(editTrack->name());
                  editor->selectAll();
                  editor->setGeometry(colx, coly, colw, colh);
                  editMode = true;
                  editor->show();
                  }
            else if (section == COL_OCHANNEL) {
                  //if (t->isMidiTrack() && t->type() != MusECore::Track::DRUM)
                  // Enabled for audio tracks. And synth channels cannot be changed ATM.   
                  if(t->type() == MusECore::Track::DRUM || t->type() == MusECore::Track::AUDIO_SOFTSYNTH)   
                  {
                    mousePressEvent(ev);
                    return;
                  } 
                  
                  // A disabled spinbox up or down button will pass the event to the parent! Causes pseudo 'wrapping'. Eat it up.
                  if(chan_edit && chan_edit->hasFocus())
                  {
                    ev->accept();    
                    return;
                  }
                  else
                  {
                      editTrack=t;
                      if (chan_edit==0) {
                            chan_edit=new QSpinBox(this);
                            chan_edit->setFrame(false);
                            chan_edit->setMinimum(1);
                            connect(chan_edit, SIGNAL(editingFinished()), SLOT(chanValueFinished()));
                            }
                      if (t->isMidiTrack())
                      {  
                        chan_edit->setMaximum(MIDI_CHANNELS);
                        chan_edit->setValue(((MusECore::MidiTrack*)editTrack)->outChannel()+1);
                      }  
                      else // if(t->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                      {
                        chan_edit->setMaximum(MAX_CHANNELS);
                        chan_edit->setValue(((MusECore::AudioTrack*)editTrack)->channels());
                      }  
                      int w=colw;
                      if (w < chan_edit->sizeHint().width()) w=chan_edit->sizeHint().width();
                      chan_edit->setGeometry(colx, coly, w, colh);
                      chan_edit->selectAll();
                      editMode = true;     
                      chan_edit->show();
                      chan_edit->setFocus();
                      ev->accept();    
                      }
                  }
            else if (section >= COL_CUSTOM_MIDICTRL_OFFSET)
            {
              if (t->isMidiTrack())
              {
                editTrack=t;

                ctrl_num=Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;
                
                MusECore::MidiTrack* mt=(MusECore::MidiTrack*)t;
                MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                MusECore::MidiController* mctl = mp->midiController(ctrl_num);

                if (ctrl_num!=MusECore::CTRL_PROGRAM)
                {
                  if (Arranger::custom_columns[section-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                      Arranger::custom_col_t::AFFECT_BEGIN)
                    ctrl_at_tick=0;
                  else
                    ctrl_at_tick=MusEGlobal::song->cpos();
                  
                  if (ctrl_edit==0)
                  {
                    ctrl_edit=new QSpinBox(this);
                    ctrl_edit->setSpecialValueText(tr("off"));
                    connect(ctrl_edit, SIGNAL(editingFinished()), SLOT(ctrlValueFinished()));
                  }
                  
                  ctrl_edit->setMinimum(mctl->minVal()-1); // -1 because of the specialValueText
                  ctrl_edit->setMaximum(mctl->maxVal());
                  ctrl_edit->setValue(((MusECore::MidiTrack*)editTrack)->getControllerChangeAtTick(0,ctrl_num)-mctl->bias());
                  int w=colw;
                  if (w < ctrl_edit->sizeHint().width()) w=ctrl_edit->sizeHint().width();
                  ctrl_edit->setGeometry(colx, coly, w, colh);
                  editMode = true;     
                  ctrl_edit->show();
                  ctrl_edit->setFocus();
                }
                // else: the CTRL_PROGRAM popup is displayed with a single click
                ev->accept();
              }
            }
            else
                  mousePressEvent(ev);
            }
      }

//---------------------------------------------------------
//   portsPopupMenu
//---------------------------------------------------------
void TList::portsPopupMenu(MusECore::Track* t, int x, int y)
      {
      switch(t->type()) {
            case MusECore::Track::MIDI:
            case MusECore::Track::DRUM:
            case MusECore::Track::AUDIO_SOFTSYNTH: 
            {
                  MusECore::MidiTrack* track = (MusECore::MidiTrack*)t;
                  
                  MusECore::MidiDevice* md = 0;
                  int potential_new_port_no=-1;
                  int port = -1; 
                  if(t->type() == MusECore::Track::AUDIO_SOFTSYNTH) 
                  {
                    md = dynamic_cast<MusECore::MidiDevice*>(t);
                    if(md)
                      port = md->midiPort(); 
                  }
                  else   
                    port = track->outPort();
                    
                  QMenu* p = MusECore::midiPortsPopup(this, port);
                  
                  if (t->isMidiTrack())
                  {
                    // extend that menu a bit


                    // find first free port number
                    // do not permit numbers already used in other tracks!
                    // except if it's only used in this track.
                    int no;
                    for (no=0;no<MIDI_PORTS;no++)
                      if (MusEGlobal::midiPorts[no].device()==NULL)
                      {
                        MusECore::ciTrack it;
                        for (it=MusEGlobal::song->tracks()->begin(); it!=MusEGlobal::song->tracks()->end(); it++)
                        {
                          MusECore::MidiTrack* mt=dynamic_cast<MusECore::MidiTrack*>(*it);
                          if (mt && mt!=t && mt->outPort()==no)
                            break;
                        }
                        if (it == MusEGlobal::song->tracks()->end())
                          break;
                      }
                  
                    if (no==MIDI_PORTS)
                    {
                      delete p;
                      printf("THIS IS VERY UNLIKELY TO HAPPEN: no free midi ports! you have used all %i!\n",MIDI_PORTS);
                      break;
                    }
                    
                    
                    potential_new_port_no=no;
                    typedef std::map<std::string, int > asmap;
                    typedef std::map<std::string, int >::iterator imap;
                    
                    asmap mapALSA;
                    asmap mapJACK;
                    
                    int aix = 0x10000000;
                    int jix = 0x20000000;
                    for(MusECore::iMidiDevice i = MusEGlobal::midiDevices.begin(); i != MusEGlobal::midiDevices.end(); ++i) 
                    {
                      if((*i)->deviceType() == MusECore::MidiDevice::ALSA_MIDI)
                      {
                        // don't add devices which are used somewhere
                        int j;
                        for (j=0;j<MIDI_PORTS;j++)
                          if (MusEGlobal::midiPorts[j].device() == *i)
                            break;
                            
                        if (j==MIDI_PORTS) mapALSA.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), aix) );
                        
                        ++aix;
                      }  
                      else if((*i)->deviceType() == MusECore::MidiDevice::JACK_MIDI)
                      {
                        // don't add devices which are used somewhere
                        int j;
                        for (j=0;j<MIDI_PORTS;j++)
                          if (MusEGlobal::midiPorts[j].device() == *i)
                            break;

                        if (j==MIDI_PORTS) mapJACK.insert( std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), jix) );
                        ++jix;
                      }
                    }

                    if (!mapALSA.empty() || !mapJACK.empty())
                    {
                      QMenu* pup = p->addMenu(tr("Unused Devices"));
                      QAction* act;
                      
                      
                      if (!mapALSA.empty())
                      {
                        pup->addAction(new MusEGui::MenuTitleItem("ALSA:", pup));
                        
                        for(imap i = mapALSA.begin(); i != mapALSA.end(); ++i) 
                        {
                          int idx = i->second;
                          QString s(i->first.c_str());
                          MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::ALSA_MIDI);
                          if(md)
                          {
                            if(md->deviceType() != MusECore::MidiDevice::ALSA_MIDI)  
                              continue;
                              
                            act = pup->addAction(md->name());
                            act->setData(idx);
                          }  
                        }
                      }
                      
                      if (!mapALSA.empty() && !mapJACK.empty())
                        pup->addSeparator();
                      
                      if (!mapJACK.empty())
                      {
                        pup->addAction(new MusEGui::MenuTitleItem("JACK:", pup));
                        
                        for(imap i = mapJACK.begin(); i != mapJACK.end(); ++i) 
                        {
                          int idx = i->second;
                          QString s(i->first.c_str());
                          MusECore::MidiDevice* md = MusEGlobal::midiDevices.find(s, MusECore::MidiDevice::JACK_MIDI);
                          if(md)
                          {
                            if(md->deviceType() != MusECore::MidiDevice::JACK_MIDI)  
                              continue;
                              
                            act = pup->addAction(md->name());
                            act->setData(idx);
                          }  
                        }
                      }
                    }
                  }
                  
                  
                  QAction* act = p->exec(mapToGlobal(QPoint(x, y)), 0);
                  if(!act) 
                  {
                    delete p;
                    break;
                  }  
                  
                  QString acttext=act->text();
                  int n = act->data().toInt();
                  delete p;
                        
                  if(n < 0)              // Invalid item.
                    break;
                  
                  if(n == MIDI_PORTS)    // Show port config dialog.
                  {
                    MusEGlobal::muse->configMidiPorts();
                    break;
                  }
                  else if (n & 0x30000000)
                  {
                    int typ;
                    if (n & 0x10000000)
                      typ = MusECore::MidiDevice::ALSA_MIDI;
                    else
                      typ = MusECore::MidiDevice::JACK_MIDI;

                    MusECore::MidiDevice* sdev = MusEGlobal::midiDevices.find(acttext, typ);

                    MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[potential_new_port_no], sdev);
                    n=potential_new_port_no;
                    
                    MusEGlobal::song->update();
                  }
                  // Changed by T356. DELETETHIS 5
                  //track->setOutPort(n);
                  //MusEGlobal::audio->msgSetTrackOutPort(track, n);
                  
                  //MusEGlobal::song->update();
                  if (t->type() == MusECore::Track::DRUM) {
                        bool change = QMessageBox::question(this, tr("Update drummap?"),
                                      tr("Do you want to use same port for all instruments in the drummap?"),
                                      tr("&Yes"), tr("&No"), QString::null, 0, 1);
                        MusEGlobal::audio->msgIdle(true);
                        if (!change) 
                        {
                              // Delete all port controller events.
                              MusEGlobal::song->changeAllPortDrumCtrlEvents(false);
                              track->setOutPort(n);
                  
                              for (int i=0; i<DRUM_MAPSIZE; i++) //Remap all drum instruments to this port
                                    MusEGlobal::drumMap[i].port = track->outPort();
                              // Add all port controller events.
                              MusEGlobal::song->changeAllPortDrumCtrlEvents(true);
                        }
                        else
                        {
                          track->setOutPortAndUpdate(n);
                        }
                        MusEGlobal::audio->msgIdle(false);
                        MusEGlobal::audio->msgUpdateSoloStates();                   // (p4.0.14)  p4.0.17
                        MusEGlobal::song->update();
                  }
                  else if (t->type() == MusECore::Track::AUDIO_SOFTSYNTH) 
                  {
                    if(md != 0)
                    {
                      // Idling is already handled in msgSetMidiDevice.
                      //MusEGlobal::audio->msgIdle(true);
                      
                      // Compiler complains if simple cast from Track to MusECore::SynthI...
                      MusEGlobal::midiSeq->msgSetMidiDevice(&MusEGlobal::midiPorts[n], (MusEGlobal::midiPorts[n].device() == md) ? 0 : md);
                      MusEGlobal::muse->changeConfig(true);     // save configuration file
                    
                      MusEGlobal::song->update();
                    }
                  }
                  else
                  {
                    MusEGlobal::audio->msgIdle(true);
                    track->setOutPortAndUpdate(n);
                    MusEGlobal::audio->msgIdle(false);
                    MusEGlobal::audio->msgUpdateSoloStates();                   // (p4.0.14) p4.0.17
                    MusEGlobal::song->update(SC_MIDI_TRACK_PROP);               //
                }
            }
            break;
                  
            case MusECore::Track::WAVE:
            case MusECore::Track::AUDIO_OUTPUT:
            case MusECore::Track::AUDIO_INPUT:
            case MusECore::Track::AUDIO_GROUP:
            case MusECore::Track::AUDIO_AUX:    //TODO
                  break;
            }
      }

//---------------------------------------------------------
//   oportPropertyPopupMenu
//---------------------------------------------------------

void TList::oportPropertyPopupMenu(MusECore::Track* t, int x, int y)
      {
      if(t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
      {
        MusECore::SynthI* synth = (MusECore::SynthI*)t;
  
        QMenu* p = new QMenu;
        QAction* gact = p->addAction(tr("show gui"));
        gact->setCheckable(true);
        gact->setEnabled(synth->hasGui());
        gact->setChecked(synth->guiVisible());
  
        QAction* nact = p->addAction(tr("show native gui"));
        nact->setCheckable(true);
        nact->setEnabled(synth->hasNativeGui());
        nact->setChecked(synth->nativeGuiVisible());
  
        // If it has a gui but we don't have OSC, disable the action.
        #ifndef OSC_SUPPORT
        #ifdef DSSI_SUPPORT
        if(dynamic_cast<MusECore::DssiSynthIF*>(synth->sif()))
        {
          nact->setChecked(false);
          nact->setEnabled(false);
        }  
        #endif
        #endif
        
        QAction* ract = p->exec(mapToGlobal(QPoint(x, y)), 0);
        if (ract == gact) {
              bool show = !synth->guiVisible();
              synth->showGui(show);
              }
        else if (ract == nact) {
              bool show = !synth->nativeGuiVisible();
              synth->showNativeGui(show);
              }
        delete p;
        return;
      }
      
      
      if (t->type() != MusECore::Track::MIDI && t->type() != MusECore::Track::DRUM)
            return;
      int oPort      = ((MusECore::MidiTrack*)t)->outPort();
      MusECore::MidiPort* port = &MusEGlobal::midiPorts[oPort];

      QMenu* p = new QMenu;
      QAction* gact = p->addAction(tr("show gui"));
      gact->setCheckable(true);
      gact->setEnabled(port->hasGui());
      gact->setChecked(port->guiVisible());

      QAction* nact = p->addAction(tr("show native gui"));
      nact->setCheckable(true);
      nact->setEnabled(port->hasNativeGui());
      nact->setChecked(port->nativeGuiVisible());
        
      // If it has a gui but we don't have OSC, disable the action.
      #ifndef OSC_SUPPORT
      #ifdef DSSI_SUPPORT
      MusECore::MidiDevice* dev = port->device();
      if(dev && dev->isSynti() && (dynamic_cast<MusECore::DssiSynthIF*>(((MusECore::SynthI*)dev)->sif())))
      {
        nact->setChecked(false);
        nact->setEnabled(false);
      }  
      #endif
      #endif
      
      QAction* ract = p->exec(mapToGlobal(QPoint(x, y)), 0);
      if (ract == gact) {
            bool show = !port->guiVisible();
            port->instrument()->showGui(show);
            }
      else if (ract == nact) {
            bool show = !port->nativeGuiVisible();
            port->instrument()->showNativeGui(show);
            }
      delete p;
      
      }

//---------------------------------------------------------
//   tracklistChanged
//---------------------------------------------------------

void TList::tracklistChanged()
      {
      redraw();
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void TList::keyPressEvent(QKeyEvent* e)
      {
      if ( e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)     
            {
            e->accept();
            return;
            }
            
      if (editMode)
            {
            if ( e->key() == Qt::Key_Escape )
                  {
                  if(editor && editor->isVisible())         
                  {  
                    editor->blockSignals(true);  
                    editor->hide();
                    editor->blockSignals(false); 
                  }  
                  if(chan_edit && chan_edit->isVisible())  
                  {  
                    chan_edit->blockSignals(true);
                    chan_edit->hide();    
                    chan_edit->blockSignals(false);
                  }  
                  if(ctrl_edit && ctrl_edit->isVisible())  
                  {  
                    ctrl_edit->blockSignals(true);
                    ctrl_edit->hide();    
                    ctrl_edit->blockSignals(false);
                  }  
                  editTrack = 0;
                  editMode = false;
                  setFocus();
                  return;           
                  }
            return;      
            }
      else if (!editJustFinished)
          emit keyPressExt(e); //redirect keypress events to main app. don't call this when confirming an editor
      else
        editJustFinished=false;

      // Works OK (if focusing allowed). But instead we won't allow focus. Part canvas has Ctrl+up/down which moves selected track only.
      /*
      int key = e->key();
      switch (key) {
            case Qt::Key_Up:
                  moveSelection(-1);
                  return;           
            case Qt::Key_Down:
                  moveSelection(1);
                  return;           
            default:
                  break;
            }  */
                        
      // keyPressExt are sent to part canvas, where they are ignored *only* if necessary.
      //e->ignore();  
      
      }

//---------------------------------------------------------
//   moveSelection
//---------------------------------------------------------

void TList::moveSelection(int n)
      {
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();

      // check for single selection
      int nselect = 0;
      for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t)
            if ((*t)->selected())
                  ++nselect;
      if (nselect != 1)
            return;
      MusECore::Track* selTrack = 0;
      for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
            MusECore::iTrack s = t;
            if ((*t)->selected()) {
                  if (n > 0) {
                        while (n--) {
                              ++t;
                              if (t == tracks->end()) {
                                    --t;
                                    break;
                              }
                              // skip over hidden tracks
                              if (!(*t)->isVisible()) {
                                    n++;
                                    continue;
                              }
                              selTrack = *t;
                              break;
                         }
                  }
                  else {
                        while (n++ != 0) {
                              if (t == tracks->begin())
                                    break;
                              --t;
                              // skip over hidden tracks
                              if (!(*t)->isVisible()) {
                                    n--;
                                    continue;
                              }
                              selTrack = *t;
                              break;
                        }
                  }
                  if(selTrack)
                  {
                    (*s)->setSelected(false);
                    selTrack->setSelected(true);

                    // rec enable track if expected
                    MusECore::TrackList recd = getRecEnabledTracks();
                    if (recd.size() == 1 && MusEGlobal::config.moveArmedCheckBox) { // one rec enabled track, move rec enabled with selection
                      MusEGlobal::song->setRecordFlag((MusECore::Track*)recd.front(),false);
                      MusEGlobal::song->setRecordFlag((selTrack),true);
                    }

                    if (editTrack && editTrack != selTrack)   
                          returnPressed();
                    redraw();
                  }
                  break;
                }
            }
      if(selTrack)
        emit selectionChanged(selTrack);
      }

MusECore::TrackList TList::getRecEnabledTracks()
{
      MusECore::TrackList recEnabled;
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      for (MusECore::iTrack t = tracks->begin(); t != tracks->end(); ++t) {
        if ((*t)->recordFlag())
          recEnabled.push_back(*t);
      }
      return recEnabled;
}

//---------------------------------------------------------
//   changeAutomation
//---------------------------------------------------------

void TList::changeAutomation(QAction* act)
{
  if(!editAutomation || editAutomation->isMidiTrack())
    return;
  if(act->data().toInt() == -1)
    return;
  int colindex = act->data().toInt() & 0xff;
  int id = (act->data().toInt() & 0x00ffffff) >> 8;
  // Is it the midi control action or clear action item?
  if (colindex == 254 || colindex == 255)
    return;
  
  if (colindex < 100)
      return; // this was meant for changeAutomationColor
              // one of these days I'll rewrite this so it's understandable
              // this is just to get it up and running...

  MusECore::CtrlListList* cll = ((MusECore::AudioTrack*)editAutomation)->controller();
  for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
    MusECore::CtrlList *cl = icll->second;
    if (id == cl->id())  // got it, change state
        cl->setVisible(act->isChecked());
  }
  MusEGlobal::song->update(SC_TRACK_MODIFIED);
}

//---------------------------------------------------------
//   changeAutomation
//---------------------------------------------------------
void TList::changeAutomationColor(QAction* act)
{
  if(!editAutomation || editAutomation->isMidiTrack())
    return;
  if(act->data().toInt() == -1)
    return;
  int colindex = act->data().toInt() & 0xff;
  int id = (act->data().toInt() & 0x00ffffff) >> 8;

  // Is it the clear midi control action item?
  if(colindex == 254)  
  {
    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
    MusECore::MidiAudioCtrlMap* macp = track->controller()->midiControls();
    MusECore::AudioMidiCtrlStructMap amcs;
    macp->find_audio_ctrl_structs(id, &amcs);
    if(!amcs.empty())
      MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
    for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
      macp->erase(*iamcs);
    if(!amcs.empty())
      MusEGlobal::audio->msgIdle(false);
    
    // Hm, need to remove the 'clear' item, and the status lines below it. Try this:
    QActionGroup* midi_actgrp = act->actionGroup();
    if(midi_actgrp)
    {
      QList<QAction*> act_list = midi_actgrp->actions();
      int sz = act_list.size();
      for(int i = 0; i < sz; ++i)
      {
        QAction* list_act = act_list.at(i);
        ///midi_actgrp->removeAction(list_act);
        // list_act has no parent now.
        ///delete list_act;
        list_act->setVisible(false); // HACK Cannot delete any actions! Causes crash with our PopupMenu due to recent fixes.
      }
    }
    return;
  }
  
  // Is it the midi control action item?
  if(colindex == 255)  
  {
    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
    MusECore::MidiAudioCtrlMap* macm = track->controller()->midiControls();
    MusECore::AudioMidiCtrlStructMap amcs;
    macm->find_audio_ctrl_structs(id, &amcs);
    
    int port = -1, chan = 0, ctrl = 0;
    for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
    {
      macm->hash_values((*iamcs)->first, &port, &chan, &ctrl);
      break; // Only a single item for now, thanks!
    }
    
    MidiAudioControl* pup = new MidiAudioControl(port, chan, ctrl);
    
    if(pup->exec() == QDialog::Accepted)
    {
      MusEGlobal::audio->msgIdle(true);  // Gain access to structures, and sync with audio
      // Erase all for now.
      for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
        macm->erase(*iamcs);
      
      port = pup->port(); chan = pup->chan(); ctrl = pup->ctrl();
      if(port >= 0 && chan >=0 && ctrl >= 0)
        // Add will replace if found.
        macm->add_ctrl_struct(port, chan, ctrl, MusECore::MidiAudioCtrlStruct(id));
      
      MusEGlobal::audio->msgIdle(false);
    }
    
    delete pup;
    return;
  }
  
  if (colindex > 100)
      return; // this was meant for changeAutomation
              // one of these days I'll rewrite this so it's understandable
              // this is just to get it up and running...

  MusECore::CtrlListList* cll = ((MusECore::AudioTrack*)editAutomation)->controller();
  for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
    MusECore::CtrlList *cl = icll->second;
    if (cl->id() == id) // got it, change color
        cl->setColor(collist[colindex]);
  }
  MusEGlobal::song->update(SC_TRACK_MODIFIED);
}

//---------------------------------------------------------
//   colorMenu
//---------------------------------------------------------
PopupMenu* TList::colorMenu(QColor c, int id, QWidget* parent)
{
  PopupMenu * m = new PopupMenu(parent, true);  

  QActionGroup* col_actgrp = new QActionGroup(m);
  col_actgrp->setExclusive(true);
  for (int i = 0; i< 6; i++) {
    QPixmap pix(10,10);
    QPainter p(&pix);
    p.fillRect(0,0,10,10,collist[i]);
    p.setPen(Qt::black);
    p.drawRect(0,0,10,10);
    QIcon icon(pix);
    QAction *act = col_actgrp->addAction(icon,"");
    act->setCheckable(true);
    if (c == collist[i])
        act->setChecked(true);
    act->setData((id<<8) + i); // Shift 8 bits. Color in the bottom 8 bits. 
  }
  m->addActions(col_actgrp->actions());
  
  //m->addSeparator();
  m->addAction(new MenuTitleItem(tr("Midi control"), m));
  
  if(editAutomation && !editAutomation->isMidiTrack()) 
  {
    QAction *act = m->addAction(tr("Assign"));
    act->setCheckable(false);
    act->setData((id<<8) + 255); // Shift 8 bits. Make midi menu the last item at 255.
    
    MusECore::AudioTrack* track = static_cast<MusECore::AudioTrack*>(editAutomation);
    MusECore::MidiAudioCtrlMap* macm = track->controller()->midiControls();
    MusECore::AudioMidiCtrlStructMap amcs;
    macm->find_audio_ctrl_structs(id, &amcs);
    
    // Group only the clear and status items so they can both be easily removed when clear is clicked.
    if(!amcs.empty())
    {
      QActionGroup* midi_actgrp = new QActionGroup(m);
      QAction *cact = midi_actgrp->addAction(tr("Clear"));
      cact->setData((id<<8) + 254); // Shift 8 bits. Make clear the second-last item at 254
      for(MusECore::iAudioMidiCtrlStructMap iamcs = amcs.begin(); iamcs != amcs.end(); ++iamcs)
      {
        int port, chan, mctrl;
        macm->hash_values((*iamcs)->first, &port, &chan, &mctrl);
        //QString s = QString("Port:%1 Chan:%2 Ctl:%3-%4").arg(port + 1)
        QString s = QString("Port:%1 Chan:%2 Ctl:%3").arg(port + 1)
                                                    .arg(chan + 1)
                                                    //.arg((mctrl >> 8) & 0xff)
                                                    //.arg(mctrl & 0xff);
                                                    .arg(MusECore::midiCtrlName(mctrl, true));
        QAction *mact = midi_actgrp->addAction(s);
        mact->setEnabled(false);
        mact->setData(-1); // Not used
      }
      m->addActions(midi_actgrp->actions());
    }
  }
  
  connect(m, SIGNAL(triggered(QAction*)), SLOT(changeAutomationColor(QAction*)));
  return m;

}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------
void TList::mousePressEvent(QMouseEvent* ev)
      {
      int x       = ev->x();
      int y       = ev->y();
      int button  = ev->button();
      bool ctrl  = ((QInputEvent*)ev)->modifiers() & Qt::ControlModifier;

      MusECore::Track* t    = y2Track(y + ypos);

      // FIXME Observed: Ancient bug: Track Info doesn't change if selecting multiple tracks in reverse order.
      // Will need to be fixed if/when adding 'multiple track global editing'. 
      
      TrackColumn col = TrackColumn(header->logicalIndexAt(x));
      if (t == 0) {
            if (button == Qt::RightButton) {
                  QMenu* p = new QMenu;
                  MusEGui::populateAddTrack(p);
                  
                  // Show the menu
                  QAction* act = p->exec(ev->globalPos(), 0);

                  // Valid click?
                  if(act)
                  {
                    t = MusEGlobal::song->addNewTrack(act);  // Add at end of list.
                    if(t && t->isVisible())
                    {
                      MusEGlobal::song->deselectTracks();
                      t->setSelected(true);

                      ///emit selectionChanged();
                      emit selectionChanged(t);
                      adjustScrollbar();
                    }  
                  }
                  
                  // Just delete p, and all its children will go too, right?
                  //delete synp;
                  delete p;
            }
            /*else if (button == Qt::LeftButton) { DELETETHIS
              if (!ctrl) 
              {
                MusEGlobal::song->deselectTracks();
                emit selectionChanged(0);
              }  
            }*/
            return;
            }

      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      dragYoff = y - (t->y() - ypos);
      startY   = y;

      if (resizeFlag) {
            mode = RESIZE;
            int y  = ev->y();
            int ty = -ypos;
            sTrack = 0;
            for (MusECore::iTrack it = tracks->begin(); it != tracks->end(); ++it, ++sTrack) {
                  int h = (*it)->height();
                  ty += h;
                  if (y >= (ty-2)) {
                   
                        if ( (*it) == tracks->back() && y > ty ) { // DELETETHIS, only retain if(foo) break;? 
                              //printf("tracks->back() && y > ty\n");
                        }
                        else if ( y > (ty+2) ) {
                              //printf(" y > (ty+2) \n");
                        }
                        else {
                              //printf("ogga ogga\n");
                        
                              break;
                              }
                   
                   
                   //&& y < (ty)) DELETETHIS
                   //     break;
                        }
                  }

            return;
            }

      mode = NORMAL;

      switch (col) {
              case COL_CLEF:
                if (t->isMidiTrack() && t->type() == MusECore::Track::MIDI) {
                  QMenu* p = new QMenu;
                  p->addAction(tr("Treble clef"))->setData(0);
                  p->addAction(tr("Bass clef"))->setData(1);
                  p->addAction(tr("Grand Staff"))->setData(2);

                  // Show the menu
                  QAction* act = p->exec(ev->globalPos(), 0);
                  if (act) {
                    switch (act->data().toInt()) {
                      case 0:
                        ((MusECore::MidiTrack*)t)->setClef(trebleClef);
                        break;
                      case 1:
                        ((MusECore::MidiTrack*)t)->setClef(bassClef);
                        break;
                      case 2:
                        ((MusECore::MidiTrack*)t)->setClef(grandStaff);
                        break;
                      default:
                        break;
                    }
                  }
                  delete p;
                }
                break;
                
              case COL_AUTOMATION:
                {
                if (!t->isMidiTrack()) {
                    editAutomation = t;
                    PopupMenu* p = new PopupMenu(true);
                    p->disconnect();
                    p->clear();
                    p->setTitle(tr("Viewable automation"));
                    MusECore::CtrlListList* cll = ((MusECore::AudioTrack*)t)->controller();
                    QAction* act = 0;
                    int last_rackpos = -1;
                    bool internal_found = false;
                    bool synth_found = false;
                    for(MusECore::CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll) {
                      MusECore::CtrlList *cl = icll->second;
                      if (cl->dontShow())
                        continue;
                      
                      int ctrl = cl->id();
                      
                      if(ctrl < AC_PLUGIN_CTL_BASE)
                      {
                        if(!internal_found)
                          p->addAction(new MusEGui::MenuTitleItem(tr("Internal"), p)); 
                        internal_found = true;
                      }
                      else
                      {
                        if(ctrl < (int)MusECore::genACnum(MAX_PLUGINS, 0))  // The beginning of the special dssi synth controller block.             
                        {
                          int rackpos = (ctrl - AC_PLUGIN_CTL_BASE) >> AC_PLUGIN_CTL_BASE_POW;
                          if(rackpos < PipelineDepth)
                          {
                            if(rackpos != last_rackpos)
                            {
                              QString s = ((MusECore::AudioTrack*)t)->efxPipe()->name(rackpos);
                              p->addAction(new MusEGui::MenuTitleItem(s, p)); 
                            }
                            last_rackpos = rackpos;
                          }
                        }
                        else
                        {
                          if(t->type() == MusECore::Track::AUDIO_SOFTSYNTH)
                          {
                            if(!synth_found)
                              p->addAction(new MusEGui::MenuTitleItem(tr("Synth"), p)); 
                            synth_found = true;
                          }
                        }
                      }
                      
                      act = p->addAction(cl->name());
                      act->setCheckable(true);
                      act->setChecked(cl->isVisible());
                      
                      int data = ctrl<<8; // shift 8 bits
                      data += 150; // illegal color > 100
                      act->setData(data);
                      PopupMenu *m = colorMenu(cl->color(), cl->id(), p);
                      act->setMenu(m);
                    }
                    connect(p, SIGNAL(triggered(QAction*)), SLOT(changeAutomation(QAction*)));
                    p->exec(QCursor::pos());

                    delete p;
                  }
                  break;
                }

            case COL_RECORD:
                  {
                      mode = START_DRAG;
                      
                      bool val = !(t->recordFlag());
                      if (button == Qt::LeftButton) {
                        if (!t->isMidiTrack()) {
                              if (t->type() == MusECore::Track::AUDIO_OUTPUT) {
                                    if (val && t->recordFlag() == false) {
                                          MusEGlobal::muse->bounceToFile((MusECore::AudioOutput*)t);
                                          }
                                    MusEGlobal::audio->msgSetRecord((MusECore::AudioOutput*)t, val);
                                    if (!((MusECore::AudioOutput*)t)->recFile())
                                          val = false;
                                    else
                                          return;
                                    }
                              MusEGlobal::song->setRecordFlag(t, val);
                              }
                        else
                              MusEGlobal::song->setRecordFlag(t, val);
                      } else if (button == Qt::RightButton) {
                        // enable or disable ALL tracks of this type
                        if (!t->isMidiTrack()) {
                              if (t->type() == MusECore::Track::AUDIO_OUTPUT) {
                                    return;
                                    }
                              MusECore::WaveTrackList* wtl = MusEGlobal::song->waves();
                              foreach (MusECore::WaveTrack *wt, *wtl) {
                                MusEGlobal::song->setRecordFlag(wt, val);
                              }
                              }
                        else {
                          MusECore::MidiTrackList* mtl = MusEGlobal::song->midis();
                          foreach (MusECore::MidiTrack *mt, *mtl) {
                            MusEGlobal::song->setRecordFlag(mt, val);
                          }
                        }
                      }
                  }
                  break;
            case COL_NONE:
                  mode = START_DRAG;
                  break;
            case COL_CLASS:
                  if (t->isMidiTrack())
                        classesPopupMenu(t, x, t->y() - ypos);
                  break;
            case COL_OPORT:
                  // Changed by Tim. p3.3.9 DELETETHIS 15
                  // Reverted.
                  if (button == Qt::LeftButton)
                        portsPopupMenu(t, x, t->y() - ypos);
                  else if (button == Qt::RightButton)
                        oportPropertyPopupMenu(t, x, t->y() - ypos);
                  //if(((button == QMouseEvent::LeftButton) && (t->type() == MusECore::Track::AUDIO_SOFTSYNTH)) || (button == QMouseEvent::RightButton))
                  //  oportPropertyPopupMenu(t, x, t->y() - ypos);      
                  //else      
                  //if(button == QMouseEvent::LeftButton)
                  //  portsPopupMenu(t, x, t->y() - ypos);
                    
                  //MusEGlobal::audio->msgUpdateSoloStates(); // p4.0.14
                  //MusEGlobal::song->update(SC_ROUTE);       //
                  break;
                  
            case COL_MUTE:
                  mode = START_DRAG;
                  // p3.3.29
                  if ((button == Qt::RightButton) || (((QInputEvent*)ev)->modifiers() & Qt::ShiftModifier))
                    t->setOff(!t->off());
                  else
                  {
                    if (t->off())
                          t->setOff(false);
                    else
                          t->setMute(!t->mute());
                  }        
                  MusEGlobal::song->update(SC_MUTE);
                  break;
            case COL_SOLO:
                  mode = START_DRAG;
                  MusEGlobal::audio->msgSetSolo(t, !t->solo());
                  MusEGlobal::song->update(SC_SOLO);
                  break;

            case COL_NAME:
                  mode = START_DRAG;
                  if (button == Qt::LeftButton) {
                        if (!ctrl) {
                              MusEGlobal::song->deselectTracks();
                              t->setSelected(true);

                              // rec enable track if expected
                              MusECore::TrackList recd = getRecEnabledTracks();
                              if (recd.size() == 1 && MusEGlobal::config.moveArmedCheckBox) { // one rec enabled track, move rec enabled with selection
                                MusEGlobal::song->setRecordFlag((MusECore::Track*)recd.front(),false);
                                MusEGlobal::song->setRecordFlag(t,true);
                              }
                              }
                        else
                              t->setSelected(!t->selected());
                        if (editTrack && editTrack != t)
                              returnPressed();
                        emit selectionChanged(t->selected() ? t : 0);
                        }
                  else if (button == Qt::RightButton) {
                        mode = NORMAL;
                        QMenu* p = new QMenu;
                        // Leave room for normal track IDs - base these at AUDIO_SOFTSYNTH.
                        p->addAction(QIcon(*automation_clear_dataIcon), tr("Delete Track"))->setData(MusECore::Track::AUDIO_SOFTSYNTH + 1);
                        p->addAction(QIcon(*track_commentIcon), tr("Track Comment"))->setData(MusECore::Track::AUDIO_SOFTSYNTH + 2);
                        p->addSeparator();
                        QMenu* pnew = new QMenu(p);
                        pnew->setTitle(tr("Insert Track"));
                        pnew->setIcon(QIcon(*edit_track_addIcon));
                        MusEGui::populateAddTrack(pnew);
                        p->addMenu(pnew);
                        QAction* act = p->exec(ev->globalPos(), 0);
                        if (act) {
                              int n = act->data().toInt();
                              if(n >= MusECore::Track::AUDIO_SOFTSYNTH && n < MENU_ADD_SYNTH_ID_BASE)
                              {
                                n -= MusECore::Track::AUDIO_SOFTSYNTH;
                                switch (n) {
                                    case 1:     // delete track
                                          MusEGlobal::song->removeTrack0(t);
                                          MusEGlobal::audio->msgUpdateSoloStates();
                                          break;

                                    case 2:     // show track comment
                                          {
                                          TrackComment* tc = new TrackComment(t, 0);
                                          tc->show();
                                          }
                                          break;

                                    default:
                                          printf("action %d\n", n);
                                          break;
                                    }
                              }
                              else
                              {
                                t = MusEGlobal::song->addNewTrack(act, t);  // Let addNewTrack handle it. Insert before clicked-on track 't'.
                                if(t)
                                {
                                  MusEGlobal::song->deselectTracks();
                                  t->setSelected(true);
                                  emit selectionChanged(t);
                                  adjustScrollbar();
                                }  
                              }
                            }
                        delete p;
                        }
                  break;

            case COL_TIMELOCK:
                  mode = START_DRAG;
                  t->setLocked(!t->locked());
                  break;

            case COL_OCHANNEL:
                  {
                    mode = START_DRAG; // or not? (flo)
                    int delta = 0;
                    if (button == Qt::RightButton) 
                      delta = 1;
                    else if (button == Qt::MidButton) 
                      delta = -1;
                    if (t->isMidiTrack()) 
                    {
                      MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(t);
                      if (mt == 0)
                        break;
                      if (mt->type() == MusECore::Track::DRUM)
                        break;

                      int channel = mt->outChannel();
                      channel += delta;
                      if(channel >= MIDI_CHANNELS)
                        channel = MIDI_CHANNELS - 1;
                      if(channel < 0)
                        channel = 0;
                      if (channel != mt->outChannel()) 
                      {
                            // Changed by T356.
                            MusEGlobal::audio->msgIdle(true);
                            mt->setOutChanAndUpdate(channel);
                            MusEGlobal::audio->msgIdle(false);
                            
                            // DELETETHIS 15?
                            /* --- I really don't like this, you can mess up the whole map "as easy as dell"
                            if (mt->type() == MusECore::MidiTrack::DRUM) {//Change channel on all drum instruments
                                  for (int i=0; i<DRUM_MAPSIZE; i++)
                                        MusEGlobal::drumMap[i].channel = channel;
                                  }*/
                            
                            // may result in adding/removing mixer strip:
                            //MusEGlobal::song->update(-1);
                            //MusEGlobal::song->update(SC_CHANNELS);
                            //MusEGlobal::song->update(SC_MIDI_TRACK_PROP);
                            MusEGlobal::audio->msgUpdateSoloStates();                   // p4.0.14
                            //MusEGlobal::song->update(SC_MIDI_TRACK_PROP | SC_ROUTE);  //
                            MusEGlobal::song->update(SC_MIDI_TRACK_PROP);               //
                      }
                    }
                    else
                    {
                        if(t->type() != MusECore::Track::AUDIO_SOFTSYNTH)
                        {
                          MusECore::AudioTrack* at = dynamic_cast<MusECore::AudioTrack*>(t);
                          if (at == 0)
                            break;
                    
                          int n = t->channels() + delta;
                          if (n > MAX_CHANNELS)
                                n = MAX_CHANNELS;
                          else if (n < 1)
                                n = 1;
                          if (n != t->channels()) {
                                MusEGlobal::audio->msgSetChannels(at, n);
                                MusEGlobal::song->update(SC_CHANNELS);
                                }
                        }         
                    }      
                  }
                  break;
            
            default:
                  mode = START_DRAG;
                  if (col>=COL_CUSTOM_MIDICTRL_OFFSET && t->isMidiTrack())
                  {
                    if (Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                        Arranger::custom_col_t::AFFECT_BEGIN)
                      ctrl_at_tick=0;
                    else
                      ctrl_at_tick=MusEGlobal::song->cpos();
                    
                    int delta = 0;
                    if (button == Qt::RightButton) 
                      delta = 1;
                    else if (button == Qt::MidButton) 
                      delta = -1;

                    if (delta!=0) 
                    {
                      MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(t);
                      if (mt == 0)
                        break;
                      
                      int ctrl_num = Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;
                      
                      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                      MusECore::MidiController* mctl = mp->midiController(ctrl_num);
                      
                      int minval=mctl->minVal()+mctl->bias();
                      int maxval=mctl->maxVal()+mctl->bias();

                      int val = mt->getControllerChangeAtTick(0,ctrl_num);
                      int oldval=val;
                      
                      if (ctrl_num!=MusECore::CTRL_PROGRAM)
                      {
                        val += delta;
                        if(val > maxval)
                          val = maxval;
                        if(val < minval-1) // "-1" because of "off"
                          val = minval-1;
                      }
                      else
                      {
                        MusECore::MidiInstrument* instr = mp->instrument();
                        if (delta>0) val=instr->getNextPatch(mt->outChannel(), val, MusEGlobal::song->mtype(), false);
                        else if (delta<0) val=instr->getPrevPatch(mt->outChannel(), val, MusEGlobal::song->mtype(), false);
                      }

                      if (val != oldval)
                      {
                        if (val!=minval-1)
                        {
                          record_controller_change_and_maybe_send(ctrl_at_tick, ctrl_num, val, mt);
                        }
                        else
                        {
                          MusECore::Undo operations;
                          for (MusECore::iPart p = mt->parts()->begin(); p!=mt->parts()->end(); p++)
                          {
                            if (p->second->tick()==0)
                            {
                              for (MusECore::iEvent ev=p->second->events()->begin(); ev!=p->second->events()->end(); ev++)
                              {
                                if (ev->second.tick()!=0) break;
                                else if (ev->second.type()==MusECore::Controller && ev->second.dataA()==ctrl_num)
                                {
                                  using MusECore::UndoOp;
                                  operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, p->second, false, false));
                                  break;
                                }
                              }
                            }
                          }
                          MusEGlobal::song->applyOperationGroup(operations);
                        }
                      }
                    }
                    else // if (delta==0)
                    {
                      ctrl_num=Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;
                      
                      if (ctrl_num==MusECore::CTRL_PROGRAM)
                      {
                        editTrack=t;

                        MusECore::MidiTrack* mt=(MusECore::MidiTrack*)t;
                        MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                        MusECore::MidiInstrument* instr = mp->instrument();
                        
                        PopupMenu* pup = new PopupMenu(true);
                        instr->populatePatchPopup(pup, mt->outChannel(), MusEGlobal::song->mtype(), mt->type()==MusECore::Track::DRUM);
                        
                        if(pup->actions().count() == 0)
                        {
                          delete pup;
                          return;
                        }
                        
                        connect(pup, SIGNAL(triggered(QAction*)), SLOT(instrPopupActivated(QAction*)));
                        
                        QAction *act = pup->exec(ev->globalPos());
                        if(act)
                        {
                          int val = act->data().toInt();
                          if(val != -1)
                            record_controller_change_and_maybe_send(ctrl_at_tick, MusECore::CTRL_PROGRAM, val, mt);
                        }
                              
                        delete pup;      
                      }
                    }
                  }
      } //end of "switch"
      redraw();
}

//---------------------------------------------------------
//   selectTrack
//---------------------------------------------------------
void TList::selectTrack(MusECore::Track* tr)
{
    MusEGlobal::song->deselectTracks();

    if (tr) {
        tr->setSelected(true);


        // rec enable track if expected
        MusECore::TrackList recd = getRecEnabledTracks();
        if (recd.size() == 1 && MusEGlobal::config.moveArmedCheckBox) { // one rec enabled track, move rec enabled with selection
            MusEGlobal::song->setRecordFlag((MusECore::Track*)recd.front(),false);
            MusEGlobal::song->setRecordFlag(tr,true);
        }
    }

    redraw();
    emit selectionChanged(tr);
}

//---------------------------------------------------------
//   selectTrackAbove
//---------------------------------------------------------
void TList::selectTrackAbove()
{
     moveSelection(-1);
}
//---------------------------------------------------------
//   selectTrackBelow
//---------------------------------------------------------
void TList::selectTrackBelow()
{
     moveSelection(1);
}

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void TList::mouseMoveEvent(QMouseEvent* ev)
      {
      if ((((QInputEvent*)ev)->modifiers() | ev->buttons()) == 0) {
            int y = ev->y();
            int ty = -ypos;
            MusECore::TrackList* tracks = MusEGlobal::song->tracks();
            MusECore::iTrack it;
            for (it = tracks->begin(); it != tracks->end(); ++it) {
                  int h = (*it)->height();
                  ty += h;
                  if (y >= (ty-2)) { 
                        if ( (*it) == tracks->back() && y >= ty ) { // DELETETHIS and cleanup
                              // outside last track don't change to splitVCursor
                        }
                        else if ( y > (ty+2) ) {
                              //printf(" y > (ty+2) \n");
                        }
                        else {
                              if (!resizeFlag) {
                                    resizeFlag = true;
                                    setCursor(QCursor(Qt::SplitVCursor));
                                    }
                              break;
                              }
                        }
                  }
            if (it == tracks->end() && resizeFlag) {
                  setCursor(QCursor(Qt::ArrowCursor));
                  resizeFlag = false;
                  }
            return;
            }
      curY      = ev->y();
      int delta = curY - startY;
      switch (mode) {
            case START_DRAG:
                  if (delta < 0)
                        delta = -delta;
                  if (delta <= 2)
                        break;
                  {
                  MusECore::Track* t = y2Track(startY + ypos);
                  if (t == 0)
                        mode = NORMAL;
                  else {
                        mode = DRAG;
                        dragHeight = t->height();
                        sTrack     = MusEGlobal::song->tracks()->index(t);
                        setCursor(QCursor(Qt::SizeVerCursor));
                        redraw();
                        }
                  }
                  break;
            case NORMAL:
                  break;
            case DRAG:
                  redraw();
                  break;
            case RESIZE:
                  {
                    if(sTrack >= 0 && (unsigned) sTrack < MusEGlobal::song->tracks()->size())
                    {
                      MusECore::Track* t = MusEGlobal::song->tracks()->index(sTrack);
                      if(t)
                      {
                        int h  = t->height() + delta;
                        startY = curY;
                        if (h < MIN_TRACKHEIGHT)
                              h = MIN_TRACKHEIGHT;
                        t->setHeight(h);
                        MusEGlobal::song->update(SC_TRACK_MODIFIED);
                      }  
                    }  
                  }
                  break;
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void TList::mouseReleaseEvent(QMouseEvent* ev)
      {
      if (mode == DRAG) {
            MusECore::Track* t = y2Track(ev->y() + ypos);
            if (t) {
                  int dTrack = MusEGlobal::song->tracks()->index(t);
                  MusEGlobal::audio->msgMoveTrack(sTrack, dTrack);
                  }
            }
      if (mode != NORMAL) {
            mode = NORMAL;
            setCursor(QCursor(Qt::ArrowCursor));
            redraw();
            }
      if (editTrack && editor && editor->isVisible())
            editor->setFocus();
      //else
      //if (editTrack && chan_edit && chan_edit->isVisible())  // p4.0.46 DELETETHIS or add the same for ctrl_edit
      //      chan_edit->setFocus();
      adjustScrollbar();
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void TList::wheelEvent(QWheelEvent* ev)
      {
      int x           = ev->x();
      int y           = ev->y();
      MusECore::Track* t        = y2Track(y + ypos);
      if (t == 0) {
            emit redirectWheelEvent(ev);
            return;
            }
      TrackColumn col = TrackColumn(header->logicalIndexAt(x));
      int delta       = ev->delta() / WHEEL_DELTA;
      ev->accept();

      switch (col) {
            case COL_RECORD:
            case COL_NONE:
            case COL_CLASS:
            case COL_NAME:
            case COL_AUTOMATION:
                  break;
            case COL_MUTE:
                  // p3.3.29
                  if (((QInputEvent*)ev)->modifiers() & Qt::ShiftModifier)
                    t->setOff(!t->off());
                  else
                  {
                    if (t->off())
                          t->setOff(false);
                    else
                          t->setMute(!t->mute());
                  }        
                  MusEGlobal::song->update(SC_MUTE);
                  break;

            case COL_SOLO:
                  MusEGlobal::audio->msgSetSolo(t, !t->solo());
                  MusEGlobal::song->update(SC_SOLO);
                  break;

            case COL_TIMELOCK:
                  t->setLocked(!t->locked());
                  break;

            case COL_OPORT:
                  if (t->isMidiTrack()) {
                        MusECore::MidiTrack* mt = (MusECore::MidiTrack*)t;
                        int port = mt->outPort() + delta;

                        if (port >= MIDI_PORTS)
                              port = MIDI_PORTS-1;
                        else if (port < 0)
                              port = 0;
                        if (port != ((MusECore::MidiTrack*)t)->outPort()) {
                              MusEGlobal::audio->msgIdle(true);
                              mt->setOutPortAndUpdate(port);
                              MusEGlobal::audio->msgIdle(false);
                              
                              MusEGlobal::audio->msgUpdateSoloStates();     // p4.0.14
                              MusEGlobal::song->update(SC_MIDI_TRACK_PROP); // p4.0.17
                              }
                        }
                  break;

            case COL_OCHANNEL:
                  if (t->isMidiTrack()) {
                        MusECore::MidiTrack* mt = (MusECore::MidiTrack*)t;
                        if (mt && mt->type() == MusECore::Track::DRUM)
                            break;

                        int channel = mt->outChannel() + delta;

                        if (channel >= MIDI_CHANNELS)
                              channel = MIDI_CHANNELS-1;
                        else if (channel < 0)
                              channel = 0;
                        if (channel != ((MusECore::MidiTrack*)t)->outChannel()) {
                              MusEGlobal::audio->msgIdle(true);
                              mt->setOutChanAndUpdate(channel);
                              MusEGlobal::audio->msgIdle(false);
                              
                              // may result in adding/removing mixer strip:
                              //MusEGlobal::song->update(-1);
                              MusEGlobal::audio->msgUpdateSoloStates();         // p4.0.14
                              MusEGlobal::song->update(SC_MIDI_TRACK_PROP);     
                              }
                        }
                  else {
                        int n = t->channels() + delta;
                        if (n > MAX_CHANNELS)
                              n = MAX_CHANNELS;
                        else if (n < 1)
                              n = 1;
                        if (n != t->channels()) {
                              MusEGlobal::audio->msgSetChannels((MusECore::AudioTrack*)t, n);
                              MusEGlobal::song->update(SC_CHANNELS);
                              }
                        }
                  break;
            default:
                  if (col>=COL_CUSTOM_MIDICTRL_OFFSET)
                  {
                    mode = START_DRAG;

                    if (t->isMidiTrack()) 
                    {
                      MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(t);
                      if (mt == 0)
                        break;
                      
                      int ctrl_num = Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].ctrl;
                      
                      MusECore::MidiPort* mp = &MusEGlobal::midiPorts[mt->outPort()];
                      MusECore::MidiController* mctl = mp->midiController(ctrl_num);
                      
                      int minval=mctl->minVal()+mctl->bias();
                      int maxval=mctl->maxVal()+mctl->bias();

                      int val = mt->getControllerChangeAtTick(0,ctrl_num);
                      int oldval=val;

                      if (ctrl_num!=MusECore::CTRL_PROGRAM)
                      {
                        val += delta;
                        if(val > maxval)
                          val = maxval;
                        if(val < minval-1) // "-1" because of "off"
                          val = minval-1;
                      }
                      else
                      {
                        MusECore::MidiInstrument* instr = mp->instrument();
                        if (delta>0) val=instr->getNextPatch(mt->outChannel(), val, MusEGlobal::song->mtype(), false);
                        else if (delta<0) val=instr->getPrevPatch(mt->outChannel(), val, MusEGlobal::song->mtype(), false);
                      }

                      if (val != oldval)
                      {
                        if (val!=minval-1)
                        {
                          int at_tick;
                          if (Arranger::custom_columns[col-COL_CUSTOM_MIDICTRL_OFFSET].affected_pos ==
                              Arranger::custom_col_t::AFFECT_BEGIN)
                            at_tick=0;
                          else
                            at_tick=MusEGlobal::song->cpos();
                          
                          record_controller_change_and_maybe_send(at_tick, ctrl_num, val, mt);
                        }
                        else
                        {
                          MusECore::Undo operations;
                          for (MusECore::iPart p = mt->parts()->begin(); p!=mt->parts()->end(); p++)
                          {
                            if (p->second->tick()==0)
                            {
                              for (MusECore::iEvent ev=p->second->events()->begin(); ev!=p->second->events()->end(); ev++)
                              {
                                if (ev->second.tick()!=0) break;
                                else if (ev->second.type()==MusECore::Controller && ev->second.dataA()==ctrl_num)
                                {
                                  using MusECore::UndoOp;
                                  operations.push_back(UndoOp(UndoOp::DeleteEvent, ev->second, p->second, false, false));
                                  break;
                                }
                              }
                            }
                          }
                          MusEGlobal::song->applyOperationGroup(operations);
                        }
                      }
                    }
                  }
                  else
                      mode = START_DRAG;

                  break;
            }
      }


//---------------------------------------------------------
//   setYPos
//---------------------------------------------------------

void TList::setYPos(int y)
      {
      int delta  = ypos - y;         // -  -> shift up
      ypos  = y;

      scroll(0, delta);
      }

//---------------------------------------------------------
//   classesPopupMenu
//---------------------------------------------------------

void TList::classesPopupMenu(MusECore::Track* t, int x, int y)
      {
      QMenu p;
      p.clear();
      p.addAction(QIcon(*addtrack_addmiditrackIcon), tr("Midi"))->setData(MusECore::Track::MIDI);
      p.addAction(QIcon(*addtrack_drumtrackIcon), tr("Drum"))->setData(MusECore::Track::DRUM);
      QAction* act = p.exec(mapToGlobal(QPoint(x, y)), 0);

      if (!act)
            return;

      int n = act->data().toInt();
      if (MusECore::Track::TrackType(n) == MusECore::Track::MIDI && t->type() == MusECore::Track::DRUM) {
            //
            //    Drum -> Midi
            //
            MusEGlobal::audio->msgIdle(true);
            MusECore::PartList* pl = t->parts();
            MusECore::MidiTrack* m = (MusECore::MidiTrack*) t;
            for (MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                  MusECore::EventList* el = ip->second->events();
                  for (MusECore::iEvent ie = el->begin(); ie != el->end(); ++ie) {
                        MusECore::Event ev = ie->second;
                        if(ev.type() == MusECore::Note)
                        {
                          int pitch = ev.pitch();
                          // Changed by T356.
                          // Tested: Notes were being mixed up switching back and forth between midi and drum.
                          //pitch = MusEGlobal::drumMap[pitch].anote; DELETETHIS
                          pitch = MusEGlobal::drumMap[pitch].enote;
                          
                          ev.setPitch(pitch);
                        }
                        else
                        if(ev.type() == MusECore::Controller)
                        {
                          int ctl = ev.dataA();
                          // Is it a drum controller event, according to the track port's instrument?
                          MusECore::MidiController *mc = MusEGlobal::midiPorts[m->outPort()].drumController(ctl);
                          if(mc)
                            // Change the controller event's index into the drum map to an instrument note.
                            ev.setA((ctl & ~0xff) | MusEGlobal::drumMap[ctl & 0x7f].enote);
                        }
                          
                      }
                  }
            t->setType(MusECore::Track::MIDI);
            MusEGlobal::audio->msgIdle(false);
            MusEGlobal::song->update(SC_EVENT_MODIFIED);
            }
      else if (MusECore::Track::TrackType(n) == MusECore::Track::DRUM && t->type() == MusECore::Track::MIDI) {
            //
            //    Midi -> Drum
            //
            bool change = QMessageBox::question(this, tr("Update drummap?"),
                           tr("Do you want to use same port and channel for all instruments in the drummap?"),
                           tr("&Yes"), tr("&No"), QString::null, 0, 1);
            
            MusEGlobal::audio->msgIdle(true);
            // Delete all port controller events.
            MusEGlobal::song->changeAllPortDrumCtrlEvents(false);
            
            if (!change) {
                  MusECore::MidiTrack* m = (MusECore::MidiTrack*) t;
                  for (int i=0; i<DRUM_MAPSIZE; i++) {
                        MusEGlobal::drumMap[i].channel = m->outChannel();
                        MusEGlobal::drumMap[i].port    = m->outPort();
                        }
                  }

            MusECore::PartList* pl = t->parts();
            MusECore::MidiTrack* m = (MusECore::MidiTrack*) t;
            for (MusECore::iPart ip = pl->begin(); ip != pl->end(); ++ip) {
                  MusECore::EventList* el = ip->second->events();
                  for (MusECore::iEvent ie = el->begin(); ie != el->end(); ++ie) {
                        MusECore::Event ev = ie->second;
                        if (ev.type() == MusECore::Note)
                        {
                          int pitch = ev.pitch();
                          pitch = MusEGlobal::drumInmap[pitch];
                          ev.setPitch(pitch);
                        }  
                        else
                        {
                          if(ev.type() == MusECore::Controller)
                          {
                            int ctl = ev.dataA();
                            // Is it a drum controller event, according to the track port's instrument?
                            MusECore::MidiController *mc = MusEGlobal::midiPorts[m->outPort()].drumController(ctl);
                            if(mc)
                              // Change the controller event's instrument note to an index into the drum map.
                              ev.setA((ctl & ~0xff) | MusEGlobal::drumInmap[ctl & 0x7f]);
                          }
                          
                        }
                        
                      }
                  }
            t->setType(MusECore::Track::DRUM);
            
            // Add all port controller events.
            MusEGlobal::song->changeAllPortDrumCtrlEvents(true);
            MusEGlobal::audio->msgIdle(false);
            MusEGlobal::song->update(SC_EVENT_MODIFIED);
            }
      }

void TList::instrPopupActivated(QAction* act)
{
  MusECore::MidiTrack* mt = dynamic_cast<MusECore::MidiTrack*>(editTrack);
  if(act && mt) 
  {
    int val = act->data().toInt();
    if(val != -1)
      record_controller_change_and_maybe_send(ctrl_at_tick, MusECore::CTRL_PROGRAM, val, mt);
  }
}


void TList::setHeader(Header* h)
{
  header=h;
  redraw();
}

} // namespace MusEGui
