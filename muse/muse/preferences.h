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

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include "ui_preferences.h"
#include "mpevent.h"

class MusE;
class Arranger;
class GlobalConfigValues;

//---------------------------------------------------------
//   PreferencesDialog
//---------------------------------------------------------

class PreferencesDialog : public QDialog, public Ui::PreferencesDialogBase {
      Q_OBJECT

      Arranger* arr;
      QColor* color;
      QString currentBg;
      GlobalConfigValues* config;
      QButtonGroup* colorGroup;
      QButtonGroup* startProjectGroup;

      void updateFonts();
      void updateColor();

   private slots:
      void apply();
      void ok();
      void cancel();
      void configCanvasBgColor();
      void configCanvasBgPixmap();
      void colorItemSelectionChanged();
      void browseFont(int);
      void browseFont0();
      void browseFont1();
      void browseFont2();
      void browseFont3();
      void browseFont4();
      void browseFont5();
      void hsliderChanged(int);
      void ssliderChanged(int);
      void vsliderChanged(int);
      void addToPaletteClicked();
      void paletteClicked(QAbstractButton*);
      void useColorToggled(bool);
      void usePixmapToggled(bool);

      void mixerCurrent1();
      void mixerCurrent2();
      void bigtimeCurrent();
      void arrangerCurrent();
      void transportCurrent();
      void recordStopToggled(bool);
      void recordRecordToggled(bool);
      void recordGotoLeftMarkToggled(bool);
      void recordPlayToggled(bool);
      void midiEventReceived(MidiEvent);

   public:
      PreferencesDialog(Arranger*, QWidget* parent=0);
      ~PreferencesDialog();
      void resetValues();
      };

#endif
