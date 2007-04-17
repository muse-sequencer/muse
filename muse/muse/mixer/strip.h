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

#ifndef __STRIP_H__
#define __STRIP_H__

#include "globaldefs.h"
#include "gui.h"

class Track;
class Meter;
class SimpleButton;
class Mixer;

static const QSize buttonSize(STRIP_WIDTH/2-2, STRIP_WIDTH/3);
static const QSize entrySize(STRIP_WIDTH/2-2, 17);

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

class Strip : public QFrame {
      Q_OBJECT

   protected:
      Mixer* mixer;
      Track* track;
      QLabel* label;
      QVBoxLayout* layout;

      SimpleButton* solo;
      SimpleButton* mute;
      void updateLabel();
      bool _align;      // align elements for mixer app

      void recordToggled(bool);
      void addAutomationButtons();

   public slots:
      void resetPeaks();
      virtual void songChanged(int) = 0;
      virtual void controllerChanged(int) {}
      void configChanged();

   public:
      Strip(Mixer* m, Track* t, bool align);
      ~Strip();
      Track* getTrack() const { return track; }
      virtual void heartBeat() = 0;
      };

#endif

