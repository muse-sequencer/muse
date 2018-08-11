//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ctrlpanel.h,v 1.2.2.5 2009/06/10 00:34:59 terminator356 Exp $
//  (C) Copyright 1999-2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012, 2017 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
class QHBoxLayout;

namespace MusECore {
class MidiController;
class MidiPort;
class MidiTrack;
}

namespace MusEGui {
class MidiEditor;
class CtrlCanvas;
class PixmapButton;
class CompactKnob;
class CompactSlider;
class LCDPatchEdit;

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

      QHBoxLayout* kbox;
      CompactKnob* _knob;
      CompactSlider* _slider;
      LCDPatchEdit* _patchEdit;
      // Current local state of knobs versus sliders preference global setting.
      bool _preferKnobs;
      // Current local state of show values preference global setting.
      bool _showval;

      PixmapButton* _veloPerNoteButton;

      void buildPanel();
      void setController();
      void setControlColor();

   signals:
      void destroyPanel();
      void controllerChanged(int);

   private slots:
      void patchCtrlChanged(int val);
      void ctrlChanged(double val, bool off, int id, int scrollMode);
      void ctrlRightClicked(const QPoint& p, int id);
      void ctrlPopupTriggered(QAction* act);
      void velPerNoteClicked();
      void songChanged(MusECore::SongChangedStruct_t type);
      void configChanged();    
      
   protected slots:
      virtual void heartBeat();
      
   public slots:
      void setHeight(int);
      void ctrlPopup();
      void setVeloPerNoteMode(bool);

   public:
      CtrlPanel(QWidget*, MidiEditor*, CtrlCanvas*, const char* name = 0);
      void setHWController(MusECore::MidiTrack* t, MusECore::MidiController* ctrl);
      };

} // namespace MusEGui

#endif
