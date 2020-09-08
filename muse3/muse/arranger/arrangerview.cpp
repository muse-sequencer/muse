//=========================================================
//  MusE
//  Linux Music Editor
//  arrangerview.cpp
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
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


#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QGridLayout>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSizeGrip>
#include <QToolButton>
#include <QToolTip>

#include "sig.h"
#include "app.h"
#include "arrangerview.h"
#include "audio.h"
#include "functions.h"
#include "gconfig.h"
#include "globals.h"
#include "helper.h"
#include "icons.h"
#include "scoreedit.h"
#include "shortcuts.h"
#include "song.h"
#include "structure.h"
#include "tb1.h"
#include "ttoolbar.h"
#include "arrangercolumns.h"
#include "tlist.h"
#include "synth.h"
#include "pcanvas.h"

// Forwards from header:
#include <QCloseEvent>
#include <QAction>
#include <QGridLayout>
#include <QMenu>
#include "arranger.h"
#include "visibletracks.h"
#include "event_tag_list.h"
#include "xml.h"
#include "tools.h"

namespace MusEGui {

//---------------------------------------------------------
//   ArrangerView
//---------------------------------------------------------

ArrangerView::ArrangerView(QWidget* parent)
   : TopWin(TopWin::ARRANGER, parent, "arrangerview", Qt::Window)
{
  isMdiWin() ? setWindowTitle(tr("Arranger")) : setWindowTitle(tr("MusE: Arranger"));
  setFocusPolicy(Qt::NoFocus);  

  // Already has an object name.
  visTracks = new VisibleTracks(this);
  
  arranger = new Arranger(this, "arranger");
  setCentralWidget(arranger);
  //setFocusProxy(arranger);

  // Toolbars ---------------------------------------------------------

  // NOTICE: Please ensure that any tool bar object names here match the names assigned 
  //          to identical or similar toolbars in class MusE or other TopWin classes. 
  //         This allows MusE::setCurrentMenuSharingTopwin() to do some magic
  //          to retain the original toolbar layout. If it finds an existing
  //          toolbar with the same object name, it /replaces/ it using insertToolBar(),
  //          instead of /appending/ with addToolBar().

  addToolBarBreak();
  
  editTools = new EditToolBar(this, arrangerTools);
  addToolBar(editTools);
  // Make sure name doesn't conflict with other TopWin edit toolbar object names.
  editTools->setObjectName("arrangerTools");

  addToolBar(visTracks);

  connect(editTools, SIGNAL(toolChanged(int)), arranger, SLOT(setTool(int)));
  connect(visTracks, SIGNAL(visibilityChanged()), MusEGlobal::song, SLOT(update()) );
  connect(arranger, SIGNAL(editPart(MusECore::Track*)), MusEGlobal::muse, SLOT(startEditor(MusECore::Track*)));
  connect(arranger, SIGNAL(dropSongFile(const QString&)), MusEGlobal::muse, SLOT(loadProjectFile(const QString&)));
  connect(arranger, SIGNAL(dropMidiFile(const QString&)), MusEGlobal::muse, SLOT(importMidi(const QString&)));
  connect(arranger, SIGNAL(startEditor(MusECore::PartList*,int)),  MusEGlobal::muse, SLOT(startEditor(MusECore::PartList*,int)));
  connect(arranger, SIGNAL(toolChanged(int)), editTools, SLOT(set(int)));
  connect(MusEGlobal::muse, SIGNAL(configChanged()), arranger, SLOT(configChanged()));
  connect(arranger, SIGNAL(setUsedTool(int)), editTools, SLOT(set(int)));



  //-------- Edit Actions
  editDeleteAction = new QAction(QIcon(*deleteIcon), tr("D&elete"), this);
  editCutAction = new QAction(QIcon(*editcutIconSet), tr("C&ut"), this);
  editCopyAction = new QAction(QIcon(*editcopyIconSet), tr("&Copy"), this);
  editCopyRangeAction = new QAction(QIcon(*editcopyIconSet), tr("Copy in Range"), this);
  editPasteAction = new QAction(QIcon(*editpasteIconSet), tr("&Paste"), this);
  editPasteCloneAction = new QAction(QIcon(*editpasteCloneIconSet), tr("Paste C&lone"), this);
  editPasteToTrackAction = new QAction(QIcon(*editpaste2TrackIconSet), tr("Paste to Selected &Track"), this);
  editPasteCloneToTrackAction = new QAction(QIcon(*editpasteClone2TrackIconSet), tr("Paste Clone to Selected Trac&k"), this);
  editPasteDialogAction = new QAction(QIcon(*editpasteIconSet), tr("Paste (Show Dialo&g)..."), this);
  editInsertEMAction = new QAction(QIcon(*editpasteIconSet), tr("&Insert Empty Measure"), this);
  editDeleteSelectedAction = new QAction(QIcon(*edit_track_delIcon), tr("Delete Selected Tracks"), this);
  editDuplicateSelTrackAction = new QAction(QIcon(*edit_track_addIcon), tr("Duplicate Selected Tracks"), this);

  editShrinkPartsAction = new QAction(tr("Shrink Selected Parts"), this);
  editExpandPartsAction = new QAction(tr("Expand Selected Parts"), this);
  editCleanPartsAction = new QAction(tr("Purge Hidden Events from Selected Parts"), this);


  addTrack = new QMenu(tr("Add Track"), this);
  addTrack->setIcon(QIcon(*edit_track_addIcon));
  insertTrack = new QMenu(tr("Insert Track"), this);
  insertTrack->setIcon(QIcon(*edit_track_addIcon));
  select = new QMenu(tr("Select"), this);
  select->setIcon(QIcon(*selectIcon));

  editSelectAllAction = new QAction(QIcon(*select_allIcon), tr("Select &All"), this);
  editDeselectAllAction = new QAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"), this);
  editInvertSelectionAction = new QAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"), this);
  editInsideLoopAction = new QAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"), this);
  editOutsideLoopAction = new QAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"), this);
  editAllPartsAction = new QAction( QIcon(*select_all_parts_on_trackIcon), tr("All &Parts on Track"), this);

  select->addAction(editSelectAllAction);
  select->addAction(editDeselectAllAction);
  select->addAction(editInvertSelectionAction);
  select->addAction(editInsideLoopAction);
  select->addAction(editOutsideLoopAction);
  select->addAction(editAllPartsAction);
  
	
  scoreSubmenu = new QMenu(tr("Score"), this);
  scoreSubmenu->setIcon(QIcon(*scoreIconSet));

  scoreAllInOneSubsubmenu = new QMenu(tr("All Tracks in One Staff"), this);
  scoreOneStaffPerTrackSubsubmenu = new QMenu(tr("One Staff per Track"), this);

  startScoreEditAction = new QAction(*scoreIconSet, tr("New Score Window..."), this);
  scoreSubmenu->addAction(startScoreEditAction);
  
  scoreSubmenu->addMenu(scoreAllInOneSubsubmenu);
  scoreSubmenu->addMenu(scoreOneStaffPerTrackSubsubmenu);
  updateScoreMenus();

  startPianoEditAction = new QAction(*pianoIconSet, tr("Pianoroll..."), this);
  startDrumEditAction = new QAction(QIcon(*edit_drummsIcon), tr("Drums..."), this);
  startListEditAction = new QAction(QIcon(*edit_listIcon), tr("Event List..."), this);
  startWaveEditAction = new QAction(QIcon(*edit_waveIcon), tr("Wave..."), this);

  openCurrentTrackSynthGuiAction =  new QAction(QIcon(*settings_midiport_softsynthsIcon), tr("Open Synth Plugin GUI..."), this);

  midiTransformerAction = new QAction(QIcon(*midi_transformIcon), tr("Midi &Transform..."), this);


  //-------- Structure Actions
  strGlobalCutAction = new QAction(tr("Global Cut"), this);
  strGlobalInsertAction = new QAction(tr("Global Insert"), this);
  strGlobalSplitAction = new QAction(tr("Global Split"), this);

  strGlobalCutSelAction = new QAction(tr("Global Cut - Selected Tracks"), this);
  strGlobalInsertSelAction = new QAction(tr("Global Insert - Selected Tracks"), this);
  strGlobalSplitSelAction = new QAction(tr("Global Split - Selected Tracks"), this);



  //-------------------------------------------------------------
  //    popup Edit
  //-------------------------------------------------------------

  QMenu* menuEdit = menuBar()->addMenu(tr("&Edit"));
  menuEdit->addActions(MusEGlobal::undoRedo->actions());
  menuEdit->addSeparator();

  menuEdit->addAction(editDeleteAction);
  menuEdit->addAction(editCutAction);
  menuEdit->addAction(editCopyAction);
  menuEdit->addAction(editCopyRangeAction);
  menuEdit->addAction(editPasteAction);
  menuEdit->addAction(editPasteToTrackAction);
  menuEdit->addAction(editPasteCloneAction);
  menuEdit->addAction(editPasteCloneToTrackAction);
  menuEdit->addAction(editPasteDialogAction);
  menuEdit->addAction(editInsertEMAction);
  menuEdit->addSeparator();
  menuEdit->addMenu(select);
  menuEdit->addSeparator();
  
  menuEdit->addSeparator();
  menuEdit->addAction(editDeleteSelectedAction);

  menuEdit->addMenu(addTrack);
  menuEdit->addMenu(insertTrack);
  menuEdit->addAction(editDuplicateSelTrackAction);
  menuEdit->addSeparator();

  menuEdit->addAction(startPianoEditAction);
  menuEdit->addMenu(scoreSubmenu);
  menuEdit->addAction(startDrumEditAction);
  menuEdit->addAction(startListEditAction);
  menuEdit->addAction(startWaveEditAction);
  menuEdit->addAction(openCurrentTrackSynthGuiAction);

  QMenu* functions_menu = menuBar()->addMenu(tr("Fu&nctions"));
  functions_menu->addAction(midiTransformerAction);
  functions_menu->addSeparator();
  QMenu* menuStructure = functions_menu->addMenu(tr("&Structure"));
    menuStructure->addAction(strGlobalCutAction);
    menuStructure->addAction(strGlobalInsertAction);
    menuStructure->addAction(strGlobalSplitAction);
    menuStructure->addSeparator();
    menuStructure->addAction(strGlobalCutSelAction);
    menuStructure->addAction(strGlobalInsertSelAction);
    menuStructure->addAction(strGlobalSplitSelAction);
  functions_menu->addSeparator();
  QAction* func_quantize_action =     functions_menu->addAction(tr("&Quantize Notes"));
  QAction* func_notelen_action =      functions_menu->addAction(tr("Change Note &Length"));
  QAction* func_velocity_action =     functions_menu->addAction(tr("Change Note &Velocity"));
  QAction* func_cresc_action =        functions_menu->addAction(tr("Crescendo/Decrescendo"));
  QAction* func_transpose_action =    functions_menu->addAction(tr("Transpose"));
  QAction* func_erase_action =        functions_menu->addAction(tr("Erase Events (Not Parts)"));
  QAction* func_move_action =         functions_menu->addAction(tr("Move Events (Not Parts)"));
  QAction* func_fixed_len_action =    functions_menu->addAction(tr("Set Fixed Note Length"));
  QAction* func_del_overlaps_action = functions_menu->addAction(tr("Delete Overlapping Notes"));
  QAction* func_legato_action =       functions_menu->addAction(tr("Legato"));
  connect(func_quantize_action,     &QAction::triggered, [this]() { cmd(CMD_QUANTIZE); } );
  connect(func_notelen_action,      &QAction::triggered, [this]() { cmd(CMD_NOTELEN); } );
  connect(func_velocity_action,     &QAction::triggered, [this]() { cmd(CMD_VELOCITY); } );
  connect(func_cresc_action,        &QAction::triggered, [this]() { cmd(CMD_CRESCENDO); } );
  connect(func_transpose_action,    &QAction::triggered, [this]() { cmd(CMD_TRANSPOSE); } );
  connect(func_erase_action,        &QAction::triggered, [this]() { cmd(CMD_ERASE); } );
  connect(func_move_action,         &QAction::triggered, [this]() { cmd(CMD_MOVE); } );
  connect(func_fixed_len_action,    &QAction::triggered, [this]() { cmd(CMD_FIXED_LEN); } );
  connect(func_del_overlaps_action, &QAction::triggered, [this]() { cmd(CMD_DELETE_OVERLAPS); } );
  connect(func_legato_action,       &QAction::triggered, [this]() { cmd(CMD_LEGATO); } );
  functions_menu->addSeparator();
  functions_menu->addAction(editShrinkPartsAction);
  functions_menu->addAction(editExpandPartsAction);
  functions_menu->addAction(editCleanPartsAction);
  
  
  QMenu* menuSettings = menuBar()->addMenu(tr("&Display"));
  menuSettings->addAction(tr("Toggle &Mixer Strip"), this, SLOT(toggleMixerStrip()),
                          MusEGui::shortcuts[MusEGui::SHRT_HIDE_MIXER_STRIP].key);
  menuSettings->addAction(tr("Configure &Custom Columns..."), this, SLOT(configCustomColumns()));
//  menuSettings->addSeparator();
//  menuSettings->addAction(subwinAction);
//  menuSettings->addAction(shareAction);
//  menuSettings->addAction(fullscreenAction);


  //-------- Edit connections
  connect(editDeleteAction,            &QAction::triggered, [this]() { cmd(CMD_DELETE); } );
  connect(editCutAction,               &QAction::triggered, [this]() { cmd(CMD_CUT); } );
  connect(editCopyAction,              &QAction::triggered, [this]() { cmd(CMD_COPY); } );
  connect(editCopyRangeAction,         &QAction::triggered, [this]() { cmd(CMD_COPY_RANGE); } );
  connect(editPasteAction,             &QAction::triggered, [this]() { cmd(CMD_PASTE); } );
  connect(editPasteCloneAction,        &QAction::triggered, [this]() { cmd(CMD_PASTE_CLONE); } );
  connect(editPasteToTrackAction,      &QAction::triggered, [this]() { cmd(CMD_PASTE_TO_TRACK); } );
  connect(editPasteCloneToTrackAction, &QAction::triggered, [this]() { cmd(CMD_PASTE_CLONE_TO_TRACK); } );
  connect(editPasteDialogAction,       &QAction::triggered, [this]() { cmd(CMD_PASTE_DIALOG); } );
  connect(editInsertEMAction,          &QAction::triggered, [this]() { cmd(CMD_INSERTMEAS); } );
  connect(editDeleteSelectedAction,    &QAction::triggered, [this]() { cmd(CMD_DELETE_TRACK); } );
  connect(editDuplicateSelTrackAction, &QAction::triggered, [this]() { cmd(CMD_DUPLICATE_TRACK); } );
  connect(editShrinkPartsAction,       &QAction::triggered, [this]() { cmd(CMD_SHRINK_PART); } );
  connect(editExpandPartsAction,       &QAction::triggered, [this]() { cmd(CMD_EXPAND_PART); } );
  connect(editCleanPartsAction,        &QAction::triggered, [this]() { cmd(CMD_CLEAN_PART); } );
  connect(editSelectAllAction,         &QAction::triggered, [this]() { cmd(CMD_SELECT_ALL); } );
  connect(editDeselectAllAction,       &QAction::triggered, [this]() { cmd(CMD_SELECT_NONE); } );
  connect(editInvertSelectionAction,   &QAction::triggered, [this]() { cmd(CMD_SELECT_INVERT); } );
  connect(editInsideLoopAction,        &QAction::triggered, [this]() { cmd(CMD_SELECT_ILOOP); } );
  connect(editOutsideLoopAction,       &QAction::triggered, [this]() { cmd(CMD_SELECT_OLOOP); } );
  connect(editAllPartsAction,          &QAction::triggered, [this]() { cmd(CMD_SELECT_PARTS); } );
  
  connect(startPianoEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startPianoroll()));
  connect(startScoreEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startScoreQuickly()));
  connect(startDrumEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startDrumEditor()));
  connect(startListEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startListEditor()));
  connect(startWaveEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startWaveEditor()));
  connect(openCurrentTrackSynthGuiAction, SIGNAL(triggered()), SLOT(openCurrentTrackSynthGui()));


  connect(midiTransformerAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startMidiTransformer()));


  //-------- Structure connections
  connect(strGlobalCutAction, SIGNAL(triggered()), SLOT(globalCut()));
  connect(strGlobalInsertAction, SIGNAL(triggered()), SLOT(globalInsert()));
  connect(strGlobalSplitAction, SIGNAL(triggered()), SLOT(globalSplit()));
  connect(strGlobalCutSelAction, SIGNAL(triggered()), SLOT(globalCutSel()));
  connect(strGlobalInsertSelAction, SIGNAL(triggered()), SLOT(globalInsertSel()));
  connect(strGlobalSplitSelAction, SIGNAL(triggered()), SLOT(globalSplitSel()));



  connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(updateShortcuts()));


  QClipboard* cb = QApplication::clipboard();
  connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));
  connect(cb, SIGNAL(selectionChanged()), SLOT(clipboardChanged()));

  finalizeInit();
}

ArrangerView::~ArrangerView()
{
  
}

void ArrangerView::closeEvent(QCloseEvent* e)
{
//  emit isDeleting(static_cast<TopWin*>(this));
//  emit closed();

// keep just in case the arranger still can get closed somehow...
    fprintf(stderr, "*** Arranger closed event caught ***\n");
    e->accept();
}

void ArrangerView::writeStatus(int level, MusECore::Xml& xml) const
{
  xml.tag(level++, "arrangerview");
  TopWin::writeStatus(level, xml);
  xml.intTag(level, "tool", editTools->curTool());
  arranger->writeStatus(level,xml);
  xml.tag(level, "/arrangerview");
}

void ArrangerView::readStatus(MusECore::Xml& xml)
{
  for (;;)
  {
    MusECore::Xml::Token token = xml.parse();
    if (token == MusECore::Xml::Error || token == MusECore::Xml::End)
      break;
      
    const QString& tag = xml.s1();
    switch (token)
    {
      case MusECore::Xml::TagStart:
        if (tag == "tool") 
          editTools->set(xml.parseInt());
        else if (tag == "topwin") 
          TopWin::readStatus(xml);
        else if (tag == "arranger") 
          arranger->readStatus(xml);
        else
          xml.unknown("ArrangerView");
        break;

      case MusECore::Xml::TagEnd:
        if (tag == "arrangerview")
          return;
          
      default:
        break;
    }
  }
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void ArrangerView::readConfiguration(MusECore::Xml& xml)
      {
      for (;;) {
            MusECore::Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case MusECore::Xml::Error:
                  case MusECore::Xml::End:
                        return;
                  case MusECore::Xml::TagStart:
                        if (tag == "topwin")
                              TopWin::readConfiguration(ARRANGER, xml);
                        else if (tag == "arranger")
                              Arranger::readConfiguration(xml);
                        else
                              xml.unknown("ArrangerView");
                        break;
                  case MusECore::Xml::TagEnd:
                        if (tag == "arrangerview")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void ArrangerView::writeConfiguration(int level, MusECore::Xml& xml)
      {
      xml.tag(level++, "arrangerview");
      TopWin::writeConfiguration(ARRANGER, level, xml);
      arranger->writeConfiguration(level,xml);
      xml.tag(level, "/arrangerview");
      }

//---------------------------------------------------------
//   tagItems
//---------------------------------------------------------

void ArrangerView::tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const
{
  const bool tagSelected = options._flags & MusECore::TagSelected;
  const bool tagAllItems = options._flags & MusECore::TagAllItems;
  const bool tagAllParts = options._flags & MusECore::TagAllParts;
  const bool range       = options._flags & MusECore::TagRange;
  const MusECore::Pos& p0 = options._p0;
  const MusECore::Pos& p1 = options._p1;
  
  if(tagAllParts || tagAllItems)
  {
    MusECore::Track* track;
    MusECore::PartList* pl;
    MusECore::Part* part;
    MusECore::Pos pos, part_pos, part_endpos;
    MusECore::TrackList* tl = MusEGlobal::song->tracks();
    
    for(MusECore::ciTrack it = tl->begin(); it != tl->end(); ++it)
    {
      track = *it;
      pl = track->parts();
      for(MusECore::ciPart ip = pl->begin(); ip != pl->end(); ++ip)
      {
        part = ip->second;
        if(!tagAllParts && !track->isVisible())
          continue;
        if(tagAllParts 
           || (tagSelected && part->selected())
           // || (tagMoving && part->isMoving()) // TODO
          )
        {
          if(tagAllItems)
          {
            if(range)
            {
              part_pos = *part;
              part_endpos = part->end();
              // Optimize: Is the part within the range?
              // p1 should be considered outside (one past) the very last position in the range.
              if(part_endpos <= p0 || part_pos >= p1)
                continue;
            }
            MusECore::EventList& el = part->nonconst_events();
            for(MusECore::iEvent ie = el.begin(); ie != el.end(); ++ie)
            {
              const MusECore::Event& e = ie->second;
              if(range)
              {
                // Don't forget to add the part's position.
                pos = e.pos() + part_pos;
                // If the event position is before p0, keep looking...
                if(pos < p0)
                  continue;
                // If the event position is at or after p1 then we are done.
                // p1 should be considered outside (one past) the very last position in the range.
                if(pos >= p1)
                  break;
              }
              tag_list->add(part, e);
            }
          }
          else
          {
            tag_list->add(part);
          }
        }
      }
    }
  }
  else
  {
    // This step uses the tagging features to mark the objects (events)
    //  as having been visited already, to avoid duplicates in the list.
    if(arranger && arranger->getCanvas())
    {
      MusECore::EventTagOptionsStruct opts = options;
      opts.removeFlags(MusECore::TagAllItems | MusECore::TagAllParts);
      arranger->getCanvas()->tagItems(tag_list, opts);
    }
  }
}

void ArrangerView::cmd(int cmd)
      {
      // Don't process if user is dragging or has clicked on the parts. 
      // Causes problems/crashes later in Canvas::viewMouseMoveEvent and viewMouseReleaseEvent.
      if(arranger && arranger->getCanvas() && arranger->getCanvas()->getCurrentDrag())
        return;
      
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      int l = MusEGlobal::song->lpos();
      int r = MusEGlobal::song->rpos();

      MusECore::TagEventList tag_list;
      
      const FunctionDialogElements_t fn_element_dflt =
        FunctionAllEventsButton |
        FunctionLoopedButton |
        FunctionAllPartsButton | 
        FunctionSelectedPartsButton;

      switch(cmd) {
            case CMD_CUT:
                  arranger->cmd(Arranger::CMD_CUT_PART);
                  break;
            case CMD_COPY:
                  arranger->cmd(Arranger::CMD_COPY_PART);
                  break;
            case CMD_COPY_RANGE:
                  arranger->cmd(Arranger::CMD_COPY_PART_IN_RANGE);
                  break;
            case CMD_PASTE:
                  arranger->cmd(Arranger::CMD_PASTE_PART);
                  break;
            case CMD_PASTE_CLONE:
                  arranger->cmd(Arranger::CMD_PASTE_CLONE_PART);
                  break;
            case CMD_PASTE_TO_TRACK:
                  arranger->cmd(Arranger::CMD_PASTE_PART_TO_TRACK);
                  break;
            case CMD_PASTE_CLONE_TO_TRACK:
                  arranger->cmd(Arranger::CMD_PASTE_CLONE_PART_TO_TRACK);
                  break;
            case CMD_PASTE_DIALOG:
                  arranger->cmd(Arranger::CMD_PASTE_DIALOG);
                  break;
            case CMD_INSERTMEAS:
                  arranger->cmd(Arranger::CMD_INSERT_EMPTYMEAS);
                  break;
            case CMD_DELETE:
                  if (!MusECore::delete_selected_parts())
                  {
                      QMessageBox::StandardButton btn = QMessageBox::warning(
                          this,tr("Remove track(s)"),tr("Are you sure you want to remove this track(s)?"),
                          QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Ok);

                      if (btn == QMessageBox::Cancel)
                          break;
                      MusEGlobal::audio->msgRemoveTracks();
                  }

                  break;
            case CMD_DELETE_TRACK: // from menu
                  {
                  MusEGlobal::audio->msgRemoveTracks();
                  }
                  break;

            case CMD_DUPLICATE_TRACK:
                  MusEGlobal::song->duplicateTracks(); 
                  break;

            case CMD_SELECT_ALL:
            case CMD_SELECT_NONE:
            case CMD_SELECT_INVERT:
            case CMD_SELECT_ILOOP:
            case CMD_SELECT_OLOOP:
                  for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); ++i) {
                        MusECore::PartList* parts = (*i)->parts();
                        for (MusECore::iPart p = parts->begin(); p != parts->end(); ++p) {
                              bool f = false;
                              int t1 = p->second->tick();
                              int t2 = t1 + p->second->lenTick();
                              bool inside =
                                 ((t1 >= l) && (t1 < r))
                                 ||  ((t2 > l) && (t2 < r))
                                 ||  ((t1 <= l) && (t2 > r));
                              switch(cmd) {
                                    case CMD_SELECT_INVERT:
                                          f = !p->second->selected();
                                          break;
                                    case CMD_SELECT_NONE:
                                          f = false;
                                          break;
                                    case CMD_SELECT_ALL:
                                          f = true;
                                          break;
                                    case CMD_SELECT_ILOOP:
                                          f = inside;
                                          break;
                                    case CMD_SELECT_OLOOP:
                                          f = !inside;
                                          break;
                                    }
                              p->second->setSelected(f);
                              }
                        }
                  MusEGlobal::song->update();
                  break;

            case CMD_SELECT_PARTS:
                  for (MusECore::iTrack i = tracks->begin(); i != tracks->end(); ++i) {
                        if (!(*i)->selected())
                              continue;
                        MusECore::PartList* parts = (*i)->parts();
                        for (MusECore::iPart p = parts->begin(); p != parts->end(); ++p)
                              p->second->setSelected(true);
                        }
                  MusEGlobal::song->update();
                  break;
                  
            case CMD_SHRINK_PART: MusECore::shrink_parts(); break;
            case CMD_EXPAND_PART: MusECore::expand_parts(); break;
            case CMD_CLEAN_PART: MusECore::clean_parts(); break;      

            case CMD_QUANTIZE:
                  {
                  FunctionDialogReturnQuantize ret =
                    quantize_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::quantize_items(&tag_list, ret._raster_index,
                                            ret._quant_len,
                                            ret._strength,
                                            ret._swing,
                                            ret._threshold);
                  }
                  break;
                  }
            
            case CMD_VELOCITY:
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
            case CMD_CRESCENDO:
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
            case CMD_NOTELEN:
                  {
                  FunctionDialogReturnGateTime ret =
                    gatetime_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::modify_notelen_items(&tag_list, ret._rateVal, ret._offsetVal);
                  }
                  break;
                  }
            case CMD_TRANSPOSE:
                  {
                  FunctionDialogReturnTranspose ret =
                    transpose_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::transpose_items(&tag_list, ret._amount);
                  }
                  break;
                  }
            
            case CMD_ERASE:
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
            
            case CMD_MOVE:
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
            case CMD_FIXED_LEN:
                  {
                  FunctionDialogReturnSetLen ret =
                    setlen_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::set_notelen_items(&tag_list, ret._len);
                  }
                  break;
                  }
            case CMD_DELETE_OVERLAPS:
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
            case CMD_LEGATO:
                  {
                  FunctionDialogReturnLegato ret =
                    legato_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                  {
                    tagItems(&tag_list, MusECore::EventTagOptionsStruct::fromOptions(
                      ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1));
                    MusECore::legato_items(&tag_list, ret._min_len, !ret._allow_shortening);
                  }
                  break;
                  }

            }
      }

void ArrangerView::scoreNamingChanged()
{
  updateScoreMenus();
}

void ArrangerView::updateScoreMenus()
{
  QAction* action;

  
  scoreOneStaffPerTrackSubsubmenu->clear();
  scoreAllInOneSubsubmenu->clear();

  
  action=new QAction(tr("New..."), this);
  connect(action, &QAction::triggered, []() { MusEGlobal::muse->openInScoreEdit_oneStaffPerTrack(nullptr); } );
  scoreOneStaffPerTrackSubsubmenu->addAction(action);
  
  
  action=new QAction(tr("New..."), this); //the above action may NOT be reused!
  connect(action, &QAction::triggered, []() { MusEGlobal::muse->openInScoreEdit_allInOne(nullptr); } );
  scoreAllInOneSubsubmenu->addAction(action);

  const ToplevelList* toplevels=MusEGlobal::muse->getToplevels();

  for (ToplevelList::const_iterator it=toplevels->begin(); it!=toplevels->end(); it++)
    if ((*it)->type()==TopWin::SCORE)
    {
      ScoreEdit* score = dynamic_cast<ScoreEdit*>(*it);
      
      action=new QAction(score->get_name(), this);
      connect(action, &QAction::triggered, [score]() { MusEGlobal::muse->openInScoreEdit_oneStaffPerTrack(score); } );
      scoreOneStaffPerTrackSubsubmenu->addAction(action);


      action=new QAction(score->get_name(), this); //the above action may NOT be reused!
      connect(action, &QAction::triggered, [score]() { MusEGlobal::muse->openInScoreEdit_allInOne(score); } );
      scoreAllInOneSubsubmenu->addAction(action);
    }
}

// populate both add and insert menus with track types
void ArrangerView::populateAddTrack()
{
      // populate add track menu
      QActionGroup *addGroup = MusEGui::populateAddTrack(addTrack, true, true);
      connect(addTrack, SIGNAL(triggered(QAction *)), SLOT(addNewTrack(QAction *)));

      int idx = 0;
      trackAMidiAction = addGroup->actions()[idx++];
      trackADrumAction = addGroup->actions()[idx++];
      trackAWaveAction = addGroup->actions()[idx++];
      trackAOutputAction = addGroup->actions()[idx++];
      trackAGroupAction = addGroup->actions()[idx++];
      trackAInputAction = addGroup->actions()[idx++];
      trackAAuxAction = addGroup->actions()[idx++];

      // populate insert track menu
      QActionGroup *insertGroup = MusEGui::populateAddTrack(insertTrack, true, true);
      connect(insertTrack, SIGNAL(triggered(QAction *)), SLOT(insertNewTrack(QAction *)));

      idx = 0;
      trackIMidiAction = insertGroup->actions()[idx++];
      trackIDrumAction = insertGroup->actions()[idx++];
      trackIWaveAction = insertGroup->actions()[idx++];
      trackIOutputAction = insertGroup->actions()[idx++];
      trackIGroupAction = insertGroup->actions()[idx++];
      trackIInputAction = insertGroup->actions()[idx++];
      trackIAuxAction = insertGroup->actions()[idx++];

      // populate right click menu on trackList
      arranger->getTrackList()->populateAddTrack();
}

void ArrangerView::addNewTrack(QAction* action)
{
  MusEGlobal::song->addNewTrack(action, NULL);  // Add at the end
}

void ArrangerView::insertNewTrack(QAction* action)
{
  auto curTrack = MusEGlobal::muse->arranger()->curTrack();
  MusEGlobal::song->addNewTrack(action, curTrack);  // Insert before current selected track or at the end
}

void ArrangerView::updateShortcuts()
{
      editDeleteAction->setShortcut(shortcuts[SHRT_DELETE].key);
      editCutAction->setShortcut(shortcuts[SHRT_CUT].key);
      editCopyAction->setShortcut(shortcuts[SHRT_COPY].key);
      editCopyRangeAction->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
      editPasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      editPasteCloneAction->setShortcut(shortcuts[SHRT_PASTE_CLONE].key);
      editPasteToTrackAction->setShortcut(shortcuts[SHRT_PASTE_TO_TRACK].key);
      editPasteCloneToTrackAction->setShortcut(shortcuts[SHRT_PASTE_CLONE_TO_TRACK].key);
      editPasteDialogAction->setShortcut(shortcuts[SHRT_PASTE_DIALOG].key);
      editInsertEMAction->setShortcut(shortcuts[SHRT_INSERTMEAS].key);
      editDuplicateSelTrackAction->setShortcut(shortcuts[SHRT_DUPLICATE_TRACK].key);

      //editDeleteSelectedAction has no acceleration
      
      trackAMidiAction->setShortcut(shortcuts[SHRT_ADD_MIDI_TRACK].key);
      trackADrumAction->setShortcut(shortcuts[SHRT_ADD_DRUM_TRACK].key);
      trackAWaveAction->setShortcut(shortcuts[SHRT_ADD_WAVE_TRACK].key);
      trackAOutputAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_OUTPUT].key);
      trackAGroupAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_GROUP].key);
      trackAInputAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_INPUT].key);
      trackAAuxAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_AUX].key);

      trackIMidiAction->setShortcut(shortcuts[SHRT_INSERT_MIDI_TRACK].key);
      trackIDrumAction->setShortcut(shortcuts[SHRT_INSERT_DRUM_TRACK].key);
      trackIWaveAction->setShortcut(shortcuts[SHRT_INSERT_WAVE_TRACK].key);
      trackIOutputAction->setShortcut(shortcuts[SHRT_INSERT_AUDIO_OUTPUT].key);
      trackIGroupAction->setShortcut(shortcuts[SHRT_INSERT_AUDIO_GROUP].key);
      trackIInputAction->setShortcut(shortcuts[SHRT_INSERT_AUDIO_INPUT].key);
      trackIAuxAction->setShortcut(shortcuts[SHRT_INSERT_AUDIO_AUX].key);

      editSelectAllAction->setShortcut(shortcuts[SHRT_SELECT_ALL].key);
      editDeselectAllAction->setShortcut(shortcuts[SHRT_SELECT_NONE].key);
      editInvertSelectionAction->setShortcut(shortcuts[SHRT_SELECT_INVERT].key);
      editInsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
      editOutsideLoopAction->setShortcut(shortcuts[SHRT_SELECT_OLOOP].key);
      editAllPartsAction->setShortcut(shortcuts[SHRT_SELECT_PRTSTRACK].key);

      startPianoEditAction->setShortcut(shortcuts[SHRT_OPEN_PIANO].key);
      startDrumEditAction->setShortcut(shortcuts[SHRT_OPEN_DRUMS].key);
      startListEditAction->setShortcut(shortcuts[SHRT_OPEN_LIST].key);
      startWaveEditAction->setShortcut(shortcuts[SHRT_OPEN_WAVE].key);
      openCurrentTrackSynthGuiAction->setShortcut(shortcuts[SHRT_OPEN_PLUGIN_GUI].key);

      midiTransformerAction->setShortcut(shortcuts[SHRT_OPEN_MIDI_TRANSFORM].key);
      strGlobalCutAction->setShortcut(shortcuts[SHRT_GLOBAL_CUT].key);
      strGlobalInsertAction->setShortcut(shortcuts[SHRT_GLOBAL_INSERT].key);
      strGlobalSplitAction->setShortcut(shortcuts[SHRT_GLOBAL_SPLIT].key);
}

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void ArrangerView::clipboardChanged()
      {
      bool flag = false;
      if(QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-midipartlist")) ||
         QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-wavepartlist")) ||
         QApplication::clipboard()->mimeData()->hasFormat(QString("text/x-muse-mixedpartlist")))
        flag = true;
      
      editPasteAction->setEnabled(flag);
      editPasteCloneAction->setEnabled(flag);
      editPasteToTrackAction->setEnabled(flag);
      editPasteCloneToTrackAction->setEnabled(flag);
      editPasteDialogAction->setEnabled(flag);
      }

//---------------------------------------------------------
//   selectionChanged
//   NOTE: This is received upon EITHER a part or track selection change from the Arranger.
//---------------------------------------------------------

void ArrangerView::selectionChanged()
      {
      bool pflag = arranger->itemsAreSelected();
      bool tflag = MusECore::tracks_are_selected();

      editDeleteAction->setEnabled(tflag || pflag);

      editDeleteSelectedAction->setEnabled(tflag);
      editDuplicateSelTrackAction->setEnabled(tflag);
   
      editCutAction->setEnabled(pflag);
      editCopyAction->setEnabled(pflag);
      editShrinkPartsAction->setEnabled(pflag);
      editExpandPartsAction->setEnabled(pflag);
      editCleanPartsAction->setEnabled(pflag);
      }


void ArrangerView::updateVisibleTracksButtons()
{
  visTracks->updateVisibleTracksButtons();
}

Arranger* ArrangerView::getArranger() const {return arranger;}

void ArrangerView::focusCanvas() { arranger->focusCanvas(); } 

void ArrangerView::globalCut() { MusECore::globalCut(); }
void ArrangerView::globalInsert() { MusECore::globalInsert(); }
void ArrangerView::globalSplit() { MusECore::globalSplit(); }

// variants only applicable for selected tracks
void ArrangerView::globalCutSel() { MusECore::globalCut(true); }
void ArrangerView::globalInsertSel() { MusECore::globalInsert(true); }
void ArrangerView::globalSplitSel() { MusECore::globalSplit(true); }

void ArrangerView::openCurrentTrackSynthGui()
{
  auto curTrack = MusEGlobal::muse->arranger()->curTrack();

  if(curTrack->isSynthTrack()) {

    MusECore::SynthI* synth = static_cast<MusECore::SynthI*>(curTrack);

    if (synth->hasNativeGui()) {

      synth->showNativeGui(!synth->nativeGuiVisible());
    }
    else if (synth->hasGui()) {

      synth->showGui(!synth->guiVisible());
    }
  }
}

void ArrangerView::configCustomColumns()
{
    auto tmp = Arranger::custom_columns;
    ArrangerColumns* dialog = new ArrangerColumns(this);
    int rc = dialog->exec();
    delete dialog;

    if (rc == QDialog::Accepted)
        arranger->updateHeaderCustomColumns();
    else
        Arranger::custom_columns = tmp;
}

void ArrangerView::toggleMixerStrip()
{
    arranger->toggleTrackInfo();
}

} // namespace MusEGui
