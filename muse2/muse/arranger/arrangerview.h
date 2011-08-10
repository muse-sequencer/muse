//=========================================================
//  MusE
//  Linux Music Editor
//  arrangerview.h
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
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

#include <values.h>
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

class VisibleTracks;


class ArrangerView : public TopWin
{
	Q_OBJECT
	
	enum cmd_enum
	     {CMD_CUT, CMD_COPY, CMD_PASTE, CMD_INSERT, CMD_INSERTMEAS, CMD_PASTE_CLONE,
	      CMD_PASTE_TO_TRACK, CMD_PASTE_CLONE_TO_TRACK, CMD_DELETE,
	      CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
	      CMD_SELECT_ILOOP, CMD_SELECT_OLOOP, CMD_SELECT_PARTS,
	      CMD_DELETE_TRACK, CMD_EXPAND_PART, CMD_SHRINK_PART, CMD_CLEAN_PART };

	private:
		virtual void closeEvent(QCloseEvent*);

      void adjustGlobalLists(Undo& operations, int startPos, int diff);
		
		QGridLayout* mainGrid;
		QWidget* mainw;

		EditToolBar *editTools;
		VisibleTracks *visTracks;
		
		Arranger* arranger;
		
      // Edit Menu actions
      QMenu* select;
      QMenu* addTrack;
      QMenu* master;

      // Structure Menu actions
      QAction *strGlobalCutAction, *strGlobalInsertAction, *strGlobalSplitAction, *strCopyRangeAction, *strCutEventsAction;

      
      QAction *trackMidiAction, *trackDrumAction, *trackWaveAction, *trackAOutputAction, *trackAGroupAction;
      QAction *trackAInputAction, *trackAAuxAction;
      QAction *editCutAction, *editCopyAction, *editPasteAction, *editInsertAction, *editPasteCloneAction, *editPaste2TrackAction;
      QAction *editInsertEMAction, *editPasteC2TAction, *editDeleteSelectedAction, *editSelectAllAction, *editDeselectAllAction;
      QAction *editInvertSelectionAction, *editInsideLoopAction, *editOutsideLoopAction, *editAllPartsAction;
      QAction *masterGraphicAction, *masterListAction;
      QAction *midiTransformerAction;
      QAction *editCleanPartsAction, *editShrinkPartsAction, *editExpandPartsAction;

			QSignalMapper *editSignalMapper;
      QSignalMapper *scoreOneStaffPerTrackMapper;
      QSignalMapper *scoreAllInOneMapper;

   public:
      QAction *startScoreEditAction, *startPianoEditAction, *startDrumEditAction, *startListEditAction, *startWaveEditAction;
      QMenu *scoreSubmenu, *scoreOneStaffPerTrackSubsubmenu, *scoreAllInOneSubsubmenu;

	private slots:
      void clearScoreMenuMappers();
      void globalCut();
      void globalInsert();
      void globalSplit();
      void copyRange();
      void cutEvents();

      void cmd(int);
		
   signals:
      void deleted(unsigned long);
      void closed();
		
	public:
		ArrangerView(QWidget* parent = 0);
		~ArrangerView();

		void writeStatus(int level, Xml& xml) const;
		void readStatus(Xml& xml);
		
		void populateAddTrack();
		
		Arranger* getArranger() {return arranger;}

	public slots:
      void scoreNamingChanged();
      void updateScoreMenus();
      void clipboardChanged();
      void selectionChanged();
			void updateShortcuts();
			void updateVisibleTracksButtons();
};




#endif

