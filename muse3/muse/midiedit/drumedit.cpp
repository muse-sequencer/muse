//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.cpp,v 1.22.2.21 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <QCursor>
#include <QPoint>
#include <QRect>
#include <QMimeData>

#include "globaldefs.h"
#include "drumedit.h"
#include "dcanvas.h"
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
#include "app.h"
#include "icons.h"
#include "filedialog.h"
#include "drummap.h"
#include "audio.h"
#include "gconfig.h"
#include "functions.h"
#include "helper.h"
#include "popupmenu.h"
#include "menutitleitem.h"
#include "widgets/function_dialogs/quantize.h"

namespace MusEGui {

int DrumEdit::_rasterInit = 96;
int DrumEdit::_dlistWidthInit = 50;
int DrumEdit::_dcanvasWidthInit = 300;
bool DrumEdit::_ignore_hide_init = false;

static const int xscale = -10;
static const int yscale = 1;
static const int drumeditTools = MusEGui::PointerTool | MusEGui::PencilTool | MusEGui::RubberTool | MusEGui::CursorTool | MusEGui::DrawTool | PanTool | ZoomTool;


//---------------------------------------------------------
//   setHeaderWhatsThis
//---------------------------------------------------------

void DrumEdit::setHeaderWhatsThis()
      {
      header->setWhatsThis(COL_HIDE, tr("hide instrument"));
      header->setWhatsThis(COL_MUTE, tr("mute instrument"));
      header->setWhatsThis(COL_NAME, tr("sound name"));
      header->setWhatsThis(COL_VOLUME, tr("volume percent"));
      header->setWhatsThis(COL_QUANT, tr("quantisation"));
      header->setWhatsThis(COL_INPUTTRIGGER, tr("this input note triggers the sound"));
      header->setWhatsThis(COL_NOTELENGTH, tr("note length"));
      header->setWhatsThis(COL_NOTE, tr("this is the note which is played"));
      header->setWhatsThis(COL_OUTCHANNEL, tr("override track output channel (hold ctl to affect all rows)"));
      header->setWhatsThis(COL_OUTPORT, tr("override track output port (hold ctl to affect all rows)"));
      header->setWhatsThis(COL_LEVEL1, tr("control + meta keys: draw velocity level 1"));
      header->setWhatsThis(COL_LEVEL2, tr("meta key: draw velocity level 2"));
      header->setWhatsThis(COL_LEVEL3, tr("draw default velocity level 3"));
      header->setWhatsThis(COL_LEVEL4, tr("meta + alt keys: draw velocity level 4"));
      }

//---------------------------------------------------------
//   setHeaderToolTips
//---------------------------------------------------------

void DrumEdit::setHeaderToolTips()
      {
      header->setToolTip(COL_HIDE, tr("hide instrument"));
      header->setToolTip(COL_MUTE, tr("mute instrument"));
      header->setToolTip(COL_NAME, tr("sound name"));
      header->setToolTip(COL_VOLUME, tr("volume percent"));
      header->setToolTip(COL_QUANT, tr("quantisation"));
      header->setToolTip(COL_INPUTTRIGGER, tr("this input note triggers the sound"));
      header->setToolTip(COL_NOTELENGTH, tr("note length"));
      header->setToolTip(COL_NOTE, tr("this is the note which is played"));
      header->setToolTip(COL_OUTCHANNEL, tr("override track output channel (ctl: affect all rows)"));
      header->setToolTip(COL_OUTPORT, tr("override track output port (ctl: affect all rows)"));
      header->setToolTip(COL_LEVEL1, tr("control + meta keys: draw velocity level 1"));
      header->setToolTip(COL_LEVEL2, tr("meta key: draw velocity level 2"));
      header->setToolTip(COL_LEVEL3, tr("draw default velocity level 3"));
      header->setToolTip(COL_LEVEL4, tr("meta + alt keys: draw velocity level 4"));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void DrumEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      QSettings settings("MusE", "MusE-qt");
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
      setFocusPolicy(Qt::NoFocus);  

      deltaMode     = false;
      tickValue     = 0;
      lenValue      = 0;
      pitchValue    = 0;
      // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//       veloOnValue   = 0;
      veloOnValue   = 1;
      veloOffValue  = 0;
      firstValueSet = false;
      tickOffset    = 0;
      lenOffset     = 0;
      pitchOffset   = 0;
      veloOnOffset  = 0;
      veloOffOffset = 0;
      lastSelections = 0;
      split1w1 = 0;
      //selPart  = 0;
      _playEvents    = true;
      QSignalMapper *signalMapper = new QSignalMapper(this);
      
      _group_mode = GROUP_SAME_CHANNEL;
      _ignore_hide = _ignore_hide_init;
      
      //---------Pulldown Menu----------------------------
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

      
      
      // throw out new-style and midi tracks if there are old-style tracks present
      bool has_old_style_tracks=false;
      for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
        if (p->second->track()->type()==MusECore::Track::DRUM)
        {
          has_old_style_tracks=true;
          break;
        }
      
      if (has_old_style_tracks)
      {
        bool thrown_out=false;
        bool again;
        do
        {
          again=false;
          for (MusECore::ciPart p = parts()->begin(); p != parts()->end();p++)
            if (p->second->track()->type()!=MusECore::Track::DRUM)
            {
              parts()->remove(p->second);
              thrown_out=true;
              again=true;
              break;
            }
        } while (again);
      
        if (thrown_out)
        {
          QTimer* timer = new QTimer(this);
          timer->setSingleShot(true);
          connect(timer,SIGNAL(timeout()), this, SLOT(display_old_new_conflict_message()));
          timer->start(10);
        }
      }

      _old_style_drummap_mode=has_old_style_tracks;
      
      

      
      if (old_style_drummap_mode())
      {
        loadAction = menuFunctions->addAction(QIcon(*openIcon), tr("Load Map"));
        saveAction = menuFunctions->addAction(QIcon(*saveIcon), tr("Save Map"));
        resetAction = menuFunctions->addAction(tr("Reset GM Map"));

        connect(loadAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        connect(saveAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        connect(resetAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

        signalMapper->setMapping(loadAction, DrumCanvas::CMD_LOAD);
        signalMapper->setMapping(saveAction, DrumCanvas::CMD_SAVE);
        signalMapper->setMapping(resetAction, DrumCanvas::CMD_RESET);

        QAction* reorderListAction = menuFunctions->addAction(tr("Re-order map"));
        connect(reorderListAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        signalMapper->setMapping(reorderListAction, DrumCanvas::CMD_REORDER_LIST);
        menuFunctions->addSeparator();
      }
      else
        loadAction=saveAction=resetAction=NULL;

      fixedAction = menuFunctions->addAction(tr("Set Fixed Length"));
      veloAction = menuFunctions->addAction(tr("Modify Velocity"));
      crescAction = menuFunctions->addAction(tr("Crescendo/Decrescendo"));
      quantizeAction = menuFunctions->addAction(tr("Quantize"));
      QAction* eraseEventAction = menuFunctions->addAction(tr("Erase Event"));
      QAction* noteShiftAction = menuFunctions->addAction(tr("Move Notes"));
      QAction* delOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));

      connect(fixedAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(veloAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(crescAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(quantizeAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(eraseEventAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(noteShiftAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
      connect(delOverlapsAction, SIGNAL(triggered()), signalMapper, SLOT(map()));

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
      if (!old_style_drummap_mode())
      {
        QMenu* menuGrouping=settingsMenu->addMenu(tr("Group"));
        groupNoneAction = menuGrouping->addAction(tr("Don't group"));
        groupChanAction = menuGrouping->addAction(tr("Group by channel"));
        groupMaxAction  = menuGrouping->addAction(tr("Group maximally"));
        QMenu* menuShowHide=settingsMenu->addMenu(tr("Show/Hide"));
        QAction* ignoreHideAction = menuShowHide->addAction(tr("Also show hidden instruments"));
        menuShowHide->addSeparator();
        QAction* showAllAction = menuShowHide->addAction(tr("Show all instruments"));
        QAction* hideAllAction = menuShowHide->addAction(tr("Hide all instruments"));
        QAction* hideUnusedAction = menuShowHide->addAction(tr("Only show used instruments"));
        QAction* hideEmptyAction = menuShowHide->addAction(tr("Only show instruments with non-empty name or used instruments"));
        settingsMenu->addSeparator();
        
        groupNoneAction->setCheckable(true);
        groupChanAction->setCheckable(true);
        groupMaxAction ->setCheckable(true);
        ignoreHideAction->setCheckable(true);
        ignoreHideAction->setChecked(_ignore_hide);
        
        connect(groupNoneAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        connect(groupChanAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        connect(groupMaxAction,  SIGNAL(triggered()), signalMapper, SLOT(map()));
        connect(ignoreHideAction,  SIGNAL(toggled(bool)), SLOT(set_ignore_hide(bool)));
        connect(showAllAction,  SIGNAL(triggered()), this, SLOT(showAllInstruments()));
        connect(hideAllAction,  SIGNAL(triggered()), this, SLOT(hideAllInstruments()));
        connect(hideUnusedAction,  SIGNAL(triggered()), this, SLOT(hideUnusedInstruments()));
        connect(hideEmptyAction,  SIGNAL(triggered()), this, SLOT(hideEmptyInstruments()));

        signalMapper->setMapping(groupNoneAction, DrumCanvas::CMD_GROUP_NONE);
        signalMapper->setMapping(groupChanAction, DrumCanvas::CMD_GROUP_CHAN);
        signalMapper->setMapping(groupMaxAction,  DrumCanvas::CMD_GROUP_MAX);
        
        updateGroupingActions();
      }
      else
      {
        groupNoneAction=NULL;
        groupChanAction=NULL;
        groupMaxAction =NULL;
      }
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      connect(signalMapper, SIGNAL(mapped(int)), SLOT(cmd(int)));

      //---------------------------------------------------
      //    Toolbars
      //---------------------------------------------------
    
      if (old_style_drummap_mode())
      {
        QToolBar* maptools = addToolBar(tr("Drum map tools"));
        maptools->setObjectName("Drum map tools");
        
        QToolButton *ldm = new QToolButton();
        ldm->setToolTip(tr("Load Drummap"));
        ldm->setIcon(*openIcon);
        ldm->setFocusPolicy(Qt::NoFocus);
        connect(ldm, SIGNAL(clicked()), SLOT(load()));
        maptools->addWidget(ldm);
        
        QToolButton *sdm = new QToolButton();
        sdm->setToolTip(tr("Store Drummap"));
        sdm->setIcon(*saveIcon);
        sdm->setFocusPolicy(Qt::NoFocus);
        connect(sdm, SIGNAL(clicked()), SLOT(save()));
        maptools->addWidget(sdm);
        
        maptools->addAction(QWhatsThis::createAction());
      }


      tools = addToolBar(tr("Drum tools"));
      tools->setObjectName("Drum tools");

      srec  = new QToolButton();
      srec->setToolTip(tr("Step Record"));
      srec->setIcon(*steprecIcon);
      srec->setCheckable(true);
      srec->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(srec);

      midiin  = new QToolButton();
      midiin->setToolTip(tr("Midi Input"));
      midiin->setIcon(*midiinIcon);
      midiin->setCheckable(true);
      midiin->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(midiin);
      
      speaker  = new QToolButton();
      speaker->setToolTip(tr("Play Events"));
      speaker->setIcon(*speakerIcon);
      speaker->setCheckable(true);
      speaker->setChecked(true);
      speaker->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(speaker);

      tools->addAction(QWhatsThis::createAction(this));

      tools2 = new MusEGui::EditToolBar(this, drumeditTools);
      addToolBar(tools2);

      QToolBar* cursorToolbar = addToolBar(tr("cursor tools"));
      cursorToolbar->setObjectName("cursor");
      QLabel *stepStr = new QLabel(tr("Cursor step:"));
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
      stepLenWidget->setFocusPolicy(Qt::TabFocus);
      connect(stepLenWidget, SIGNAL(currentIndexChanged(QString)), SLOT(setStep(QString)));
      cursorToolbar->addWidget(stepLenWidget);

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
      ctrl = new QPushButton(tr("ctrl"), mainw);
      ctrl->setObjectName("Ctrl");
      ctrl->setFont(MusEGlobal::config.fonts[3]);
      ctrl->setFocusPolicy(Qt::NoFocus);
      // Increased scale to -1. To resolve/select/edit 1-tick-wide (controller graph) events. 
      hscroll           = new MusEGui::ScrollScale(-25, -1 /* formerly -2 */, xscale, 20000, Qt::Horizontal, mainw);
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
      vscroll             = new MusEGui::ScrollScale(-2, 1, yscale, dynamic_cast<DrumCanvas*>(canvas)->getOurDrumMapSize()*TH, Qt::Vertical, split1w2);
      int offset = -(MusEGlobal::config.division/4);
      canvas->setOrigin(offset, 0);
      canvas->setCanvasTools(drumeditTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(canvas, SIGNAL(horizontalZoom(bool,const QPoint&)), SLOT(horizontalZoom(bool, const QPoint&)));
      connect(canvas, SIGNAL(horizontalZoom(int, const QPoint&)), SLOT(horizontalZoom(int, const QPoint&)));
      connect(canvas, SIGNAL(ourDrumMapChanged(bool)), SLOT(ourDrumMapChanged(bool)));
      time->setOrigin(offset, 0);

      QList<int> mops;
      mops.append(_dlistWidthInit);
      mops.append(_dcanvasWidthInit);
      split2->setSizes(mops);
      // By T356. Not much choice but to disable this for now, to stop runaway resize bug.
      // Can't seem to get the splitter to readjust when manually setting sizes.
      //split2->setResizeMode(split1w1, QSplitter::KeepSize); DELETETHIS or FIXME?

      gridS2->setRowStretch(1, 100);
      gridS2->setColumnStretch(0, 100);
      
      gridS2->addWidget(time,  0, 0, 1, 2);
      gridS2->addWidget(MusECore::hLine(split1w2), 1, 0, 1, 2);
      gridS2->addWidget(canvas,  2, 0);
      
      gridS2->addWidget(vscroll, 2, 1);

      //  Ordering is hardcoded in dlist.c ("Dcols")
      header = new MusEGui::Header(split1w1, "header");
      header->setFixedHeight(31);
      header->setColumnLabel(tr("H"), COL_HIDE, 20);
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

      if (!old_style_drummap_mode())
      {
        header->hideSection(COL_OUTPORT);
        header->hideSection(COL_OUTCHANNEL);
      }
      
      if (!old_style_drummap_mode() && _ignore_hide)
        header->showSection(COL_HIDE);
      else
        header->hideSection(COL_HIDE);

      dlist = new DList(header, split1w1, yscale, (DrumCanvas*)canvas, old_style_drummap_mode());
      setCurDrumInstrument(dlist->getSelectedInstrument());
      
      connect(dlist, SIGNAL(keyPressed(int, int)), canvas, SLOT(keyPressed(int, int)));
      connect(dlist, SIGNAL(keyReleased(int, bool)), canvas, SLOT(keyReleased(int, bool)));
      connect(dlist, SIGNAL(mapChanged(int, int)), canvas, SLOT(mapChanged(int, int)));
      connect(dlist, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(dlist, SIGNAL(curDrumInstrumentChanged(int)), SLOT(setCurDrumInstrument(int)));
      connect(dlist, SIGNAL(curDrumInstrumentChanged(int)), canvas, SLOT(setCurDrumInstrument(int)));

      gridS1->setRowStretch(1, 100);
      gridS1->setColumnStretch(0, 100);
      gridS1->addWidget(header, 0, 0);
      gridS1->addWidget(dlist, 1, 0);
      
      connect(canvas, SIGNAL(newWidth(int)), SLOT(newCanvasWidth(int)));
      connect(canvas, SIGNAL(verticalScroll(unsigned)), vscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScroll(unsigned)),hscroll, SLOT(setPos(unsigned)));
      connect(canvas,  SIGNAL(horizontalScrollNoLimit(unsigned)),hscroll, SLOT(setPosNoLimit(unsigned))); 
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)), SLOT(songChanged1(MusECore::SongChangedFlags_t)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedFlags_t)),      dlist, SLOT(songChanged(MusECore::SongChangedFlags_t)));
      connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setYMag(int)));
      connect(vscroll, SIGNAL(scaleChanged(int)),   dlist, SLOT(setYMag(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)),  canvas, SLOT(setXMag(int)));
      connect(srec, SIGNAL(toggled(bool)),         canvas, SLOT(setSteprec(bool)));
      connect(midiin, SIGNAL(toggled(bool)),       canvas, SLOT(setMidiin(bool)));
      connect(speaker, SIGNAL(toggled(bool)),              SLOT(setSpeaker(bool)));

      connect(vscroll, SIGNAL(scrollChanged(int)),   dlist,   SLOT(setYPos(int)));
      connect(hscroll, SIGNAL(scrollChanged(int)),   time,   SLOT(setXPos(int)));
      connect(hscroll, SIGNAL(scaleChanged(int)), time,   SLOT(setXMag(int)));

      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));  // in Canvas
      connect(tools2, SIGNAL(toolChanged(int)), canvas, SLOT(setTool2(int))); // in DrumCanvas

      connect(canvas, SIGNAL(selectionChanged(int, MusECore::Event&, MusECore::Part*, bool)), this,
         SLOT(setSelection(int, MusECore::Event&, MusECore::Part*, bool)));
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
      connect(info, SIGNAL(deltaModeChanged(bool)), SLOT(deltaModeChanged(bool)));
      connect(info, SIGNAL(returnPressed()), SLOT(focusCanvas()));
      connect(info, SIGNAL(escapePressed()), SLOT(focusCanvas()));
      
      connect(ctrl, SIGNAL(clicked()), SLOT(addCtrlClicked()));

      connect(MusEGlobal::song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int,int)));

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
      if(initPos >= INT_MAX)
        pos = MusEGlobal::song->cpos();
      if(pos > INT_MAX)
        pos = INT_MAX;
      if (pos)
        hscroll->setOffset((int)pos);

      if(canvas->track())
        toolbar->setSolo(canvas->track()->solo());
      
      
      initTopwinState();
      finalizeInit();
      }

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void DrumEdit::songChanged1(MusECore::SongChangedFlags_t bits)
      {
        if(_isDeleting)  // Ignore while deleting to prevent crash.
          return;
        
        if (bits & SC_SOLO)
        {
            toolbar->setSolo(canvas->track()->solo());
            return;
        }      
        if ( !old_style_drummap_mode() && 
             ( bits & (SC_DRUMMAP | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                       SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED) ) )
          ((DrumCanvas*)(canvas))->rebuildOurDrumMap();
        
        songChanged(bits);
      }

//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------
void DrumEdit::midiNote(int pitch, int velo)
{
  //if (debugMsg)
      printf("DrumEdit::midiNote: pitch=%i, velo=%i\n", pitch, velo);
  int index=0;

  //      *note = old_style_drummap_mode ? ourDrumMap[index].anote : instrument_map[index].pitch;

  if ((DrumCanvas*)(canvas)->midiin())
  {
    if (old_style_drummap_mode()) {
        MusECore::DrumMap *dmap= ((DrumCanvas*)canvas)->getOurDrumMap();
        for (index = 0; index < ((DrumCanvas*)canvas)->getOurDrumMapSize(); ++index) {

          if ((&dmap[index])->anote == pitch)
            break;
        }

    }
    else
    {
        for (index = 0; index < get_instrument_map().size(); ++index) {
          if (get_instrument_map().at(index).pitch == pitch)
            break;
        }
    }
    dlist->setCurDrumInstrument(index);
  }
}
//---------------------------------------------------------
//   horizontalZoom
//---------------------------------------------------------

void DrumEdit::horizontalZoom(bool zoom_in, const QPoint& glob_pos)
{
  int mag = hscroll->mag();
  int zoomlvl = MusEGui::ScrollScale::getQuickZoomLevel(mag);
  if(zoom_in)
  {
    if (zoomlvl < MusEGui::ScrollScale::zoomLevels-1)
        zoomlvl++;
  }
  else
  {
    if (zoomlvl > 1)
        zoomlvl--;
  }
  int newmag = MusEGui::ScrollScale::convertQuickZoomLevelToMag(zoomlvl);

  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = split1->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < split1->height())
    hscroll->setMag(newmag, cp.x());
}

void DrumEdit::horizontalZoom(int mag, const QPoint& glob_pos)
{
  QPoint cp = canvas->mapFromGlobal(glob_pos);
  QPoint sp = split1->mapFromGlobal(glob_pos);
  if(cp.x() >= 0 && cp.x() < canvas->width() && sp.y() >= 0 && sp.y() < split1->height())
    hscroll->setMag(hscroll->mag() + mag, cp.x());
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
      }

//---------------------------------------------------------
//   setSelection
//    update Info Line
//---------------------------------------------------------

void DrumEdit::setSelection(int tick, MusECore::Event& e, MusECore::Part*, bool update)
      {
      int selections = canvas->selectionSize();

      // Diagnostics:
      //printf("DrumEdit::setSelection selections:%d event empty:%d firstValueSet:%d\n", selections, e.empty(), firstValueSet); 
      //if(!e.empty())
      //  e.dump(); 
      
      if(update)
      {
        // Selections have changed. Reset these.
        tickOffset    = 0;
        lenOffset     = 0;
        pitchOffset   = 0;
        veloOnOffset  = 0;
        veloOffOffset = 0;
        
        // Force 'suggested' modes:
        if (selections == 1) 
        {
          deltaMode = false;
          info->setDeltaMode(deltaMode); 
        }
        else
        if (selections > 1)
        {
          // A feeble attempt to hold on to the user's setting. Should try to bring back 
          //  selEvent (removed), but there were problems using it (it's a reference).
          //if(lastSelections <= 1) 
          {
            deltaMode = true;
            info->setDeltaMode(deltaMode); 
          }
        }
      }
        
      lastSelections = selections;
      
      if ((selections == 1) || (selections > 1 && !firstValueSet)) 
      {
        tickValue    = tick; 
        lenValue     = e.lenTick();
        pitchValue   = e.pitch();
        veloOnValue  = e.velo();
        // REMOVE Tim. Noteoff. Added. Zero note on vel is not allowed now.
        if(veloOnValue == 0)
        {
          veloOnValue = 1;
          fprintf(stderr, "DrumEdit::setSelection: Warning: Zero note on velocity!\n");
        }
        veloOffValue = e.veloOff();
        firstValueSet = true;
      }

      if (selections > 0) {
            info->setEnabled(true);
            if (deltaMode) 
              info->setValues(tickOffset, lenOffset, pitchOffset, veloOnOffset, veloOffOffset);
            else  
              info->setValues(tickValue, lenValue, pitchValue, veloOnValue, veloOffValue);
            }
      else {
            info->setEnabled(false);
            // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//             info->setValues(0, 0, 0, 0, 0);
            info->setValues(0, 0, 0, deltaMode ? 0 : 1, 0);
            firstValueSet = false;
            tickValue     = 0;
            lenValue      = 0;
            pitchValue    = 0;
            // REMOVE Tim. Noteoff. Changed. Zero note on vel is not allowed now.
//             veloOnValue   = 0;
            veloOnValue   = 1;
            veloOffValue  = 0;
            tickOffset    = 0;
            lenOffset     = 0;
            pitchOffset   = 0;
            veloOnOffset  = 0;
            veloOffOffset = 0;
            }
            
      info->setReturnMode(selections >= 2);      
      selectionChanged();
      }

//---------------------------------------------------------
//   focusCanvas
//---------------------------------------------------------

void DrumEdit::focusCanvas()
{
  if(MusEGlobal::config.smartFocus)
  {
    canvas->setFocus();
    canvas->activateWindow();
  }
}

//---------------------------------------------------------
//   deltaModeChanged
//---------------------------------------------------------

void DrumEdit::deltaModeChanged(bool delta_on)
{
      if(deltaMode == delta_on)
        return;
      deltaMode = delta_on;
      
      int selections = canvas->selectionSize();
      
      if(deltaMode)
      {
        if(selections > 0)
          info->setValues(tickOffset, lenOffset, pitchOffset, veloOnOffset, veloOffOffset);
      }
      else
      {
        if(selections > 0)
          info->setValues(tickValue, lenValue, pitchValue, veloOnValue, veloOffValue);
      }
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
      focusCanvas();     // give back focus after kb input
      }

//---------------------------------------------------------
//    edit currently selected Event
//---------------------------------------------------------

void DrumEdit::noteinfoChanged(MusEGui::NoteInfo::ValType type, int val)
      {
      int selections = canvas->selectionSize();

      if (selections == 0) {
            printf("noteinfoChanged while nothing selected\n");
            }
      else if (selections > 0) {
            if(deltaMode) {
                  // treat noteinfo values as offsets to event values
                  int delta = 0;
                  switch (type) {
                        case MusEGui::NoteInfo::VAL_TIME:
                              delta = val - tickOffset;
                              tickOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_LEN:
                              delta = val - lenOffset;
                              lenOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_VELON:
                              delta = val - veloOnOffset;
                              veloOnOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_VELOFF:
                              delta = val - veloOffOffset;
                              veloOffOffset = val;
                              break;
                        case MusEGui::NoteInfo::VAL_PITCH:
                              delta = val - pitchOffset;
                              pitchOffset = val;
                              break;
                        }
                  if (delta)
                        canvas->modifySelected(type, delta);
                  }
            else {
                      switch (type) {
                            case MusEGui::NoteInfo::VAL_TIME:
                                  tickValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_LEN:
                                  lenValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_VELON:
                                  veloOnValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_VELOFF:
                                  veloOffValue = val;
                                  break;
                            case MusEGui::NoteInfo::VAL_PITCH:
                                  pitchValue = val;
                                  break;
                            }
                      canvas->modifySelected(type, val, false); // No delta mode.
                 }
            }
      
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
      xml.intTag(level, "tool", int(canvas->tool()));
      xml.intTag(level, "playEvents", _playEvents);
      xml.intTag(level, "xmag", hscroll->mag());
      xml.intTag(level, "xpos", hscroll->pos());
      xml.intTag(level, "ymag", vscroll->mag());
      xml.intTag(level, "ypos", vscroll->pos());
      xml.intTag(level, "ignore_hide", _ignore_hide);
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
                        else if (tag == "tool") {
                              int tool = xml.parseInt();
                              canvas->setTool(tool);
                              tools2->set(tool);
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
                        else if (tag == "playEvents") {
                              _playEvents = xml.parseInt();
                              canvas->playEvents(_playEvents);
                              speaker->setChecked(_playEvents);
                              }
                        else if (tag == "xmag")
                              hscroll->setMag(xml.parseInt());
                        else if (tag == "xpos")
                              hscroll->setPos(xml.parseInt());
                        else if (tag == "ymag")
                              vscroll->setMag(xml.parseInt());
                        else if (tag == "ypos")
                              vscroll->setPos(xml.parseInt());
                        else if (tag == "ignore_hide")
                              _ignore_hide=xml.parseInt();
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
                        else if (tag == "ignore_hide_init")
                              _ignore_hide_init = xml.parseInt();
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
      xml.intTag(level, "ignore_hide_init", _ignore_hide_init);
      TopWin::writeConfiguration(DRUM, level,xml);
      xml.tag(level, "/drumedit");
      }

//---------------------------------------------------------
//   load
//---------------------------------------------------------

void DrumEdit::load()
      {
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
      // Don't process if user is dragging or has clicked on the events. 
      // Causes crashes later in Canvas::viewMouseMoveEvent and viewMouseReleaseEvent.
      if(canvas->getCurrentDrag())
        return;
      
      switch(cmd) {
            case DrumCanvas::CMD_CUT:
                  copy_notes(partlist_to_set(parts()), 1);
                  erase_notes(partlist_to_set(parts()), 1);
                  break;
            case DrumCanvas::CMD_COPY: copy_notes(partlist_to_set(parts()), 1); break;
            case DrumCanvas::CMD_COPY_RANGE: copy_notes(partlist_to_set(parts()), MusECore::any_event_selected(partlist_to_set(parts())) ? 3 : 2); break;
            case DrumCanvas::CMD_PASTE: 
                  ((DrumCanvas*)canvas)->cmd(DrumCanvas::CMD_SELECT_NONE);
                  MusECore::paste_notes(3072, false, true, canvas->part());
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
                                       /* quant_len= */false, quantize_dialog->strength,  // DELETETHIS
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
            
            case DrumCanvas::CMD_GROUP_NONE: _group_mode=DONT_GROUP; updateGroupingActions(); ((DrumCanvas*)(canvas))->rebuildOurDrumMap(); break;
            case DrumCanvas::CMD_GROUP_CHAN: _group_mode=GROUP_SAME_CHANNEL; updateGroupingActions(); ((DrumCanvas*)(canvas))->rebuildOurDrumMap(); break;
            case DrumCanvas::CMD_GROUP_MAX: _group_mode=GROUP_MAX; updateGroupingActions(); ((DrumCanvas*)(canvas))->rebuildOurDrumMap(); break;
            
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
//   ctrlPopupTriggered
//---------------------------------------------------------

void DrumEdit::ctrlPopupTriggered(QAction* act)
{
  // TODO Merge most of this with duplicate code in piano roll,
  //       maybe by putting it in a new function near populateMidiCtrlMenu. 
  
  if(!act || (act->data().toInt() == -1))
    return;
  
  int newCtlNum = -1;
  MusECore::Part* part       = curCanvasPart();
  MusECore::MidiTrack* track = (MusECore::MidiTrack*)(part->track());
  int channel      = track->outChannel();
  MusECore::MidiPort* port   = &MusEGlobal::midiPorts[track->outPort()];
  MusECore::MidiCtrlValListList* cll = port->controller();
  const int min = channel << 24;
  const int max = min + 0x1000000;
  const int edit_ins = max + 3;
  const int velo = max + 0x101;
  int rv = act->data().toInt();
  
  if (rv == velo) {    // special case velocity
        newCtlNum = MusECore::CTRL_VELOCITY;
        }
  else if (rv == edit_ins) {             // edit instrument
        MusECore::MidiInstrument* instr = port->instrument();
        MusEGlobal::muse->startEditInstrument(instr ? instr->iname() : QString(), EditInstrumentControllers);
        }
  else {                           // Select a control
        if(cll->find(channel, rv) == cll->end())
          cll->add(channel, new MusECore::MidiCtrlValList(rv));
        newCtlNum = rv;
        if(port->drumController(rv))
          newCtlNum |= 0xff;
      }

  if(newCtlNum != -1)
  {
    CtrlEdit* ctrlEdit = new CtrlEdit(split1, this, xscale, true, "drumCtrlEdit");
    ctrlEdit->setController(newCtlNum);
    setupNewCtrl(ctrlEdit);
  }
}

//---------------------------------------------------------
//   addCtrlClicked
//---------------------------------------------------------

void DrumEdit::addCtrlClicked()
{
  PopupMenu* pup = new PopupMenu(true);  // true = enable stay open. Don't bother with parent. 
  connect(pup, SIGNAL(triggered(QAction*)), SLOT(ctrlPopupTriggered(QAction*)));

  int cur_instr = curDrumInstrument();
  // HACK! New drum ctrl canvas current drum index is not the same as the editor current drum index.
  //       Should try to fix this situation - two different values exist. Tim.
  if(!old_style_drummap_mode())
    cur_instr = (cur_instr & ~0xff) | get_instrument_map()[cur_instr].pitch;
  
  int est_width = populateMidiCtrlMenu(pup, parts(), curCanvasPart(), cur_instr);
  
  QPoint ep = ctrl->mapToGlobal(QPoint(0,0));
  //int newx = ep.x() - pup->width();  // Too much! Width says 640. Maybe because it hasn't been shown yet  .
  int newx = ep.x() - est_width;  
  if(newx < 0)
    newx = 0;
  ep.setX(newx);
  pup->exec(ep);
  delete pup;

  ctrl->setDown(false);
}

//---------------------------------------------------------
//   addCtrl
//---------------------------------------------------------

CtrlEdit* DrumEdit::addCtrl(int ctl_num)
      {
      CtrlEdit* ctrlEdit = new CtrlEdit(split1, this, xscale, true, "drumCtrlEdit");
      ctrlEdit->setController(ctl_num);
      setupNewCtrl(ctrlEdit);
      return ctrlEdit;
      }

//---------------------------------------------------------
//   setupNewCtrl
//---------------------------------------------------------

void DrumEdit::setupNewCtrl(CtrlEdit* ctrlEdit)
{      
      connect(hscroll,  SIGNAL(scrollChanged(int)), ctrlEdit, SLOT(setXPos(int)));
      connect(hscroll,  SIGNAL(scaleChanged(int)),  ctrlEdit, SLOT(setXMag(int)));
      connect(ctrlEdit, SIGNAL(timeChanged(unsigned)),   SLOT(setTime(unsigned)));
      connect(ctrlEdit, SIGNAL(destroyedCtrl(CtrlEdit*)), SLOT(removeCtrl(CtrlEdit*)));
      connect(ctrlEdit, SIGNAL(yposChanged(int)), toolbar, SLOT(setInt(int)));
      connect(ctrlEdit, SIGNAL(redirectWheelEvent(QWheelEvent*)), canvas, SLOT(redirectedWheelEvent(QWheelEvent*)));
      connect(tools2,   SIGNAL(toolChanged(int)),   ctrlEdit, SLOT(setTool(int)));
      connect(canvas,   SIGNAL(curPartHasChanged(MusECore::Part*)), ctrlEdit, SLOT(curPartHasChanged(MusECore::Part*)));

      setCurDrumInstrument(dlist->getSelectedInstrument());

      ctrlEdit->setTool(tools2->curTool());
      ctrlEdit->setXPos(hscroll->pos());
      ctrlEdit->setXMag(hscroll->getScaleValue());

      if(split1w1)
      {
        split2->setCollapsible(split2->indexOf(split1w1), false);
        split1w1->setMinimumWidth(CTRL_PANEL_FIXED_WIDTH);
      }

      int dw = vscroll->width() - 18;// 18 is the fixed width of the CtlEdit VScale widget.
      if(dw < 1)
        dw = 1;
      ctrlEdit->setCanvasWidth(canvas->width() + dw);
      
      ctrlEdit->show();
      ctrlEditList.push_back(ctrlEdit);
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
         i != ctrlEditList.end(); ++i)
            (*i)->setCanvasWidth(nw);
            
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
            dlist->lineEdit(dlist->getSelectedInstrument(),(int)COL_NAME);
            return;
            }
      else if (key == shortcuts[SHRT_INSTRUMENT_STEP_UP].key) {
            dlist->setCurDrumInstrument(dlist->getSelectedInstrument()-1);
            dlist->redraw();
            ((DrumCanvas*)canvas)->selectCursorEvent(((DrumCanvas*)canvas)->getEventAtCursorPos());
            ((DrumCanvas*)canvas)->keyPressed(dlist->getSelectedInstrument(),100);

            MusEGlobal::song->update(SC_DRUMMAP);
            return;
            }
      else if (key == shortcuts[SHRT_INSTRUMENT_STEP_DOWN].key) {
            dlist->setCurDrumInstrument(dlist->getSelectedInstrument()+1);
            dlist->redraw();
            ((DrumCanvas*)canvas)->selectCursorEvent(((DrumCanvas*)canvas)->getEventAtCursorPos());
            ((DrumCanvas*)canvas)->keyPressed(dlist->getSelectedInstrument(),100);
            MusEGlobal::song->update(SC_DRUMMAP);
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
      else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key) {
            tools2->set(MusEGui::DrawTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_CURSOR].key) {
            tools2->set(MusEGui::CursorTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_PAN].key) {
            tools2->set(MusEGui::PanTool);
            return;
            }
      else if (key == shortcuts[SHRT_TOOL_ZOOM].key) {
            tools2->set(MusEGui::ZoomTool);
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_IN].key) {
            horizontalZoom(true, QCursor::pos());
            return;
            }
      else if (key == shortcuts[SHRT_ZOOM_OUT].key) {
            horizontalZoom(false, QCursor::pos());
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
      else if (key == shortcuts[SHRT_INSERT_AT_LOCATION].key) { DELETETHIS
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
      else if (key == shortcuts[SHRT_EVENT_COLOR].key) { DELETETHIS
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
      else if (key == shortcuts[SHRT_MOVE_PLAY_TO_NOTE].key){
        movePlayPointerToSelectedEvent();
        return;
      }
      else if (key == shortcuts[SHRT_STEP_RECORD].key) {
          canvas->setSteprec(!srec->isChecked());
          srec->setChecked(!srec->isChecked());
          return;
      }
      else if (key == shortcuts[SHRT_MIDI_INPUT].key) {
          canvas->setMidiin(!midiin->isChecked());
          midiin->setChecked(!midiin->isChecked());
          return;

      }
      else if (key == shortcuts[SHRT_PLAY_EVENTS].key) {
          canvas->playEvents(!speaker->isChecked());
          speaker->setChecked(!speaker->isChecked());
          return;
      }
      else if (key == shortcuts[SHRT_INC_VELOCITY].key) {
          modify_velocity(partlist_to_set(parts()), 1, 100, 1);
          return;
      }
      else if (key == shortcuts[SHRT_DEC_VELOCITY].key) {
          modify_velocity(partlist_to_set(parts()), 1, 100, -1);
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
//   setSpeaker
//---------------------------------------------------------

void DrumEdit::setSpeaker(bool val)
      {
      _playEvents = val;
      canvas->playEvents(_playEvents);
      }

//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void DrumEdit::initShortcuts()
      {
      if (loadAction) loadAction->setShortcut(shortcuts[SHRT_OPEN].key);
      if (saveAction) saveAction->setShortcut(shortcuts[SHRT_SAVE].key);

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
  focusCanvas();
}

void DrumEdit::ourDrumMapChanged(bool instrMapChanged)
{
  if (instrMapChanged)
  {
    int vmin,vmax;
    vscroll->range(&vmin, &vmax);
    vscroll->setRange(vmin, dynamic_cast<DrumCanvas*>(canvas)->getOurDrumMapSize()*TH);
  }
}

void DrumEdit::updateGroupingActions()
{
  if (groupNoneAction==NULL || groupChanAction==NULL || groupMaxAction==NULL)
  {
    printf("THIS SHOULD NEVER HAPPEN: DrumEdit::updateGroupingActions() called, but one of the actions is NULL!\n");
    return;
  }
  
  groupNoneAction->setChecked(_group_mode==DONT_GROUP);
  groupChanAction->setChecked(_group_mode==GROUP_SAME_CHANNEL);
  groupMaxAction ->setChecked(_group_mode==GROUP_MAX);
}

void DrumEdit::set_ignore_hide(bool val)
{
  _ignore_hide=val;
  _ignore_hide_init=val;
  // this may only called be from the action's toggled signal.
  // if called otherwise, the action's checked state isn't updated!

  if (!old_style_drummap_mode() && _ignore_hide)
    header->showSection(COL_HIDE);
  else
    header->hideSection(COL_HIDE);
  
  ((DrumCanvas*)(canvas))->rebuildOurDrumMap();
}

void DrumEdit::showAllInstruments()
{
  using MusECore::MidiTrack;
  
  QSet<MidiTrack*> tracks;
  for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
    tracks.insert((MidiTrack*)p->second->track());
    
  for (QSet<MidiTrack*>::iterator it=tracks.begin(); it!=tracks.end(); it++)
  {
    MidiTrack* track=*it;
    
    for (int i=0;i<128;i++)
      track->drummap_hidden()[i]=false;
  }
  
  MusEGlobal::song->update(SC_DRUMMAP);
}

void DrumEdit::hideAllInstruments()
{
  using MusECore::MidiTrack;
  
  QSet<MidiTrack*> tracks;
  for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
    tracks.insert((MidiTrack*)p->second->track());
    
  for (QSet<MidiTrack*>::iterator it=tracks.begin(); it!=tracks.end(); it++)
  {
    MidiTrack* track=*it;
    
    for (int i=0;i<128;i++)
      track->drummap_hidden()[i]=true;
  }
  
  MusEGlobal::song->update(SC_DRUMMAP);
}

void DrumEdit::hideUnusedInstruments()
{
  using MusECore::MidiTrack;
  using MusECore::ciEvent;
  using MusECore::EventList;
  using MusECore::ciPart;
  
  QSet<MidiTrack*> tracks;
  for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
    tracks.insert((MidiTrack*)p->second->track());
    
  for (QSet<MidiTrack*>::iterator it=tracks.begin(); it!=tracks.end(); it++)
  {
    MidiTrack* track=*it;
    
    bool hide[128];
    for (int i=0;i<128;i++) hide[i]=true;
    
    for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
      if (p->second->track() == track)
      {
        const EventList& el = p->second->events();
        for (ciEvent ev=el.begin(); ev!=el.end(); ev++)
          hide[ev->second.pitch()]=false;
      }
    
    for (int i=0;i<128;i++)
      track->drummap_hidden()[i]=hide[i];
  }
  
  MusEGlobal::song->update(SC_DRUMMAP);
}

void DrumEdit::hideEmptyInstruments()
{
  using MusECore::MidiTrack;
  using MusECore::ciEvent;
  using MusECore::EventList;
  using MusECore::ciPart;
  
  QSet<MidiTrack*> tracks;
  for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
    tracks.insert((MidiTrack*)p->second->track());
    
  for (QSet<MidiTrack*>::iterator it=tracks.begin(); it!=tracks.end(); it++)
  {
    MidiTrack* track=*it;
    
    bool hide[128];
    for (int i=0;i<128;i++) hide[i]=track->drummap()[i].name.isEmpty();
    
    for (MusECore::ciPart p = parts()->begin(); p != parts()->end(); ++p)
      if (p->second->track() == track)
      {
        const EventList& el = p->second->events();
        for (ciEvent ev=el.begin(); ev!=el.end(); ev++)
          hide[ev->second.pitch()]=false;
      }
    
    for (int i=0;i<128;i++)
      track->drummap_hidden()[i]=hide[i];
  }
  
  MusEGlobal::song->update(SC_DRUMMAP);
}


void DrumEdit::display_old_new_conflict_message()
{
  QMessageBox::information(this, tr("Not all parts are displayed"), tr("You selected both old-style-drumtracks and others (that is: new-style or midi tracks), but they cannot displayed in the same drum edit.\nI'll only display the old-style drumtracks in this editor, dropping the others."));
}

} // namespace MusEGui
