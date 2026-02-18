//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: genset.h,v 1.3 2004/01/25 09:55:17 wschweer Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __GENSET_H__
#define __GENSET_H__

#include "ui_gensetbase.h"

#include <QDialog>
#include <QString>
#include <QList>

#include "note_names.h"


// Forward declarations:
class QButtonGroup;
class QShowEvent;
class QWidget;

namespace MusEGui {

//---------------------------------------------------------
//   GlobalSettingsConfig
//---------------------------------------------------------

class GlobalSettingsConfig : public QDialog, public Ui::GlobalSettingsDialogBase {
      Q_OBJECT

      enum PathTab { LadspaTab = 0, DssiTab, VstTab, LinuxVstTab, Lv2Tab };
      MusECore::NoteNameList _noteNamesBackup;

   private slots:
      void updateSettings();
      void apply();
      void ok();
      void cancel();
      void editPluginPath();
      void addPluginPath();
      void removePluginPath();
      void movePluginPathUp();
      void movePluginPathDown();
      void browseProjDir();
      void browseStartSongFile();
      void startSongReset();
      void showAudioConverterSettings();
      void updateBackendDeviceSettings();
      // Update the midi starting note range.
      void updateStartingMidiNote(const MusECore::NoteNameList &nnl);
      void updateNoteNames(const MusECore::NoteNameList &nnl);
      void applyNoteNames(MusECore::NoteNameList &nnl);
      void loadNoteNames();
      void saveNoteNames();
      void addNoteName();
      void insertNoteName();
      void delNoteName();
      void moveNoteNameUp();
      void moveNoteNameDown();
      void noteNamesItemChanged(QTableWidgetItem *item);

    protected:
      void showEvent(QShowEvent*);
      QButtonGroup *startSongGroup;
      QButtonGroup *recDrumGroup;
      
      QString browsePluginPath(const QString& path);
      // Returns a new unique note name.
      QString newNoteName() const;
      // Sets up a row. Returns true on success.
      bool setupNoteNameRow(int row, const QString &sharpName, const QString &flatName);
      // If row is past the end, this appends. Returns true on success.
      bool newNoteNameRow(int row = -1);
      // Updates the note number column starting at startRow.
      void updateNoteNumbers(int startRow = 0);
      // Returns true on success.
      bool moveNoteName(int from, int to);

   public:
      GlobalSettingsConfig(QWidget* parent=nullptr);
      void selectTab(int tab) const;
      };

} // namespace MusEGui

#endif
