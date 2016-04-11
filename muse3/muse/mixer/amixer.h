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

#include <QScrollArea>

#include "type_defs.h"
#include "cobject.h"
#include "synth.h"
#include "node.h"
#include "routedialog.h"

#define EFX_HEIGHT     16

class QHBoxLayout;
class QLabel;
class QMenu;
class QToolButton;
class QWidget;
class QShowEvent;
class QCloseEvent;

namespace MusECore {
class Xml;
class AudioTrack;
class Meter;
class Track;
}

namespace MusEGlobal {
  struct MixerConfig;
}

namespace MusEGui {
class ComboBox;
class DoubleLabel;
class Knob;
class RouteDialog;
class Slider;
class Strip;
// class ExpanderHandle;

// struct StripItem {
//   Strip* _strip;
//   ExpanderHandle* _handle;
//   
//   StripItem() : _strip(0), _handle(0) { }
//   StripItem(Strip* strip, ExpanderHandle* handle) : _strip(strip), _handle(handle) { }
// };

typedef QList<Strip*> StripList;
// typedef QList<StripItem> StripList;

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
    ScrollArea(QWidget* parent = 0) : QScrollArea(parent) { } 
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
      //QString name;
      MusEGlobal::MixerConfig* cfg;
      StripList stripList;

      QScrollArea* view;
      QWidget* central;
      //QSplitter* splitter;
      //Strip* master;
      QHBoxLayout* mixerLayout;
      QMenu* menuStrips;
      MusEGui::RouteDialog* routingDialog;
      QAction* routingId;
      int oldAuxsSize;

      QAction* showMidiTracksId;
      QAction* showDrumTracksId;
      QAction* showNewDrumTracksId;
      QAction* showInputTracksId;
      QAction* showOutputTracksId;
      QAction* showWaveTracksId;
      QAction* showGroupTracksId;
      QAction* showAuxTracksId;
      QAction* showSyntiTracksId;

      bool mixerClicked;

      //virtual void closeEvent(QCloseEvent*);

      bool stripIsVisible(Strip* s);
      void redrawMixer();
      void addStrip(MusECore::Track* t, bool visible=true);
      //void addStrip(MusECore::Track* t);
      void showRouteDialog(bool);

      void updateStripList();
      void fillStripListTraditional();
      Strip* findStripForTrack(StripList &s, MusECore::Track *t);

      enum UpdateAction {
            NO_UPDATE, UPDATE_ALL, UPDATE_MIDI, STRIP_INSERTED, STRIP_REMOVED
            };
      void initMixer();
      void addStripsTraditionalLayout();
      void addStripToLayoutIfVisible(Strip *s);


   signals:
      void closed();
      //void layoutRequest();

   private slots:
      void songChanged(MusECore::SongChangedFlags_t);
      //void configChanged()    { songChanged(-1); }
      void configChanged();
      //void addNewTrack(QAction*);
      void setSizing();
      void toggleRouteDialog();
      void routingDialogClosed();
      //void showTracksChanged(QAction*);
      void showMidiTracksChanged(bool);
      void showDrumTracksChanged(bool);
      void showNewDrumTracksChanged(bool);
      void showWaveTracksChanged(bool);
      void showInputTracksChanged(bool);
      void showOutputTracksChanged(bool);
      void showGroupTracksChanged(bool);
      void showAuxTracksChanged(bool);
      void showSyntiTracksChanged(bool);
      void stripsMenu();
      void handleMenu(QAction *);

   protected:
   //   virtual bool event(QEvent* event);
      virtual void closeEvent(QCloseEvent*);
   
      void mousePressEvent(QMouseEvent* ev);
      void mouseReleaseEvent(QMouseEvent* ev);

   public:
      //AudioMixerApp(QWidget* parent);
      AudioMixerApp(QWidget* parent, MusEGlobal::MixerConfig* c);
      //void write(Xml&, const char* name);
      //void write(int level, Xml& xml, const char* name);
      void write(int level, MusECore::Xml& xml);
      void clearAndDelete();
      void moveStrip(Strip *s);
      bool isMixerClicked() { return mixerClicked; }
      };

} // namespace MusEGui

#endif

