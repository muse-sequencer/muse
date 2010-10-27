//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.cpp,v 1.5.2.12 2009/04/06 01:24:54 terminator356 Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "app.h"
#include "xml.h"
#include "waveedit.h"
#include "mtscale.h"
#include "scrollscale.h"
#include "waveview.h"
#include "ttoolbar.h"
#include "globals.h"
#include "audio.h"
#include "utils.h"
#include "song.h"
#include "poslabel.h"
#include "gconfig.h"
#include "icons.h"
#include "shortcuts.h"

//#include <q3toolbar.h>
#include <QToolBar>
#include <qtoolbutton.h>
#include <q3accel.h>
#include <qlayout.h>
#include <q3hbox.h>
#include <qsizegrip.h>
#include <qscrollbar.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <q3buttongroup.h>
#include <q3listbox.h>
#include <qcombobox.h>
#include <q3header.h>
#include <q3popupmenu.h>
#include <qmenubar.h>
#include <qaction.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QResizeEvent>
#include <QKeyEvent>

extern QColor readColor(Xml& xml);

int WaveEdit::_widthInit = 600;
int WaveEdit::_heightInit = 400;

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void WaveEdit::closeEvent(QCloseEvent* e)
      {
      emit deleted((unsigned long)this);
      e->accept();
      }

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

WaveEdit::WaveEdit(PartList* pl)
   : MidiEditor(1, 1, pl)
      {
      resize(_widthInit, _heightInit);

      //---------Pulldown Menu----------------------------
      Q3PopupMenu* menuFile = new Q3PopupMenu(this);
      Q3PopupMenu* menuEdit = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("&File"), menuFile);
      menuBar()->insertItem(tr("&Edit"), menuEdit);
      menuFunctions = new Q3PopupMenu(this);
      menuBar()->insertItem(tr("Func&tions"), menuFunctions);

      menuGain = new Q3PopupMenu(this);
      menuGain->insertItem(tr("200%"), CMD_GAIN_200);
      menuGain->insertItem(tr("150%"), CMD_GAIN_150);
      menuGain->insertItem(tr("75%"),  CMD_GAIN_75);
      menuGain->insertItem(tr("50%"),  CMD_GAIN_50);
      menuGain->insertItem(tr("25%"),  CMD_GAIN_25);
      menuGain->insertItem(tr("Other"), CMD_GAIN_FREE);
      menuFunctions->insertItem(tr("&Gain"), menuGain);
      menuFunctions->insertSeparator();

      menuEdit->insertItem(tr("Edit in E&xternal Editor"), CMD_EDIT_EXTERNAL);
      menuFunctions->insertItem(tr("Mute Selection"), CMD_MUTE);
      menuFunctions->insertItem(tr("Normalize Selection"), CMD_NORMALIZE);
      menuFunctions->insertItem(tr("Fade In Selection"), CMD_FADE_IN);
      menuFunctions->insertItem(tr("Fade Out Selection"), CMD_FADE_OUT);
      menuFunctions->insertItem(tr("Reverse Selection"), CMD_REVERSE);

      select = new Q3PopupMenu(this);
      select->insertItem(QIcon(*select_allIcon), tr("Select &All"),  CMD_SELECT_ALL);
      select->insertItem(QIcon(*select_deselect_allIcon), tr("&Deselect All"), CMD_SELECT_NONE);

      menuEdit->insertItem(QIcon(*selectIcon), tr("Select"), select);

      connect(menuFunctions, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(menuFile, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(select, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(menuGain, SIGNAL(activated(int)), SLOT(cmd(int)));
      connect(menuEdit, SIGNAL(activated(int)), SLOT(cmd(int)));

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("waveedit-tools"));          
      tools->addActions(undoRedo->actions());

      Q3Accel* qa = new Q3Accel(this);
      qa->connectItem(qa->insertItem(Qt::CTRL+Qt::Key_Z), song, SLOT(undo()));
      qa->connectItem(qa->insertItem(Qt::CTRL+Qt::Key_Y), song, SLOT(redo()));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));

      //--------------------------------------------------
      //    Transport Bar
      QToolBar* transport = addToolBar(tr("transport"));          
      transport->addActions(transportAction->actions());

      //--------------------------------------------------
      //    ToolBar:   Solo  Cursor1 Cursor2

      addToolBarBreak();
      tb1 = addToolBar(tr("pianoroll-tools"));          

      //tb1->setLabel(tr("weTools"));
      solo = new QToolButton();
      solo->setText(tr("Solo"));
      solo->setCheckable(true);
      tb1->addWidget(solo);
      connect(solo,  SIGNAL(toggled(bool)), SLOT(soloChanged(bool)));
      
      QLabel* label = new QLabel(tr("Cursor"));
      tb1->addWidget(label);
      label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
      label->setIndent(3);
      pos1 = new PosLabel(0);
      pos1->setFixedHeight(22);
      tb1->addWidget(pos1);
      pos2 = new PosLabel(0);
      pos2->setFixedHeight(22);
      pos2->setSmpte(true);
      tb1->addWidget(pos2);

      //---------------------------------------------------
      //    Rest
      //---------------------------------------------------

      int yscale = 256;
      int xscale;

      if (!parts()->empty()) { // Roughly match total size of part
            Part* firstPart = parts()->begin()->second;
            xscale = 0 - firstPart->lenFrame()/_widthInit;
            }
      else {
            xscale = -8000;
            }

      hscroll = new ScrollScale(1, -32768, xscale, 10000, Qt::Horizontal, mainw, 0, true, 10000.0);
      view    = new WaveView(this, mainw, xscale, yscale);
      wview   = view;   // HACK!

      QSizeGrip* corner    = new QSizeGrip(mainw);
      ymag                 = new QSlider(1, 256, 256, yscale, Qt::Vertical, mainw);
      time                 = new MTScale(&_raster, mainw, xscale, true);
      ymag->setFixedWidth(16);
      connect(ymag, SIGNAL(valueChanged(int)), view, SLOT(setYScale(int)));
      time->setOrigin(0, 0);

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColStretch(0, 100);

      mainGrid->addMultiCellWidget(time,   0, 0, 0, 1);
      mainGrid->addMultiCellWidget(hLine(mainw),    1, 1, 0, 1);
      mainGrid->addWidget(view,    2, 0);
      mainGrid->addWidget(ymag,    2, 1);
      mainGrid->addWidget(hscroll, 3, 0);
      mainGrid->addWidget(corner,  3, 1, Qt::AlignBottom | Qt::AlignRight);

      connect(hscroll, SIGNAL(scrollChanged(int)), view, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  view, SLOT(setXMag(int)));
      setCaption(view->getCaption());
      connect(view, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));

      connect(hscroll, SIGNAL(scrollChanged(int)), time,  SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  time,  SLOT(setXMag(int)));
//      connect(time,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(view,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));
      
      updateHScrollRange();
      configChanged();
      
      if(!parts()->empty())
      {
        WavePart* part = (WavePart*)(parts()->begin()->second);
        solo->setOn(part->track()->solo());
      }
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void WaveEdit::configChanged()
      {
      view->setBg(config.waveEditBackgroundColor);
      select->setAccel(shortcuts[SHRT_SELECT_ALL].key, CMD_SELECT_ALL);
      select->setAccel(shortcuts[SHRT_SELECT_NONE].key, CMD_SELECT_NONE);
      }

//---------------------------------------------------------
//   updateHScrollRange
//---------------------------------------------------------
void WaveEdit::updateHScrollRange()
{
      int s, e;
      wview->range(&s, &e);
      // Show one more measure.
      e += sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += sigmap.ticksMeasure(e) / 4;
      // Compensate for the vscroll width. 
      //e += wview->rmapxDev(-vscroll->width()); 
      int s1, e1;
      hscroll->range(&s1, &e1);
      if(s != s1 || e != e1) 
        hscroll->setRange(s, e);
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void WaveEdit::setTime(unsigned samplepos)
      {
//    printf("setTime %d %x\n", samplepos, samplepos);
      unsigned tick = tempomap.frame2tick(samplepos);
      pos1->setValue(tick);
      //pos2->setValue(tick);
      pos2->setValue(samplepos);
      time->setPos(3, tick, false);
      }

//---------------------------------------------------------
//   ~WaveEdit
//---------------------------------------------------------

WaveEdit::~WaveEdit()
      {
      // undoRedo->removeFrom(tools); // p4.0.6 Removed
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void WaveEdit::cmd(int n)
      {
      view->cmd(n);
      }

//---------------------------------------------------------
//   loadConfiguration
//---------------------------------------------------------

void WaveEdit::readConfiguration(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "bgcolor")
                              config.waveEditBackgroundColor = readColor(xml);
                        else if (tag == "width")
                              _widthInit = xml.parseInt();
                        else if (tag == "height")
                              _heightInit = xml.parseInt();
                        else
                              xml.unknown("WaveEdit");
                        break;
                  case Xml::TagEnd:
                        if (tag == "waveedit")
                              return;
                  default:
                        break;
                  case Xml::Error:
                  case Xml::End:
                        return;
                  }
            }
      }

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void WaveEdit::writeConfiguration(int level, Xml& xml)
      {
      xml.tag(level++, "waveedit");
      xml.colorTag(level, "bgcolor", config.waveEditBackgroundColor);
      xml.intTag(level, "width", _widthInit);
      xml.intTag(level, "height", _heightInit);
      xml.tag(level, "/waveedit");
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void WaveEdit::writeStatus(int level, Xml& xml) const
      {
      writePartList(level, xml);
      xml.tag(level++, "waveedit");
      MidiEditor::writeStatus(level, xml);
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "ymag", ymag->value());
      xml.tag(level, "/waveedit");
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void WaveEdit::readStatus(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            if (token == Xml::Error || token == Xml::End)
                  break;
            QString tag = xml.s1();
            switch (token) {
                  case Xml::TagStart:
                        if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "ymag")
                              ymag->setValue(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else
                              xml.unknown("WaveEdit");
                        break;
                  case Xml::TagEnd:
                        if (tag == "waveedit")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void WaveEdit::resizeEvent(QResizeEvent* ev)
      {
      QWidget::resizeEvent(ev);
      _widthInit = ev->size().width();
      _heightInit = ev->size().height();
      }

//---------------------------------------------------------
//   songChanged1
//    signal from "song"
//---------------------------------------------------------

void WaveEdit::songChanged1(int bits)
      {
        
        if (bits & SC_SOLO)
        {
          WavePart* part = (WavePart*)(parts()->begin()->second);
          solo->blockSignals(true);
          solo->setOn(part->track()->solo());
          solo->blockSignals(false);
        }  
        
        songChanged(bits);
      }


//---------------------------------------------------------
//   soloChanged
//    signal from solo button
//---------------------------------------------------------

void WaveEdit::soloChanged(bool flag)
      {
      WavePart* part = (WavePart*)(parts()->begin()->second);
      audio->msgSetSolo(part->track(), flag);
      song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void WaveEdit::keyPressEvent(QKeyEvent* event)
      {
      int key = event->key();
      if (key == Qt::Key_Escape) {
            close();
            return;
            }
      else {
            event->ignore();
            }
      }


