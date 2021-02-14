//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: metronome.h,v 1.1.1.1.2.1 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
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

#ifndef __METRONOME_H__
#define __METRONOME_H__

#include "ui_metronomebase.h"

#include "type_defs.h"
#include "metronome_class.h"

#include <QFrame>

class QDialog;
class QPaintEvent;
class QIcon;

namespace MusEGui {

class MetronomePresetItemWidget : public QFrame {
  protected:
    QIcon* _onIcon;
    QIcon* _offIcon;
    bool _hasFixedIconSize;
    int _margin;
    QSize _iconSize;
    MusECore::MetroAccentsStruct _accents;

    virtual void paintEvent(QPaintEvent* );

  public:
    MetronomePresetItemWidget(
      QIcon* on_icon, QIcon* off_icon, const MusECore::MetroAccentsStruct& mas,
      bool hasFixedIconSize = true, int margin = 4, QWidget* parent = 0, const char* name = 0);

    virtual QSize minimumSizeHint () const;
    virtual QSize sizeHint() const;

    bool margin() const { return _margin; }
    void setMargin(int v);

    QIcon* offIcon() const { return _offIcon; }
    void setOffIcon(QIcon*);
    QIcon* onIcon() const { return _onIcon; }
    void setOnIcon(QIcon*);

    QSize iconSize() const { return _iconSize; }
    void setIconSize(const QSize sz);
};
  
//---------------------------------------------------------
//   MetronomeConfig
//---------------------------------------------------------

class MetronomeConfig : public QDialog, public Ui::MetronomeConfigBase {
      Q_OBJECT

   private:
      enum AccentPresetsDataRole { BeatsRole = Qt::UserRole, PresetIdRole, PresetTypeRole };
      enum AccentPresetTypeIndex { FactoryPresetType = 0, UserPresetType = 1 };

      //-----------------------
      // BEGIN Lambda 'slots'.
      //-----------------------
      void apply();
      void audioBeepRoutesClicked();
      void midiClickChanged(bool);
      void precountEnableChanged(bool);
      void precountFromMastertrackChanged(bool);
      void volumeChanged(int);
      void measVolumeChanged(int);
      void beatVolumeChanged(int);
      void accent1VolumeChanged(int);
      void accent2VolumeChanged(int);
      void switchSamples();
      void changeAccents();
      void clearAccents(MusECore::MetroAccent::AccentType row);
      void songChanged(MusECore::SongChangedStruct_t type);
      void accentPresetsItemActivated(QListWidgetItem*);
      void switchSettings();
      void addAccentsPresetClicked();
      void delAccentsPresetClicked();
      void useAccentsPresetClicked();
      void accentsResetDefaultClicked();
      // FIXME These replacements don't work in the code.
      //void accentBeatsChanged(int);
      //void accentPresetsTypeItemActivated(int);

      //-----------------------
      // END Lambda 'slots'.
      //-----------------------

      void fillSoundFiles();
      bool addAccentPreset(int beats, const MusECore::MetroAccentsStruct& mas);
      void fillAccentPresets(int beats);
      void configureAccentButtons(int beats);
      void updateAccentButtons(int beats);
      void updateAccentPresetAddButton();
      void updateAccentPresetDelButton();
      // Safely and synchronously changes the accent settings.
      void setAccentsSettings(int beats, const MusECore::MetroAccentsStruct& mas);
      // Fills the given MetroAccentsStruct from the state of the visual buttons.
      void getAccents(int beats, MusECore::MetroAccentsStruct* mas) const;

   private slots:
      void accept();
      void reject();
      void accentBeatsChanged(int); // See above
      void accentPresetsTypeItemActivated(int); // See above

   public:
      MetronomeConfig(QWidget* parent=0);

      void updateValues();
      };

} // namespace MusEGui

#endif
