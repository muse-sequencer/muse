//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: pianoroll.cpp,v 1.25.2.15 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <q3accel.h>
#include <qlayout.h>
#include <q3hbox.h>
#include <qsizegrip.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <q3buttongroup.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <q3popupmenu.h>
#include <qmenubar.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qdir.h>
#include <qaction.h>
//Added by qt3to4:
#include <QKeyEvent>
#include <Q3GridLayout>
#include <QResizeEvent>
#include <QCloseEvent>

#include <stdio.h>

#include "xml.h"
#include "mtscale.h"
#include "prcanvas.h"
#include "pianoroll.h"
#include "scrollscale.h"
#include "piano.h"
#include "../ctrl/ctrledit.h"
#include "splitter.h"
#include "ttoolbar.h"
#include "tb1.h"
#include "utils.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "audio.h"

#include "cmd.h"
#include "quantconfig.h"
#include "shortcuts.h"

int PianoRoll::_quantInit = 96;
int PianoRoll::_rasterInit = 96;
int PianoRoll::_widthInit = 600;
int PianoRoll::_heightInit = 400;
int PianoRoll::_quantStrengthInit = 80;      // 1 - 100%
int PianoRoll::_quantLimitInit = 50;         // tick value
bool PianoRoll::_quantLenInit = false;
int PianoRoll::_toInit = 0;
int PianoRoll::colorModeInit = 0;

static const int xscale = -10;
static const int yscale = 1;
static const int pianoWidth = 40;
static int pianorollTools = PointerTool | PencilTool | RubberTool | DrawTool;




//---------------------------------------------------------
//   PianoRoll
//---------------------------------------------------------

PianoRoll::PianoRoll(PartList* pl, QWidget* parent, const char* name, unsigned initPos)
   : MidiEditor(_quantInit, _rasterInit, pl, parent, name)
      {
      deltaMode = false;
      resize(_widthInit, _heightInit);
      selPart        = 0;
      quantConfig    = 0;
      _playEvents    = false;
      _quantStrength = _quantStrengthInit;
      _quantLimit    = _quantLimitInit;
      _quantLen      = _quantLenInit;
      _to            = _toInit;
      colorMode      = colorModeInit;
      //---------Men----------------------------------
      menuEdit = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&Edit"), menuEdit);
      undoRedo->addTo(menuEdit);
      Q3Accel* ud = new Q3Accel(this);
      ud->connectItem(ud->insertItem(Qt::CTRL+Qt::Key_Z), song, SLOT(undo()));
      Q3Accel* rd = new Q3Accel(this);
      rd->connectItem(rd->insertItem(Qt::CTRL+Qt::Key_Y), song, SLOT(redo()));

      menuEdit->insertSeparator();
      menuEdit->insertItem(tr("Cut"),   PianoCanvas::CMD_CUT);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_X,    PianoCanvas::CMD_CUT);
      menuEdit->insertItem(tr("Copy"),  PianoCanvas::CMD_COPY);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_C,    PianoCanvas::CMD_COPY);
      menuEdit->insertItem(tr("Paste"), PianoCanvas::CMD_PASTE);
      menuEdit->setAccel(Qt::CTRL+Qt::Key_V,    PianoCanvas::CMD_PASTE);
      menuEdit->insertSeparator();
      menuEdit->insertItem(tr("Delete Events"), PianoCanvas::CMD_DEL);
      menuEdit->setAccel(Qt::Key_Delete, PianoCanvas::CMD_DEL);
      menuEdit->insertSeparator();

      menuSelect = new Q3PopupMenu(this);
      menuSelect->insertItem(tr("Select All"),   PianoCanvas::CMD_SELECT_ALL);
      menuSelect->insertItem(tr("Select None"),  PianoCanvas::CMD_SELECT_NONE);
      menuSelect->insertItem(tr("Invert"),       PianoCanvas::CMD_SELECT_INVERT);

      menuSelect->insertSeparator();
      menuSelect->insertItem(tr("Inside Loop"),  PianoCanvas::CMD_SELECT_ILOOP);

      menuSelect->insertItem(tr("Outside Loop"), PianoCanvas::CMD_SELECT_OLOOP);
      
      menuSelect->insertSeparator();

      menuSelect->insertItem(tr("Previous Part"), PianoCanvas::CMD_SELECT_PREV_PART);
      menuSelect->insertItem(tr("Next Part"), PianoCanvas::CMD_SELECT_NEXT_PART);
      
      menuEdit->insertItem(tr("&Select"), menuSelect);

      eventColor = new Q3PopupMenu(this);
      eventColor->insertItem(tr("blue"), 0);
      eventColor->insertItem(tr("pitch colors"), 1);
      eventColor->insertItem(tr("velocity colors"), 2);
      connect(eventColor, SIGNAL(activated(int)), SLOT(setEventColorMode(int)));

      menuConfig = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&Config"), menuConfig);
      menu_ids[CMD_EVENT_COLOR] = menuConfig->insertItem(tr("event color"), eventColor, 0);

      menuFunctions = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&Functions"), menuFunctions);
      menuFunctions->insertTearOffHandle();
      menuFunctions->insertItem(tr("Over Quantize"),        PianoCanvas::CMD_OVER_QUANTIZE);

      menuFunctions->insertItem(tr("Note On Quantize"),     PianoCanvas::CMD_ON_QUANTIZE);

      menuFunctions->insertItem(tr("Note On/Off Quantize"), PianoCanvas::CMD_ONOFF_QUANTIZE);
      menuFunctions->insertItem(tr("Iterative Quantize"),   PianoCanvas::CMD_ITERATIVE_QUANTIZE);

      menuFunctions->insertSeparator();
      menu_ids[CMD_CONFIG_QUANT] = menuFunctions->insertItem(tr("Config Quant..."), this, SLOT(configQuant()), 0);
      menuFunctions->insertSeparator();
      menuFunctions->insertItem(tr("Modify Gate Time"),             PianoCanvas::CMD_MODIFY_GATE_TIME);
      menuFunctions->insertItem(tr("Modify Velocity"),              PianoCanvas::CMD_MODIFY_VELOCITY);
      menuFunctions->insertItem(tr("Crescendo"),                    PianoCanvas::CMD_CRESCENDO);
      menuFunctions->insertItem(tr("Transpose"),                    PianoCanvas::CMD_TRANSPOSE);
      menuFunctions->insertItem(tr("Thin Out"),                     PianoCanvas::CMD_THIN_OUT);
      menuFunctions->insertItem(tr("Erase Event"),                  PianoCanvas::CMD_ERASE_EVENT);
      menuFunctions->insertItem(tr("Note Shift"),                   PianoCanvas::CMD_NOTE_SHIFT);
      menuFunctions->insertItem(tr("Move Clock"),                   PianoCanvas::CMD_MOVE_CLOCK);
      menuFunctions->insertItem(tr("Copy Measure"),                 PianoCanvas::CMD_COPY_MEASURE);
      menuFunctions->insertItem(tr("Erase Measure"),                PianoCanvas::CMD_ERASE_MEASURE);
      menuFunctions->insertItem(tr("Delete Measure"),               PianoCanvas::CMD_DELETE_MEASURE);
      menuFunctions->insertItem(tr("Create Measure"),               PianoCanvas::CMD_CREATE_MEASURE);
      menuFunctions->insertItem(tr("Set fixed length"),             PianoCanvas::CMD_FIXED_LEN);
      menuFunctions->insertItem(tr("Delete overlaps"),              PianoCanvas::CMD_DELETE_OVERLAPS);

      menuFunctions->setItemEnabled(PianoCanvas::CMD_CRESCENDO, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_TRANSPOSE, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_THIN_OUT, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_ERASE_EVENT, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_NOTE_SHIFT, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_MOVE_CLOCK, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_COPY_MEASURE, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_ERASE_MEASURE, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_DELETE_MEASURE, false);
      menuFunctions->setItemEnabled(PianoCanvas::CMD_CREATE_MEASURE, false);

      menuPlugins = new Q3PopupMenu(this);
      song->populateScriptMenu(menuPlugins, this);
      menuBar()->insertItem(tr("&Plugins"), menuPlugins);
      connect(menuEdit, SIGNAL(activated(int)),      SLOT(cmd(int)));
      connect(menuSelect, SIGNAL(activated(int)),    SLOT(cmd(int)));
      connect(menuFunctions, SIGNAL(activated(int)), SLOT(cmd(int)));

      //---------ToolBar----------------------------------
      tools = new Q3ToolBar(this, "pianoroll-tools");
      tools->setLabel(tr("Pianoroll Tools"));
      undoRedo->addTo(tools);
      tools->addSeparator();

      srec  = new QToolButton(tools, "srec");
      QToolTip::add(srec, tr("Step Record"));
      srec->setPixmap(*steprecIcon);
      srec->setToggleButton(true);

      midiin  = new QToolButton(tools, "midiin");
      QToolTip::add(midiin, tr("Midi Input"));
      midiin->setPixmap(*midiinIcon);
      midiin->setToggleButton(true);

      speaker  = new QToolButton(tools, "speaker");
      QToolTip::add(speaker, tr("Play Events"));
      speaker->setPixmap(*speakerIcon);
      speaker->setToggleButton(true);

      tools2 = new EditToolBar(this, pianorollTools);

      Q3ToolBar* panicToolbar = new Q3ToolBar(this);
      panicAction->addTo(panicToolbar);

      //-------------------------------------------------------------
      //    Transport Bar
      Q3ToolBar* transport = new Q3ToolBar(this);
      transportAction->addTo(transport);

      toolbar = new Toolbar1(this, _rasterInit, _quantInit);
      info    = new NoteInfo(this);

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      splitter = new Splitter(Qt::Vertical, mainw, "splitter");
      QPushButton* ctrl = new QPushButton(tr("ctrl"), mainw, "Ctrl");
      ctrl->setFont(config.fonts[3]);
      QToolTip::add(ctrl, tr("Add Controller View"));
      hscroll = new ScrollScale(-25, -2, xscale, 20000, Qt::Horizontal, mainw);
      ctrl->setFixedSize(pianoWidth, hscroll->sizeHint().height());

      QSizeGrip* corner = new QSizeGrip(mainw);

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColStretch(1, 100);
      mainGrid->addMultiCellWidget(splitter, 0, 0, 0, 2);
      mainGrid->addWidget(ctrl,    1, 0);
      mainGrid->addWidget(hscroll, 1, 1);
      mainGrid->addWidget(corner,  1, 2, Qt::AlignBottom|Qt::AlignRight);
      mainGrid->addRowSpacing(1, hscroll->sizeHint().height());

      QWidget* split1     = new QWidget(splitter, "split1");
      Q3GridLayout* gridS1 = new Q3GridLayout(split1);
      time                = new MTScale(&_raster, split1, xscale);
      Piano* piano        = new Piano(split1, yscale);
      canvas              = new PianoCanvas(this, split1, xscale, yscale);
      vscroll             = new ScrollScale(-3, 7, yscale, KH * 75, Qt::Vertical, split1);

      int offset = -(config.division/4);
      canvas->setOrigin(offset, 0);
      canvas->setCanvasTools(pianorollTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      time->setOrigin(offset, 0);

      gridS1->setRowStretch(2, 100);
      gridS1->setColStretch(1, 100);

      gridS1->addMultiCellWidget(time,          0, 0, 1, 2);
      gridS1->addMultiCellWidget(hLine(split1), 1, 1, 0, 2);
      gridS1->addWidget(piano,                  2,    0);
      gridS1->addWidget(canvas,                 2,    1);
      
      gridS1->addWidget(vscroll,                2,    2);
//      gridS1->addWidget(time,                   0,    1);
//      gridS1->addWidget(hLine(split1),          1,    1);
//      gridS1->addWidget(piano,                  2,    0);
//      gridS1->addWidget(canvas,                 2,    1);
//      gridS1->addMultiCellWidget(vscroll,        1,  2, 2, 2);

      piano->setFixedWidth(pianoWidth);

      connect(tools2, SIGNAL(toolChanged(int)), canvas,   SLOT(setTool(int)));

      connect(ctrl, SIGNAL(clicked()), SLOT(addCtrl()));
      connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)), SLOT(noteinfoChanged(NoteInfo::ValType, int)));
      connect(vscroll, SIGNAL(scrollChanged(int)), piano,  SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setYMag(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  piano,  SLOT(setYMag(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), canvas,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), time,     SLOT(setXPos(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas,   SLOT(setXMag(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,     SLOT(setXMag(int)));

      connect(canvas, SIGNAL(pitchChanged(int)), piano, SLOT(setPitch(int)));   
      connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(canvas, SIGNAL(selectionChanged(int, Event&, Part*)), this,
         SLOT(setSelection(int, Event&, Part*)));

      connect(piano, SIGNAL(keyPressed(int, int, bool)), canvas, SLOT(pianoPressed(int, int, bool)));
      connect(piano, SIGNAL(keyReleased(int, bool)), canvas, SLOT(pianoReleased(int, bool)));
      connect(srec, SIGNAL(toggled(bool)), SLOT(setSteprec(bool)));
      connect(midiin, SIGNAL(toggled(bool)), canvas, SLOT(setMidiin(bool)));
      connect(speaker, SIGNAL(toggled(bool)), SLOT(setSpeaker(bool)));
      connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      piano->setYPos(KH * 30);
      canvas->setYPos(KH * 30);
      vscroll->setPos(KH * 30);
      //setSelection(0, 0, 0); //Really necessary? Causes segfault when only 1 item selected, replaced by the following:
      info->setEnabled(false);

      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));

      setCaption(canvas->getCaption());
      
      updateHScrollRange();
      // connect to toolbar
      connect(canvas,   SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));  
      connect(canvas,   SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(piano,    SIGNAL(pitchChanged(int)), toolbar, SLOT(setPitch(int)));
      connect(time,     SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(toolbar,  SIGNAL(quantChanged(int)), SLOT(setQuant(int)));
      connect(toolbar,  SIGNAL(rasterChanged(int)),SLOT(setRaster(int)));
      connect(toolbar,  SIGNAL(toChanged(int)),    SLOT(setTo(int)));
      connect(toolbar,  SIGNAL(soloChanged(bool)), SLOT(soloChanged(bool)));

      setFocusPolicy(Qt::StrongFocus);
      setEventColorMode(colorMode);

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"
      initShortcuts(); // initialize shortcuts

      const Pos cpos=song->cPos();
      canvas->setPos(0, cpos.tick(), true);
      canvas->selectAtTick(cpos.tick());
      //canvas->selectFirst();
//      
      if(canvas->track())
        toolbar->setSolo(canvas->track()->solo());
        
      unsigned pos;
      if(initPos >= MAXINT)
        pos = song->cpos();
      else
        pos = initPos;
      if(pos > MAXINT)
        pos = MAXINT;
      hscroll->setOffset((int)pos);
      }

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void PianoRoll::songChanged1(int bits)
      {
        
        if (bits & SC_SOLO)
        {
            toolbar->setSolo(canvas->track()->solo());
            return;
        }      
        songChanged(bits);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void PianoRoll::configChanged()
      {
      initShortcuts();
      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

void PianoRoll::updateHScrollRange()
{
      int s, e;
      canvas->range(&s, &e);
      // Show one more measure.
      e += sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += sigmap.ticksMeasure(e) / 4;
      // Compensate for the fixed piano and vscroll widths. 
      e += canvas->rmapxDev(pianoWidth - vscroll->width()); 
      int s1, e1;
      hscroll->range(&s1, &e1);
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);
}

//---------------------------------------------------------
//   follow
//---------------------------------------------------------

void PianoRoll::follow(int pos)
      {
      int s, e;
      canvas->range(&s, &e);

      if (pos < e && pos >= s)
            hscroll->setOffset(pos);
      if (pos < s)
            hscroll->setOffset(s);
      }

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void PianoRoll::setTime(unsigned tick)
      {
      toolbar->setTime(tick);                      
      time->setPos(3, tick, false);               
      }

//---------------------------------------------------------
//   ~Pianoroll
//---------------------------------------------------------

PianoRoll::~PianoRoll()
      {
      // undoRedo->removeFrom(tools);  // p4.0.6 Removed
      }

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PianoRoll::cmd(int cmd)
      {
      ((PianoCanvas*)canvas)->cmd(cmd, _quantStrength, _quantLimit, _quantLen, _to);
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void PianoRoll::setSelection(int tick, Event& e, Part* p)
      {
      int selections = canvas->selectionSize();

      selEvent = e;
      selPart  = (MidiPart*)p;
      selTick  = tick;

      if (selections > 1) {
            info->setEnabled(true);
            info->setDeltaMode(true);
            if (!deltaMode) {
                  deltaMode = true;
                  info->setValues(0, 0, 0, 0, 0);
                  tickOffset    = 0;
                  lenOffset     = 0;
                  pitchOffset   = 0;
                  veloOnOffset  = 0;
                  veloOffOffset = 0;
                  }
            }
      else if (selections == 1) {
            deltaMode = false;
            info->setEnabled(true);
            info->setDeltaMode(false);
            info->setValues(tick,
               selEvent.lenTick(),
               selEvent.pitch(),
               selEvent.velo(),
               selEvent.veloOff());
            }
      else {
            deltaMode = false;
            info->setEnabled(false);
            }
      selectionChanged();
      }

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void PianoRoll::noteinfoChanged(NoteInfo::ValType type, int val)
      {
      int selections = canvas->selectionSize();

      if (selections == 0) {
            printf("noteinfoChanged while nothing selected\n");
            }
      else if (selections == 1) {
            Event event = selEvent.clone();
            switch(type) {
                  case NoteInfo::VAL_TIME:
                        event.setTick(val - selPart->tick());
                        break;
                  case NoteInfo::VAL_LEN:
                        event.setLenTick(val);
                        break;
                  case NoteInfo::VAL_VELON:
                        event.setVelo(val);
                        break;
                  case NoteInfo::VAL_VELOFF:
                        event.setVeloOff(val);
                        break;
                  case NoteInfo::VAL_PITCH:
                        event.setPitch(val);
                        break;
                  }
            // Indicate do undo, and do not do port controller values and clone parts. 
            //audio->msgChangeEvent(selEvent, event, selPart);
            audio->msgChangeEvent(selEvent, event, selPart, true, false, false);
            }
      else {
            // multiple events are selected; treat noteinfo values
            // as offsets to event values

            int delta = 0;
            switch (type) {
                  case NoteInfo::VAL_TIME:
                        delta = val - tickOffset;
                        tickOffset = val;
                        break;
                  case NoteInfo::VAL_LEN:
                        delta = val - lenOffset;
                        lenOffset = val;
                        break;
                  case NoteInfo::VAL_VELON:
                        delta = val - veloOnOffset;
                        veloOnOffset = val;
                        break;
                  case NoteInfo::VAL_VELOFF:
                        delta = val - veloOffOffset;
                        veloOffOffset = val;
                        break;
                  case NoteInfo::VAL_PITCH:
                        delta = val - pitchOffset;
                        pitchOffset = val;
                        break;
                  }
            if (delta)
                  canvas->modifySelected(type, delta);
            }
      }

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* PianoRoll::addCtrl()
      {
      CtrlEdit* ctrlEdit = new CtrlEdit(splitter, this, xscale, false, "pianoCtrlEdit");
      connect(tools2,   SIGNAL(toolChanged(int)),   ctrlEdit, SLOT(setTool(int)));
      connect(hscroll,  SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
      connect(hscroll,  SIGNAL(scaleChanged(int)),  ctrlEdit, SLOT(setXMag(int)));
      connect(ctrlEdit, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
      connect(ctrlEdit, SIGNAL(yposChanged(int)), toolbar, SLOT(setInt(int)));

      ctrlEdit->setTool(tools2->curTool());
      ctrlEdit->setXPos(hscroll->pos());
      ctrlEdit->setXMag(hscroll->getScaleValue());

      ctrlEdit->show();
      ctrlEditList.push_back(ctrlEdit);
      return ctrlEdit;
      }

//---------------------------------------------------------
//   removeCtrl
//---------------------------------------------------------

void PianoRoll::removeCtrl(CtrlEdit* ctrl)
      {
      for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            if (*i == ctrl) {
                  ctrlEditList.erase(i);
                  break;
                  }
            }
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void PianoRoll::closeEvent(QCloseEvent* e)
      {
      emit deleted((unsigned long)this);
      e->accept();
      }

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void PianoRoll::readConfiguration(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "quant")
                              _quantInit = xml.parseInt();
                        else if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "quantStrength")
                              _quantStrengthInit = xml.parseInt();
                        else if (tag == "quantLimit")
                              _quantLimitInit = xml.parseInt();
                        else if (tag == "quantLen")
                              _quantLenInit = xml.parseInt();
                        else if (tag == "to")
                              _toInit = xml.parseInt();
                        else if (tag == "colormode")
                              colorModeInit = xml.parseInt();
                        else if (tag == "width")
                              _widthInit = xml.parseInt();
                        else if (tag == "height")
                              _heightInit = xml.parseInt();
                        else
                              xml.unknown("PianoRoll");
                        break;
                  case Xml::TagEnd:
                        if (tag == "pianoroll")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void PianoRoll::writeConfiguration(int level, Xml& xml)
      {
      xml.tag(level++, "pianoroll");
      xml.intTag(level, "quant", _quantInit);
      xml.intTag(level, "raster", _rasterInit);
      xml.intTag(level, "quantStrength", _quantStrengthInit);
      xml.intTag(level, "quantLimit", _quantLimitInit);
      xml.intTag(level, "quantLen", _quantLenInit);
      xml.intTag(level, "to", _toInit);
      xml.intTag(level, "width", _widthInit);
      xml.intTag(level, "height", _heightInit);
      xml.intTag(level, "colormode", colorModeInit);
      xml.etag(level, "pianoroll");
      }

//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void PianoRoll::soloChanged(bool flag)
      {
      audio->msgSetSolo(canvas->track(), flag);
      song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void PianoRoll::setRaster(int val)
      {
      _rasterInit = val;
      MidiEditor::setRaster(val);
      canvas->redrawGrid();
      canvas->setFocus();     // give back focus after kb input
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void PianoRoll::setQuant(int val)
      {
      _quantInit = val;
      MidiEditor::setQuant(val);
      canvas->setFocus();
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void PianoRoll::writeStatus(int level, Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "pianoroll");
      MidiEditor::writeStatus(level, xml);
      splitter->writeStatus(level, xml);

      for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            (*i)->writeStatus(level, xml);
            }

      xml.intTag(level, "steprec", canvas->steprec());
      xml.intTag(level, "midiin", canvas->midiin());
      xml.intTag(level, "tool", int(canvas->tool()));
      xml.intTag(level, "quantStrength", _quantStrength);
      xml.intTag(level, "quantLimit", _quantLimit);
      xml.intTag(level, "quantLen", _quantLen);
      xml.intTag(level, "playEvents", _playEvents);
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ypos", vscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      xml.tag(level, "/pianoroll");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void PianoRoll::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "steprec") {
                              int val = xml.parseInt();
                              canvas->setSteprec(val);
                              srec->setOn(val);
                              }
                        else if (tag == "midiin") {
                              int val = xml.parseInt();
                              canvas->setMidiin(val);
                              midiin->setOn(val);
                              }
                        else if (tag == "tool") {
                              int tool = xml.parseInt();
                              canvas->setTool(tool);
                              tools2->set(tool);
                              }
                        else if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "ctrledit") {
                              CtrlEdit* ctrl = addCtrl();
                              ctrl->readStatus(xml);
                              }
                        else if (tag == splitter->name())
                              splitter->readStatus(xml);
                        else if (tag == "quantStrength")
                              _quantStrength = xml.parseInt();
                        else if (tag == "quantLimit")
                              _quantLimit = xml.parseInt();
                        else if (tag == "quantLen")
                              _quantLen = xml.parseInt();
                        else if (tag == "playEvents") {
                              _playEvents = xml.parseInt();
                              canvas->playEvents(_playEvents);
                              speaker->setOn(_playEvents);
                              }
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else if (tag == "ymag")
                              vscroll->setMag(xml.parseInt());
                        else if (tag == "ypos")
                              vscroll->setPos(xml.parseInt());
                        else
                              xml.unknown("PianoRoll");
                        break;
                  case Xml::TagEnd:
                        if (tag == "pianoroll") {
                              _quantInit  = _quant;
                              _rasterInit = _raster;
                              toolbar->setRaster(_raster);
                              toolbar->setQuant(_quant);
                              canvas->redrawGrid();
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

static int rasterTable[] = {
      //-9----8-  7    6     5     4    3(1/4)     2   1
      4,  8, 16, 32,  64, 128, 256,  512, 1024,  // triple
      6, 12, 24, 48,  96, 192, 384,  768, 1536,
      9, 18, 36, 72, 144, 288, 576, 1152, 2304   // dot
      };

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void PianoRoll::keyPressEvent(QKeyEvent* event)
      {
      if (info->hasFocus()) {
            event->ignore();
            return;
            }

      int index;
      int n = sizeof(rasterTable)/sizeof(*rasterTable);
      for (index = 0; index < n; ++index)
            if (rasterTable[index] == raster())
                  break;
      if (index == n) {
            index = 0;
            // raster 1 is not in table
            }
      int off = (index / 9) * 9;
      index   = index % 9;

      int val = 0;

      PianoCanvas* pc = (PianoCanvas*)canvas;
      int key = event->key();

      if (event->state() & Qt::ShiftButton)
            key += Qt::SHIFT;
      if (event->state() & Qt::AltButton)
            key += Qt::ALT;
      if (event->state() & Qt::ControlButton)
            key+= Qt::CTRL;

      if (key == Qt::Key_Escape) {
            close();
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_POINTER].key) {
            tools2->set(PointerTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PENCIL].key) {
            tools2->set(PencilTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_RUBBER].key) {
            tools2->set(RubberTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key) {
            tools2->set(DrawTool);
            return;
            }
      else if (key == shortcuts[SHRT_POS_INC].key) {
            pc->pianoCmd(CMD_RIGHT);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            pc->pianoCmd(CMD_LEFT);
            return;
            }
      else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key) {
            pc->pianoCmd(CMD_INSERT);
            return;
            }
      else if (key == Qt::Key_Delete) {
            pc->pianoCmd(CMD_DELETE);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_IN].key) {
            int mag = hscroll->mag();
            int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
            if (zoomlvl < 23)
                  zoomlvl++;

            int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
            hscroll->setMag(newmag);
            //printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
            int mag = hscroll->mag();
            int zoomlvl = ScrollScale::getQuickZoomLevel(mag);
            if (zoomlvl > 1)
                  zoomlvl--;

            int newmag = ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
            hscroll->setMag(newmag);
            //printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
            return;
            }
      else if (key == shortcuts[SHRT_GOTO_CPOS].key) {
            PartList* p = this->parts();
            Part* first = p->begin()->second;
            hscroll->setPos(song->cpos() - first->tick() );
            return;
            }
      else if (key == shortcuts[SHRT_SCROLL_LEFT].key) {
            int pos = hscroll->pos() - config.division;
            if (pos < 0)
                  pos = 0;
            hscroll->setPos(pos);
            return;
            }
      else if (key == shortcuts[SHRT_SCROLL_RIGHT].key) {
            int pos = hscroll->pos() + config.division;
            hscroll->setPos(pos);
            return;
            }
      else if (key == shortcuts[SHRT_SET_QUANT_1].key)
            val = rasterTable[8 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_2].key)
            val = rasterTable[7 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_3].key)
            val = rasterTable[6 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_4].key)
            val = rasterTable[5 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_5].key)
            val = rasterTable[4 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_6].key)
            val = rasterTable[3 + off];
      else if (key == shortcuts[SHRT_SET_QUANT_7].key)
            val = rasterTable[2 + off];
      else if (key == shortcuts[SHRT_TOGGLE_TRIOL].key)
            val = rasterTable[index + ((off == 0) ? 9 : 0)];
      else if (key == shortcuts[SHRT_EVENT_COLOR].key) {
            if (colorMode == 0)
                  colorMode = 1;
            else if (colorMode == 1)
                  colorMode = 2;
            else
                  colorMode = 0;
            setEventColorMode(colorMode);
            return;
            }
      else if (key == shortcuts[SHRT_TOGGLE_PUNCT].key)
            val = rasterTable[index + ((off == 18) ? 9 : 18)];

      else if (key == shortcuts[SHRT_TOGGLE_PUNCT2].key) {//CDW
            if ((off == 18) && (index > 2)) {
                  val = rasterTable[index + 9 - 1];
                  }
            else if ((off == 9) && (index < 8)) {
                  val = rasterTable[index + 18 + 1];
                  }
            else
                  return;
            }
      else { //Default:
            event->ignore();
            return;
            }
      setQuant(val);
      setRaster(val);
      toolbar->setQuant(_quant);
      toolbar->setRaster(_raster);
      }

//---------------------------------------------------------
//   configQuant
//---------------------------------------------------------

void PianoRoll::configQuant()
      {
      if (!quantConfig) {
            quantConfig = new QuantConfig(_quantStrength, _quantLimit, _quantLen);
            connect(quantConfig, SIGNAL(setQuantStrength(int)), SLOT(setQuantStrength(int)));
            connect(quantConfig, SIGNAL(setQuantLimit(int)), SLOT(setQuantLimit(int)));
            connect(quantConfig, SIGNAL(setQuantLen(bool)), SLOT(setQuantLen(bool)));
            }
      quantConfig->show();
      }

//---------------------------------------------------------
//   setSteprec
//---------------------------------------------------------

void PianoRoll::setSteprec(bool flag)
      {
      canvas->setSteprec(flag);
      if (flag == false)
            midiin->setOn(flag);
      }

//---------------------------------------------------------
//   setEventColorMode
//---------------------------------------------------------

void PianoRoll::setEventColorMode(int mode)
      {
      colorMode = mode;
      colorModeInit = colorMode;
      eventColor->setItemChecked(0, mode == 0);
      eventColor->setItemChecked(1, mode == 1);
      eventColor->setItemChecked(2, mode == 2);
      ((PianoCanvas*)(canvas))->setColorMode(colorMode);
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void PianoRoll::clipboardChanged()
      {
      QMimeSource* ms = QApplication::clipboard()->data(QClipboard::Clipboard);
      bool flag = false;
      if (ms) {
            for (int i = 0;; ++i) {
                  if (ms->format(i) == 0)
                        break;
//                  printf("clipboard changed %s\n", ms->format(i));
                  flag = strcmp(ms->format(i), "text/eventlist;charset=UTF-8") == 0;
                  if (flag)
                        break;
                  }
            }
      menuEdit->setItemEnabled(PianoCanvas::CMD_PASTE, flag);
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void PianoRoll::selectionChanged()
      {
      bool flag = canvas->selectionSize() > 0;
      menuEdit->setItemEnabled(PianoCanvas::CMD_CUT, flag);
      menuEdit->setItemEnabled(PianoCanvas::CMD_COPY, flag);
      menuEdit->setItemEnabled(PianoCanvas::CMD_DEL, flag);
      }

//---------------------------------------------------------
//   setSpeaker
//---------------------------------------------------------

void PianoRoll::setSpeaker(bool val)
      {
      _playEvents = val;
      canvas->playEvents(_playEvents);
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void PianoRoll::resizeEvent(QResizeEvent* ev)
      {
      QWidget::resizeEvent(ev);
      _widthInit = ev->size().width();
      _heightInit = ev->size().height();
      }


//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void PianoRoll::initShortcuts()
      {
      menuSelect->setAccel(shortcuts[SHRT_SELECT_ALL].key, PianoCanvas::CMD_SELECT_ALL);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_NONE].key, PianoCanvas::CMD_SELECT_NONE);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_INVERT].key, PianoCanvas::CMD_SELECT_INVERT);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_ILOOP].key, PianoCanvas::CMD_SELECT_ILOOP);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_OLOOP].key, PianoCanvas::CMD_SELECT_OLOOP);
      
      menuSelect->setAccel(shortcuts[SHRT_SELECT_PREV_PART].key, PianoCanvas::CMD_SELECT_PREV_PART);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_NEXT_PART].key, PianoCanvas::CMD_SELECT_NEXT_PART);

      menuConfig->setAccel(shortcuts[SHRT_EVENT_COLOR].key, menu_ids[CMD_EVENT_COLOR]);

      menuFunctions->setAccel(shortcuts[SHRT_OVER_QUANTIZE].key,    PianoCanvas::CMD_OVER_QUANTIZE);
      menuFunctions->setAccel(shortcuts[SHRT_ON_QUANTIZE].key,      PianoCanvas::CMD_ON_QUANTIZE);
      menuFunctions->setAccel(shortcuts[SHRT_ONOFF_QUANTIZE].key,   PianoCanvas::CMD_ONOFF_QUANTIZE);
      menuFunctions->setAccel(shortcuts[SHRT_ITERATIVE_QUANTIZE].key,    PianoCanvas::CMD_ITERATIVE_QUANTIZE);
      menuFunctions->setAccel(shortcuts[SHRT_MODIFY_GATE_TIME].key, PianoCanvas::CMD_MODIFY_GATE_TIME);
      menuFunctions->setAccel(shortcuts[SHRT_MODIFY_VELOCITY].key,  PianoCanvas::CMD_MODIFY_VELOCITY);
      menuFunctions->setAccel(shortcuts[SHRT_CRESCENDO].key,        PianoCanvas::CMD_CRESCENDO);
      menuFunctions->setAccel(shortcuts[SHRT_TRANSPOSE].key,        PianoCanvas::CMD_TRANSPOSE);
      menuFunctions->setAccel(shortcuts[SHRT_THIN_OUT].key,         PianoCanvas::CMD_THIN_OUT);
      menuFunctions->setAccel(shortcuts[SHRT_ERASE_EVENT].key,      PianoCanvas::CMD_ERASE_EVENT);
      menuFunctions->setAccel(shortcuts[SHRT_NOTE_SHIFT].key,       PianoCanvas::CMD_NOTE_SHIFT);
      menuFunctions->setAccel(shortcuts[SHRT_MOVE_CLOCK].key,       PianoCanvas::CMD_MOVE_CLOCK);
      menuFunctions->setAccel(shortcuts[SHRT_COPY_MEASURE].key,     PianoCanvas::CMD_COPY_MEASURE);
      menuFunctions->setAccel(shortcuts[SHRT_ERASE_MEASURE].key,    PianoCanvas::CMD_ERASE_MEASURE);
      menuFunctions->setAccel(shortcuts[SHRT_DELETE_MEASURE].key,   PianoCanvas::CMD_DELETE_MEASURE);
      menuFunctions->setAccel(shortcuts[SHRT_CREATE_MEASURE].key,   PianoCanvas::CMD_CREATE_MEASURE);
      menuFunctions->setAccel(shortcuts[SHRT_CONFIG_QUANT].key, menu_ids[CMD_CONFIG_QUANT]);
      menuFunctions->setAccel(shortcuts[SHRT_FIXED_LEN].key,        PianoCanvas::CMD_FIXED_LEN);
      menuFunctions->setAccel(shortcuts[SHRT_DELETE_OVERLAPS].key,  PianoCanvas::CMD_DELETE_OVERLAPS);
      }

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------
void PianoRoll::execDeliveredScript(int id)
{
      //QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
      QString scriptfile = song->getScriptPath(id, true);
      song->executeScript(scriptfile.latin1(), parts(), quant(), true); 
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void PianoRoll::execUserScript(int id)
{
      QString scriptfile = song->getScriptPath(id, false);
      song->executeScript(scriptfile.latin1(), parts(), quant(), true);
}

