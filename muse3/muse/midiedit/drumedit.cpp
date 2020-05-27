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

#include <QClipboard>
#include <QGridLayout>
#include <QKeyEvent>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QSizeGrip>
#include <QWhatsThis>
#include <QSettings>
#include <QCursor>
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
#include "operations.h"
// #include "function_dialogs/quantize.h"
#include "trackinfo_layout.h"
#include "midi_editor_layout.h"

namespace MusEGui {

int DrumEdit::_rasterInit = 96;
int DrumEdit::_trackInfoWidthInit = 50;
int DrumEdit::_canvasWidthInit = 300;
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
      header->setWhatsThis(COL_HIDE, tr("Hide instrument"));
      header->setWhatsThis(COL_MUTE, tr("Mute instrument"));
      header->setWhatsThis(COL_NAME, tr("Sound name"));
      header->setWhatsThis(COL_VOLUME, tr("Volume percent"));
      header->setWhatsThis(COL_QUANT, tr("Quantisation"));
      header->setWhatsThis(COL_INPUTTRIGGER, tr("This input note triggers the sound"));
      header->setWhatsThis(COL_NOTELENGTH, tr("Note length"));
      header->setWhatsThis(COL_NOTE, tr("This is the note which is played"));
      header->setWhatsThis(COL_OUTCHANNEL, tr("Override track output channel (hold ctl to affect all rows)"));
      header->setWhatsThis(COL_OUTPORT, tr("Override track output port (hold ctl to affect all rows)"));
      header->setWhatsThis(COL_LEVEL1, tr("Control + meta keys: Draw velocity level 1"));
      header->setWhatsThis(COL_LEVEL2, tr("Meta key: Draw velocity level 2"));
      header->setWhatsThis(COL_LEVEL3, tr("Draw default velocity level 3"));
      header->setWhatsThis(COL_LEVEL4, tr("Meta + alt keys: Draw velocity level 4"));
      }

//---------------------------------------------------------
//   setHeaderToolTips
//---------------------------------------------------------

void DrumEdit::setHeaderToolTips()
      {
      header->setToolTip(COL_HIDE, tr("Hide instrument"));
      header->setToolTip(COL_MUTE, tr("Mute instrument"));
      header->setToolTip(COL_NAME, tr("Sound name"));
      header->setToolTip(COL_VOLUME, tr("Volume percent"));
      header->setToolTip(COL_QUANT, tr("Quantisation"));
      header->setToolTip(COL_INPUTTRIGGER, tr("This input note triggers the sound"));
      header->setToolTip(COL_NOTELENGTH, tr("Note length"));
      header->setToolTip(COL_NOTE, tr("This is the note which is played"));
      header->setToolTip(COL_OUTCHANNEL, tr("Override track output channel (ctl: affect all rows)"));
      header->setToolTip(COL_OUTPORT, tr("Override track output port (ctl: affect all rows)"));
      header->setToolTip(COL_LEVEL1, tr("Control + meta keys: Draw velocity level 1"));
      header->setToolTip(COL_LEVEL2, tr("Meta key: Draw velocity level 2"));
      header->setToolTip(COL_LEVEL3, tr("Draw default velocity level 3"));
      header->setToolTip(COL_LEVEL4, tr("Meta + alt keys: Draw velocity level 4"));
      }

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void DrumEdit::closeEvent(QCloseEvent* e)
      {
      _isDeleting = true;  // Set flag so certain signals like songChanged, which may cause crash during delete, can be ignored.

      QSettings settings;
      settings.setValue("Drumedit/windowState", saveState());

      //Store values of the horizontal splitter
      QList<int> sizes = split2->sizes();
      QList<int>::iterator it = sizes.begin();
      //There are only 2 values stored in the sizelist, size of dlist widget and dcanvas widget      
      _dlistWidthInit = *it;
      it++;
      _dcanvasWidthInit = *it;
      
      //Store values of the horizontal splitter
      sizes.clear();
      sizes = hsplitter->sizes();
      it = sizes.begin();
      //There are only 2 values stored in the sizelist, size of trackinfo widget and canvas widget
      _trackInfoWidthInit = *it;
      it++;
      _canvasWidthInit = *it;
    
      emit isDeleting(static_cast<TopWin*>(this));
      e->accept();
      }

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

DrumEdit::DrumEdit(MusECore::PartList* pl, QWidget* parent, const char* name, unsigned initPos, bool showDefaultControls)
   : MidiEditor(TopWin::DRUM, _rasterInit, pl, parent, name)
      {
      setFocusPolicy(Qt::NoFocus);  

      deltaMode     = false;
      tickValue     = 0;
      lenValue      = 0;
      pitchValue    = 0;
      // Zero note on vel is not allowed now.
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
      
      _group_mode = GROUP_SAME_CHANNEL;
      _ignore_hide = _ignore_hide_init;
      
      const MusECore::PartList* part_list = parts();
      // Default initial pianoroll view state.
      _viewState = MusECore::MidiPartViewState (0, 0, xscale, yscale);
      // Include a velocity controller in the default initial view state.
      _viewState.addController(MusECore::MidiCtrlViewState(MusECore::CTRL_VELOCITY));
      if(part_list && !part_list->empty())
      {
        // If the parts' view states have never been initialized before,
        //  do it now with the desired pianoroll initial state.
        for(MusECore::ciPart i = part_list->begin(); i != part_list->end(); ++i)
        {
          if(!i->second->viewState().isValid())
            i->second->setViewState(_viewState);
        }
        // Now take our initial view state from the first part found in the list.
        // Don't bother if not showing default controls, since something else
        //  will likely take care of it, like the song file loading routines.
        if(showDefaultControls)
          _viewState = part_list->begin()->second->viewState();
      }
      
      //---------Pulldown Menu----------------------------
      menuEdit = menuBar()->addMenu(tr("&Edit"));
      menuEdit->addActions(MusEGlobal::undoRedo->actions());
      
      menuEdit->addSeparator();
      cutAction = menuEdit->addAction(QIcon(*editcutIconSet), tr("Cut"));
      copyAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy"));
      copyRangeAction = menuEdit->addAction(QIcon(*editcopyIconSet), tr("Copy events in range"));
      pasteAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste"));
      pasteToCurPartAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste to current part"));
      pasteDialogAction = menuEdit->addAction(QIcon(*editpasteIconSet), tr("Paste (with Dialog)"));
      menuEdit->addSeparator();
      deleteAction = menuEdit->addAction(tr("Delete Events"));

      connect(cutAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_CUT); } );
      connect(copyAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_COPY); } );
      connect(copyRangeAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_COPY_RANGE); } );
      connect(pasteAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_PASTE); } );
      connect(pasteToCurPartAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_PASTE_TO_CUR_PART); } );
      connect(pasteDialogAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_PASTE_DIALOG); } );
      connect(deleteAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_DEL); } );
      
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

      connect(sallAction,  &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_ALL); } );
      connect(snoneAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_NONE); } );
      connect(invAction,   &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_INVERT); } );
      connect(inAction,    &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_ILOOP); } );
      connect(outAction,   &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_OLOOP); } );
      connect(prevAction,  &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_PREV_PART); } );
      connect(nextAction,  &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_SELECT_NEXT_PART); } );

      // Functions
      menuFunctions = menuBar()->addMenu(tr("Fu&nctions"));
      
      menuFunctions->setTearOffEnabled(true);

      fixedAction = menuFunctions->addAction(tr("Set Fixed Length"));
      veloAction = menuFunctions->addAction(tr("Modify Velocity"));
      crescAction = menuFunctions->addAction(tr("Crescendo/Decrescendo"));
      quantizeAction = menuFunctions->addAction(tr("Quantize"));
      QAction* eraseEventAction = menuFunctions->addAction(tr("Erase Event"));
      QAction* noteShiftAction = menuFunctions->addAction(tr("Move Notes"));
      QAction* delOverlapsAction = menuFunctions->addAction(tr("Delete Overlaps"));

      connect(fixedAction,       &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_FIXED_LEN); } );
      connect(veloAction,        &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_MODIFY_VELOCITY); } );
      connect(crescAction,       &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_CRESCENDO); } );
      connect(quantizeAction,    &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_QUANTIZE); } );
      connect(eraseEventAction,  &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_ERASE_EVENT); } );
      connect(noteShiftAction,   &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_NOTE_SHIFT); } );
      connect(delOverlapsAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_DELETE_OVERLAPS); } );


      //----------------------
      // Scripts:
      //----------------------

      QMenu* menuScriptPlugins = menuBar()->addMenu(tr("&Plugins"));
      connect(&_scriptReceiver,
              &MusECore::ScriptReceiver::execDeliveredScriptReceived,
              [this](int id) { execDeliveredScript(id); } );
      connect(&_scriptReceiver,
              &MusECore::ScriptReceiver::execUserScriptReceived,
              [this](int id) { execUserScript(id); } );
      MusEGlobal::song->populateScriptMenu(menuScriptPlugins, &_scriptReceiver);


      QMenu* settingsMenu = menuBar()->addMenu(tr("&Display"));

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
      
      connect(ignoreHideAction,  SIGNAL(toggled(bool)), SLOT(set_ignore_hide(bool)));
      connect(showAllAction,  SIGNAL(triggered()), this, SLOT(showAllInstruments()));
      connect(hideAllAction,  SIGNAL(triggered()), this, SLOT(hideAllInstruments()));
      connect(hideUnusedAction,  SIGNAL(triggered()), this, SLOT(hideUnusedInstruments()));
      connect(hideEmptyAction,  SIGNAL(triggered()), this, SLOT(hideEmptyInstruments()));

      connect(groupNoneAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_GROUP_NONE); } );
      connect(groupChanAction, &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_GROUP_CHAN); } );
      connect(groupMaxAction,  &QAction::triggered, [this]() { cmd(DrumCanvas::CMD_GROUP_MAX); } );

      updateGroupingActions();
      settingsMenu->addAction(subwinAction);
      settingsMenu->addAction(shareAction);
      settingsMenu->addAction(fullscreenAction);

      //---------------------------------------------------
      //    Toolbars
      //---------------------------------------------------
    
      // NOTICE: Please ensure that any tool bar object names here match the names assigned 
      //          to identical or similar toolbars in class MusE or other TopWin classes. 
      //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
      //          to retain the original toolbar layout. If it finds an existing
      //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
      //          instead of /appending/ with addToolBar().

      addToolBarBreak();
      
      // Already has an object name.
      tools2 = new MusEGui::EditToolBar(this, drumeditTools);
      addToolBar(tools2);

      tools = addToolBar(tr("Drum tools"));
      tools->setObjectName("Drum tools");

      srec  = new QToolButton();
      srec->setToolTip(tr("Step record"));
      srec->setIcon(*steprecSVGIcon);
      srec->setCheckable(true);
      srec->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(srec);

      midiin  = new QToolButton();
      midiin->setToolTip(tr("Midi input"));
      midiin->setIcon(*midiinSVGIcon);
      midiin->setCheckable(true);
      midiin->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(midiin);
      
      speaker  = new QToolButton();
      speaker->setToolTip(tr("Play events"));
      speaker->setIcon(*speakerSVGIcon);
      speaker->setCheckable(true);
      speaker->setChecked(true);
      speaker->setFocusPolicy(Qt::NoFocus);
      tools->addWidget(speaker);

      QAction* whatsthis = QWhatsThis::createAction(this);
      whatsthis->setIcon(*whatsthisSVGIcon);
      tools->addAction(whatsthis);

      // don't show pitch value in toolbar
      toolbar = new MusEGui::Toolbar1(this, _rasterInit, false);
      toolbar->setObjectName("Drum Pos/Snap/Solo-tools");
      addToolBar(toolbar);
      
      addToolBarBreak();
      
      info    = new MusEGui::NoteInfo(this);
      info->setObjectName("Drum note info");
      addToolBar(info);

      QToolBar* cursorToolbar = addToolBar(tr("Cursor tools"));
      cursorToolbar->setObjectName("Cursor step tools");
      QLabel *stepStr = new QLabel(tr("Cursor step"));
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

      //---------------------------------------------------
      //    split
      //---------------------------------------------------

      hsplitter = new MusEGui::Splitter(Qt::Horizontal, mainw, "hsplitter");
      hsplitter->setChildrenCollapsible(true);
      //hsplitter->setHandleWidth(4);
      
      trackInfoWidget = new TrackInfoWidget(hsplitter);
      genTrackInfo(trackInfoWidget);
      
      split1            = new MusEGui::Splitter(Qt::Vertical, mainw, "split1");
      ctrl = new CompactToolButton(mainw);
      ctrl->setIcon(*midiControllerNewSVGIcon);
      ctrl->setIconSize(QSize(14, 14));
      ctrl->setHasFixedIconSize(true);
      ctrl->setContentsMargins(4, 4, 4, 4);
      ctrl->setObjectName("Ctrl");
      ctrl->setFocusPolicy(Qt::NoFocus);
      ctrl->setFixedWidth(40);
      ctrl->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
      ctrl->setToolTip(tr("Add controller view"));

      // Increased scale to -1. To resolve/select/edit 1-tick-wide (controller graph) events. 
      hscroll           = new MusEGui::ScrollScale(-25, -1 /* formerly -2 */, _viewState.xscale(), 20000, Qt::Horizontal, mainw);
      hscroll->setFocusPolicy(Qt::NoFocus);
      hscroll->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

      QSizeGrip* corner = new QSizeGrip(mainw);
      corner->setFixedHeight(hscroll->sizeHint().height());
      
      MidiEditorHScrollLayout* gridSplit1 = 
        new MidiEditorHScrollLayout(nullptr, ctrl, nullptr, hscroll, corner, nullptr);
      gridSplit1->setContentsMargins(0, 0, 0, 0);
      gridSplit1->setSpacing(0);  

      QWidget* split1_w = new QWidget(hsplitter);
      split1_w->setObjectName("split1_w");
      split1_w->setContentsMargins(0, 0, 0, 0);
      QVBoxLayout* split1_w_vbox = new QVBoxLayout(split1_w);
      split1_w_vbox->setContentsMargins(0, 0, 0, 0);
      split1_w_vbox->setSpacing(0);  
      // Let the vertical splitter take up room so that the next widget
      //  (gridSplit1 - the bottom space layout) does not expand.
      split1_w_vbox->addWidget(split1, 1000);
      split1_w_vbox->addLayout(gridSplit1);

      QSizePolicy tipolicy, epolicy;
      hsplitter->setStretchFactor(hsplitter->indexOf(trackInfoWidget), 0);
      tipolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
      tipolicy.setHorizontalStretch(0);
      tipolicy.setVerticalStretch(100);
      trackInfoWidget->setSizePolicy(tipolicy);

      hsplitter->setStretchFactor(hsplitter->indexOf(split1_w), 1);
      epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      epolicy.setHorizontalStretch(255);
      epolicy.setVerticalStretch(100);
      split1->setSizePolicy(epolicy);
      
      mainGrid->addWidget(hsplitter, 0, 0,  1, 1);

      split2              = new MusEGui::Splitter(Qt::Horizontal, split1, "split2");

      split1w1            = new QWidget(split2);
      QWidget* split1w2   = new QWidget(split2);

      split1w1->setContentsMargins(0, 0, 0, 0);
      split1w2->setContentsMargins(0, 0, 0, 0);
      split1->setContentsMargins(0, 0, 0, 0);
      split2->setContentsMargins(0, 0, 0, 0);
      hsplitter->setContentsMargins(0, 0, 0, 0);
      // NOTICE: For vertical splitter split1, we need to ignore the horizontal size
      //          otherwise we get strange runaway resizing when split2 is moved towards
      //          the left, which also depends oddly on the minimum size of the widget
      //          in the left of hsplitter. For the other splitters, I guess we'll
      //          do the same since it took a whole day to solve this problem. Tim.
      split1->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
      split2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
      hsplitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
      
      split2->setStretchFactor(split2->indexOf(split1w1), 0);
      tipolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
      tipolicy.setHorizontalStretch(0);
      tipolicy.setVerticalStretch(100);
      split1w1->setSizePolicy(tipolicy);

      split2->setStretchFactor(split2->indexOf(split1w2), 1);
      epolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      epolicy.setHorizontalStretch(255);
      epolicy.setVerticalStretch(100);
      split1w2->setSizePolicy(epolicy);
      
      // Now that we have the second widget in splitter 2 (split1w2),
      //  set up the horizontal scroll layout to follow its width.
      gridSplit1->setEditor(split1w2);
      
      QGridLayout* gridS1 = new QGridLayout(split1w1);
      MidiEditorCanvasLayout* gridS2 = new MidiEditorCanvasLayout(split1w2, gridSplit1);
      gridS1->setContentsMargins(0, 0, 0, 0);
      gridS1->setSpacing(0);  
      gridS2->setContentsMargins(0, 0, 0, 0);
      gridS2->setSpacing(0);  
      time                = new MusEGui::MTScale(&_raster, split1w2, _viewState.xscale());
      canvas              = new DrumCanvas(this, split1w2, _viewState.xscale(), _viewState.yscale());
      vscroll             = new MusEGui::ScrollScale(-2, 1, _viewState.yscale(),
                            dynamic_cast<DrumCanvas*>(canvas)->getOurDrumMapSize()*TH, Qt::Vertical, split1w2);
      int offset = -(MusEGlobal::config.division/4);
      canvas->setOrigin(offset, 0);
      canvas->setCanvasTools(drumeditTools);
      canvas->setFocus();
      connect(canvas, SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
      connect(canvas, SIGNAL(horizontalZoom(bool,const QPoint&)), SLOT(horizontalZoom(bool, const QPoint&)));
      connect(canvas, SIGNAL(horizontalZoom(int, const QPoint&)), SLOT(horizontalZoom(int, const QPoint&)));
      connect(canvas, SIGNAL(ourDrumMapChanged(bool)), SLOT(ourDrumMapChanged(bool)));
      connect(canvas, SIGNAL(curPartHasChanged(MusECore::Part*)), SLOT(updateTrackInfo()));
      time->setOrigin(offset, 0);

      QList<int> mops;
      mops.append(_dlistWidthInit);
      mops.append(_dcanvasWidthInit);
      split2->setSizes(mops);
      
      mops.clear();
      mops.append(_trackInfoWidthInit);
      mops.append(_canvasWidthInit);
      hsplitter->setSizes(mops);
      
      gridS2->setRowStretch(1, 100);
      gridS2->setColumnStretch(0, 100);
      
      gridS2->addWidget(time,  0, 0, 1, 2);
      gridS2->addWidget(MusECore::hLine(split1w2), 1, 0, 1, 2);
      gridS2->addWidget(canvas,  2, 0);
      
      gridS2->addWidget(vscroll, 2, 1);

      //  Ordering is hardcoded in dlist.c ("Dcols")
      header = new MusEGui::Header(split1w1, "header");
      header->setFixedHeight(31);
      //: hide
      header->setColumnLabel(tr("H"), COL_HIDE, 20);
      //: mute
      header->setColumnLabel(tr("M"), COL_MUTE, 20);
      header->setColumnLabel(tr("Sound"), COL_NAME, 120);
      header->setColumnLabel(tr("Vol"), COL_VOLUME);
      header->setColumnLabel(tr("QNT"), COL_QUANT, 40);
      header->setColumnLabel(tr("E-Note"), COL_INPUTTRIGGER, 50);
      header->setColumnLabel(tr("Len"), COL_NOTELENGTH, 40);
      header->setColumnLabel(tr("A-Note"), COL_NOTE, 50);
      header->setColumnLabel(tr("Ch"), COL_OUTCHANNEL);
      header->setColumnLabel(tr("Port"), COL_OUTPORT, 70);
      header->setColumnLabel(tr("LV1"), COL_LEVEL1);
      header->setColumnLabel(tr("LV2"), COL_LEVEL2);
      header->setColumnLabel(tr("LV3"), COL_LEVEL3);
      header->setColumnLabel(tr("LV4"), COL_LEVEL4);

      setHeaderToolTips();
      setHeaderWhatsThis();

      if (_ignore_hide)
        header->showSection(COL_HIDE);
      else
        header->hideSection(COL_HIDE);

      dlist = new DList(header, split1w1, _viewState.yscale(), (DrumCanvas*)canvas);
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
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), SLOT(songChanged1(MusECore::SongChangedStruct_t)));
      connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)),      dlist, SLOT(songChanged(MusECore::SongChangedStruct_t)));
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
      
      dlist->setYPos(_viewState.yscroll());
      canvas->setYPos(_viewState.yscroll());
      vscroll->setOffset(_viewState.yscroll());

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
      configChanged();  // set configuration values, initialize shortcuts

      // Don't bother if not showing default stuff, since something else
      //  will likely take care of it, like the song file loading routines.
      if(showDefaultControls)
      {
        const MusECore::Pos cpos=MusEGlobal::song->cPos();
        canvas->setPos(0, cpos.tick(), true);
        canvas->selectAtTick(cpos.tick());
        //canvas->selectFirst();
          
        unsigned pos=0;
        if(initPos >= INT_MAX)
          pos = MusEGlobal::song->cpos();
        else
          pos = initPos;
        if(pos > INT_MAX)
          pos = INT_MAX;
        //if (pos)
          hscroll->setOffset((int)pos);
      }

      if(canvas->track())
      {
        updateTrackInfo();
        toolbar->setSolo(canvas->track()->solo());
      }
      
      initTopwinState();
      finalizeInit();

      // Add initial controllers.
      // Don't bother if not showing default stuff, since something else
      //  will likely take care of it, like the song file loading routines.
      if(showDefaultControls)
      {
        CtrlEdit* ctrl_edit;
        const MusECore::MidiCtrlViewStateList& mcvsl = _viewState.controllers();
        for(MusECore::ciMidiCtrlViewState i = mcvsl.cbegin(); i != mcvsl.cend(); ++i)
        {
          const MusECore::MidiCtrlViewState& mcvs = *i;
          ctrl_edit = addCtrl(mcvs._num);
          if(ctrl_edit)
            ctrl_edit->setPerNoteVel(mcvs._perNoteVel);
        }
      }
      }

//---------------------------------------------------------
//   songChanged1
//---------------------------------------------------------

void DrumEdit::songChanged1(MusECore::SongChangedStruct_t bits)
      {
        if(_isDeleting)  // Ignore while deleting to prevent crash.
          return;
        
        // We must catch this first and be sure to update the strips.
        if(bits & SC_TRACK_REMOVED)
          checkTrackInfoTrack();
        
        if (bits & SC_SOLO)
        {
            if(canvas->track())
              toolbar->setSolo(canvas->track()->solo());
        }      

        if ( ( bits & (SC_DRUMMAP | SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED |
                       SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED) ) )
          ((DrumCanvas*)(canvas))->rebuildOurDrumMap();
        
        songChanged(bits);
        
        // We'll receive SC_SELECTION if a different part is selected.
        // Addition - also need to respond here to moving part to another track. (Tim)
        if (bits & (SC_PART_INSERTED | SC_PART_REMOVED))
          updateTrackInfo();
        
        // We must marshall song changed instead of connecting to the strip's song changed
        //  otherwise it crashes when loading another song because track is no longer valid
        //  and the strip's songChanged() seems to be called before Pianoroll songChanged()
        //  gets called and has a chance to stop the crash.
        // Also, calling updateTrackInfo() from here is too heavy, it destroys and recreates
        //  the strips each time no matter what the flags are !
        else  
          trackInfoSongChange(bits);
      }

//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------
void DrumEdit::midiNote(int pitch, int velo)
{
  if (MusEGlobal::debugMsg)
      printf("DrumEdit::midiNote: pitch=%i, velo=%i\n", pitch, velo);
  int index=0;

  //      *note = old_style_drummap_mode ? ourDrumMap[index].anote : instrument_map[index].pitch;

  if ((DrumCanvas*)(canvas)->midiin())
  {
    for (index = 0; index < get_instrument_map().size(); ++index) {
      if (get_instrument_map().at(index).pitch == pitch)
        break;
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
      e += MusEGlobal::sigmap.ticksMeasure(e);  
      // Show another quarter measure due to imprecise drawing at canvas end point.
      e += MusEGlobal::sigmap.ticksMeasure(e) / 4;
      // Compensate for drum list, splitter handle, and vscroll widths. 
      e += canvas->rmapxDev(split2->handleWidth() - vscroll->width()); 
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
        // Zero note on vel is not allowed now.
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
            // Zero note on vel is not allowed now.
            info->setValues(0, 0, 0, deltaMode ? 0 : 1, 0);
            firstValueSet = false;
            tickValue     = 0;
            lenValue      = 0;
            pitchValue    = 0;
            // Zero note on vel is not allowed now.
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
      if(canvas->track())
      {
        // This is a minor operation easily manually undoable. Let's not clog the undo list with it.
        MusECore::PendingOperationList operations;
        operations.add(MusECore::PendingOperationItem(canvas->track(), flag, MusECore::PendingOperationItem::SetTrackSolo));
        MusEGlobal::audio->msgExecutePendingOperations(operations, true);
      }
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void DrumEdit::setRaster(int val)
      {
      _rasterInit = val;
      MidiEditor::setRaster(val);
      canvas->redrawGrid();
      for (auto it : ctrlEditList)
          it->redrawCanvas();
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
                              canvas->setPlayEvents(_playEvents);
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
                        else if (tag == "trackinfowidth")
                              _trackInfoWidthInit = xml.parseInt();
                        else if (tag == "canvaswidth")
                              _canvasWidthInit = xml.parseInt();
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
      xml.intTag(level, "trackinfowidth", _trackInfoWidthInit);
      xml.intTag(level, "canvaswidth", _canvasWidthInit);
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
                              MusEGlobal::audio->msgIdle(true);
                              // Delete all port controller events.
                              MusEGlobal::song->changeMidiCtrlCacheEvents(false, true, false, true, true);

                              readDrumMap(xml, true);

                              // Add all port controller events.
                              MusEGlobal::song->changeMidiCtrlCacheEvents(true, true, false, true, true);
                              MusEGlobal::audio->msgIdle(false);

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
    MusEGlobal::audio->msgIdle(true);
    // Delete all port controller events.
    MusEGlobal::song->changeMidiCtrlCacheEvents(false, true, false, true, true);

    MusECore::resetGMDrumMap();

    // Add all port controller events.
    MusEGlobal::song->changeMidiCtrlCacheEvents(true, true, false, true, true);
    MusEGlobal::audio->msgIdle(false);

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

      MusECore::TagEventList tag_list;

      const FunctionDialogElements_t fn_element_dflt =
        FunctionAllEventsButton |
        FunctionSelectedEventsButton |
        FunctionLoopedButton |
        FunctionSelectedLoopedButton |
        FunctionAllPartsButton |
        FunctionSelectedPartsButton;
      
      switch(cmd) {
            case DrumCanvas::CMD_CUT:
                  tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
                  MusECore::cut_items(&tag_list);
                  break;
            case DrumCanvas::CMD_COPY:
                  tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
                  MusECore::copy_items(&tag_list);
                  break;
            case DrumCanvas::CMD_COPY_RANGE:
                  tagItems(&tag_list, 
                           MusECore::EventTagOptionsStruct::fromOptions(!itemsAreSelected(),
                           true, true, MusEGlobal::song->lPos(), MusEGlobal::song->rPos()));
                  MusECore::copy_items(&tag_list);
                  break;
            case DrumCanvas::CMD_PASTE:
                              ((DrumCanvas*)canvas)->cmd(DrumCanvas::CMD_SELECT_NONE);
                              MusECore::paste_items(partlist_to_set(parts()), 3072,
                                MusECore::FunctionOptionsStruct(
                                  MusECore::FunctionEraseItemsDefault | MusECore::FunctionPasteNeverNewPart));
                              break;
            case DrumCanvas::CMD_PASTE_TO_CUR_PART:
                              ((DrumCanvas*)canvas)->cmd(DrumCanvas::CMD_SELECT_NONE);
                              MusECore::paste_items(partlist_to_set(parts()), 3072,
                                MusECore::FunctionOptionsStruct(
                                  MusECore::FunctionEraseItemsDefault | MusECore::FunctionPasteNeverNewPart),
                                canvas->part());
                              break;
            case DrumCanvas::CMD_PASTE_DIALOG:
                              ((DrumCanvas*)canvas)->cmd(DrumCanvas::CMD_SELECT_NONE);
                              MusECore::paste_items(partlist_to_set(parts()), (canvas->part()));
                              break;
                              
            case DrumCanvas::CMD_MODIFY_VELOCITY:
                  {
                  FunctionDialogReturnVelocity ret =
                    velocity_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::modify_velocity_items(&tag_list, ret._rateVal, ret._offsetVal);
                  }
                  break;
                  }
            case DrumCanvas::CMD_CRESCENDO:
                  {
                  FunctionDialogReturnCrescendo ret =
                    crescendo_items_dialog(FunctionDialogMode(
                      FunctionLoopedButton |
                      FunctionSelectedLoopedButton |
                      FunctionAllPartsButton | 
                      FunctionSelectedPartsButton));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::crescendo_items(&tag_list, ret._start_val, ret._end_val, ret._absolute);
                  }
                  break;
                  }
            case DrumCanvas::CMD_QUANTIZE:
                  {
                  FunctionDialogReturnQuantize ret =
                    quantize_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::quantize_items(&tag_list, ret._raster_index,
                                           /*ret._quant_len*/ false,  // DELETETHIS
                                           ret._strength,
                                           ret._swing,
                                           ret._threshold);
                  }
                  break;
                  }
            case DrumCanvas::CMD_ERASE_EVENT:
            {
              FunctionDialogReturnErase ret =
                erase_items_dialog(FunctionDialogMode(fn_element_dflt));
              if(ret._valid)
              {
                tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                MusECore::erase_items(&tag_list, ret._veloThreshold, ret._veloThresUsed, ret._lenThreshold, ret._lenThresUsed);
              }
            }
            break;
            case DrumCanvas::CMD_DEL:
              tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
              MusECore::erase_items(&tag_list);
            break;
            case DrumCanvas::CMD_DELETE_OVERLAPS:
                  {
                  FunctionDialogReturnDelOverlaps ret =
                    deloverlaps_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::delete_overlaps_items(&tag_list);
                  }
                  break;
                  }
            case DrumCanvas::CMD_NOTE_SHIFT:
                  {
                  FunctionDialogReturnMove ret =
                    move_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::move_items(&tag_list, ret._amount);
                  }
                  break;
                  }

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
      const bool has_gel = QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-groupedeventlists"));
      pasteAction->setEnabled(has_gel);
      pasteToCurPartAction->setEnabled(has_gel);
      pasteDialogAction->setEnabled(has_gel);
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void DrumEdit::selectionChanged()
      {
      bool flag = itemsAreSelected();
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
    CtrlEdit* ctrlEdit = new CtrlEdit(split1, this, _viewState.xscale(), true, "drumCtrlEdit");
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
      CtrlEdit* ctrlEdit = new CtrlEdit(split1, this, _viewState.xscale(), true, "drumCtrlEdit");
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
//   getViewState
//---------------------------------------------------------

MusECore::MidiPartViewState DrumEdit::getViewState() const
{
  MusECore::MidiPartViewState vs;
  vs.setXScroll(hscroll->offset());
  vs.setYScroll(vscroll->offset());
  vs.setXScale(hscroll->getScaleValue());
  vs.setYScale(vscroll->getScaleValue());
  CtrlEdit* ce;
  for(CtrlEditList::const_iterator i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i) {
    ce = *i;
    vs.addController(MusECore::MidiCtrlViewState(ce->ctrlNum(), ce->perNoteVel()));
    }
  return vs;
}

void DrumEdit::storeInitialViewState() const
{
  const MusECore::PartList* pl = parts();
  if(pl)
  {
    const MusECore::MidiPartViewState vs = getViewState();
    for(MusECore::ciPart i = pl->begin(); i != pl->end(); ++i)
      i->second->setViewState(vs);
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
      if (MusEGlobal::config.canvasBgPixmap.isEmpty()) {
            canvas->setBg(MusEGlobal::config.midiCanvasBg);
            canvas->setBg(QPixmap());
      }
      else {
            canvas->setBg(QPixmap(MusEGlobal::config.canvasBgPixmap));
      }
      dlist->setBg(MusEGlobal::config.drumListBg);
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
          canvas->setPlayEvents(!speaker->isChecked());
          speaker->setChecked(!speaker->isChecked());
          return;
      }
      else if (key == shortcuts[SHRT_INC_VELOCITY].key) {
//           modify_velocity(partlist_to_set(parts()), 1, 100, 1);
          MusECore::TagEventList tag_list;
          tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
          MusECore::modify_velocity_items(&tag_list, 100, 1);
          return;
      }
      else if (key == shortcuts[SHRT_DEC_VELOCITY].key) {
//           modify_velocity(partlist_to_set(parts()), 1, 100, -1);
          MusECore::TagEventList tag_list;
          tagItems(&tag_list, MusECore::EventTagOptionsStruct(MusECore::TagSelected | MusECore::TagAllParts));
          MusECore::modify_velocity_items(&tag_list, 100, -1);
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
      canvas->setPlayEvents(_playEvents);
      }

//---------------------------------------------------------
//   initShortcuts
//---------------------------------------------------------

void DrumEdit::initShortcuts()
      {
      cutAction->setShortcut(shortcuts[SHRT_CUT].key);
      copyAction->setShortcut(shortcuts[SHRT_COPY].key);
      copyRangeAction->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
      pasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      pasteToCurPartAction->setShortcut(shortcuts[SHRT_PASTE_TO_CUR_PART].key);
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
      MusEGlobal::song->executeScript(this, scriptfile.toLatin1().constData(), parts(), raster(), true);
}

//---------------------------------------------------------
//   execUserScript
//---------------------------------------------------------
void DrumEdit::execUserScript(int id)
{
      QString scriptfile = MusEGlobal::song->getScriptPath(id, false);
      MusEGlobal::song->executeScript(this, scriptfile.toLatin1().constData(), parts(), raster(), true);
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

  if (_ignore_hide)
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
      track->drummap()[i].hide=false;
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
      track->drummap()[i].hide=true;
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
      track->drummap()[i].hide=hide[i];
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
      track->drummap()[i].hide=hide[i];
  }
  
  MusEGlobal::song->update(SC_DRUMMAP);
}


} // namespace MusEGui
