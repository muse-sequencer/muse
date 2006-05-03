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

#ifndef __APPEARANCE_H__
#define __APPEARANCE_H__

#include "ui_appearance.h"

class MusE;
class Arranger;
class GlobalConfigValues;

//---------------------------------------------------------
//   Appearance Dialog
//---------------------------------------------------------

class Appearance : public QDialog, public Ui::AppearanceDialogBase {
      Arranger* arr;
      QColor* color;
      QString currentBg;
      GlobalConfigValues* config;
      QButtonGroup* colorGroup;

      Q_OBJECT
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

   public:
      Appearance(Arranger*, QWidget* parent=0);
      ~Appearance();
      void resetValues();
      };

#endif
