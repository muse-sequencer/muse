//=========================================================
//  MusE
//  Linux Music Editor
//  arrangerview.h
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

#ifndef __ARRANGERVIEW_H__
#define __ARRANGERVIEW_H__

#include <limits.h>
#include "type_defs.h"
#include "cobject.h"
#include "scripts.h"

#include <QMetaObject>

// Forward declarations:
class QCloseEvent;
class QAction;
class QGridLayout;
class QMenu;

namespace MusECore {
class TagEventList;
struct EventTagOptionsStruct;
}

namespace MusEGui {

class EditToolBar;
class VisibleTracks;
class PartColorToolbar;
class Arranger;
class Xml;
class AutomationModeToolBar;

class ArrangerView : public TopWin
{
    Q_OBJECT

private:

    enum cmd_enum
    {CMD_CUT, CMD_COPY, CMD_COPY_RANGE, CMD_PASTE, CMD_INSERTMEAS, CMD_PASTE_CLONE,
        CMD_PASTE_TO_TRACK, CMD_PASTE_CLONE_TO_TRACK, CMD_PASTE_DIALOG, CMD_DELETE,
        CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
        CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PARTS,
        CMD_DELETE_TRACK, CMD_DUPLICATE_TRACK, CMD_EXPAND_PART, CMD_SHRINK_PART, CMD_CLEAN_PART,
        CMD_QUANTIZE, CMD_VELOCITY, CMD_CRESCENDO, CMD_NOTELEN, CMD_TRANSPOSE,
        CMD_ERASE, CMD_MOVE, CMD_FIXED_LEN, CMD_DELETE_OVERLAPS, CMD_LEGATO,
        CMD_RANGE_TO_SELECTION,
        CMD_MOVEUP_TRACK, CMD_MOVEDOWN_TRACK, CMD_MOVETOP_TRACK, CMD_MOVEBOTTOM_TRACK
    };

    void closeEvent(QCloseEvent*) override;

    QGridLayout* mainGrid;
    QWidget* mainw;

    EditToolBar* editTools;
    VisibleTracks* visTracks;
    PartColorToolbar *partColorToolBar;
    AutomationModeToolBar* automationModeToolBar;

    Arranger* arranger;

    // Edit Menu actions
    QMenu* select;
    QMenu* addTrack;
    QMenu* insertTrack;


    QAction *strGlobalCutAction, *strGlobalInsertAction, *strGlobalSplitAction;
    QAction *strGlobalCutSelAction, *strGlobalInsertSelAction, *strGlobalSplitSelAction;
    QAction *trackAMidiAction, *trackADrumAction, *trackAWaveAction, *trackAOutputAction, *trackAGroupAction;
    QAction *trackAInputAction, *trackAAuxAction, *trackASynthAction;
    QAction *trackIMidiAction, *trackIDrumAction, *trackIWaveAction, *trackIOutputAction, *trackIGroupAction;
    QAction *trackIInputAction, *trackIAuxAction, *trackISynthAction;

    QAction *editDeleteAction,*editCutAction, *editCopyAction, *editCopyRangeAction;
    QAction *editPasteAction, *editPasteCloneAction, *editPasteToTrackAction, *editPasteCloneToTrackAction, *editPasteDialogAction;
    QAction *editInsertEMAction, *editPasteC2TAction, *editDeleteSelectedTrackAction, *editSelectAllAction, *editDeselectAllAction;
    QAction *editDuplicateSelTrackAction;
    QAction *editMoveUpSelTrackAction, *editMoveDownSelTrackAction, *editMoveTopSelTrackAction, *editMoveBottomSelTrackAction;
    QAction *editInvertSelectionAction, *editInsideLoopAction, *editOutsideLoopAction, *editAllPartsAction;
    QAction *editRangeToSelection;
    QAction *midiTransformerAction;
    QAction *editCleanPartsAction, *editShrinkPartsAction, *editExpandPartsAction;

    QAction* func_quantize_action;
    QAction* func_notelen_action;
    QAction* func_velocity_action;
    QAction* func_cresc_action;
    QAction* func_transpose_action;
    QAction* func_erase_action;
    QAction* func_move_action;
    QAction* func_fixed_len_action;
    QAction* func_del_overlaps_action;
    QAction* func_legato_action;

    MusECore::Scripts scripts;
    QMenu* menuScripts;

// REMOVE Tim. wave. Removed.
//     QMetaObject::Connection _configChangedEditToolsMetaConn;
    QMetaObject::Connection _deliveredScriptReceivedMetaConn;
    QMetaObject::Connection _userScriptReceivedMetaConn;
    // REMOVE Tim. wave. Added.
    QMetaObject::Connection _configChangedConnection;

private slots:
    void globalCut();
    void globalInsert();
    void globalSplit();
    void openCurrentTrackSynthGui();
    void globalCutSel();
    void globalInsertSel();
    void globalSplitSel();
    void cmd(int);
    void addNewTrack(QAction* action);
    void insertNewTrack(QAction* action);
    void configCustomColumns();
    void toggleMixerStrip();
    void execDeliveredScript(int);
    void execUserScript(int);
    void automationInterpolateModeChanged(int);
    void automationBoxModeChanged(int);
    void automationOptimizeChanged(bool);
    // REMOVE Tim. wave. Added.
    void configChanged();

signals:
    void isDeleting(MusEGui::TopWin*);
    void closed();

public slots:
    void scoreNamingChanged();
    void updateScoreMenus();
    void clipboardChanged();
    void selectionChanged(); // NOTE: This is received upon EITHER a part or track selection change from the Arranger.
    void updateShortcuts();
    void updateVisibleTracksButtons();
    virtual void focusCanvas() override;

public:
    ArrangerView(QWidget* parent = nullptr);
    virtual ~ArrangerView() override;

    QAction *startScoreEditAction, *startPianoEditAction, *startDrumEditAction, *startListEditAction, *startWaveEditAction;
    QMenu* editorNewSubmenu;
    QAction *startPianoEditNewAction, *startDrumEditNewAction, *startListEditNewAction, *startWaveEditNewAction;
    QAction *openCurrentTrackSynthGuiAction;
    QAction *openAddTrackMenuAction;
    QMenu *scoreSubmenu, *scoreOneStaffPerTrackSubsubmenu, *scoreAllInOneSubsubmenu;

    void populateAddTrack();

    Arranger* getArranger() const;

    void writeStatus(int level, MusECore::Xml& xml) const override;
    void readStatus(MusECore::Xml& xml) override;
    static void readConfiguration(MusECore::Xml&);
    void writeConfiguration(int, MusECore::Xml&);
    void storeSettings() override;

    // Appends given tag list with item objects according to options. Avoids duplicate events or clone events.
    // Special: We 'abuse' a controller event's length, normally 0, to indicate visual item length.
    void tagItems(MusECore::TagEventList* tag_list, const MusECore::EventTagOptionsStruct& options) const;
};

}  // namespace MusEGui

#endif

