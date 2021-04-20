//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: amixer.h,v 1.27.2.2 2009/10/18 06:13:00 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
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

#ifndef __AMIXER_H__
#define __AMIXER_H__

#include <QMainWindow>
#include <QScrollArea>

#include "type_defs.h"
#include "gconfig.h"

//#define EFX_HEIGHT     16


// Forward declarations:
class QWidget;
class QMenu;
class QAction;
class QHBoxLayout;
class QResizeEvent;
class QMoveEvent;
class QCloseEvent;
class QKeyEvent;
class QEvent;

namespace MusECore {
class Xml;
class AudioTrack;
class Meter;
class Track;
}

namespace MusEGui {
class ComboBox;
class DoubleLabel;
class Knob;
class Slider;
class Strip;
class RouteDialog;

typedef QList<Strip*> StripList;

//---------------------------------------------------------
//   ScrollArea
//---------------------------------------------------------

class ScrollArea : public QScrollArea 
{
  Q_OBJECT
  
  signals:
    void layoutRequest();
  
  protected:
    virtual bool viewportEvent(QEvent* event);
    
  public:
    ScrollArea(QWidget* parent = 0);
};

//---------------------------------------------------------
//   AudioMixerApp
//---------------------------------------------------------

class AudioMixerApp : public QMainWindow {
      Q_OBJECT
    
      enum StripMenuOperations {
        UNHIDE_STRIPS = -1000,
        UNHANDLED_NUMBER = -1001
      };
      MusEGlobal::MixerConfig* cfg;
      StripList stripList;

      QScrollArea* view;
      QWidget* central;
      QHBoxLayout* mixerLayout;
      QMenu* menuStrips;
      RouteDialog* routingDialog;
      QAction* routingId;
      int oldAuxsSize;

      QAction* showMidiTracksId;
//      QAction* showDrumTracksId;
      QAction* showNewDrumTracksId;
      QAction* showInputTracksId;
      QAction* showOutputTracksId;
      QAction* showWaveTracksId;
      QAction* showGroupTracksId;
      QAction* showAuxTracksId;
      QAction* showSyntiTracksId;

    // Current local state of knobs versus sliders preference global setting.
      bool _preferKnobs;
      // Hack flag to prevent overwriting the config geometry when resizing.
      bool _resizeFlag;

      bool stripIsVisible(Strip* s);
      void redrawMixer();
      void addStrip(const MusECore::Track* t, const MusEGlobal::StripConfig& sc = MusEGlobal::StripConfig(), int insert_pos = -1);
      void showRouteDialog(bool);

      // Returns true if anything changed.
      bool updateStripList();
      void fillStripListTraditional();
      Strip* findStripForTrack(StripList &s, MusECore::Track *t);
      void updateSelectedStrips();
      void moveConfig(const Strip* s, int new_pos);

//      enum UpdateAction {
//            NO_UPDATE, UPDATE_ALL, UPDATE_MIDI, STRIP_INSERTED, STRIP_REMOVED
//            };
      void initMixer();
      void addStripsTraditionalLayout();
      void addStripToLayoutIfVisible(Strip *s);
      void selectNextStrip(bool isRight);

      bool eventFilter(QObject *obj,QEvent *event);

   signals:
      void closed();

   private slots:
      void songChanged(MusECore::SongChangedStruct_t);
      void configChanged();
      void setSizing();
      void toggleRouteDialog();
      void routingDialogClosed();
      void showMidiTracksChanged(bool);
//      void showDrumTracksChanged(bool);
      void showNewDrumTracksChanged(bool);
      void showWaveTracksChanged(bool);
      void showInputTracksChanged(bool);
      void showOutputTracksChanged(bool);
      void showGroupTracksChanged(bool);
      void showAuxTracksChanged(bool);
      void showSyntiTracksChanged(bool);
      void stripsMenu();
      void handleMenu(QAction *);
      void clearStripSelection();
      void moveStrip(Strip*);
      void stripVisibleChanged(Strip*, bool);
      void stripUserWidthChanged(Strip*, int);

   protected:
      //virtual bool event(QEvent* event);
      virtual void closeEvent(QCloseEvent*);
      virtual void keyPressEvent(QKeyEvent*);
      virtual void resizeEvent(QResizeEvent*);
      virtual void moveEvent(QMoveEvent*);

   public:
      AudioMixerApp(QWidget* parent, MusEGlobal::MixerConfig* c);
      void clearAndDelete();
      
      // Sets up tabbing for the entire mixer. Strip by strip.
      // Accepts a previousWidget which can be null and returns the last widget in the last strip,
      //  which allows chaining other widgets.
      virtual QWidget* setupComponentTabbing(QWidget* previousWidget = 0);
      };

} // namespace MusEGui

#endif

