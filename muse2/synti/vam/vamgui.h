//=========================================================
//  MusE
//  Linux Music Editor
//  vamgui.h
//
//  (C) Copyright 2002 Jotsif Lindman Hï¿½nlund (jotsif@linux.nu)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02111-1301, USA or point your web browser to http://www.gnu.org.
//=========================================================

#ifndef __VAMGUI_H
#define __VAMGUI_H

#include "ui_vamguibase.h"
#include "vam.h"
#include "libsynti/gui.h"

class QListWidgetItem;
class Preset;
class QString;
class QSignalMapper;

//---------------------------------------------------------
//   VAMGui
//---------------------------------------------------------

class VAMGui : public QWidget, public Ui::VAMGuiBase, public MessGui {
      Q_OBJECT
    
      QSignalMapper* map;
      int ctrlHi;
      int ctrlLo;
      int dataHi;
      int dataLo;
      SynthGuiCtrl dctrl[NUM_CONTROLLER];
      QString * presetFileName;

      
      void sendControllerChange(int ctrl, int val);
      void initParameter();
      void setParam(int, int);
      void setPreset(Preset* preset);
      void addNewPreset(const QString&);
      void deleteNamedPreset(const QString&);
      void activatePreset(Preset* preset);
      virtual void processEvent(const MusECore::MidiPlayEvent&);

   private slots:
      void ctrlChanged(int idx);

      void presetClicked(QListWidgetItem*);
      void setPreset();
      void loadPresetsPressed();
      void savePresetsPressed();
      void deletePresetPressed();
      void doSavePresets(const QString&, bool);
      void savePresetsToFilePressed();
      void readMessage(int);

   protected:
      virtual void sysexReceived(const unsigned char*, int);

   public:
      int getController(int idx);
      int getControllerInfo(int id, const char** name, int* controller, int* min, int* max, int* initval) const;
      VAMGui();
      };

#endif /* __VAMGUI_H */


