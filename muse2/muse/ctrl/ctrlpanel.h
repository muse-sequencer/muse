//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlpanel.h,v 1.2.2.5 2009/06/10 00:34:59 terminator356 Exp $
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
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

#ifndef __CTRL_PANEL_H__
#define __CTRL_PANEL_H__

#include <QWidget>

#include "type_defs.h"

class QPushButton;
class QAction;

namespace MusECore {
class MidiController;
class MidiPort;
class MidiTrack;
}

namespace MusEGui {
class DoubleLabel;
class Knob;
class MidiEditor;
class CtrlCanvas;
class PixmapButton;

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

class CtrlPanel: public QWidget {
      Q_OBJECT
    
      QPushButton* selCtrl;
      MidiEditor* editor;
      CtrlCanvas* ctrlcanvas;
      
      MusECore::MidiTrack* _track;
      MusECore::MidiController* _ctrl;
      int _dnum;
      bool inHeartBeat;
      MusEGui::Knob* _knob;
      MusEGui::DoubleLabel* _dl;
      int _val;
      PixmapButton* _veloPerNoteButton;

   signals:
      void destroyPanel();
      void controllerChanged(int);

   private slots:
      void ctrlChanged(double val);
      void labelDoubleClicked();
      void ctrlRightClicked(const QPoint& p, int id);
      void ctrlPopupTriggered(QAction* act);
      void velPerNoteClicked();
      void songChanged(MusECore::SongChangedFlags_t type);
      void configChanged();    
      
   protected slots:
      virtual void heartBeat();
      
   public slots:
      void setHeight(int);
      void ctrlPopup();

   public:
      CtrlPanel(QWidget*, MidiEditor*, CtrlCanvas*, const char* name = 0);
      void setHWController(MusECore::MidiTrack* t, MusECore::MidiController* ctrl);
      };

} // namespace MusEGui

#endif
