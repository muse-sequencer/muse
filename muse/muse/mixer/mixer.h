//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __AMIXER_H__
#define __AMIXER_H__

#include "gconfig.h"

class Meter;
class Track;
class Slider;
class Knob;
class RouteDialog;
class Strip;

typedef std::list<Strip*> StripList;

//---------------------------------------------------------
//   Mixer
//---------------------------------------------------------

class Mixer : public QMainWindow {
      Q_OBJECT

      MixerConfig* cfg;
      StripList stripList;
      QWidget* central;
      QHBoxLayout* lbox;
      Strip* master;
      QHBoxLayout* layout;
      QMenu* menuView;
      RouteDialog* routingDialog;
      QAction* routingAction;

      QAction* routingId;
      QAction* showMidiTracksId;
      QAction* showMidiInPortId;
      QAction* showMidiOutPortId;
      QAction* showOutputTracksId;
      QAction* showWaveTracksId;
      QAction* showGroupTracksId;
      QAction* showInputTracksId;
      QAction* showSyntiTracksId;

      bool mustUpdateMixer;

      virtual void closeEvent(QCloseEvent*);
      void addStrip(Track*, int);
      void showRouteDialog(bool);

      enum {
            NO_UPDATE      = 0,
            STRIP_INSERTED = 1,
            STRIP_REMOVED  = 2,
            UPDATE_ALL     = 4
            };
      void updateMixer(int);

   signals:
      void closed();

   private slots:
      void songChanged(int);
      void configChanged()    { songChanged(-1); }
      void toggleRouteDialog();
      void routingDialogClosed();
      void showTracksChanged(QAction*);
      void heartBeat();

   public:
      Mixer(QWidget* parent, MixerConfig*);
      void clear();
      void write(Xml&, const char* name);
      void setUpdateMixer() { mustUpdateMixer = true; }
      };

#endif

