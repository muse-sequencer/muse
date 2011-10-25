//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.cpp,v 1.22.2.21 2009/11/16 11:29:33 lunar_shuttle Exp $
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

#include <QAction>
#include <QClipboard>
#include <QCloseEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalMapper>
#include <QSizeGrip>
#include <QToolButton>
#include <QWhatsThis>
#include <QSettings>
#include <QComboBox>
#include <QLabel>

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
#include "functions.h"
#include "helper.h"
#include "widgets/function_dialogs/quantize.h"

namespace MusEGui {

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

int DrumEdit::_rasterInit = 96;
int DrumEdit::_dlistWidthInit = 50;
int DrumEdit::_dcanvasWidthInit = 300;

static const int xscale = -10;
static const int yscale = 1;
static const int drumeditTools = MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool | MusEGui::CursorTool | MusEGui::DrawTool;

enum DrumColumn {
  COL_MUTE = 0,
  COL_NAME,
  COL_VOLUME,
  COL_QUANT,
  COL_INPUTTRIGGER,
  COL_NOTELENGTH,
  COL_NOTE,
  COL_OUTCHANNEL,
  COL_OUTPORT,
  COL_LEVEL1,
  COL_LEVEL2,
  COL_LEVEL3,
  COL_LEVEL4,
  COL_NONE = -1
};

//---------------------------------------------------------
//   setHeaderWhatsThis
//---------------------------------------------------------

void DrumEdit::setHeaderWhatsThis()
      {
      header->setWhatsThis(COL_MUTE, tr("mute instrument"));
      header->setWhatsThis(COL_NAME, tr("sound name"));
      header->setWhatsThis(COL_VOLUME, tr("volume percent"));
      header->setWhatsThis(COL_QUANT, tr("quantisation"));
      header->setWhatsThis(COL_INPUTTRIGGER, tr("this input note triggers the sound"));
      header->setWhatsThis(COL_NOTELENGTH, tr("note length"));
      header->setWhatsThis(COL_NOTE, tr("this is the note which is played"));
      header->setWhatsThis(COL_OUTCHANNEL, tr("output channel (hold ctl to affect all rows)"));
      header->setWhatsThis(COL_OUTPORT, tr("output port (hold ctl to affect all rows)"));
      header->setWhatsThis(COL_LEVEL1, tr("shift + control key: draw velocity level 1"));
      header->setWhatsThis(COL_LEVEL2, tr("control key: draw velocity level 2"));
      header->setWhatsThis(COL_LEVEL3, tr("shift key: draw velocity level 3"));
      header->setWhatsThis(COL_LEVEL4, tr("draw velocity level 4"));
      }

//---------------------------------------------------------
//   setHeaderToolTips
//---------------------------------------------------------

void DrumEdit::setHeaderToolTips()
      {
      header->setToolTip(COL_MUTE, tr("mute instrument"));
      header->setToolTip(COL_NAME, tr("sound name"));
      header->setToolTip(COL_VOLUME, tr("volume percent"));
      header->setToolTip(COL_QUANT, tr("quantisation"));
      header->setToolTip(COL_INPUTTRIGGER, tr("this input note triggers the sound"));
      header->setToolTip(COL_NOTELENGTH, tr("note length"));
      header->setToolTip(COL_NOTE, tr("this is the note which is played"));
      header->setToolTip(COL_OUTCHANNEL, tr("output channel (ctl: affect all rows)"));
      header->setToolTip(COL_OUTPORT, tr("output port (ctl: affect all rows)"));
      header->setToolTip(COL_LEVEL1, tr("shift + control key: draw velocity level 1"));
      header->setToolTip(COL_LEVEL2, tr("control key: draw velocity level 2"));
      header->setToolTip(COL_LEVEL3, tr("shift key: draw velocity level 3"));
      header->setToolTip(COL_LEVEL4, tr("draw velocity level 4"));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void DrumEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      QSettings settings("MusE", "MusE-qt");
      //settings.setValue("Drumedit/geometry", saveGeometry());
      settings.setValue("Drumedit/windowState", saveState());

      //Store values of the horizontal splitter
      QList<int> sizes = split2->sizes();
      QList<int>::iterator it = sizes.begin();
      _dlistWidthInit = *it; //There are only 2 values stored in the sizelist, size of dlist widget and dcanvas widget
      it++;
      _dcanvasWidthInit = *it;
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

DrumEdit::DrumEdit(MusECore::PartList* pl, QWidget* parent, const char* name, unsigned initPos)
   : MidiEditor(TopWin::DRUM, _rasterInit, pl, parent, name)
      {
      setFocusPolicy(Qt::StrongFocus);

      split1w1 = 0;
      selPart  = 0;
      QSignalMapper *signalMapper = new QSignalMapper(this);
      
      //---------Pulldown Menu----------------------------
      menuFile = menuBar()->addMenu(tr("&File"));

      loadAction = menuFile->addAction(QIcon(*openIcon), tr("Load Map"));
      saveAction = menuFile->addAction(QIcon(*saveIcon), tr("Save Map"));
      resetAction = menuFile->addAction(tr("Reset GM Map"));

      connect(loadAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(saveAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(resetAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

      signalMapper->setMapping(loadAction, DrumCanvas::CMD_LOAD);
      signalMapper->setMapping(saveAction, DrumCanvas::CMD_SAVE);
      signalMapper->setMapping(resetAction, DrumCanvas::CMD_RESET);

      menuEdit = menuBar()->addMenu(tr("&Edit"));
      menuEdit->addActions(MusEGlobal::undoRedo->actions());
      
      menuEdit->addSeparator();
      cutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("Cut"));
      copyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy"));
      copyRangeAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy events in range"));
      pasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste"));
      pasteDialogAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste (with Dialog)"));
      menuEdit->addSeparator();
      deleteAction = menuEdit->addAction(tr("Delete Events"));

      connect(cutAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(copyAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(copyRangeAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(pasteAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(pasteDialogAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(deleteAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

      signalMapper->setMapping(cutAction, DrumCanvas::CMD_CUT);
      signalMapper->setMapping(copyAction, DrumCanvas::CMD_COPY);
      signalMapper->setMapping(copyRangeAction, DrumCanvas::CMD_COPY_RANGE);
      signalMapper->setMapping(pasteAction, DrumCanvas::CMD_PASTE);
      signalMapper->setMapping(pasteDialogAction, DrumCanvas::CMD_PASTE_DIALOG);
      signalMapper->setMapping(deleteAction, DrumCanvas::CMD_DEL);

      menuSelect = menuEdit->addMenu(QIcon(*selectIcon), tr("&Select"));

      sallAction = menuSelect->addAction(QIcon(*select_allIcon), tr("Select All"));
      snoneAction = menuSelect->addAction(QIcon(*select_deselect_allIcon), tr("Select None"));
      invAction = menuSelect->addAction(QIcon(*select_invert_selectionIcon), tr("Invert"));
      menuSelect->addSeparator();
      inAction = menuSelect->addAction(QIcon(*select_inside_loopIcon), tr("Inside Loop"));
      outAction = menuSelect->addAction(QIcon(*select_outside_loopIcon), tr("Outside Loop"));
      
      menuSelect->addSeparator();

      prevAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("Previous Part"));
      nextAction = menuSelect->addAction(QIcon(*select_all_parts_on_trackIcon), tr("Next Part"));

      connect(sallAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(snoneAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(invAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(inAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(outAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(prevAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(nextAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

      signalMapper->setMapping(sallAction, DrumCanvas::CMD_SELECT_ALL);
      signalMapper->setMapping(snoneAction, DrumCanvas::CMD_SELECT_NONE);
      signalMapper->setMapping(invAction, DrumCanvas::CMD_SELECT_INVERT);
      signalMapper->setMapping(inAction, DrumCanvas::CMD_SELECT_ILOOP);
      signalMapper->setMapping(outAction, DrumCanvas::CMD_SELECT_OLOOP);
      signalMapper->setMapping(prevAction, DrumCanvas::CMD_SELECT_PREV_PART);
      signalMapper->setMapping(nextAction, DrumCanvas::CMD_SELECT_NEXT_PART);

      // Functions
      menuFunctions = menuBar()->addMenu(tr("Fu&nctions"));
      
      menuFunctions->setTearOffEnabled(true);
      
      QAction* reorderListAction = menuFunctions->addAction(tr("Re-order list"));
      menuFunctions->addSeparator();
      fixedAction = menuFunctions->addAction(tr("Set Fixed Length"));
      veloAction = menuFunctions->addAction(tr("Modify Velocity"));
      crescAction = menuFunctions->addAction(tr("Crescendo/Decrescendo"));
      quantizeAction = menuFunctions->addAction(tr("Quantize"));
      QAction* eraseEventAction = menuFunctions->addAction(tr("Erase Event"));
      QAction* noteShiftAction = menuFunctions->addAction(tr("Move Notes"));
      QAction* delOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));

      connect(reorderListAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(fixedAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(veloAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(crescAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(quantizeAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(eraseEventAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(noteShiftAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(delOverlapsAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

      signalMapper->setMapping(reorderListAction, DrumCanvas::CMD_REORDER_LIST);
      signalMapper->setMapping(fixedAction, DrumCanvas::CMD_FIXED_LEN);
      signalMapper->setMapping(veloAction, DrumCanvas::CMD_MODIFY_VELOCITY);
      signalMapper->setMapping(crescAction, DrumCanvas::CMD_CRESCENDO);
      signalMapper->setMapping(quantizeAction, DrumCanvas::CMD_QUANTIZE);
      signalMapper->setMapping(eraseEventAction, DrumCanvas::CMD_ERASE_EVENT);
      signalMapper->setMapping(noteShiftAction, DrumCanvas::CMD_NOTE_SHIFT);
      signalMapper->setMapping(delOverlapsAction, DrumCanvas::CMD_DELETE_OVERLAPS);

      QMenu* menuScriptPlugins = menuBar()->addMenu(tr("&Plugins"));
      MusEGlobal::song->populateScriptMenu(menuScriptPlugins, this);
      
      QMenu* settingsMenu = menuBar()->addMenu(tr("Window &Config"));
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      connect(signalMapper, SIGNAL(mapped(int)), SLOT(cmd(int)));

      //---------------------------------------------------
      //    Toolbars
      //---------------------------------------------------

      tools = addToolBar(tr("Drum tools"));
      tools->setObjectName("Drum tools");
      
      QToolButton *ldm = new QToolButton();
      ldm->setToolTip(tr("Load Drummap"));
      ldm->setIcon(*openIcon);
      connect(ldm, SIGNAL(clicked()), SLOT(load()));
      tools->addWidget(ldm);
      
      QToolButton *sdm = new QToolButton();
      sdm->setToolTip(tr("Store Drummap"));
      sdm->setIcon(*saveIcon);
      connect(sdm, SIGNAL(clicked()), SLOT(save()));
      tools->addWidget(sdm);
      
      tools->addAction(QWhatsThis::createAction());

      tools->addSeparator();
      tools->addActions(MusEGlobal::undoRedo->actions());
      tools->addSeparator();

      srec  = new QToolButton();
      srec->setToolTip(tr("Step Record"));
      srec->setIcon(*steprecIcon);
      srec->setCheckable(true);
      tools->addWidget(srec);

      midiin  = new QToolButton();
      midiin->setToolTip(tr("Midi Input"));
      midiin->setIcon(*midiinIcon);
      midiin->setCheckable(true);
      tools->addWidget(midiin);
      
      
      tools2 = new MusEGui::EditToolBar(this, drumeditTools);
      addToolBar(tools2);

      QToolBar* cursorToolbar = addToolBar(tr("cursor tools"));
      cursorToolbar->setObjectName("cursor");
      QLabel *stepStr = new QLabel("Cursor step:");
      cursorToolbar->addWidget(stepStr);
      stepLenWidget = new QComboBox();
      stepLenWidget->setToolTip(tr("Set step size for cursor edit"));
      stepLenWidget->addItem("1");
      stepLenWidget->addItem("2");
      stepLenWidget->addItem("3");
      stepLenWidget->addItem("4");
      stepLenWidget->addItem("5");
      stepLenWidget->addItem("6");
      stepLenWidget->addItem("7");
      stepLenWidget->addItem("8");
      stepLenWidget->addItem("16");
      stepLenWidget->setCurrentIndex(0);
      connect(stepLenWidget, SIGNAL(currentIndexChanged(QString)), SLOT(setStep(QString)));
      cursorToolbar->addWidget(stepLenWidget);

      QToolBar* panicToolbar = addToolBar(tr("panic"));
      panicToolbar->setObjectName("panic");
      panicToolbar->addAction(MusEGlobal::panicAction);
      
      QToolBar* transport = addToolBar(tr("transport"));
      transport->setObjectName("transport");
      transport->addActions(MusEGlobal::transportAction->actions());
      
      addToolBarBreak();
      // don't show pitch value in toolbar
      toolbar = new MusEGui::Toolbar1(this, _rasterInit, false);
      addToolBar(toolbar);
      
      addToolBarBreak();
      info    = new MusEGui::NoteInfo(this);
      addToolBar(info);

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      split1            = new MusEGui::Splitter(Qt::Vertical, mainw, "split1");
      QPushButton* ctrl = new QPushButton(tr("ctrl"), mainw);
      ctrl->setObjectName("Ctrl");
      ctrl->setFont(MusEGlobal::config.fonts[3]);
      //hscroll           = new MusEGui::ScrollScale(-25, -2, xscale, 20000, Qt::Horizontal, mainw);
      // Increased scale to -1. To resolve/select/edit 1-tick-wide (controller graph) events. p4.0.18 Tim.
      hscroll           = new MusEGui::ScrollScale(-25, -1, xscale, 20000, Qt::Horizontal, mainw);
      ctrl->setFixedSize(40, hscroll->sizeHint().height());
      ctrl->setToolTip(tr("Add Controller View"));

      QSizeGrip* corner = new QSizeGrip(mainw);
      corner->setFixedHeight(hscroll->sizeHint().height());

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(1, 100);

      mainGrid->addWidget(split1, 0, 0,  1, 3);
      mainGrid->addWidget(ctrl,    1, 0);
      mainGrid->addWidget(hscroll, 1, 1);
      mainGrid->addWidget(corner,  1, 2, Qt::AlignBottom|Qt::AlignRight);
//      mainGrid->addRowSpacing(1, hscroll->sizeHint().height());
//      mainGrid->addItem(new QSpacerItem(0, hscroll->sizeHint().height()), 1, 0); 

      split2              = new MusEGui::Splitter(Qt::Horizontal, split1, "split2");
      split1w1            = new QWidget(split2);
      QWidget* split1w2   = new QWidget(split2);
      QGridLayout* gridS1 = new QGridLayout(split1w1);
      QGridLayout* gridS2 = new QGridLayout(split1w2);
      gridS1->setContentsMargins(0, 0, 0, 0);
      gridS1->setSpacing(0);  
      gridS2->setContentsMargins(0, 0, 0, 0);
      gridS2->setSpacing(0);  
      time                = new MusEGui::MTScale(&_raster, split1w2, xscale);
      canvas              = new DrumCanvas(this, split1w2, xscale, yscale);
      vscroll             = new MusEGui::ScrollScale(-4, 1, yscale, DRUM_MAPSIZE*TH, Qt::Vertical, split1w2);
      int offset = -(MusEGlobal::config.division/4);
      canvas->setOrigin(offset, 0);
      canvas->setCanvasTools(drumeditTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(canvas, SIGNAL(horizontalZoomIn()), SLOT(horizontalZoomIn()));
      connect(canvas, SIGNAL(horizontalZoomOut()), SLOT(horizontalZoomOut()));
      time->setOrigin(offset, 0);

      QList<int> mops;
      mops.append(_dlistWidthInit);
      mops.append(_dcanvasWidthInit);
      split2->setSizes(mops);
      // By T356. Not much choice but to disable this for now, to stop runaway resize bug.
      // Can't seem to get the splitter to readjust when manually setting sizes.
      //split2->setResizeMode(split1w1, QSplitter::KeepSize);

      gridS2->setRowStretch(1, 100);
      gridS2->setColumnStretch(0, 100);
      
      gridS2->addWidget(time,  0, 0, 1, 2);
      gridS2->addWidget(MusECore::hLine(split1w2), 1, 0, 1, 2);
      gridS2->addWidget(canvas,  2, 0);
      
      gridS2->addWidget(vscroll, 2, 1);
      //
      //  Reihenfolge in dlist.c festgeschrieben ("Dcols")
      //
      header = new MusEGui::Header(split1w1, "header");
      header->setFixedHeight(31);
      header->setColumnLabel(tr("M"), COL_MUTE, 20);
      header->setColumnLabel(tr("Sound"), COL_NAME, 120);
      header->setColumnLabel(tr("Vol"), COL_VOLUME);
      header->setColumnLabel(tr("QNT"), COL_QUANT, 30);
      header->setColumnLabel(tr("E-Note"), COL_INPUTTRIGGER, 50);
      header->setColumnLabel(tr("Len"), COL_NOTELENGTH);
      header->setColumnLabel(tr("A-Note"), COL_NOTE, 50);
      header->setColumnLabel(tr("Ch"), COL_OUTCHANNEL);
      header->setColumnLabel(tr("Port"), COL_OUTPORT, 70);
      header->setColumnLabel(tr("LV1"), COL_LEVEL1);
      header->setColumnLabel(tr("LV2"), COL_LEVEL2);
      header->setColumnLabel(tr("LV3"), COL_LEVEL3);
      header->setColumnLabel(tr("LV4"), COL_LEVEL4);

      setHeaderToolTips();
      setHeaderWhatsThis();

      dlist = new DList(header, split1w1, yscale);
      // p3.3.44
      setCurDrumInstrument(dlist->getSelectedInstrument());
      
      connect(dlist, SIGNAL(keyPressed(int, int)), canvas, SLOT(keyPressed(int, int)));
      connect(dlist, SIGNAL(keyReleased(int, bool)), canvas, SLOT(keyReleased(int, bool)));
      connect(dlist, SIGNAL(mapChanged(int, int)), canvas, SLOT(mapChanged(int, int)));

      gridS1->setRowStretch(1, 100);
      gridS1->setColumnStretch(0, 100);
      gridS1->addWidget(header, 0, 0);
      gridS1->addWidget(dlist, 1, 0);
      
      connect(canvas, SIGNAL(newWidth(int)), SLOT(newCanvasWidth(int)));
      connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(MusEGlobal::song, SIGNAL(songChanged(int)), SLOT(songChanged1(int)));
      connect(MusEGlobal::song, SIGNAL(songChanged(int)),      dlist, SLOT(songChanged(int)));
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

      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));  // in Canvas
      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool2(int))); // in DrumCanvas

      connect(canvas, SIGNAL(selectionChanged(int, MusECore::Event&, MusECore::Part*)), this,
         SLOT(setSelection(int, MusECore::Event&, MusECore::Part*)));
      connect(canvas, SIGNAL(followEvent(int)), SLOT(follow(int)));

      connect(hscroll, SIGNAL(scaleChanged(int)),  SLOT(updateHScrollRange()));
      setWindowTitle(canvas->getCaption());
      
      updateHScrollRange();
      
      // connect toolbar
      connect(canvas,  SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(time,    SIGNAL(timeChanged(unsigned)),  SLOT(setTime(unsigned)));
      connect(toolbar, SIGNAL(rasterChanged(int)),         SLOT(setRaster(int)));
      connect(toolbar, SIGNAL(soloChanged(bool)),          SLOT(soloChanged(bool)));
      connect(info, SIGNAL(valueChanged(MusEGui::NoteInfo::ValType, int)), SLOT(noteinfoChanged(MusEGui::NoteInfo::ValType, int)));

      connect(ctrl, SIGNAL(clicked()), SLOT(addCtrl()));

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));

      clipboardChanged(); // enable/disable "Paste"
      selectionChanged(); // enable/disable "Copy" & "Paste"
      initShortcuts();

      const MusECore::Pos cpos=MusEGlobal::song->cPos();
      canvas->setPos(0, cpos.tick(), true);
      canvas->selectAtTick(cpos.tick());
      //canvas->selectFirst();
        
      unsigned pos=0;
      if(initPos >= MAXINT)
        pos = MusEGlobal::song->cpos();
      if(pos > MAXINT)
        pos = MAXINT;
      if (pos)
        hscroll->setOffset((int)pos);

      if(canvas->track())
        toolbar->setSolo(canvas->track()->solo());
      
      
      initTopwinState();
      }

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void DrumEdit::songChanged1(int bits)
      {
        if(_isDeleting)  // Ignore while while deleting to prevent crash.
          return;
        
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
      e += AL::sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += AL::sigmap.ticksMeasure(e) / 4;
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
      //MusEGlobal::undoRedo->removeFrom(tools);   // p4.0.6 Removed
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void DrumEdit::setSelection(int tick, MusECore::Event& e, MusECore::Part* p)
      {
      selEvent = e;
      selPart  = (MusECore::MidiPart*)p;
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
      MusEGlobal::audio->msgSetSolo(canvas->track(), flag);
      MusEGlobal::song->update(SC_SOLO);
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
//    edit currently selected Event
//---------------------------------------------------------

void DrumEdit::noteinfoChanged(MusEGui::NoteInfo::ValType type, int val)
      {
      if (selEvent.empty()) {
            printf("noteinfoChanged while note is zero %d\n", type);
            return;
            }
      MusECore::Event event = selEvent.clone();
      switch (type) {
            case MusEGui::NoteInfo::VAL_TIME:
                  event.setTick(val - selPart->tick());
                  break;
            case MusEGui::NoteInfo::VAL_LEN:
                  event.setLenTick(val);
                  break;
            case MusEGui::NoteInfo::VAL_VELON:
                  event.setVelo(val);
                  break;
            case MusEGui::NoteInfo::VAL_VELOFF:
                  event.setVeloOff(val);
                  break;
            case MusEGui::NoteInfo::VAL_PITCH:
                  event.setPitch(val);
                  break;
            }
      // Indicate do undo, and do not do port controller values and clone parts. 
      //MusEGlobal::audio->msgChangeEvent(selEvent, event, selPart);
      MusEGlobal::audio->msgChangeEvent(selEvent, event, selPart, true, false, false);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void DrumEdit::writeStatus(int level, MusECore::Xml& xml) const
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

void DrumEdit::readStatus(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "steprec") {
                              int val = xml.parseInt();
                              canvas->setSteprec(val);
                              srec->setChecked(val);
                              }
                        else if (tag == "midiin") {
                              int val = xml.parseInt();
                              canvas->setMidiin(val);
                              midiin->setChecked(val);
                              }
                        else if (tag == "ctrledit") {
                              CtrlEdit* ctrl = addCtrl();
                              ctrl->readStatus(xml);
                              }
                        else if (tag == split1->objectName())
                              split1->readStatus(xml);
                        else if (tag == split2->objectName())
                              split2->readStatus(xml);
                        else if (tag == "midieditor")
                              MidiEditor::readStatus(xml);
                        else if (tag == header->objectName())
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
                  case MusECore::Xml::TagEnd:
                        if (tag == "drumedit") {
                              _rasterInit = _raster;
                              toolbar->setRaster(_raster);
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

void DrumEdit::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "raster")
                              _rasterInit = xml.parseInt();
                        else if (tag == "dcanvaswidth")
                              _dcanvasWidthInit = xml.parseInt();
                        else if (tag == "dlistwidth")
                              _dlistWidthInit = xml.parseInt();
                        else if (tag == "topwin")
                              TopWin::readConfiguration(DRUM, xml);
                        else
                              xml.unknown("DrumEdit");
                        break;
                  case MusECore::Xml::TagEnd:
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

void DrumEdit::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "drumedit");
      xml.intTag(level, "raster", _rasterInit);
      xml.intTag(level, "dlistwidth", _dlistWidthInit);
      xml.intTag(level, "dcanvaswidth", _dcanvasWidthInit);
      TopWin::writeConfiguration(DRUM, level,xml);
      xml.tag(level, "/drumedit");
      }

//---------------------------------------------------------
//   load
//---------------------------------------------------------

void DrumEdit::load()
      {
      //QString fn = MusEGui::getOpenFileName("drummaps", map_file_pattern,
      QString fn = MusEGui::getOpenFileName("drummaps", MusEGlobal::drum_map_file_pattern,
         this, tr("Muse: Load Drum Map"), 0);
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = MusEGui::fileOpen(this, fn, QString(".map"), "r", popenFlag, true);
      if (f == 0)
            return;

      MusECore::Xml xml(f);
      int mode = 0;
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (mode == 0 && tag == "muse")
                              mode = 1;
                        else if (mode == 1 && tag == "drummap") {
                              readDrumMap(xml, true);
                              mode = 0;
                              }
                        else
                              xml.unknown("DrumEdit");
                        break;
                  case MusECore::Xml::Attribut:
                        break;
                  case MusECore::Xml::TagEnd:
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
      //QString fn = MusEGui::getSaveFileName(QString("drummaps"), map_file_pattern,
      QString fn = MusEGui::getSaveFileName(QString("drummaps"), MusEGlobal::drum_map_file_save_pattern,
        this, tr("MusE: Store Drum Map"));
      if (fn.isEmpty())
            return;
      bool popenFlag;
      FILE* f = MusEGui::fileOpen(this, fn, QString(".map"), "w", popenFlag, false, true);
      if (f == 0)
            return;
      MusECore::Xml xml(f);
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
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Ok)
  {    
    MusECore::resetGMDrumMap();
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
            case DrumCanvas::CMD_CUT:
                  copy_notes(partlist_to_set(parts()), 1);
                  erase_notes(partlist_to_set(parts()), 1);
                  break;
            case DrumCanvas::CMD_COPY: copy_notes(partlist_to_set(parts()), 1); break;
            case DrumCanvas::CMD_COPY_RANGE: copy_notes(partlist_to_set(parts()), MusECore::any_event_selected(partlist_to_set(parts())) ? 3 : 2); break;
            case DrumCanvas::CMD_PASTE: 
                  ((DrumCanvas*)canvas)->cmd(DrumCanvas::CMD_SELECT_NONE);
                  MusECore::paste_notes(3072);
                  break;
            case DrumCanvas::CMD_PASTE_DIALOG: 
                  ((DrumCanvas*)canvas)->cmd(DrumCanvas::CMD_SELECT_NONE);
                  MusECore::paste_notes((canvas->part()));
                  break;
            case DrumCanvas::CMD_LOAD: load(); break;
            case DrumCanvas::CMD_SAVE: save(); break;
            case DrumCanvas::CMD_RESET: reset(); break;
            case DrumCanvas::CMD_MODIFY_VELOCITY: modify_velocity(partlist_to_set(parts())); break;
            case DrumCanvas::CMD_CRESCENDO: crescendo(partlist_to_set(parts())); break;
            case DrumCanvas::CMD_QUANTIZE:
                  {
                  int raster = MusEGui::rasterVals[MusEGui::quantize_dialog->raster_index];
                  if (quantize_dialog->exec())
                        quantize_notes(partlist_to_set(parts()), quantize_dialog->range, 
                                       (MusEGlobal::config.division*4)/raster,
                                       /* quant_len= */false, quantize_dialog->strength, 
                                       quantize_dialog->swing, quantize_dialog->threshold);
                  break;
                  }
            case DrumCanvas::CMD_ERASE_EVENT: erase_notes(partlist_to_set(parts())); break;
            case DrumCanvas::CMD_DEL: erase_notes(partlist_to_set(parts()),1); break; //delete selected events
            case DrumCanvas::CMD_DELETE_OVERLAPS: delete_overlaps(partlist_to_set(parts())); break;
            case DrumCanvas::CMD_NOTE_SHIFT: move_notes(partlist_to_set(parts())); break;
            case DrumCanvas::CMD_REORDER_LIST: ((DrumCanvas*)(canvas))->moveAwayUnused(); break;
            //case DrumCanvas::CMD_FIXED_LEN: // this must be handled by the drum canvas, due to its
                                              // special nature (each drum has its own length)

            default: ((DrumCanvas*)(canvas))->cmd(cmd);
            }
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void DrumEdit::clipboardChanged()
      {
      pasteAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-groupedeventlists")));
      pasteDialogAction->setEnabled(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-groupedeventlists")));
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void DrumEdit::selectionChanged()
      {
      bool flag = canvas->selectionSize() > 0;
      cutAction->setEnabled(flag);
      copyAction->setEnabled(flag);
      deleteAction->setEnabled(flag);
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
      connect(dlist,    SIGNAL(curDrumInstrumentChanged(int)), canvas, SLOT(setCurDrumInstrument(int)));

      //printf("DrumEdit::addCtrl curDrumInstrument:%d\n", dlist->getSelectedInstrument());
      
      setCurDrumInstrument(dlist->getSelectedInstrument());

      // p3.3.44
      ctrlEdit->setTool(tools2->curTool());
      
      ctrlEdit->setXPos(hscroll->pos());
      ctrlEdit->setXMag(hscroll->getScaleValue());

      if(split1w1)
      {
        ///split2->setCollapsible(split1w1, false);
        split2->setCollapsible(split2->indexOf(split1w1), false);
        split1w1->setMinimumWidth(CTRL_PANEL_FIXED_WIDTH);
      }

      int dw = vscroll->width() - 18;// 18 is the fixed width of the CtlEdit VScale widget.
      if(dw < 1)
        dw = 1;
      ctrlEdit->setCanvasWidth(canvas->width() + dw);
      
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
          ///split2->setCollapsible(split1w1, true);
          split2->setCollapsible(split2->indexOf(split1w1), true);
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

      //TODO: Make the dlist not expand/shrink, but the canvas instead


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

      if (((QInputEvent*)event)->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (((QInputEvent*)event)->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (((QInputEvent*)event)->modifiers() & Qt::ControlModifier)
            key+= Qt::CTRL;

      if (key == Qt::Key_Escape) {
            close();
            return;
            }
      else if (key == shortcuts[SHRT_CURSOR_STEP_DOWN].key) {
            int newIndex=stepLenWidget->currentIndex()-1;
            if (newIndex<0)
              newIndex=0;
            stepLenWidget->setCurrentIndex(newIndex);
            return;
      }
      else if (key == shortcuts[SHRT_CURSOR_STEP_UP].key) {
            int newIndex=stepLenWidget->currentIndex()+1;
            if (newIndex > stepLenWidget->count()-1)
              newIndex=stepLenWidget->count()-1;
            stepLenWidget->setCurrentIndex(newIndex);
            return;
      }
      else if (key == Qt::Key_F2) {
            dlist->lineEdit(dlist->getSelectedInstrument(),(int)DList::COL_NAME);
            return;
            }
      else if (key == shortcuts[SHRT_INSTRUMENT_STEP_UP].key) {
            dlist->setCurDrumInstrument(dlist->getSelectedInstrument()-1);
            dlist->redraw();
            ((DrumCanvas*)canvas)->selectCursorEvent(((DrumCanvas*)canvas)->getEventAtCursorPos());
            return;
            }
      else if (key == shortcuts[SHRT_INSTRUMENT_STEP_DOWN].key) {
            dlist->setCurDrumInstrument(dlist->getSelectedInstrument()+1);
            dlist->redraw();
            ((DrumCanvas*)canvas)->selectCursorEvent(((DrumCanvas*)canvas)->getEventAtCursorPos());
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

      else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key) {
            dc->cmd(DrumCanvas::CMD_RIGHT_NOSNAP);
            return;
            }
      else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key) {
            dc->cmd(DrumCanvas::CMD_LEFT_NOSNAP);
            return;
            }

      else if (key == shortcuts[SHRT_TOOL_POINTER].key) {
            tools2->set(MusEGui::PointerTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PENCIL].key) {
            tools2->set(MusEGui::PencilTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_RUBBER].key) {
            tools2->set(MusEGui::RubberTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_CURSOR].key) {
            tools2->set(MusEGui::CursorTool);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_IN].key) {
            int mag = hscroll->mag();
            int zoomlvl = MusEGui::ScrollScale::getQuickZoomLevel(mag);
            if (zoomlvl < 23)
                  zoomlvl++;

            int newmag = MusEGui::ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

            hscroll->setMag(newmag);
            //printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
            int mag = hscroll->mag();
            int zoomlvl = MusEGui::ScrollScale::getQuickZoomLevel(mag);
            if (zoomlvl > 1)
                  zoomlvl--;

            int newmag = MusEGui::ScrollScale::convertQuickZoomLevelToMag(zoomlvl);
            hscroll->setMag(newmag);
            //printf("mag = %d zoomlvl = %d newmag = %d\n", mag, zoomlvl, newmag);
            return;
            }
      else if (key == shortcuts[SHRT_SCROLL_LEFT].key) {
            int pos = hscroll->pos() - MusEGlobal::config.division;
            if (pos < 0)
                  pos = 0;
            hscroll->setPos(pos);
            return;
            }
      else if (key == shortcuts[SHRT_SCROLL_RIGHT].key) {
            int pos = hscroll->pos() + MusEGlobal::config.division;
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
      setRaster(val);
      toolbar->setRaster(_raster);
      }



//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void DrumEdit::initShortcuts()
      {
      loadAction->setShortcut(shortcuts[SHRT_OPEN].key);
      saveAction->setShortcut(shortcuts[SHRT_SAVE].key);

      cutAction->setShortcut(shortcuts[SHRT_CUT].key);
      copyAction->setShortcut(shortcuts[SHRT_COPY].key);
      copyRangeAction->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
      pasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      pasteDialogAction->setShortcut(shortcuts[SHRT_PASTE_DIALOG].key);
      deleteAction->setShortcut(shortcuts[SHRT_DELETE].key);

      fixedAction->setShortcut(shortcuts[SHRT_FIXED_LEN].key);
      veloAction->setShortcut(shortcuts[SHRT_MODIFY_VELOCITY].key);

      sallAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
      snoneAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      invAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
      inAction->setShortcut(shortcuts[SHRT_SELECT_ILOOP].key);
      outAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
      
      prevAction->setShortcut(shortcuts[SHRT_SELECT_PREV_PART].key);
      nextAction->setShortcut(shortcuts[SHRT_SELECT_NEXT_PART].key);
      }

//---------------------------------------------------------
//   execDeliveredScript
//---------------------------------------------------------
void DrumEdit::execDeliveredScript(int id)
{
      //QString scriptfile = QString(INSTPREFIX) + SCRIPTSSUFFIX + deliveredScriptNames[id];
      QString scriptfile = MusEGlobal::song->getScriptPath(id, true);
      MusEGlobal::song->executeScript(scriptfile.toLatin1().constData(), parts(), raster(), true);
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void DrumEdit::execUserScript(int id)
{
      QString scriptfile = MusEGlobal::song->getScriptPath(id, false);
      MusEGlobal::song->executeScript(scriptfile.toLatin1().constData(), parts(), raster(), true);
}

void DrumEdit::setStep(QString v)
{
  ((DrumCanvas*)canvas)->setStep(v.toInt());
  stepLenWidget->setFocusPolicy(Qt::NoFocus);
  canvas->setFocus();
}

} // namespace MusEGui
