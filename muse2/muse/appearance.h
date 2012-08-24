//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/appearance.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
#ifndef __APPEARANCE_H__
#define __APPEARANCE_H__

#include "ui_appearancebase.h"

class QColor;
class QDialog;

namespace MusEGlobal {
  class GlobalConfigValues;
}

namespace MusEGui {
class Arranger;
class MusE;

//---------------------------------------------------------
//   Appearance Dialog
//---------------------------------------------------------

class Appearance : public QDialog, public Ui::AppearanceDialogBase {
  
    Q_OBJECT
    
 private:
      Arranger* arr;
      QColor* color;
      MusEGlobal::GlobalConfigValues* config;
      QButtonGroup* aPalette;
      QTreeWidgetItem* user_bg;
      QTreeWidgetItem* global_bg;
      QTreeWidgetItem* lastSelectedBgItem;
      QTreeWidgetItem* lastSelectedColorItem;

      
      void updateFonts();
      void updateColor();

   private slots:
      void apply();
      void ok();
      void cancel();
      void addBackground();
      void removeBackground();
      void clearBackground();
      void colorItemSelectionChanged();
      void browseStyleSheet();
      void setDefaultStyleSheet();
      void browseFont(int);
      void browseFont0();
      void browseFont1();
      void browseFont2();
      void browseFont3();
      void browseFont4();
      void browseFont5();
      void browseFont6();
      void asliderChanged(int);
      void aValChanged(int);
      void rsliderChanged(int);
      void gsliderChanged(int);
      void bsliderChanged(int);
      void hsliderChanged(int);
      void ssliderChanged(int);
      void vsliderChanged(int);
      void addToPaletteClicked();
      void paletteClicked(int);
      void bgSelectionChanged(QTreeWidgetItem*);
      void colorNameEditFinished();

   public:
      Appearance(Arranger*, QWidget* parent=0);
      ~Appearance();
      void resetValues();
      static QString defaultStyle;
      };

} // namespace MusEGui

#endif
