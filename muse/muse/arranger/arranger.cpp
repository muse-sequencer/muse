//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: arranger.cpp,v 1.33.2.21 2009/11/17 22:08:22 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "config.h"

#include <stdio.h>

#include <values.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qtoolbutton.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qaccel.h>
#include <qcombobox.h>
#include <qwhatsthis.h>
#include <qtoolbar.h>
#include <qtooltip.h>
#include <qpopupmenu.h>
#include <qhbox.h>
#include <qstringlist.h>
#include <qfiledialog.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qmainwindow.h>
#include <qwidgetstack.h>
#include <qscrollbar.h>

#include "arranger.h"
#include "song.h"
#include "mtscale.h"
#include "scrollscale.h"
#include "pcanvas.h"
#include "poslabel.h"
#include "xml.h"
#include "splitter.h"
#include "lcombo.h"
#include "midiport.h"
#include "mididev.h"
#include "utils.h"
#include "globals.h"
#include "tlist.h"
#include "icons.h"
#include "mtrackinfobase.h"
#include "header.h"
#include "utils.h"
#include "alayout.h"
#include "audio.h"
#include "event.h"
#include "midiseq.h"
#include "midictrl.h"
#include "mpevent.h"
#include "gconfig.h"
#include "mixer/astrip.h"
#include "spinbox.h"
//---------------------------------------------------------
//   TWhatsThis::text
//---------------------------------------------------------

QString TWhatsThis::text(const QPoint& pos)
      {
      int section = header->sectionAt(pos.x());
      if (section == -1)
            return QString::null;
      switch(section) {
            case COL_RECORD:   return QHeader::tr("Enable recording. Click to toggle."); break;
            case COL_MUTE:     return QHeader::tr("Mute indicator. Click to toggle.\nRight-click to toggle track on/off.\nMute is designed for rapid, repeated action.\nOn/Off is not!"); break;
            case COL_SOLO:     return QHeader::tr("Solo indicator. Click to toggle.\nConnected tracks are also 'phantom' soloed,\n indicated by a dark square."); break;
            case COL_CLASS:    return QHeader::tr("Track type. Right-click to change\n midi and drum track types."); break;
            case COL_NAME:     return QHeader::tr("Track name. Double-click to edit.\nRight-click for more options."); break;
            case COL_OCHANNEL: return QHeader::tr("Midi/drum track: Output channel number.\nAudio track: Channels.\nMid/right-click to change."); break;
            case COL_OPORT:    return QHeader::tr("Midi/drum track: Output port.\nSynth track: Assigned midi port.\nLeft-click to change.\nRight-click to show GUI."); break;
            case COL_TIMELOCK: return QHeader::tr("Time lock"); break;
            default: break;
            }
      return QString::null;
      }

//---------------------------------------------------------
//   Arranger
//    is the central widget in app
//---------------------------------------------------------

Arranger::Arranger(QMainWindow* parent, const char* name)
   : QWidget(parent, name)
      {
      _raster  = 0;      // measure
      selected = 0;
      // Since program covers 3 controls at once, it is in 'midi controller' units rather than 'gui control' units.
      //program  = -1;
      program  = CTRL_VAL_UNKNOWN;
      pan      = -65;
      volume   = -1;
      setMinimumSize(600, 50);
      showTrackinfoFlag = true;
      
      cursVal = MAXINT;
      //---------------------------------------------------
      //  ToolBar
      //    create toolbar in toplevel widget
      //---------------------------------------------------

      QToolBar* toolbar = new QToolBar(tr("Arranger"), parent);

      QLabel* label = new QLabel(tr("Cursor"), toolbar, "Cursor");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      cursorPos = new PosLabel(toolbar);
      cursorPos->setEnabled(false);

      const char* rastval[] = {
            QT_TR_NOOP("Off"), QT_TR_NOOP("Bar"), "1/2", "1/4", "1/8", "1/16"
            };
      label = new QLabel(tr("Snap"), toolbar, "Snap");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      QComboBox* raster = new QComboBox(toolbar);
      for (int i = 0; i < 6; i++)
            raster->insertItem(tr(rastval[i]), i);
      raster->setCurrentItem(1);
      connect(raster, SIGNAL(activated(int)), SLOT(_setRaster(int)));
      raster->setFocusPolicy(QWidget::NoFocus);

      // Song len
      label = new QLabel(tr("Len"), toolbar, "Len");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);

      // song length is limited to 10000 bars; the real song len is limited
      // by overflows in tick computations
      //
      lenEntry = new SpinBox(1, 10000, 1, toolbar);
      lenEntry->setValue(song->len());
      connect(lenEntry, SIGNAL(valueChanged(int)), SLOT(songlenChanged(int)));
      QToolTip::add(lenEntry, tr("song length - bars"));
      QWhatsThis::add(lenEntry, tr("song length - bars"));

      typeBox = new LabelCombo(tr("Type"), toolbar);
      typeBox->insertItem(tr("NO"), 0);
      typeBox->insertItem(tr("GM"), 1);
      typeBox->insertItem(tr("GS"), 2);
      typeBox->insertItem(tr("XG"), 3);
      typeBox->setCurrentItem(0);
      connect(typeBox, SIGNAL(activated(int)), SLOT(modeChange(int)));
      QToolTip::add(typeBox, tr("midi song type"));
      QWhatsThis::add(typeBox, tr("midi song type"));
      typeBox->setFocusPolicy(QWidget::NoFocus);

      label = new QLabel(tr("Pitch"), toolbar, "Pitch");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      globalPitchSpinBox = new SpinBox(-127, 127, 1, toolbar);
      globalPitchSpinBox->setValue(song->globalPitchShift());
      QToolTip::add(globalPitchSpinBox, tr("midi pitch"));
      QWhatsThis::add(globalPitchSpinBox, tr("global midi pitch shift"));
      connect(globalPitchSpinBox, SIGNAL(valueChanged(int)), SLOT(globalPitchChanged(int)));
      label = new QLabel(tr("Tempo"), toolbar, "Tempo");
      label->setAlignment(AlignRight|AlignVCenter);
      label->setIndent(3);
      globalTempoSpinBox = new SpinBox(50, 200, 1, toolbar);
      globalTempoSpinBox->setSuffix(QString("%"));
      globalTempoSpinBox->setValue(tempomap.globalTempo());
      QToolTip::add(globalTempoSpinBox, tr("midi tempo"));
      QWhatsThis::add(globalTempoSpinBox, tr("midi tempo"));
      connect(globalTempoSpinBox, SIGNAL(valueChanged(int)), SLOT(globalTempoChanged(int)));
      QToolButton* tempo50  = new QToolButton(toolbar, "tempo50");
      tempo50->setText(QString("50%"));
      connect(tempo50, SIGNAL(clicked()), SLOT(setTempo50()));
      QToolButton* tempo100 = new QToolButton(toolbar, "tempo100");
      tempo100->setText(tr("N"));
      connect(tempo100, SIGNAL(clicked()), SLOT(setTempo100()));
      QToolButton* tempo200 = new QToolButton(toolbar, "tempo200");
      tempo200->setText(QString("200%"));
      connect(tempo200, SIGNAL(clicked()), SLOT(setTempo200()));

      QVBoxLayout* box  = new QVBoxLayout(this);
      box->addWidget(hLine(this), AlignTop);

      //---------------------------------------------------
      //  Tracklist
      //---------------------------------------------------

      int xscale = -100;
      int yscale = 1;

      split  = new Splitter(Horizontal, this, "split");
      split->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
      box->addWidget(split, 1000);

      QWidget* tracklist = new QWidget(split);
      split->setResizeMode(tracklist, QSplitter::KeepSize);
      tracklist->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding, 0, 100));

      QWidget* editor = new QWidget(split);
      split->setResizeMode(editor, QSplitter::Stretch);
      editor->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding,
        // Changed by T356. Was causing "large int implicitly truncated" warning. These are UCHAR values...
        //1000, 100));
        //232, 100)); // 232 is what it was being truncated to, but what is the right value?...
        255, 100));

      //---------------------------------------------------
      //    Track Info
      //---------------------------------------------------

      infoScroll = new QScrollBar(Vertical, tracklist, "infoScrollBar");
      genTrackInfo(tracklist);

      // Track-Info Button
      ib  = new QToolButton(tracklist);
      ib->setText(tr("TrackInfo"));
      ib->setToggleButton(true);
      ib->setOn(showTrackinfoFlag);

      connect(ib, SIGNAL(toggled(bool)), SLOT(showTrackInfo(bool)));

      // Changed by T356 Mar 8 2008. Did someone accidentally change to header1 recently? 
      // Muse was saying Tlist: unknown tag <header> at line 32
      //header = new Header(tracklist, "header1");
      header = new Header(tracklist, "header");
      
      header->setFixedHeight(30);

      QFontMetrics fm1(header->font());
      int fw = 8;

      header->addLabel(tr("R"), fm1.width('R')+fw);
      header->addLabel(tr("M"), fm1.width('M')+fw);
      header->addLabel(tr("S"), fm1.width('S')+fw);
      header->addLabel(tr("C"), fm1.width('C')+fw);
      header->addLabel(tr("Track"), 100);
      //header->addLabel(tr("O-Port"), 60);
      header->addLabel(tr("Port"), 60);
      header->addLabel(tr("Ch"), 30);
      header->addLabel(tr("T"), fm1.width('T')+fw);
      //header->addLabel(tr("Automation"),30);
      header->setResizeEnabled(false, COL_RECORD);
      header->setResizeEnabled(false, COL_MUTE);
      header->setResizeEnabled(false, COL_SOLO);
      header->setResizeEnabled(false, COL_CLASS);
      header->setResizeEnabled(false, COL_OCHANNEL);
      header->setResizeEnabled(false, COL_TIMELOCK);
      //header->setResizeEnabled(true, COL_AUTOMATION);
      header->setResizeEnabled(true, COL_NAME);
      header->setResizeEnabled(true, COL_OPORT);

      header->setTracking(true);

      new THeaderTip(header);
      new TWhatsThis(header, header);

      list = new TList(header, tracklist, "tracklist");

      connect(list, SIGNAL(selectionChanged()), SLOT(trackSelectionChanged()));
      connect(header, SIGNAL(sizeChange(int,int,int)), list, SLOT(redraw()));
      connect(header, SIGNAL(moved(int,int)), list, SLOT(redraw()));
      connect(header, SIGNAL(moved(int,int)), this, SLOT(headerMoved()));

      //  tracklist:
      //
      //         0         1         2
      //   +-----------+--------+---------+
      //   | Trackinfo | scroll | Header  | 0
      //   |           | bar    +---------+
      //   |           |        | TList   | 1
      //   +-----------+--------+---------+
      //   |             hline            | 2
      //   +-----+------------------------+
      //   | ib  |                        | 3
      //   +-----+------------------------+

      connect(infoScroll, SIGNAL(valueChanged(int)), SLOT(trackInfoScroll(int)));
      tgrid  = new TLLayout(tracklist); // layout manager for this
      tgrid->wadd(0, trackInfo);
      tgrid->wadd(1, infoScroll);
      tgrid->wadd(2, header);
      tgrid->wadd(3, list);
      tgrid->wadd(4, hLine(tracklist));
      tgrid->wadd(5, ib);

      //---------------------------------------------------
      //    Editor
      //---------------------------------------------------

      int offset = sigmap.ticksMeasure(0);
      hscroll = new ScrollScale(-1000, -10, xscale, song->len(), Horizontal, editor, -offset);
      ib->setFixedHeight(hscroll->sizeHint().height());

      // Changed p3.3.43 Too small steps for me...
      //vscroll = new QScrollBar(1, 20*20, 1, 5, 0, Vertical, editor);
      vscroll = new QScrollBar(1, 20*20, 5, 25, 0, Vertical, editor);
      
      list->setScroll(vscroll);

      QValueList<int> vallist;
      vallist.append(tgrid->maximumSize().width());
      split->setSizes(vallist);

      QGridLayout* egrid  = new QGridLayout(editor);
      egrid->setColStretch(0, 50);
      egrid->setRowStretch(2, 50);

      time = new MTScale(&_raster, editor, xscale);
      time->setOrigin(-offset, 0);
      canvas = new PartCanvas(&_raster, editor, xscale, yscale);
      canvas->setBg(config.partCanvasBg);
      canvas->setCanvasTools(arrangerTools);
      canvas->setOrigin(-offset, 0);
      canvas->setFocus();
      connect(canvas, SIGNAL(setUsedTool(int)), this, SIGNAL(setUsedTool(int)));
      connect(canvas, SIGNAL(trackChanged(Track*)), list, SLOT(selectTrack(Track*)));
      connect(list, SIGNAL(keyPressExt(QKeyEvent*)), canvas, SLOT(redirKeypress(QKeyEvent*)));
      connect(canvas, SIGNAL(selectTrackAbove()), list, SLOT(selectTrackAbove()));
      connect(canvas, SIGNAL(selectTrackBelow()), list, SLOT(selectTrackBelow()));

      connect(this, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(list, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      
      egrid->addMultiCellWidget(time,           0, 0, 0, 1);
      egrid->addMultiCellWidget(hLine(editor),  1, 1, 0, 1);
      egrid->addWidget(canvas,  2, 0);
      egrid->addWidget(vscroll, 2, 1);
      egrid->addWidget(hscroll, 3, 0, AlignBottom);

      connect(vscroll, SIGNAL(valueChanged(int)), canvas, SLOT(setYPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setXMag(int)));
      connect(vscroll, SIGNAL(valueChanged(int)), list,   SLOT(setYPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), time,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,   SLOT(setXMag(int)));
      connect(canvas,  SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(canvas,  SIGNAL(verticalScroll(unsigned)),SLOT(verticalScrollSetYpos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(time,    SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));

      connect(canvas, SIGNAL(tracklistChanged()), list, SLOT(tracklistChanged()));
      connect(canvas, SIGNAL(dclickPart(Track*)), SIGNAL(editPart(Track*)));
      connect(canvas, SIGNAL(startEditor(PartList*,int)),   SIGNAL(startEditor(PartList*, int)));

      connect(song,   SIGNAL(songChanged(int)), SLOT(songChanged(int)));
      connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
      connect(canvas, SIGNAL(selectionChanged()), SIGNAL(selectionChanged()));
      connect(canvas, SIGNAL(dropSongFile(const QString&)), SIGNAL(dropSongFile(const QString&)));
      connect(canvas, SIGNAL(dropMidiFile(const QString&)), SIGNAL(dropMidiFile(const QString&)));

      connect(canvas, SIGNAL(toolChanged(int)), SIGNAL(toolChanged(int)));
//      connect(song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(seek()));

      // Removed p3.3.43 
      // Song::addMarker() already emits a 'markerChanged'.
      //connect(time, SIGNAL(addMarker(int)), SIGNAL(addMarker(int)));
      
      configChanged();  // set configuration values
      showTrackInfo(showTrackinfoFlag);
      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

//void Arranger::updateHScrollRange()
//{
//      int s = 0, e = song->len();
      // Show one more measure.
//      e += sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
//      e += sigmap.ticksMeasure(e) / 4;
      // Compensate for the fixed vscroll width. 
//      e += canvas->rmapxDev(-vscroll->width()); 
//      int s1, e1;
//      hscroll->range(&s1, &e1);
//      if(s != s1 || e != e1) 
//        hscroll->setRange(s, e);
//}

//---------------------------------------------------------
//   headerMoved
//---------------------------------------------------------

void Arranger::headerMoved()
      {
      header->setStretchEnabled(true, COL_NAME);
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void Arranger::setTime(unsigned tick)
      {
      if (tick == MAXINT)
            cursorPos->setEnabled(false);
      else {
            cursVal = tick;
            cursorPos->setEnabled(true);
            cursorPos->setValue(tick);
            time->setPos(3, tick, false);
            }
      }

//---------------------------------------------------------
//   toolChange
//---------------------------------------------------------

void Arranger::setTool(int t)
      {
      canvas->setTool(t);
      }

//---------------------------------------------------------
//   dclickPart
//---------------------------------------------------------

void Arranger::dclickPart(Track* t)
      {
      emit editPart(t);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void Arranger::configChanged()
      {
      // Added by Tim. p3.3.6
      //printf("Arranger::configChanged\n");
      
      if (config.canvasBgPixmap.isEmpty()) {
            canvas->setBg(config.partCanvasBg);
            canvas->setBg(QPixmap());
            //printf("Arranger::configChanged - no bitmap!\n");
      }
      else {
        
            //printf("Arranger::configChanged - bitmap %s!\n", config.canvasBgPixmap.ascii());
            canvas->setBg(QPixmap(config.canvasBgPixmap));
      }
      midiTrackInfo->setFont(config.fonts[2]);
      //updateTrackInfo(type);
      }

//---------------------------------------------------------
//   songlenChanged
//---------------------------------------------------------

void Arranger::songlenChanged(int n)
      {
      int newLen = sigmap.bar2tick(n, 0, 0);
      song->setLen(newLen);
      }
//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void Arranger::songChanged(int type)
      {
      // Is it simply a midi controller value adjustment? Forget it.
      if(type != SC_MIDI_CONTROLLER)
      {
        unsigned endTick = song->len();
        int offset  = sigmap.ticksMeasure(endTick);
        hscroll->setRange(-offset, endTick + offset);  //DEBUG
        canvas->setOrigin(-offset, 0);
        time->setOrigin(-offset, 0);
  
        int bar, beat;
        unsigned tick;
        sigmap.tickValues(endTick, &bar, &beat, &tick);
        if (tick || beat)
              ++bar;
        lenEntry->blockSignals(true);
        lenEntry->setValue(bar);
        lenEntry->blockSignals(false);
  
        trackSelectionChanged();
        canvas->partsChanged();
        typeBox->setCurrentItem(int(song->mtype()));
        if (type & SC_SIG)
              time->redraw();
        if (type & SC_TEMPO)
              setGlobalTempo(tempomap.globalTempo());
              
        if(type & SC_TRACK_REMOVED)
        {
          AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
          if(w)
          {
            Track* t = w->getTrack();
            if(t)
            {
              TrackList* tl = song->tracks();
              iTrack it = tl->find(t);
              if(it == tl->end())
              {
                delete w;
                trackInfo->addWidget(0, 2);
                selected = 0;
              } 
            }   
          } 
        }
      }
            
      updateTrackInfo(type);
    }

//---------------------------------------------------------
//   trackSelectionChanged
//---------------------------------------------------------

void Arranger::trackSelectionChanged()
      {
      TrackList* tracks = song->tracks();
      Track* track = 0;
      for (iTrack t = tracks->begin(); t != tracks->end(); ++t) {
            if ((*t)->selected()) {
                  track = *t;
                  break;
                  }
            }
      if (track == selected)
            return;
      selected = track;
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   modeChange
//---------------------------------------------------------

void Arranger::modeChange(int mode)
      {
      song->setMType(MType(mode));
      updateTrackInfo(-1);
      }

//---------------------------------------------------------
//   setMode
//---------------------------------------------------------

void Arranger::setMode(int mode)
      {
      typeBox->setCurrentItem(mode);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Arranger::writeStatus(int level, Xml& xml)
      {
      xml.tag(level++, "arranger");
      xml.intTag(level, "info", ib->isOn());
      split->writeStatus(level, xml);
      list->writeStatus(level, xml, "list");

      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ypos", vscroll->value());
      xml.etag(level, "arranger");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Arranger::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token(xml.parse());
            const QString& tag(xml.s1());
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "info")
                              showTrackinfoFlag = xml.parseInt();
                        else if (tag == split->name())
                              split->readStatus(xml);
                        else if (tag == "list")
                              list->readStatus(xml, "list");
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos") {
                              int hpos = xml.parseInt();
                              hscroll->setPos(hpos);
                              }
                        else if (tag == "ypos")
                              vscroll->setValue(xml.parseInt());
                        else
                              xml.unknown("Arranger");
                        break;
                  case Xml::TagEnd:
                        if (tag == "arranger") {
                              ib->setOn(showTrackinfoFlag);
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void Arranger::_setRaster(int index)
      {
      static int rasterTable[] = {
            1, 0, 768, 384, 192, 96
            };
      _raster = rasterTable[index];
      // Set the audio record part snapping.
      song->setRecRaster(_raster);
      canvas->redraw();
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Arranger::reset()
      {
      canvas->setXPos(0);
      canvas->setYPos(0);
      hscroll->setPos(0);
      vscroll->setValue(0);
      time->setXPos(0);
      time->setYPos(0);
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void Arranger::cmd(int cmd)
      {
      int ncmd;
      switch (cmd) {
            case CMD_CUT_PART:
                  ncmd = PartCanvas::CMD_CUT_PART;
                  break;
            case CMD_COPY_PART:
                  ncmd = PartCanvas::CMD_COPY_PART;
                  break;
            case CMD_PASTE_PART:
                  ncmd = PartCanvas::CMD_PASTE_PART;
                  break;
            case CMD_PASTE_CLONE_PART:
                  ncmd = PartCanvas::CMD_PASTE_CLONE_PART;
                  break;
            case CMD_PASTE_PART_TO_TRACK:
                  ncmd = PartCanvas::CMD_PASTE_PART_TO_TRACK;
                  break;
            case CMD_PASTE_CLONE_PART_TO_TRACK:
                  ncmd = PartCanvas::CMD_PASTE_CLONE_PART_TO_TRACK;
                  break;
            default:
                  return;
            }
      canvas->cmd(ncmd);
      }

//---------------------------------------------------------
//   globalPitchChanged
//---------------------------------------------------------

void Arranger::globalPitchChanged(int val)
      {
      song->setGlobalPitchShift(val);
      }

//---------------------------------------------------------
//   globalTempoChanged
//---------------------------------------------------------

void Arranger::globalTempoChanged(int val)
      {
      audio->msgSetGlobalTempo(val);
      song->tempoChanged();
      }

//---------------------------------------------------------
//   setTempo50
//---------------------------------------------------------

void Arranger::setTempo50()
      {
      setGlobalTempo(50);
      }

//---------------------------------------------------------
//   setTempo100
//---------------------------------------------------------

void Arranger::setTempo100()
      {
      setGlobalTempo(100);
      }

//---------------------------------------------------------
//   setTempo200
//---------------------------------------------------------

void Arranger::setTempo200()
      {
      setGlobalTempo(200);
      }

//---------------------------------------------------------
//   setGlobalTempo
//---------------------------------------------------------

void Arranger::setGlobalTempo(int val)
      {
      if(val != globalTempoSpinBox->value())
        globalTempoSpinBox->setValue(val);
      }

//---------------------------------------------------------
//   verticalScrollSetYpos
//---------------------------------------------------------
void Arranger::verticalScrollSetYpos(unsigned ypos)
      {
      vscroll->setValue(ypos);
      }

//---------------------------------------------------------
//   progRecClicked
//---------------------------------------------------------

void Arranger::progRecClicked()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      int program      = port->hwCtrlState(channel, CTRL_PROGRAM);
      if(program == CTRL_VAL_UNKNOWN || program == 0xffffff) 
        return;

      unsigned tick = song->cpos();
      Event a(Controller);
      a.setTick(tick);
      a.setA(CTRL_PROGRAM);
      a.setB(program);

      song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   volRecClicked
//---------------------------------------------------------

void Arranger::volRecClicked()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      int volume       = port->hwCtrlState(channel, CTRL_VOLUME);
      if(volume == CTRL_VAL_UNKNOWN) 
        return;

      unsigned tick = song->cpos();
      Event a(Controller);
      a.setTick(tick);
      a.setA(CTRL_VOLUME);
      a.setB(volume);

      song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   panRecClicked
//---------------------------------------------------------

void Arranger::panRecClicked()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      int pan          = port->hwCtrlState(channel, CTRL_PANPOT);
      if(pan == CTRL_VAL_UNKNOWN) 
        return;

      unsigned tick = song->cpos();
      Event a(Controller);
      a.setTick(tick);
      a.setA(CTRL_PANPOT);
      a.setB(pan);

      song->recordEvent(track, a);
      }

//---------------------------------------------------------
//   recordClicked
//---------------------------------------------------------

void Arranger::recordClicked()
      {
      MidiTrack* track = (MidiTrack*)selected;
      int portno       = track->outPort();
      int channel      = track->outChannel();
      MidiPort* port   = &midiPorts[portno];
      unsigned tick = song->cpos();
      
      int program = port->hwCtrlState(channel, CTRL_PROGRAM);
      if(program != CTRL_VAL_UNKNOWN && program != 0xffffff) 
      {
        Event a(Controller);
        a.setTick(tick);
        a.setA(CTRL_PROGRAM);
        a.setB(program);
        song->recordEvent(track, a);
      }
      int volume = port->hwCtrlState(channel, CTRL_VOLUME);
      if(volume != CTRL_VAL_UNKNOWN) 
      {
        Event a(Controller);
        a.setTick(tick);
        a.setA(CTRL_VOLUME);
        a.setB(volume);
        song->recordEvent(track, a);
      }
      int pan = port->hwCtrlState(channel, CTRL_PANPOT);
      if(pan != CTRL_VAL_UNKNOWN) 
      {
        Event a(Controller);
        a.setTick(tick);
        a.setA(CTRL_PANPOT);
        a.setB(pan);
        song->recordEvent(track, a);
      }
    }

//---------------------------------------------------------
//   trackInfoScroll
//---------------------------------------------------------

void Arranger::trackInfoScroll(int y)
      {
      if (trackInfo->visibleWidget())
            trackInfo->visibleWidget()->move(0, -y);
      }

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

WidgetStack::WidgetStack(QWidget* parent, const char* name)
   : QWidget(parent, name)
      {
      top = -1;
      }

//---------------------------------------------------------
//   raiseWidget
//---------------------------------------------------------

void WidgetStack::raiseWidget(int idx)
      {
      if (top != -1) {
            if (stack[top])
                  stack[top]->hide();
            }
      top = idx;
      if (idx == -1)
            return;
      int n = stack.size();
      if (idx >= n)
            return;
      if (stack[idx])
            stack[idx]->show();
      }

//---------------------------------------------------------
//   addWidget
//---------------------------------------------------------

void WidgetStack::addWidget(QWidget* w, unsigned int n)
      {
      if (w)
            w->hide();
      if (stack.size() <= n )
            stack.push_back(w);
      else
            stack[n] = w;
      }

QWidget* WidgetStack::getWidget(unsigned int n)
      {
      if (stack.size() <= n )
            return 0;
      return stack[n];
      }

//---------------------------------------------------------
//   visibleWidget
//---------------------------------------------------------

QWidget* WidgetStack::visibleWidget() const
      {
      if (top != -1)
            return stack[top];
      return 0;
      }

//---------------------------------------------------------
//   minimumSizeHint
//---------------------------------------------------------

QSize WidgetStack::minimumSizeHint() const
      {
      if (top == -1)
            return (QSize(0, 0));
      QSize s(0,0);
      for (unsigned int i = 0; i < stack.size(); ++i) {
            if (stack[i]) {
                  QSize ss = stack[i]->minimumSizeHint();
                  if (!ss.isValid())
                        ss = stack[i]->minimumSize();
                  s = s.expandedTo(ss);
                  }
            }
      return s;
      }

//---------------------------------------------------------
//   clear
//---------------------------------------------------------

void Arranger::clear()
      {
      AudioStrip* w = (AudioStrip*)(trackInfo->getWidget(2));
      if (w)
            delete w;
      trackInfo->addWidget(0, 2);
      selected = 0;
      }

void Arranger::wheelEvent(QWheelEvent* ev)
      {
      emit redirectWheelEvent(ev);
      }

void Arranger::controllerChanged(Track *t)
{
      canvas->controllerChanged(t);
}
