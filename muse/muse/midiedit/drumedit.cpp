//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.cpp,v 1.22.2.21 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <qtoolbutton.h>
#include <qaccel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qsizegrip.h>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlistbox.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qmessagebox.h>
#include <qaction.h>

#include "drumedit.h"
#include "mtscale.h"
#include "scrollscale.h"
#include "xml.h"
#include "dlist.h"
#include "dcanvas.h"
#include "ttoolbar.h"
#include "tb1.h"
#include "splitter.h"
#include "utils.h"
#include "../ctrl/ctrledit.h"
#include "vscale.h"
#include "swidget.h"
#include "globals.h"
#include "icons.h"
#include "filedialog.h"
#include "drummap.h"
#include "audio.h"
#include "gconfig.h"

/*
static const char* map_file_pattern[] = {
      "Presets (*.map *.map.gz *.map.bz2)",
      "All Files (*)",
      0
      };
static const char* map_file_save_pattern[] = {
      "Presets (*.map)",
      "gzip compressed presets (*.map.gz)",
      "bzip2 compressed presets (*.map.bz2)",
      "All Files (*)",
      0
      };
*/      

int DrumEdit::_quantInit = 96;
int DrumEdit::_rasterInit = 96;
int DrumEdit::_widthInit = 600;
int DrumEdit::_heightInit = 400;
int DrumEdit::_dlistWidthInit = 50;
int DrumEdit::_dcanvasWidthInit = 300;
int DrumEdit::_toInit = 0;

static const int xscale = -10;
static const int yscale = 1;
static const int drumeditTools = PointerTool | PencilTool | RubberTool;

//---------------------------------------------------------
//   DWhatsThis::text
//---------------------------------------------------------

QString DWhatsThis::text(const QPoint& pos)
      {
      int section = header->sectionAt(pos.x());
      if (section == -1)
            return QString::null;
      switch(section) {
            case 0: return  QHeader::tr("mute instrument"); break;
            case 1: return  QHeader::tr("sound name"); break;
            case 2: return  QHeader::tr("volume percent"); break;
            case 3: return  QHeader::tr("quantisation"); break;
            case 4: return  QHeader::tr("this input note triggers the sound"); break;
            case 5: return  QHeader::tr("note length"); break;
            case 6: return  QHeader::tr("this is the note which is played"); break;
            case 7: return  QHeader::tr("output channel (hold ctl to affect all rows)"); break;
            case 8: return  QHeader::tr("output port"); break;
            case 9: return  QHeader::tr("shift + control key: draw velocity level 1"); break;
            case 10: return  QHeader::tr("control key: draw velocity level 2"); break;
            case 11: return QHeader::tr("shift key: draw velocity level 3"); break;
            case 12: return QHeader::tr("draw velocity level 4"); break;
            default: break;
            }
      return QString::null;
      }

//---------------------------------------------------------
//   DHeaderTip::maybeTip
//---------------------------------------------------------

void DHeaderTip::maybeTip(const QPoint &pos)
      {
      Header* w  = (Header*)parentWidget();
      int section = w->sectionAt(pos.x());
      if (section == -1)
            return;
      QRect r(w->sectionPos(section), 0, w->sectionSize(section),
         w->height());
      QString p;
      switch(section) {
            case 0:  p = QHeader::tr("mute instrument"); break;
            case 1:  p = QHeader::tr("sound name"); break;
            case 2:  p = QHeader::tr("volume percent"); break;
            case 3:  p = QHeader::tr("quantisation"); break;
            case 4:  p = QHeader::tr("this input note triggers the sound"); break;
            case 5:  p = QHeader::tr("note length"); break;
            case 6:  p = QHeader::tr("this is the note which is played"); break;
            case 7:  p = QHeader::tr("output channel (ctl: affect all rows)"); break;
            case 8:  p = QHeader::tr("output port"); break;
            case 9:  p = QHeader::tr("shift + control key: draw velocity level 1"); break;
            case 10:  p = QHeader::tr("control key: draw velocity level 2"); break;
            case 11: p = QHeader::tr("shift key: draw velocity level 3"); break;
            case 12: p = QHeader::tr("draw velocity level 4"); break;
            default: return;
            }
      tip(r, p);
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void DrumEdit::closeEvent(QCloseEvent* e)
      {
      //Store values of the horizontal splitter
      QValueList<int> sizes = split2->sizes();
      QValueList<int>::Iterator it = sizes.begin();
      _dlistWidthInit = *it; //There are only 2 values stored in the sizelist, size of dlist widget and dcanvas widget
      it++;
      _dcanvasWidthInit = *it;
      emit deleted((unsigned long)this);
      e->accept();
      }

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

DrumEdit::DrumEdit(PartList* pl, QWidget* parent, const char* name, unsigned initPos)
   : MidiEditor(_quantInit, _rasterInit, pl, parent, name)
      {
      split1w1 = 0;
      resize(_widthInit, _heightInit);
      selPart  = 0;
      _to = _toInit;
      //---------Pulldown Menu----------------------------
      menuFile = new QPopupMenu(this);
      menuBar()->insertItem(tr("&File"), menuFile);

      menuFile->insertItem(*openIcon, tr("Load Map"), DrumCanvas::CMD_LOAD);
      menuFile->insertItem(*saveIcon, tr("Save Map"), DrumCanvas::CMD_SAVE);
      menuFile->insertItem(tr("Reset GM Map"), DrumCanvas::CMD_RESET);

      menuEdit = new QPopupMenu(this);
      menuBar()->insertItem(tr("&Edit"), menuEdit);
      undoRedo->addTo(menuEdit);
      QAccel* qa = new QAccel(this);
      qa->connectItem(qa->insertItem(CTRL+Key_Z), song, SLOT(undo()));
      menuEdit->insertSeparator();
      menuEdit->insertItem(tr("Cut"),   DrumCanvas::CMD_CUT);
      menuEdit->insertItem(tr("Copy"),  DrumCanvas::CMD_COPY);
      menuEdit->insertItem(tr("Paste"), DrumCanvas::CMD_PASTE);
      menuEdit->insertSeparator();
      menuEdit->insertItem(tr("Delete Events"), DrumCanvas::CMD_DEL);

      // Functions
      menuFunctions = new QPopupMenu(this);
      menuBar()->insertItem(tr("&Functions"), menuFunctions);
      menuFunctions->insertTearOffHandle();
      menuFunctions->insertItem(tr("Set fixed length"), DrumCanvas::CMD_FIXED_LEN);
      menuFunctions->insertItem(tr("Modify Velocity"), DrumCanvas::CMD_MODIFY_VELOCITY);

      menuSelect = new QPopupMenu(this);
      menuSelect->insertItem(tr("Select All"),   DrumCanvas::CMD_SELECT_ALL);
      menuSelect->insertItem(tr("Select None"),  DrumCanvas::CMD_SELECT_NONE);
      menuSelect->insertItem(tr("Invert"),       DrumCanvas::CMD_SELECT_INVERT);
      menuSelect->insertSeparator();
      menuSelect->insertItem(tr("Inside Loop"),  DrumCanvas::CMD_SELECT_ILOOP);
      menuSelect->insertItem(tr("Outside Loop"), DrumCanvas::CMD_SELECT_OLOOP);
      
      menuSelect->insertSeparator();

      menuSelect->insertItem(tr("Previous Part"), DrumCanvas::CMD_SELECT_PREV_PART);
      menuSelect->insertItem(tr("Next Part"), DrumCanvas::CMD_SELECT_NEXT_PART);
      
      menuEdit->insertItem(tr("&Select"), menuSelect);

      QPopupMenu* menuScriptPlugins = new QPopupMenu(this);
      song->populateScriptMenu(menuScriptPlugins, this);
      menuBar()->insertItem(tr("&Plugins"), menuScriptPlugins);

      connect(menuFile, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(menuEdit, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(menuSelect, SIGNAL(activated(int)),    SLOT(cmd(int)));
      connect(menuFunctions, SIGNAL(activated(int)), SLOT(cmd(int)));

      //---------------------------------------------------
      //    Toolbars
      //---------------------------------------------------

      tools = new QToolBar(this, "drum-tools");
      new QToolButton(*openIcon, tr("Load Drummap"),
                  QString::null, this, SLOT(load()),
                  tools, "load drummap from file");

      new QToolButton(*saveIcon, tr("Store Drummap"),
                  QString::null,
                  this, SLOT(save()),
                  tools, "store drummap to file");

      QWhatsThis::whatsThisButton(tools);

      tools->addSeparator();
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

      tools2 = new EditToolBar(this, drumeditTools);

      QToolBar* transport = new QToolBar(this);
      transportAction->addTo(transport);

      // dontt show pitch value in toolbar
      //toolbar = new Toolbar1(this, _rasterInit, _quantInit, false);
      toolbar = new Toolbar1(this, _rasterInit, _quantInit);
      info    = new NoteInfo(this);

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      split1            = new Splitter(Vertical, mainw, "split1");
      QPushButton* ctrl = new QPushButton(tr("ctrl"), mainw, "Ctrl");
      ctrl->setFont(config.fonts[3]);
      hscroll           = new ScrollScale(-25, -2, xscale, 20000, Horizontal, mainw);
      ctrl->setFixedSize(40, hscroll->sizeHint().height());
      QToolTip::add(ctrl, tr("Add Controller View"));

      QSizeGrip* corner = new QSizeGrip(mainw);
      corner->setFixedHeight(hscroll->sizeHint().height());

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColStretch(1, 100);

      mainGrid->addMultiCellWidget(split1, 0, 0, 0, 2);
      mainGrid->addWidget(ctrl,    1, 0);
      mainGrid->addWidget(hscroll, 1, 1);
      mainGrid->addWidget(corner,  1, 2, AlignBottom|AlignRight);
//      mainGrid->addRowSpacing(1, hscroll->sizeHint().height());

      split2              = new Splitter(Horizontal, split1, "split2");
      split1w1            = new QWidget(split2, "split1w1");
      QWidget* split1w2   = new QWidget(split2, "split1w2");
      QGridLayout* gridS1 = new QGridLayout(split1w1);
      QGridLayout* gridS2 = new QGridLayout(split1w2);
      time                = new MTScale(&_raster, split1w2, xscale);
      canvas              = new DrumCanvas(this, split1w2, xscale, yscale);
      vscroll             = new ScrollScale(-4, 1, yscale, DRUM_MAPSIZE*TH, Vertical, split1w2);
      int offset = -(config.division/4);
      canvas->setOrigin(offset, 0);
      canvas->setCanvasTools(drumeditTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      time->setOrigin(offset, 0);

      QValueList<int> mops;
      mops.append(_dlistWidthInit);
      mops.append(_dcanvasWidthInit);
      split2->setSizes(mops);
      // By T356. Not much choice but to disable this for now, to stop runaway resize bug.
      // Can't seem to get the splitter to readjust when manually setting sizes.
      //split2->setResizeMode(split1w1, QSplitter::KeepSize);

      gridS2->setRowStretch(1, 100);
      gridS2->setColStretch(0, 100);
      
      gridS2->addMultiCellWidget(time,  0, 0, 0, 1);
      gridS2->addMultiCellWidget(hLine(split1w2), 1, 1, 0, 1);
      gridS2->addWidget(canvas,  2, 0);
      
      gridS2->addWidget(vscroll, 2, 1);
      //
      //  Reihenfolge in dlist.c festgeschrieben ("Dcols")
      //
      header = new Header(split1w1, "header");
      header->setFixedHeight(30);
      header->addLabel(tr("M"), 20);
      header->addLabel(tr("Sound"), 100);
      header->addLabel(tr("Vol"));
      header->addLabel(tr("QNT"));
      header->addLabel(tr("E-Note"));
      header->addLabel(tr("Len"));
      header->addLabel(tr("A-Note"));
      header->addLabel(tr("Ch"));
      header->addLabel(tr("Port"), 60);
      header->addLabel(tr("LV1"));
      header->addLabel(tr("LV2"));
      header->addLabel(tr("LV3"));
      header->addLabel(tr("LV4"));
      new DHeaderTip(header);
      new DWhatsThis(header, header);

      dlist = new DList(header, split1w1, yscale);
      // p3.3.44
      setCurDrumInstrument(dlist->getSelectedInstrument());
      
      connect(dlist, SIGNAL(keyPressed(int, bool)), canvas, SLOT(keyPressed(int, bool)));
      connect(dlist, SIGNAL(keyReleased(int, bool)), canvas, SLOT(keyReleased(int, bool)));
      connect(dlist, SIGNAL(mapChanged(int, int)), canvas, SLOT(mapChanged(int, int)));

      gridS1->setRowStretch(1, 100);
      gridS1->setColStretch(0, 100);
      gridS1->addWidget(header, 0, 0);
      gridS1->addWidget(dlist, 1, 0);
      
      connect(canvas, SIGNAL(newWidth(int)), SLOT(newCanvasWidth(int)));
      connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));
      connect(song, SIGNAL(songChanged(int)),      dlist, SLOT(songChanged(int)));
      connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setYMag(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  dlist, SLOT(setYMag(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setXMag(int)));
      connect(srec, SIGNAL(toggled(bool)),         canvas, SLOT(setSteprec(bool)));
      connect(midiin, SIGNAL(toggled(bool)),       canvas, SLOT(setMidiin(bool)));

      connect(vscroll, SIGNAL(scrollChanged(int)),   dlist,   SLOT(setYPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)),   time,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), time,   SLOT(setXMag(int)));

      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));

      connect(canvas, SIGNAL(selectionChanged(int, Event&, Part*)), this,
         SLOT(setSelection(int, Event&, Part*)));
      connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      setCaption(canvas->getCaption());
      
      updateHScrollRange();
      
      // connect toolbar
      connect(canvas,  SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(time,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(toolbar, SIGNAL(quantChanged(int)),          SLOT(setQuant(int)));
      connect(toolbar, SIGNAL(rasterChanged(int)),         SLOT(setRaster(int)));
      connect(toolbar, SIGNAL(soloChanged(bool)),          SLOT(soloChanged(bool)));
      connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)), SLOT(noteinfoChanged(NoteInfo::ValType, int)));

      connect(ctrl, SIGNAL(clicked()), SLOT(addCtrl()));

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"
      initShortcuts();

      const Pos cpos=song->cPos();
      canvas->setPos(0, cpos.tick(), true);
      canvas->selectAtTick(cpos.tick());
      //canvas->selectFirst();

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

void DrumEdit::songChanged1(int bits)
      {
        if (bits & SC_SOLO)
        {
            toolbar->setSolo(canvas->track()->solo());
            return;
        }      
        songChanged(bits);

      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------

void DrumEdit::updateHScrollRange()
{
      int s, e;
      canvas->range(&s, &e);
      // Show one more measure.
      e += sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += sigmap.ticksMeasure(e) / 4;
      // Compensate for drum list, splitter handle, and vscroll widths. 
      e += canvas->rmapxDev(dlist->width() + split2->handleWidth() - vscroll->width()); 
      int s1, e1;
      hscroll->range(&s1, &e1);
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);
}

//---------------------------------------------------------
//   follow
//---------------------------------------------------------

void DrumEdit::follow(int pos)
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

void DrumEdit::setTime(unsigned tick)
      {
      toolbar->setTime(tick);
      time->setPos(3, tick, false);
      }

//---------------------------------------------------------
//   ~DrumEdit
//---------------------------------------------------------

DrumEdit::~DrumEdit()
      {
      undoRedo->removeFrom(tools);
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void DrumEdit::setSelection(int tick, Event& e, Part* p)
      {
      selEvent = e;
      selPart  = (MidiPart*)p;
      selTick  = tick;
      info->setEnabled(!e.empty());
      if (!e.empty()) {
            info->setValues(tick,
               selEvent.lenTick(),
               selEvent.pitch(),
               selEvent.velo(),
               selEvent.veloOff());
            }
      selectionChanged();
      }

//---------------------------------------------------------
//   soloChanged
//---------------------------------------------------------

void DrumEdit::soloChanged(bool flag)
      {
      audio->msgSetSolo(canvas->track(), flag);
      song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void DrumEdit::setRaster(int val)
      {
      _rasterInit = val;
      MidiEditor::setRaster(val);
      canvas->redrawGrid();
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void DrumEdit::setQuant(int val)
      {
      _quantInit = val;
      MidiEditor::setQuant(val);
      }

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void DrumEdit::noteinfoChanged(NoteInfo::ValType type, int val)
      {
      if (selEvent.empty()) {
            printf("noteinfoChanged while note is zero %d\n", type);
            return;
            }
      Event event = selEvent.clone();
      switch (type) {
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

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void DrumEdit::writeStatus(int level, Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "drumedit");
      MidiEditor::writeStatus(level, xml);

      for (std::list<CtrlEdit*>::const_iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            (*i)->writeStatus(level, xml);
            }

      split1->writeStatus(level, xml);
      split2->writeStatus(level, xml);

      header->writeStatus(level, xml);
      xml.intTag(level, "steprec", canvas->steprec());
      xml.intTag(level, "midiin",  canvas->midiin());
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ypos", vscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      xml.tag(level, "/drumedit");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void DrumEdit::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
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
                        else if (tag == "ctrledit") {
                              CtrlEdit* ctrl = addCtrl();
                              ctrl->readStatus(xml);
                              }
                        else if (tag == split1->name())
                              split1->readStatus(xml);
                        else if (tag == split2->name())
                              split2->readStatus(xml);
                        else if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == header->name())
                              header->readStatus(xml);
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else if (tag == "ymag")
                              vscroll->setMag(xml.parseInt());
                        else if (tag == "ypos")
                              vscroll->setPos(xml.parseInt());
                        else
                              xml.unknown("DrumEdit");
                        break;
                  case Xml::TagEnd:
                        if (tag == "drumedit") {
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

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void DrumEdit::readConfiguration(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (tag == "quant")
                              _quantInit = xml.parseInt();
                        else if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "width")
                              _widthInit = xml.parseInt();
                        else if (tag == "height")
                              _heightInit = xml.parseInt();
                        else if (tag == "dcanvaswidth")
                              _dcanvasWidthInit = xml.parseInt();
                        else if (tag == "dlistwidth")
                              _dlistWidthInit = xml.parseInt();
                        else if (tag == "to") {
                              _toInit = xml.parseInt();
                              }
                        else
                              xml.unknown("DrumEdit");
                        break;
                  case Xml::TagEnd:
                        if (tag == "drumedit") {
                              return;
                              }
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void DrumEdit::writeConfiguration(int level, Xml& xml)
      {
      xml.tag(level++, "drumedit");
      xml.intTag(level, "quant", _quantInit);
      xml.intTag(level, "raster", _rasterInit);
      xml.intTag(level, "width", _widthInit);
      xml.intTag(level, "height", _heightInit);
      xml.intTag(level, "dlistwidth", _dlistWidthInit);
      xml.intTag(level, "dcanvaswidth", _dcanvasWidthInit);
      xml.intTag(level, "to", _toInit);
      xml.tag(level, "/drumedit");
      }

//---------------------------------------------------------
//   load
//---------------------------------------------------------

void DrumEdit::load()
      {
      //QString fn = getOpenFileName("drummaps", map_file_pattern,
      QString fn = getOpenFileName("drummaps", drum_map_file_pattern,
         this, tr("Muse: Load Drum Map"), 0);
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".map"), "r", popenFlag, true);
      if (f == 0)
            return;

      Xml xml(f);
      int mode = 0;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::TagStart:
                        if (mode == 0 && tag == "muse")
                              mode = 1;
                        else if (mode == 1 && tag == "drummap") {
                              readDrumMap(xml, true);
                              mode = 0;
                              }
                        else
                              xml.unknown("DrumEdit");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (!mode && tag == "muse")
                              goto ende;
                  default:
                        break;
                  }
            }
ende:
      if (popenFlag)
            pclose(f);
      else
            fclose(f);
      dlist->redraw();
      canvas->redraw();
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void DrumEdit::save()
      {
      //QString fn = getSaveFileName(QString("drummaps"), map_file_pattern,
      QString fn = getSaveFileName(QString("drummaps"), drum_map_file_save_pattern,
        this, tr("MusE: Store Drum Map"));
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = fileOpen(this, fn, QString(".map"), "w", popenFlag, false, true);
      if (f == 0)
            return;
      Xml xml(f);
      xml.header();
      xml.tag(0, "muse version=\"1.0\"");
      writeDrumMap(1, xml, true);
      xml.tag(1, "/muse");

      if (popenFlag)
            pclose(f);
      else
            fclose(f);
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void DrumEdit::reset()
{
  if(QMessageBox::warning(this, tr("Drum map"),
      tr("Reset the drum map with GM defaults?"),
      QMessageBox::Ok | QMessageBox::Default, QMessageBox::Cancel | QMessageBox::Escape, QMessageBox::NoButton) == QMessageBox::Ok)
  {    
    resetGMDrumMap();
    dlist->redraw();
    canvas->redraw();
  }  
}

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void DrumEdit::cmd(int cmd)
      {
      switch(cmd) {
            case DrumCanvas::CMD_LOAD:
                  load();
                  break;
            case DrumCanvas::CMD_SAVE:
                  save();
                  break;
            case DrumCanvas::CMD_RESET:
                  reset();
                  break;
            default:
                  ((DrumCanvas*)(canvas))->cmd(cmd);
                  break;
            }
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void DrumEdit::clipboardChanged()
      {
      QMimeSource* ms = QApplication::clipboard()->data(QClipboard::Clipboard);
      if (ms && ms->format(0)) {
            bool flag = strcmp(ms->format(0), "text/eventlist;charset=UTF-8") == 0;
            menuEdit->setItemEnabled(DrumCanvas::CMD_PASTE, flag);
            }
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void DrumEdit::selectionChanged()
      {
      bool flag = canvas->selectionSize() > 0;
      menuEdit->setItemEnabled(DrumCanvas::CMD_CUT, flag);
      menuEdit->setItemEnabled(DrumCanvas::CMD_COPY, flag);
      menuEdit->setItemEnabled(DrumCanvas::CMD_DEL, flag);
      }

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* DrumEdit::addCtrl()
      {
      CtrlEdit* ctrlEdit = new CtrlEdit(split1, this, xscale, true, "drumCtrlEdit");
      connect(hscroll,  SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
      connect(hscroll,  SIGNAL(scaleChanged(int)),  ctrlEdit, SLOT(setXMag(int)));
      connect(ctrlEdit, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
      connect(ctrlEdit, SIGNAL(yposChanged(int)), toolbar, SLOT(setInt(int)));
      connect(tools2,   SIGNAL(toolChanged(int)),   ctrlEdit, SLOT(setTool(int)));
      connect(dlist,    SIGNAL(curDrumInstrumentChanged(int)), SLOT(setCurDrumInstrument(int)));

      //printf("DrumEdit::addCtrl curDrumInstrument:%d\n", dlist->getSelectedInstrument());
      
      setCurDrumInstrument(dlist->getSelectedInstrument());

      // p3.3.44
      ctrlEdit->setTool(tools2->curTool());
      
      ctrlEdit->setXPos(hscroll->pos());
      ctrlEdit->setXMag(hscroll->getScaleValue());

      if(split1w1)
      {
        split2->setCollapsible(split1w1, false);
        split1w1->setMinimumWidth(CTRL_PANEL_FIXED_WIDTH);
      }
      ctrlEdit->setCanvasWidth(canvas->width());
      
      ctrlEdit->show();
      ctrlEditList.push_back(ctrlEdit);
      return ctrlEdit;
      }

//---------------------------------------------------------
//   removeCtrl
//---------------------------------------------------------

void DrumEdit::removeCtrl(CtrlEdit* ctrl)
      {
      for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            if (*i == ctrl) {
                  ctrlEditList.erase(i);
                  break;
                  }
            }
      
      if(split1w1)
      {
        if(ctrlEditList.empty())
        {
          split1w1->setMinimumWidth(0);
          split2->setCollapsible(split1w1, true);
        }  
      }
      }
//---------------------------------------------------------
//   newCanvasWidth
//---------------------------------------------------------

void DrumEdit::newCanvasWidth(int w)
      {
      int nw = w + (vscroll->width() - 18); // 18 is the fixed width of the CtlEdit VScale widget.
      if(nw < 1)
        nw = 1;
        
      for (std::list<CtrlEdit*>::iterator i = ctrlEditList.begin();
         i != ctrlEditList.end(); ++i) {
            // Changed by Tim. p3.3.7
            //(*i)->setCanvasWidth(w);
            (*i)->setCanvasWidth(nw);
            }
            
      updateHScrollRange();
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void DrumEdit::resizeEvent(QResizeEvent* ev)
      {
      QWidget::resizeEvent(ev);
      _widthInit = ev->size().width();
      _heightInit = ev->size().height();
      
      //TODO: Make the dlist not expand/shrink, but the canvas instead
      }


//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void DrumEdit::configChanged()
      {
      initShortcuts();
      }

static int rasterTable[] = {
      //-9----8-  7    6     5     4    3(1/4)     2   1
      4,  8, 16, 32,  64, 128, 256,  512, 1024,  // triple
      6, 12, 24, 48,  96, 192, 384,  768, 1536,
      9, 18, 36, 72, 144, 288, 576, 1152, 2304   // dot
      };

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------
void DrumEdit::keyPressEvent(QKeyEvent* event)
      {
      DrumCanvas* dc = (DrumCanvas*)canvas;
      int index = 0;
      int n = sizeof(rasterTable);
      for (; index < n; ++index)
            if (rasterTable[index] == raster())
                  break;
      int off = (index / 9) * 9;
      index   = index % 9;
      int val;
      int key = event->key();

      if (event->state() & ShiftButton)
            key += SHIFT;
      if (event->state() & AltButton)
            key += ALT;
      if (event->state() & ControlButton)
            key+= CTRL;

      if (key == Key_Escape) {
            close();
            return;
            }
      else if (key == Key_Up) {
            dlist->setCurDrumInstrument(dlist->getSelectedInstrument()-1);
            dlist->redraw();
            return;
            }
      else if (key == Key_F2) {
            dlist->lineEdit(dlist->getSelectedInstrument(),(int)DList::COL_NAME);
            return;
            }
      else if (key == Key_Down) {
            dlist->setCurDrumInstrument(dlist->getSelectedInstrument()+1);
            dlist->redraw();
            return;
            }
      
      else if (key == shortcuts[SHRT_POS_INC].key) {
            dc->cmd(DrumCanvas::CMD_RIGHT);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC].key) {
            dc->cmd(DrumCanvas::CMD_LEFT);
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

            /*
      else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key) {
            pc->pianoCmd(CMD_INSERT);
            return;
            }
            */
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
            /*
      else if (key == shortcuts[SHRT_EVENT_COLOR].key) {
            if (colorMode == 0)
                  colorMode = 1;
            else if (colorMode == 1)
                  colorMode = 2;
            else
                  colorMode = 0;
            setEventColorMode(colorMode);
            return;
            }*/
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
//   initShortcuts
//---------------------------------------------------------

void DrumEdit::initShortcuts()
      {
      menuEdit->setAccel(shortcuts[SHRT_CUT].key,             DrumCanvas::CMD_CUT);
      menuEdit->setAccel(shortcuts[SHRT_COPY].key,            DrumCanvas::CMD_COPY);;
      menuEdit->setAccel(shortcuts[SHRT_PASTE].key,           DrumCanvas::CMD_PASTE);
      menuEdit->setAccel(shortcuts[SHRT_DELETE].key,          DrumCanvas::CMD_DEL);
      menuFile->setAccel(shortcuts[SHRT_OPEN].key,            DrumCanvas::CMD_LOAD);
      menuFile->setAccel(shortcuts[SHRT_SAVE].key,            DrumCanvas::CMD_SAVE);
      menuFunctions->setAccel(shortcuts[SHRT_FIXED_LEN].key,  DrumCanvas::CMD_FIXED_LEN);
      menuFunctions->setAccel(shortcuts[SHRT_MODIFY_VELOCITY].key, DrumCanvas::CMD_MODIFY_VELOCITY);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_ALL].key,    DrumCanvas::CMD_SELECT_ALL);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_NONE].key,   DrumCanvas::CMD_SELECT_NONE);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_INVERT].key, DrumCanvas::CMD_SELECT_INVERT);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_ILOOP].key,  DrumCanvas::CMD_SELECT_ILOOP);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_OLOOP].key,  DrumCanvas::CMD_SELECT_OLOOP);
      
      menuSelect->setAccel(shortcuts[SHRT_SELECT_PREV_PART].key, DrumCanvas::CMD_SELECT_PREV_PART);
      menuSelect->setAccel(shortcuts[SHRT_SELECT_NEXT_PART].key, DrumCanvas::CMD_SELECT_NEXT_PART);
      }

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------
void DrumEdit::execDeliveredScript(int id)
{
      //QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
      QString scriptfile = song->getScriptPath(id, true);
      song->executeScript(scriptfile.latin1(), parts(), quant(), true); 
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void DrumEdit::execUserScript(int id)
{
      QString scriptfile = song->getScriptPath(id, false);
      song->executeScript(scriptfile.latin1(), parts(), quant(), true);
}

