//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: drumedit.h,v 1.9.2.7 2009/11/16 11:29:33 lunar_shuttle Exp $
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

#ifndef __DRUM_EDIT_H__
#define __DRUM_EDIT_H__

#include <limits.h>
#include "type_defs.h"
#include "part.h"
#include "midieditor.h"
#include "noteinfo.h"
#include "dcanvas.h"

#include <QString>
#include <QVector>


// Forward declarations:
class QAction;
class QCloseEvent;
class QMenu;
class QKeyEvent;
class QToolButton;
class QWidget;
class QComboBox;
class QPoint;
class QToolBar;

namespace MusECore {
class Event;
class Xml;
}

namespace MusEGui {

class TopWin;
class CtrlEdit;
class DList;
class Header;
class ScrollScale;
class Splitter;
class Toolbar1;
class PopupMenu;
class EditToolBar;

//---------------------------------------------------------
//   DrumEdit
//---------------------------------------------------------

class DrumEdit : public MidiEditor {
    Q_OBJECT

public:
    enum group_mode_t { DONT_GROUP, GROUP_SAME_CHANNEL, GROUP_MAX };

private:
    group_mode_t _group_mode;
    bool _ignore_hide;

    QMenu* menuEdit, *menuFunctions, *menuSelect, *menuScripts;
    PopupMenu* addControllerMenu;

    int tickValue;
    int lenValue;
    int pitchValue;
    int veloOnValue;
    int veloOffValue;
    bool firstValueSet;
    int tickOffset;
    int lenOffset;
    int pitchOffset;
    int veloOnOffset;
    int veloOffOffset;
    bool deltaMode;
    int lastSelections;

    MusEGui::NoteInfo* info;
    QToolButton* addctrl;
    QToolButton* srec;
    QToolButton* midiin;
    QToolButton* speaker;
    MusEGui::EditToolBar* tools2;
    bool _playEvents;

    MusEGui::Toolbar1* toolbar;
    MusEGui::Splitter* hsplitter;
    MusEGui::Splitter* split1;
    MusEGui::Splitter* split2;
    QWidget* split1w1;
    DList* dlist;
    MusEGui::Header* header;
    QToolBar* tools;
    QComboBox *stepLenWidget;

    static int _rasterInit;
    static int _trackInfoWidthInit;
    static int _canvasWidthInit;
    static int _dlistWidthInit, _dcanvasWidthInit;
    static bool _ignore_hide_init;

    // Initial view state.
    MusECore::MidiPartViewState _viewState;

    QAction *cutAction, *copyAction, *copyRangeAction, *pasteAction;
    QAction *pasteToCurPartAction, *pasteDialogAction, *deleteAction;
    QAction *fixedAction, *veloAction, *crescAction, *quantizeAction;
    QAction *sallAction, *snoneAction, *invAction, *inAction , *outAction;
    QAction *prevAction, *nextAction;
    QAction *groupNoneAction, *groupChanAction, *groupMaxAction;
    QAction *addControllerAction;
    QAction *startListEditAction;

    void initShortcuts();
    void setupNewCtrl(CtrlEdit* ctrlEdit);

    void closeEvent(QCloseEvent*) override;
    QWidget* genToolbar(QWidget* parent);
    void keyPressEvent(QKeyEvent*) override;

    void setHeaderToolTips();
    void setHeaderWhatsThis();
    void setHeaderStatusTip();

    // Sets up a reasonable zoom minimum and/or maximum based on
    //  the current global midi division (ticks per quarter note)
    //  which has a very wide range (48 - 12288).
    // Also sets the canvas and time scale offsets accordingly.
    void setupHZoomRange();

private slots:
    void setRaster(int) override;
    void noteinfoChanged(MusEGui::NoteInfo::ValType type, int val);
    void removeCtrl(CtrlEdit* ctrl);
    void cmd(int);
    void clipboardChanged(); // enable/disable "Paste"
    void selectionChanged(); // enable/disable "Copy" & "Paste"
    void load();
    void save();
    void reset();
    void setTime(unsigned);
    void follow(int);
    void newCanvasWidth(int);
    void configChanged();
    void songChanged1(MusECore::SongChangedStruct_t);
    void setStep(QString);
    void setSpeaker(bool);
    void addCtrlClicked();
    void ctrlPopupTriggered(QAction* act);
    void ctrlMenuAboutToShow();
    void ctrlMenuAboutToHide();

    void updateGroupingActions();
    void set_ignore_hide(bool);
    void showAllInstruments();
    void hideAllInstruments();
    void hideUnusedInstruments();
    void hideEmptyInstruments();

    void deltaModeChanged(bool);
    void midiNote(int pitch, int velo);

public slots:
    void setSelection(int tick, MusECore::Event&, MusECore::Part*, bool update);
    void soloChanged(bool);       // called by Solo button
    void execDeliveredScript(int);
    void execUserScript(int);
    void focusCanvas() override;
    void ourDrumMapChanged(bool);
    void horizontalZoom(bool zoom_in, const QPoint& glob_pos);
    void horizontalZoom(int mag, const QPoint& glob_pos);
    virtual void updateHScrollRange() override;
    void storeInitialViewState() const override;

signals:
    void isDeleting(MusEGui::TopWin*);

public:
    DrumEdit(MusECore::PartList*, QWidget* parent = nullptr, const char* name = nullptr,
             unsigned initPos = INT_MAX, bool showDefaultControls = false);
    virtual void readStatus(MusECore::Xml&) override;
    virtual void writeStatus(int, MusECore::Xml&) const override;
    static void readConfiguration(MusECore::Xml& xml);
    static void writeConfiguration(int, MusECore::Xml&);

    CtrlEdit* addCtrl(int ctl_num = MusECore::CTRL_VELOCITY);

    MusECore::MidiPartViewState getViewState() const;

    int changeRaster(int val);

    group_mode_t group_mode() { return _group_mode; }
    bool ignore_hide() { return _ignore_hide; }

    QVector<instrument_number_mapping_t>& get_instrument_map() { return static_cast<DrumCanvas*>(canvas)->get_instrument_map(); }

    void storeSettings() override;
};

} // namespace MusEGui

#endif
