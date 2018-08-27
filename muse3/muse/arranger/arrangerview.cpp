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
#include "tlist.h"

namespace MusEGui {

//---------------------------------------------------------
//   ArrangerView
//---------------------------------------------------------

ArrangerView::ArrangerView(QWidget* parent)
   : TopWin(TopWin::ARRANGER, parent, "arrangerview", Qt::Window)
{
  setWindowTitle(tr("MusE: Arranger"));
  setFocusPolicy(Qt::NoFocus);  

  // Already has an object name.
  visTracks = new VisibleTracks(this);
  
  arranger = new Arranger(this, "arranger");
  setCentralWidget(arranger);
  //setFocusProxy(arranger);

  scoreOneStaffPerTrackMapper = new QSignalMapper(this);
  scoreAllInOneMapper = new QSignalMapper(this);

  editSignalMapper = new QSignalMapper(this);

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
// REMOVE Tim. citem. Removed. Moved contents into Arranger songChanged() and configChanged().
//   connect(MusEGlobal::song, SIGNAL(songChanged(MusECore::SongChangedStruct_t)), this, SLOT(songChanged(MusECore::SongChangedStruct_t)));






  //-------- Edit Actions
  editDeleteAction = new QAction(QIcon(*deleteIcon), tr("D&elete"), this);
  editCutAction = new QAction(QIcon(*editcutIconSet), tr("C&ut"), this);
  editCopyAction = new QAction(QIcon(*editcopyIconSet), tr("&Copy"), this);
  editCopyRangeAction = new QAction(QIcon(*editcopyIconSet), tr("Copy in range"), this);
  editPasteAction = new QAction(QIcon(*editpasteIconSet), tr("&Paste"), this);
  editPasteCloneAction = new QAction(QIcon(*editpasteCloneIconSet), tr("Paste c&lone"), this);
  editPasteToTrackAction = new QAction(QIcon(*editpaste2TrackIconSet), tr("Paste to selected &track"), this);
  editPasteCloneToTrackAction = new QAction(QIcon(*editpasteClone2TrackIconSet), tr("Paste clone to selected trac&k"), this);
  editPasteDialogAction = new QAction(QIcon(*editpasteIconSet), tr("Paste (show dialo&g)"), this);
  editInsertEMAction = new QAction(QIcon(*editpasteIconSet), tr("&Insert Empty Measure"), this);
  editDeleteSelectedAction = new QAction(QIcon(*edit_track_delIcon), tr("Delete Selected Tracks"), this);
  editDuplicateSelTrackAction = new QAction(QIcon(*edit_track_addIcon), tr("Duplicate Selected Tracks"), this);

  editShrinkPartsAction = new QAction(tr("Shrink selected parts"), this);
  editExpandPartsAction = new QAction(tr("Expand selected parts"), this);
  editCleanPartsAction = new QAction(tr("Purge hidden events from selected parts"), this);


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

  select->addAction(editSelectAllAction);
  select->addAction(editDeselectAllAction);
  select->addAction(editInvertSelectionAction);
  select->addAction(editInsideLoopAction);
  select->addAction(editOutsideLoopAction);
  select->addAction(editAllPartsAction);
  
	
  scoreSubmenu = new QMenu(tr("Score"), this);
  scoreSubmenu->setIcon(QIcon(*scoreIconSet));

  scoreAllInOneSubsubmenu = new QMenu(tr("all tracks in one staff"), this);
  scoreOneStaffPerTrackSubsubmenu = new QMenu(tr("one staff per track"), this);

  startScoreEditAction = new QAction(*scoreIconSet, tr("New score window"), this);
  scoreSubmenu->addAction(startScoreEditAction);
  
  scoreSubmenu->addMenu(scoreAllInOneSubsubmenu);
  scoreSubmenu->addMenu(scoreOneStaffPerTrackSubsubmenu);
  updateScoreMenus();

  startPianoEditAction = new QAction(*pianoIconSet, tr("Pianoroll"), this);
  startDrumEditAction = new QAction(QIcon(*edit_drummsIcon), tr("Drums"), this);
  startListEditAction = new QAction(QIcon(*edit_listIcon), tr("List"), this);
  startWaveEditAction = new QAction(QIcon(*edit_waveIcon), tr("Wave"), this);

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
  menuEdit->addAction(editDuplicateSelTrackAction);
  menuEdit->addSeparator();

  menuEdit->addAction(startPianoEditAction);
  menuEdit->addMenu(scoreSubmenu);
//   menuEdit->addAction(startScoreEditAction);
  menuEdit->addAction(startDrumEditAction);
  menuEdit->addAction(startListEditAction);
  menuEdit->addAction(startWaveEditAction);

  QMenu* functions_menu = menuBar()->addMenu(tr("Functions"));
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
  functions_menu->addSeparator();
  functions_menu->addAction(editShrinkPartsAction);
  functions_menu->addAction(editExpandPartsAction);
  functions_menu->addAction(editCleanPartsAction);
  
  
  QMenu* menuSettings = menuBar()->addMenu(tr("Window &Config"));
  menuSettings->addAction(tr("Configure &custom columns"), this, SLOT(configCustomColumns()));
  menuSettings->addSeparator();
  menuSettings->addAction(subwinAction);
  menuSettings->addAction(shareAction);
  menuSettings->addAction(fullscreenAction);


  //-------- Edit connections
  connect(editDeleteAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editCutAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editCopyAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editCopyRangeAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteCloneAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteToTrackAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteCloneToTrackAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
  connect(editPasteDialogAction, SIGNAL(triggered()), editSignalMapper, SLOT(map()));
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

  editSignalMapper->setMapping(editDeleteAction, CMD_DELETE);
  editSignalMapper->setMapping(editCutAction, CMD_CUT);
  editSignalMapper->setMapping(editCopyAction, CMD_COPY);
  editSignalMapper->setMapping(editCopyRangeAction, CMD_COPY_RANGE);
  editSignalMapper->setMapping(editPasteAction, CMD_PASTE);
  editSignalMapper->setMapping(editPasteCloneAction, CMD_PASTE_CLONE);
  editSignalMapper->setMapping(editPasteToTrackAction, CMD_PASTE_TO_TRACK);
  editSignalMapper->setMapping(editPasteCloneToTrackAction, CMD_PASTE_CLONE_TO_TRACK);
  editSignalMapper->setMapping(editPasteDialogAction, CMD_PASTE_DIALOG);
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

  connect(startPianoEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startPianoroll()));
  connect(startScoreEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startScoreQuickly()));
  connect(startDrumEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startDrumEditor()));
  connect(startListEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startListEditor()));
  connect(startWaveEditAction, SIGNAL(triggered()), MusEGlobal::muse, SLOT(startWaveEditor()));
  connect(scoreOneStaffPerTrackMapper, SIGNAL(mapped(QWidget*)), MusEGlobal::muse, SLOT(openInScoreEdit_oneStaffPerTrack(QWidget*)));
  connect(scoreAllInOneMapper, SIGNAL(mapped(QWidget*)), MusEGlobal::muse, SLOT(openInScoreEdit_allInOne(QWidget*)));

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
  emit isDeleting(static_cast<TopWin*>(this));
  emit closed();
  e->accept();
}

// REMOVE Tim. citem. Removed. Moved contents into Arranger songChanged() and configChanged().
// //---------------------------------------------------------
// //   songChanged
// //---------------------------------------------------------
// 
// void ArrangerView::songChanged(MusECore::SongChangedStruct_t type)
// {
// // REMOVE Tim. citem. Removed. Moved into Arranger::configChanged().
// //   // TEST Try these, may need more/less. Esp more: Originally songChanged was directly connected to updateVisibleTracksButtons, so... 
// //   if(type._flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
// //              SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED | 
// //              //SC_EVENT_INSERTED | SC_EVENT_REMOVED | SC_EVENT_MODIFIED |
// //              //SC_SIG | SC_TEMPO | SC_MASTER |
// //              //SC_MIDI_TRACK_PROP |
// //              SC_CONFIG | 
// //              SC_DRUMMAP)) 
// //     visTracks->updateVisibleTracksButtons();
//   
// // REMOVE Tim. citem. Removed. Moved into Arranger::songChanged().
// //   if(type._flags & (SC_TRACK_SELECTION | SC_PART_SELECTION | 
// //              SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | 
// //              SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED))
// //     selectionChanged();
// }
      
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


// REMOVE Tim. citem. Added.
//---------------------------------------------------------
//   tagItems
//---------------------------------------------------------

void ArrangerView::tagItems(bool tagAllItems, bool tagAllParts, bool range,
        const MusECore::Pos& p0, const MusECore::Pos& p1) const
{
  if(tagAllParts || tagAllItems)
  {
    MusECore::Track* track;
    MusECore::PartList* pl;
    MusECore::Part* part;
    MusECore::Pos pos;
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
        if(tagAllParts || part->selected())
        {
          if(tagAllItems)
          {
            MusECore::EventList& el = part->nonconst_events();
            for(MusECore::iEvent ie = el.begin(); ie != el.end(); ++ie)
            {
              MusECore::Event& e = ie->second;
              if(range)
              {
                pos = e.pos();
                if(!(pos >= p0 && pos < p1))
                  continue;
              }
              e.setTagged(true);
              part->setEventsTagged(true);
            }
          }
          else
          {
            part->setTagged(true);
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
      arranger->getCanvas()->tagItems(false, false, range, p0, p1);
  }
  
//   // If tagging all items, don't bother with the controller editors below,
//   //  since everything that they could tag will already be tagged.
//   if(tagAllItems)
//   {
//     MusECore::Part* part;
//     MusECore::Pos pos;
//     if(tagAllParts)
//     {
//       if(_pl)
//       {
//         for(MusECore::ciPart ip = _pl->begin(); ip != _pl->end(); ++ip)
//         {
//           part = ip->second;
//           MusECore::EventList& el = part->nonconst_events();
//           for(MusECore::iEvent ie = el.begin(); ie != el.end(); ++ie)
//           {
//             MusECore::Event& e = ie->second;
//             if(range)
//             {
//               pos = e.pos();
//               if(pos >= p0 && pos < p1)
//               {
//                 e.setTagged(true);
//                 part->setEventsTagged(true);
//               }
//             }
//             else
//             {
//               e.setTagged(true);
//               part->setEventsTagged(true);
//             }
//           }
//         }
//       }
//     }
//     else
//     {
//       if(canvas && canvas->part())
//       {
//         part = canvas->part();
//         MusECore::EventList& el = part->nonconst_events();
//         for(MusECore::iEvent ie = el.begin(); ie != el.end(); ++ie)
//         {
//           MusECore::Event& e = ie->second;
//           if(range)
//           {
//             pos = e.pos();
//             if(pos >= p0 && pos < p1)
//             {
//               e.setTagged(true);
//               part->setEventsTagged(true);
//             }
//           }
//           else
//           {
//             e.setTagged(true);
//             part->setEventsTagged(true);
//           }
//         }
//       }
//     }
//   }
//   else
//   {
//     // These two steps use the tagging features to mark the objects (events)
//     //  as having been visited already, to avoid duplicates in the list.
//     if(canvas)
//       canvas->tagItems(false, tagAllParts, range, p0, p1);
//     for(ciCtrlEdit i = ctrlEditList.begin(); i != ctrlEditList.end(); ++i)
//       (*i)->tagItems(false, tagAllParts, range, p0, p1);
//   }
//   
//   
//   
//   
//   CItem* item;
//   MusECore::Part* part;
//   if(range)
//   {
//     if(tagAllItems || tagAllParts)
//     {
//       for(ciCItemSet i = items.begin(); i != items.end(); ++i)
//       {
//         item = *i;
//         part = item->part();
//         if(!tagAllParts && (part != curPart || (part && part->track() != curTrack)))
//           continue;
//         if(!tagAllItems && !item->isSelected())
//           continue;
//         if(item->isObjectInRange(p0, p1))
//           item->setObjectTagged(true);
//       }
//     }
//     else
//     {
//       for(ciCItemSet i = selection.begin(); i != selection.end(); ++i)
//       {
//         item = *i;
//         part = item->part();
//         if(part != curPart || (part && part->track() != curTrack))
//           continue;
//         if(item->isObjectInRange(p0, p1))
//           item->setObjectTagged(true);
//       }
//     }
//   }
//   else
//   {
//     if(tagAllItems || tagAllParts)
//     {
//       for(ciCItemSet i = items.begin(); i != items.end(); ++i)
//       {
//         item = *i;
//         part = item->part();
//         if(!tagAllParts && (part != curPart || (part && part->track() != curTrack)))
//           continue;
//         if(!tagAllItems && !item->isSelected())
//           continue;
//         item->setObjectTagged(true);
//       }
//     }
//     else
//     {
//       for(ciCItemSet i = selection.begin(); i != selection.end(); ++i)
//       {
//         item = *i;
//         part = item->part();
//         if(part != curPart || (part && part->track() != curTrack))
//           continue;
//         item->setObjectTagged(true);
//       }
//     }
//   }
  
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

// REMOVE Tim. citem. Changed.
//             case CMD_QUANTIZE: MusECore::quantize_notes(); break;
            case CMD_QUANTIZE:
                  {
                  FunctionDialogReturnQuantize ret =
                    quantize_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::quantize_items(ret._raster_index,
                                           /*ret._quant_len*/ false,  // DELETETHIS
                                           ret._strength,
                                           ret._swing,
                                           ret._threshold);
                  break;
                  }
            
//             case CMD_VELOCITY: MusECore::modify_velocity(); break;
            case CMD_VELOCITY:
                  {
                  FunctionDialogReturnVelocity ret =
                    velocity_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::modify_velocity_items(ret._rateVal, ret._offsetVal);
                  break;
                  }
//             case CMD_CRESCENDO: MusECore::crescendo(); break;
            case CMD_CRESCENDO:
                  {
                  FunctionDialogReturnCrescendo ret =
                    crescendo_items_dialog(FunctionDialogMode(
                      FunctionLoopedButton |
                      FunctionSelectedLoopedButton |
                      FunctionAllPartsButton | 
                      FunctionSelectedPartsButton));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::crescendo_items(ret._start_val, ret._end_val, ret._absolute);
                  break;
                  }
//             case CMD_NOTELEN: MusECore::modify_notelen(); break;
            case CMD_NOTELEN:
                  {
                  FunctionDialogReturnGateTime ret =
                    gatetime_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::modify_notelen_items(ret._rateVal, ret._offsetVal);
                  break;
                  }
//             case CMD_TRANSPOSE: MusECore::transpose_notes(); break;
            case CMD_TRANSPOSE:
                  {
                  FunctionDialogReturnTranspose ret =
                    transpose_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::transpose_items(ret._amount);
                  break;
                  }
            
            case CMD_ERASE:
            {
// REMOVE Tim. citem. Changed.
//               MusECore::erase_notes();
              FunctionDialogReturnErase ret =
                erase_items_dialog(FunctionDialogMode(fn_element_dflt));
              if(ret._valid)
                tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
              MusECore::erase_items(ret._veloThreshold, ret._veloThresUsed, ret._lenThreshold, ret._lenThresUsed);
            }
            break;
            
//             case CMD_MOVE: MusECore::move_notes(); break;
            case CMD_MOVE:
                  {
                  FunctionDialogReturnMove ret =
                    move_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::move_items(ret._amount);
                  break;
                  }
//             case CMD_FIXED_LEN: MusECore::set_notelen(); break;
            case CMD_FIXED_LEN:
                  {
                  FunctionDialogReturnSetLen ret =
                    setlen_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::set_notelen_items(ret._len);
                  break;
                  }
//             case CMD_DELETE_OVERLAPS: MusECore::delete_overlaps(); break;
            case CMD_DELETE_OVERLAPS:
                  {
                  FunctionDialogReturnDelOverlaps ret =
                    deloverlaps_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::delete_overlaps_items();
                  break;
                  }
//             case CMD_LEGATO: MusECore::legato(); break;
            case CMD_LEGATO:
                  {
                  FunctionDialogReturnLegato ret =
                    legato_items_dialog(FunctionDialogMode(fn_element_dflt));
                  if(ret._valid)
                    tagItems(ret._allEvents, ret._allParts, ret._range, ret._pos0, ret._pos1);
                  MusECore::legato_items(ret._min_len, !ret._allow_shortening);
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

  
  action=new QAction(tr("New"), this);
  connect(action, SIGNAL(triggered()), scoreOneStaffPerTrackMapper, SLOT(map()));
  scoreOneStaffPerTrackMapper->setMapping(action, (QWidget*)NULL);
  scoreOneStaffPerTrackSubsubmenu->addAction(action);
  
  
  action=new QAction(tr("New"), this); //the above action may NOT be reused!
  connect(action, SIGNAL(triggered()), scoreAllInOneMapper, SLOT(map()));
  scoreAllInOneMapper->setMapping(action, (QWidget*)NULL);
  scoreAllInOneSubsubmenu->addAction(action);

  const ToplevelList* toplevels=MusEGlobal::muse->getToplevels();

  for (ToplevelList::const_iterator it=toplevels->begin(); it!=toplevels->end(); it++)
    if ((*it)->type()==TopWin::SCORE)
    {
      ScoreEdit* score = dynamic_cast<ScoreEdit*>(*it);
      
      action=new QAction(score->get_name(), this);
      connect(action, SIGNAL(triggered()), scoreOneStaffPerTrackMapper, SLOT(map()));
      scoreOneStaffPerTrackMapper->setMapping(action, (QWidget*)score);
      scoreOneStaffPerTrackSubsubmenu->addAction(action);


      action=new QAction(score->get_name(), this); //the above action may NOT be reused!
      connect(action, SIGNAL(triggered()), scoreAllInOneMapper, SLOT(map()));
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
      QActionGroup *grp = MusEGui::populateAddTrack(addTrack, true, true);
      connect(addTrack, SIGNAL(triggered(QAction *)), SLOT(addNewTrack(QAction *)));
      
      int idx = 0;
      trackMidiAction = grp->actions()[idx++];
      trackDrumAction = grp->actions()[idx++];
      trackWaveAction = grp->actions()[idx++];
      trackAOutputAction = grp->actions()[idx++];
      trackAGroupAction = grp->actions()[idx++];
      trackAInputAction = grp->actions()[idx++];
      trackAAuxAction = grp->actions()[idx++];


      arranger->getTrackList()->populateAddTrack();
}

void ArrangerView::addNewTrack(QAction* action)
{
  MusEGlobal::song->addNewTrack(action, MusEGlobal::muse->arranger()->curTrack());  // Insert at current selected track.
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
