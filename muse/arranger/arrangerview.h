//=========================================================
//  MusE
//  Linux Music Editor
//  arrangerview.h
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

#ifndef __ARRANGERVIEW_H__
#define __ARRANGERVIEW_H__

#include <QCloseEvent>
#include <QResizeEvent>
#include <QLabel>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QScrollBar>
#include <QComboBox>
#include <QSignalMapper>
#include <QAction>
#include <QActionGroup>
#include <QGridLayout>
#include <QByteArray>
#include <QToolButton>

#include <limits.h>
#include "noteinfo.h"
#include "cobject.h"
#include "event.h"
#include "view.h"
#include "gconfig.h"
#include "part.h"
#include "keyevent.h"
#include "mtscale_flo.h"
#include "steprec.h"
#include "cleftypes.h"
#include "helper.h"
#include "arranger.h"


namespace MusEGui {

class VisibleTracks;

class ArrangerView : public TopWin
{
	Q_OBJECT

	private:

		enum cmd_enum
			{CMD_CUT, CMD_COPY, CMD_COPY_RANGE, CMD_PASTE, CMD_INSERTMEAS, CMD_PASTE_CLONE,
			CMD_PASTE_DIALOG, CMD_PASTE_CLONE_DIALOG, CMD_DELETE,
			CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
			CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PARTS,
			CMD_DELETE_TRACK, CMD_DUPLICATE_TRACK, CMD_EXPAND_PART, CMD_SHRINK_PART, CMD_CLEAN_PART,
			CMD_QUANTIZE, CMD_VELOCITY, CMD_CRESCENDO, CMD_NOTELEN, CMD_TRANSPOSE,
			CMD_ERASE, CMD_MOVE, CMD_FIXED_LEN, CMD_DELETE_OVERLAPS, CMD_LEGATO     };

		virtual void closeEvent(QCloseEvent*);

		QGridLayout* mainGrid;
		QWidget* mainw;

		EditToolBar* editTools;
		VisibleTracks* visTracks;

		Arranger* arranger;

		// Edit Menu actions
		QMenu* select;
		QMenu* addTrack;
		QMenu* master;

		QAction *strGlobalCutAction, *strGlobalInsertAction, *strGlobalSplitAction;
    QAction *strGlobalCutSelAction, *strGlobalInsertSelAction, *strGlobalSplitSelAction;
    QAction *trackMidiAction, *trackDrumAction, *trackWaveAction, *trackAOutputAction, *trackAGroupAction;
		QAction *trackAInputAction, *trackAAuxAction;
		QAction *editCutAction, *editCopyAction, *editCopyRangeAction;
		QAction *editPasteAction, *editPasteCloneAction, *editPasteDialogAction, *editPasteCloneDialogAction;
		QAction *editInsertEMAction, *editPasteC2TAction, *editDeleteSelectedAction, *editSelectAllAction, *editDeselectAllAction;
    QAction *editDuplicateSelTrackAction;
		QAction *editInvertSelectionAction, *editInsideLoopAction, *editOutsideLoopAction, *editAllPartsAction;
		QAction *masterGraphicAction, *masterListAction;
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

		QSignalMapper *editSignalMapper;
		QSignalMapper *scoreOneStaffPerTrackMapper;
		QSignalMapper *scoreAllInOneMapper;

	private slots:
		void clearScoreMenuMappers();
		void globalCut();
		void globalInsert();
		void globalSplit();
    void globalCutSel();
    void globalInsertSel();
    void globalSplitSel();
    void cmd(int);
    void addNewTrack(QAction* action);
    void configCustomColumns();

	signals:
		void isDeleting(MusEGui::TopWin*);
		void closed();

	public slots:
		void scoreNamingChanged();
		void updateScoreMenus();
		void clipboardChanged();
		void selectionChanged();
		void updateShortcuts();
		void updateVisibleTracksButtons();
		virtual void focusCanvas() { arranger->focusCanvas(); } 

	public:
		ArrangerView(QWidget* parent = 0);
		~ArrangerView();

		QAction *startScoreEditAction, *startPianoEditAction, *startDrumEditAction, *startListEditAction, *startWaveEditAction;
		QMenu *scoreSubmenu, *scoreOneStaffPerTrackSubsubmenu, *scoreAllInOneSubsubmenu;

		void populateAddTrack();

		Arranger* getArranger() {return arranger;}

		void writeStatus(int level, MusECore::Xml& xml) const;
		void readStatus(MusECore::Xml& xml);
		static void readConfiguration(MusECore::Xml&);
		void writeConfiguration(int, MusECore::Xml&);
};

}  // namespace MusEGui

#endif

