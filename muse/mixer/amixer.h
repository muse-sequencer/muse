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

#include "cobject.h"
#include "synth.h"
#include "node.h"
#include "routedialog.h"

class QHBoxLayout;
class QLabel;
class QMenu;
class QToolButton;
class QWidget;

class Xml;
class AudioTrack;
class Meter;
class Track;
class Slider;
class Knob;
class DoubleLabel;
class ComboBox;
class RouteDialog;
class Strip;

struct MixerConfig;

#define EFX_HEIGHT     16

typedef std::list<Strip*> StripList;

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
    
      //QString name;
      MixerConfig* cfg;
      StripList stripList;
      QScrollArea* view;
      QWidget* central;
      QHBoxLayout* lbox;
      //Strip* master;
      QHBoxLayout* layout;
      QMenu* menuView;
      RouteDialog* routingDialog;
      QAction* routingId;
      int oldAuxsSize;

      QAction* showMidiTracksId;
      QAction* showDrumTracksId;
      QAction* showInputTracksId;
      QAction* showOutputTracksId;
      QAction* showWaveTracksId;
      QAction* showGroupTracksId;
      QAction* showAuxTracksId;
      QAction* showSyntiTracksId;

      

      virtual void closeEvent(QCloseEvent*);
      void addStrip(Track*, int);
      void showRouteDialog(bool);

      enum UpdateAction {
            NO_UPDATE, UPDATE_ALL, UPDATE_MIDI, STRIP_INSERTED, STRIP_REMOVED
            };
      void updateMixer(UpdateAction);
      
   signals:
      void closed();
      //void layoutRequest();

   private slots:
      void songChanged(int);
      //void configChanged()    { songChanged(-1); }
      void configChanged();
      void setSizing();
      void toggleRouteDialog();
      void routingDialogClosed();
      //void showTracksChanged(QAction*);
      void showMidiTracksChanged(bool);
      void showDrumTracksChanged(bool);
      void showWaveTracksChanged(bool);
      void showInputTracksChanged(bool);
      void showOutputTracksChanged(bool);
      void showGroupTracksChanged(bool);
      void showAuxTracksChanged(bool);
      void showSyntiTracksChanged(bool);

   //protected:
   //   virtual bool event(QEvent* event);
   
   public:
      //AudioMixerApp(QWidget* parent);
      AudioMixerApp(QWidget* parent, MixerConfig* c);
      //void write(Xml&, const char* name);
      //void write(int level, Xml& xml, const char* name);
      void write(int level, Xml& xml);
      void clear();
      };

#endif

