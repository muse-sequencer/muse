//=========================================================
//  MusE
//  Linux Music Editor
//  arrangerview.cpp
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
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
#include <QCloseEvent>
#include <QDir>
#include <QGridLayout>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSignalMapper>
#include <QSizeGrip>
#include <QToolButton>
#include <QToolTip>

#include "al/sig.h"
#include "app.h"
#include "arrangerview.h"
#include "audio.h"
#include "functions.h"
#include "gconfig.h"
#include "globals.h"
#include "helper.h"
#include "icons.h"
#include "mtscale.h"
#include "scoreedit.h"
#include "shortcuts.h"
#include "song.h"
#include "structure.h"
#include "tb1.h"
#include "tools.h"
#include "ttoolbar.h"
#include "visibletracks.h"
#include "xml.h"
#include "arrangercolumns.h"

namespace MusEGui {

//---------------------------------------------------------
//   ArrangerView
//---------------------------------------------------------

ArrangerView::ArrangerView(QWidget* parent)
   : TopWin(TopWin::ARRANGER, parent, "arrangerview", Qt::Window)
{
  setWindowTitle(tr("MusE: Arranger"));
  setFocusPolicy(Qt::NoFocus);  

  arranger = new Arranger(this, "arranger");
  setCentralWidget(arranger);
  //setFocusProxy(arranger);

  scoreOneStaffPerTrackMapper = new QSignalMapper(this);
  scoreAllInOneMapper = new QSignalMapper(this);

  editSignalMapper = new QSignalMapper(this);
  QShortcut* sc = new QShortcut(shortcuts[SHRT_DELETE].key, this);
  sc->setContext(Qt::WindowShortcut);
  connect(sc, SIGNAL(activated()), editSignalMapper, SLOT(map()));
  editSignalMapper->setMapping(sc, CMD_DELETE);

  // Toolbars ---------------------------------------------------------
  editTools = new EditToolBar(this, arrangerTools);
  addToolBar(editTools);
  editTools->setObjectName("arrangerTools");

  visTracks = new VisibleTracks(this);
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
  connect(arranger, SIGNAL(selectionChanged()), SLOT(selectionChanged()));
  connect(MusEGlobal::song, SIGNAL(songChanged(int)), visTracks, SLOT(updateVisibleTracksButtons()));






  //-------- Edit Actions
  editCutAction = new QAction(QIcon(*editcutIconSet), tr("C&ut"), this);
  editCopyAction = new QAction(QIcon(*editcopyIconSet), tr("&Copy"), this);
  editCopyRangeAction = new QAction(QIcon(*editcopyIconSet), tr("Copy in range"), this);
  editPasteAction = new QAction(QIcon(*editpasteIconSet), tr("&Paste"), this);
  editPasteDialogAction = new QAction(QIcon(*editpasteIconSet), tr("Paste (show dialog)"), this);
  editPasteCloneAction = new QAction(QIcon(*editpasteCloneIconSet), tr("Paste c&lone"), this);
  editPasteCloneDialogAction = new QAction(QIcon(*editpasteCloneIconSet), tr("Paste clone (show dialog)"), this);
  editInsertEMAction = new QAction(QIcon(*editpasteIconSet), tr("&Insert Empty Measure"), this);
  editDeleteSelectedAction = new QAction(QIcon(*edit_track_delIcon), tr("Delete Selected Tracks"), this);
  editDuplicateSelTrackAction = new QAction(QIcon(*edit_track_addIcon), tr("Duplicate Selected Tracks"), this);

  editShrinkPartsAction = new QAction(tr("Shrink selected parts"), this);
  editExpandPartsAction = new QAction(tr("Expand selected parts"), this);
  editCleanPartsAction = new QAction(tr("Clean selected parts"), this);


  addTrack = new QMenu(tr("Add Track"), this);
  addTrack->setIcon(QIcon(*edit_track_addIcon));
  select = new QMenu(tr("Select"), this);
  select->setIcon(QIcon(*selectIcon));

  editSelectAllAction = new QAction(QIcon(*select_allIcon), tr("Select &All"), this);
  editDeselectAllAction = new QAction(QIcon(*select_deselect_allIcon), tr("&Deselect All"), this);
  editInvertSelectionAction = new QAction(QIcon(*select_invert_selectionIcon), tr("Invert &Selection"), this);
  editInsideLoopAction = new QAction(QIcon(*select_inside_loopIcon), tr("&Inside Loop"), this);
  editOutsideLoopAction = new QAction(QIcon(*select_outside_loopIcon), tr("&Outside Loop"), this);
  editAllPartsAction = new QAction( QIcon(*select_all_parts_on_trackIcon), tr("All &Parts on Track"), this);

	
  scoreSubmenu = new QMenu(tr("Score"), this);
  scoreSubmenu->setIcon(QIcon(*scoreIconSet));

  scoreAllInOneSubsubmenu = new QMenu(tr("all tracks in one staff"), this);
  scoreOneStaffPerTrackSubsubmenu = new QMenu(tr("one staff per track"), this);

  scoreSubmenu->addMenu(scoreAllInOneSubsubmenu);
  scoreSubmenu->addMenu(scoreOneStaffPerTrackSubsubmenu);
  updateScoreMenus();

  startScoreEditAction = new QAction(*scoreIconSet, tr("New score window"), this);
  startPianoEditAction = new QAction(*pianoIconSet, tr("Pianoroll"), this);
  startDrumEditAction = new QAction(QIcon(*edit_drummsIcon), tr("Drums"), this);
  startListEditAction = new QAction(QIcon(*edit_listIcon), tr("List"), this);
  startWaveEditAction = new QAction(QIcon(*edit_waveIcon), tr("Wave"), this);

  master = new QMenu(tr("Mastertrack"), this);
  master->setIcon(QIcon(*edit_mastertrackIcon));
  masterGraphicAction = new QAction(QIcon(*mastertrack_graphicIcon),tr("Graphic"), this);
  masterListAction = new QAction(QIcon(*mastertrack_listIcon),tr("List"), this);

  midiTransformerAction = new QAction(QIcon(*midi_transformIcon), tr("Midi &Transform"), this);


  //-------- Structure Actions
  strGlobalCutAction = new QAction(tr("Global Cut"), this);
  strGlobalInsertAction = new QAction(tr("Global Insert"), this);
  strGlobalSplitAction = new QAction(tr("Global Split"), this);

  strGlobalCutSelAction = new QAction(tr("Global Cut - selected tracks"), this);
  strGlobalInsertSelAction = new QAction(tr("Global Insert - selected tracks"), this);
  strGlobalSplitSelAction = new QAction(tr("Global Split - selected tracks"), this);



  //-------------------------------------------------------------
  //    popup Edit
  //-------------------------------------------------------------

  QMenu* menuEdit = menuBar()->addMenu(tr("&Edit"));
  menuEdit->addActions(MusEGlobal::undoRedo->actions());
  menuEdit->addSeparator();

  menuEdit->addAction(editCutAction);
  menuEdit->addAction(editCopyAction);
  menuEdit->addAction(editCopyRangeAction);
  menuEdit->addAction(editPasteAction);
  menuEdit->addAction(editPasteDialogAction);
  menuEdit->addAction(editPasteCloneAction);
  menuEdit->addAction(editPasteCloneDialogAction);
  menuEdit->addAction(editInsertEMAction);
  menuEdit->addSeparator();
  menuEdit->addAction(editShrinkPartsAction);
  menuEdit->addAction(editExpandPartsAction);
  menuEdit->addAction(editCleanPartsAction);
  menuEdit->addSeparator();
  menuEdit->addAction(editDeleteSelectedAction);

  menuEdit->addMenu(addTrack);
  menuEdit->addAction(editDuplicateSelTrackAction);
  menuEdit->addMenu(select);
    select->addAction(editSelectAllAction);
    select->addAction(editDeselectAllAction);
    select->addAction(editInvertSelectionAction);
    select->addAction(editInsideLoopAction);
    select->addAction(editOutsideLoopAction);
    select->addAction(editAllPartsAction);
  menuEdit->addSeparator();

  menuEdit->addAction(startPianoEditAction);
  menuEdit->addMenu(scoreSubmenu);
  menuEdit->addAction(startScoreEditAction);
  menuEdit->addAction(startDrumEditAction);
  menuEdit->addAction(startListEditAction);
  menuEdit->addAction(startWaveEditAction);

  menuEdit->addMenu(master);
    master->addAction(masterGraphicAction);
    master->addAction(masterListAction);
  menuEdit->addSeparator();

  menuEdit->addAction(midiTransformerAction);

  QMenu* menuStructure = menuEdit->addMenu(tr("&Structure"));
    menuStructure->addAction(strGlobalCutAction);
    menuStructure->addAction(strGlobalInsertAction);
    menuStructure->addAction(strGlobalSplitAction);
    menuStructure->addSeparator();
    menuStructure->addAction(strGlobalCutSelAction);
    menuStructure->addAction(strGlobalInsertSelAction);
    menuStructure->addAction(strGlobalSplitSelAction);

  
  
  QMenu* functions_menu = menuBar()->addMenu(tr("Functions"));
		QAction* func_quantize_action = functions_menu->addAction(tr("&Quantize Notes"), editSignalMapper, SLOT(map()));
		QAction* func_notelen_action = functions_menu->addAction(tr("Change note &length"), editSignalMapper, SLOT(map()));
		QAction* func_velocity_action = functions_menu->addAction(tr("Change note &velocity"), editSignalMapper, SLOT(map()));
		QAction* func_cresc_action = functions_menu->addAction(tr("Crescendo/Decrescendo"), editSignalMapper, SLOT(map()));
		QAction* func_transpose_action = functions_menu->addAction(tr("Transpose"), editSignalMapper, SLOT(map()));
		QAction* func_erase_action = functions_menu->addAction(tr("Erase Events (Not Parts)"), editSignalMapper, SLOT(map()));
		QAction* func_move_action = functions_menu->addAction(tr("Move Events (Not Parts)"), editSignalMapper, SLOT(map()));
		QAction* func_fixed_len_action = functions_menu->addAction(tr("Set Fixed Note Length"), editSignalMapper, SLOT(map()));
		QAction* func_del_overlaps_action = functions_menu->addAction(tr("Delete Overlapping Notes"), editSignalMapper, SLOT(map()));
		QAction* func_legato_action = functions_menu->addAction(tr("Legato"), editSignalMapper, SLOT(map()));
		editSignalMapper->setMapping(func_quantize_action, CMD_QUANTIZE);
		editSignalMapper->setMapping(func_notelen_action, CMD_NOTELEN);
		editSignalMapper->setMapping(func_velocity_action, CMD_VELOCITY);
		editSignalMapper->setMapping(func_cresc_action, CMD_CRESCENDO);
		editSignalMapper->setMapping(func_transpose_action, CMD_TRANSPOSE);
		editSignalMapper->setMapping(func_erase_action, CMD_ERASE);
		editSignalMapper->setMapping(func_move_action, CMD_MOVE);
		editSignalMapper->setMapping(func_fixed_len_action, CMD_FIXED_LEN);
		editSignalMapper->setMapping(func_del_overlaps_action, CMD_DELETE_OVERLAPS);
		editSignalMapper->setMapping(func_legato_action, CMD_LEGATO);

  
  
  QMenu* menuSettings = menuBar()->addMenu(tr("Window &Config"));
  menuSettings->addAction(tr("Configure &custom columns"), this, SLOT(configCustomColumns()));
  menuSettings->addSeparator();
  menuSettings->addAction(subwinAction);
  menuSettings->addAction(shareAction);
  menuSettings->addAction(fullscreenAction);


  //-------- Edit connections
  connect(editCutAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editCopyAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editCopyRangeAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteCloneAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteDialogAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteCloneDialogAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editInsertEMAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editDeleteSelectedAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editDuplicateSelTrackAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));

  connect(editShrinkPartsAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editExpandPartsAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editCleanPartsAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));

  connect(editSelectAllAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editDeselectAllAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editInvertSelectionAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editInsideLoopAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editOutsideLoopAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editAllPartsAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));

  editSignalMapper->setMapping(editCutAction, CMD_CUT);
  editSignalMapper->setMapping(editCopyAction, CMD_COPY);
  editSignalMapper->setMapping(editCopyRangeAction, CMD_COPY_RANGE);
  editSignalMapper->setMapping(editPasteAction, CMD_PASTE);
  editSignalMapper->setMapping(editPasteCloneAction, CMD_PASTE_CLONE);
  editSignalMapper->setMapping(editPasteDialogAction, CMD_PASTE_DIALOG);
  editSignalMapper->setMapping(editPasteCloneDialogAction, CMD_PASTE_CLONE_DIALOG);
  editSignalMapper->setMapping(editInsertEMAction, CMD_INSERTMEAS);
  editSignalMapper->setMapping(editDeleteSelectedAction, CMD_DELETE_TRACK);
  editSignalMapper->setMapping(editDuplicateSelTrackAction, CMD_DUPLICATE_TRACK);
  editSignalMapper->setMapping(editShrinkPartsAction, CMD_SHRINK_PART);
  editSignalMapper->setMapping(editExpandPartsAction, CMD_EXPAND_PART);
  editSignalMapper->setMapping(editCleanPartsAction, CMD_CLEAN_PART);
  editSignalMapper->setMapping(editSelectAllAction, CMD_SELECT_ALL);
  editSignalMapper->setMapping(editDeselectAllAction, CMD_SELECT_NONE);
  editSignalMapper->setMapping(editInvertSelectionAction, CMD_SELECT_INVERT);
  editSignalMapper->setMapping(editInsideLoopAction, CMD_SELECT_ILOOP);
  editSignalMapper->setMapping(editOutsideLoopAction, CMD_SELECT_OLOOP);
  editSignalMapper->setMapping(editAllPartsAction, CMD_SELECT_PARTS);

  connect(editSignalMapper, SIGNAL(mapped(int)), this, SLOT(cmd(int)));

  connect(startPianoEditAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startPianoroll()));
  connect(startScoreEditAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startScoreQuickly()));
  connect(startDrumEditAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startDrumEditor()));
  connect(startListEditAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startListEditor()));
  connect(startWaveEditAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startWaveEditor()));
  connect(scoreOneStaffPerTrackMapper, SIGNAL(mapped(QWidget*)), MusEGlobal::muse, SLOT(openInScoreEdit_oneStaffPerTrack(QWidget*)));
  connect(scoreAllInOneMapper, SIGNAL(mapped(QWidget*)), MusEGlobal::muse, SLOT(openInScoreEdit_allInOne(QWidget*)));


  connect(masterGraphicAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startMasterEditor()));
  connect(masterListAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startLMasterEditor()));

  connect(midiTransformerAction, SIGNAL(activated()), MusEGlobal::muse, SLOT(startMidiTransformer()));


  //-------- Structure connections
  connect(strGlobalCutAction, SIGNAL(activated()), SLOT(globalCut()));
  connect(strGlobalInsertAction, SIGNAL(activated()), SLOT(globalInsert()));
  connect(strGlobalSplitAction, SIGNAL(activated()), SLOT(globalSplit()));
  connect(strGlobalCutSelAction, SIGNAL(activated()), SLOT(globalCutSel()));
  connect(strGlobalInsertSelAction, SIGNAL(activated()), SLOT(globalInsertSel()));
  connect(strGlobalSplitSelAction, SIGNAL(activated()), SLOT(globalSplitSel()));



  connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(updateShortcuts()));


  QClipboard* cb = QApplication::clipboard();
  connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));
  connect(cb, SIGNAL(selectionChanged()), SLOT(clipboardChanged()));

  finalizeInit();

  // work around for probable QT/WM interaction bug.
  // for certain window managers, e.g xfce, this window is
  // is displayed although not specifically set to show();
  // bug: 2811156     Softsynth GUI unclosable with XFCE4 (and a few others)
  show();
  hide();
}

ArrangerView::~ArrangerView()
{
  
}

void ArrangerView::closeEvent(QCloseEvent* e)
{
  emit isDeleting(static_cast<TopWin*>(this));
  emit closed();
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


void ArrangerView::cmd(int cmd)
      {
      MusECore::TrackList* tracks = MusEGlobal::song->tracks();
      int l = MusEGlobal::song->lpos();
      int r = MusEGlobal::song->rpos();

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
            case CMD_PASTE_DIALOG:
                  arranger->cmd(Arranger::CMD_PASTE_DIALOG);
                  break;
            case CMD_PASTE_CLONE_DIALOG:
                  arranger->cmd(Arranger::CMD_PASTE_CLONE_DIALOG);
                  break;
            case CMD_INSERTMEAS:
                  arranger->cmd(Arranger::CMD_INSERT_EMPTYMEAS);
                  break;
            case CMD_DELETE:
                  if (!MusEGlobal::song->msgRemoveParts()) //automatically does undo if neccessary and returns true then
                  {
                        //msgRemoveParts() returned false -> no parts to remove?
                        MusEGlobal::song->startUndo();
                        MusEGlobal::audio->msgRemoveTracks(); //TODO FINDME this could still be speeded up!
                        MusEGlobal::song->endUndo(SC_TRACK_REMOVED);
                  }
                  break;
            case CMD_DELETE_TRACK:
                  MusEGlobal::song->startUndo();
                  MusEGlobal::audio->msgRemoveTracks();
                  MusEGlobal::song->endUndo(SC_TRACK_REMOVED);
                  MusEGlobal::audio->msgUpdateSoloStates();
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

            case CMD_QUANTIZE: MusECore::quantize_notes(); break;
            case CMD_VELOCITY: MusECore::modify_velocity(); break;
            case CMD_CRESCENDO: MusECore::crescendo(); break;
            case CMD_NOTELEN: MusECore::modify_notelen(); break;
            case CMD_TRANSPOSE: MusECore::transpose_notes(); break;
            case CMD_ERASE: MusECore::erase_notes(); break;
            case CMD_MOVE: MusECore::move_notes(); break;
            case CMD_FIXED_LEN: MusECore::set_notelen(); break;
            case CMD_DELETE_OVERLAPS: MusECore::delete_overlaps(); break;
            case CMD_LEGATO: MusECore::legato(); break;

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

  
  action=new QAction(tr("New"), this);
  connect(action, SIGNAL(activated()), scoreOneStaffPerTrackMapper, SLOT(map()));
  scoreOneStaffPerTrackMapper->setMapping(action, (QWidget*)NULL);
  scoreOneStaffPerTrackSubsubmenu->addAction(action);
  
  
  action=new QAction(tr("New"), this); //the above action may NOT be reused!
  connect(action, SIGNAL(activated()), scoreAllInOneMapper, SLOT(map()));
  scoreAllInOneMapper->setMapping(action, (QWidget*)NULL);
  scoreAllInOneSubsubmenu->addAction(action);

  const ToplevelList* toplevels=MusEGlobal::muse->getToplevels();

  for (ToplevelList::const_iterator it=toplevels->begin(); it!=toplevels->end(); it++)
    if ((*it)->type()==TopWin::SCORE)
    {
      ScoreEdit* score = dynamic_cast<ScoreEdit*>(*it);
      
      action=new QAction(score->get_name(), this);
      connect(action, SIGNAL(activated()), scoreOneStaffPerTrackMapper, SLOT(map()));
      scoreOneStaffPerTrackMapper->setMapping(action, (QWidget*)score);
      scoreOneStaffPerTrackSubsubmenu->addAction(action);


      action=new QAction(score->get_name(), this); //the above action may NOT be reused!
      connect(action, SIGNAL(activated()), scoreAllInOneMapper, SLOT(map()));
      scoreAllInOneMapper->setMapping(action, (QWidget*)score);
      scoreAllInOneSubsubmenu->addAction(action);
    }
}

void ArrangerView::clearScoreMenuMappers()
{
  delete scoreOneStaffPerTrackMapper;
  delete scoreAllInOneMapper;
  
  scoreOneStaffPerTrackMapper = new QSignalMapper(this);
  scoreAllInOneMapper = new QSignalMapper(this);
  
  connect(scoreOneStaffPerTrackMapper, SIGNAL(mapped(QWidget*)), this, SLOT(openInScoreEdit_oneStaffPerTrack(QWidget*)));
  connect(scoreAllInOneMapper, SIGNAL(mapped(QWidget*)), this, SLOT(openInScoreEdit_allInOne(QWidget*)));
}

void ArrangerView::populateAddTrack()
{
      QActionGroup *grp = MusEGui::populateAddTrack(addTrack, true);
      connect(addTrack, SIGNAL(triggered(QAction *)), SLOT(addNewTrack(QAction *)));
      
      trackMidiAction = grp->actions()[0];
      trackDrumAction = grp->actions()[1];
      trackWaveAction = grp->actions()[2];
      trackAOutputAction = grp->actions()[3];
      trackAGroupAction = grp->actions()[4];
      trackAInputAction = grp->actions()[5];
      trackAAuxAction = grp->actions()[6];
}

void ArrangerView::addNewTrack(QAction* action)
{
  MusEGlobal::song->addNewTrack(action, MusEGlobal::muse->arranger()->curTrack());  // Insert at current selected track.
}

void ArrangerView::updateShortcuts()
{
      editCutAction->setShortcut(shortcuts[SHRT_CUT].key);
      editCopyAction->setShortcut(shortcuts[SHRT_COPY].key);
      editCopyRangeAction->setShortcut(shortcuts[SHRT_COPY_RANGE].key);
      editPasteAction->setShortcut(shortcuts[SHRT_PASTE].key);
      editPasteCloneAction->setShortcut(shortcuts[SHRT_PASTE_CLONE].key);
      editPasteDialogAction->setShortcut(shortcuts[SHRT_PASTE_DIALOG].key);
      editPasteCloneDialogAction->setShortcut(shortcuts[SHRT_PASTE_CLONE_DIALOG].key);
      editInsertEMAction->setShortcut(shortcuts[SHRT_INSERTMEAS].key);

      //editDeleteSelectedAction has no acceleration
      
      trackMidiAction->setShortcut(shortcuts[SHRT_ADD_MIDI_TRACK].key);
      trackDrumAction->setShortcut(shortcuts[SHRT_ADD_DRUM_TRACK].key);
      trackWaveAction->setShortcut(shortcuts[SHRT_ADD_WAVE_TRACK].key);
      trackAOutputAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_OUTPUT].key);
      trackAGroupAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_GROUP].key);
      trackAInputAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_INPUT].key);
      trackAAuxAction->setShortcut(shortcuts[SHRT_ADD_AUDIO_AUX].key);

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

      masterGraphicAction->setShortcut(shortcuts[SHRT_OPEN_GRAPHIC_MASTER].key);
      masterListAction->setShortcut(shortcuts[SHRT_OPEN_LIST_MASTER].key);
  
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
      editPasteDialogAction->setEnabled(flag);
      editPasteCloneDialogAction->setEnabled(flag);
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void ArrangerView::selectionChanged()
      {
      //bool flag = arranger->isSingleSelection();  // -- Hmm, why only single? 
      bool flag = arranger->selectionSize() > 0;    // -- Test OK cut and copy. For muse2. Tim.
      editCutAction->setEnabled(flag);
      editCopyAction->setEnabled(flag);
      }


void ArrangerView::updateVisibleTracksButtons()
{
  visTracks->updateVisibleTracksButtons();
}

void ArrangerView::globalCut() { MusECore::globalCut(); }
void ArrangerView::globalInsert() { MusECore::globalInsert(); }
void ArrangerView::globalSplit() { MusECore::globalSplit(); }

// variants only applicable for selected tracks
void ArrangerView::globalCutSel() { MusECore::globalCut(true); }
void ArrangerView::globalInsertSel() { MusECore::globalInsert(true); }
void ArrangerView::globalSplitSel() { MusECore::globalSplit(true); }

void ArrangerView::configCustomColumns()
{
  ArrangerColumns* dialog = new ArrangerColumns(this);
  dialog->exec();
  delete dialog;
  
  QMessageBox::information(this, tr("Changed Settings"), tr("Unfortunately, the changed arranger column settings\ncannot be applied while MusE is running.\nTo apply the changes, please restart MusE. Sorry.\n(we'll try to fix that)"));
}

} // namespace MusEGui
