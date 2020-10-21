//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/appearance.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#include <QDialog>
#include <QTreeWidgetItem>


// Forward declarations:
class QTimer;
class QColorDialog;
class QCloseEvent;
class QButtonGroup;
class QTreeWidget;
class QColor;

namespace MusEGlobal {
struct GlobalConfigValues;
}

namespace MusEGui {

//---------------------------------------------------------
//   IdListViewItem
//---------------------------------------------------------

class IdListViewItem : public QTreeWidgetItem {
   private:
      int _id;

   public:
      IdListViewItem(int id, QTreeWidgetItem* parent, QString s);
      IdListViewItem(int id, QTreeWidget* parent, QString s);
      int id() const;
      };

//---------------------------------------------------------
//   Appearance Dialog
//---------------------------------------------------------

class Appearance : public QDialog, public Ui::AppearanceDialogBase {
  
    Q_OBJECT

 protected:
      virtual void closeEvent(QCloseEvent*);
      
 private:
      QTimer* _configChangedTimer;
      QColor* color;
      MusEGlobal::GlobalConfigValues* backupConfig;
      MusEGlobal::GlobalConfigValues* config;
      QButtonGroup* aPalette;
      QTreeWidgetItem* user_bg;
      QTreeWidgetItem* global_bg;
      QTreeWidgetItem* lastSelectedBgItem;
      QTreeWidgetItem* lastSelectedColorItem;      

      QColorDialog* _colorDialog;
      
      QColor* globalConfigColorFromId(int id) const;
      long int configOffsetFromColorId(int id) const;
      QColor* backupConfigColorFromId(int id) const;
      QColor* workingConfigColorFromId(int id) const;
      
      bool isColorDirty(IdListViewItem* item) const;
      bool isColorsDirty() const;
      // Sets current (last) item dirty.
      void setColorItemDirty();
      // Sets an item dirty.
      void setColorItemDirty(IdListViewItem* item);
      // Update the dirty states of all items.
      void updateColorItems();
      void resetColorItem(IdListViewItem* item);
      void resetAllColorItems();
      void updateFonts();
      void updateColor();
      void changeColor(const QColor& c);
      void changeGlobalColor();
      void setConfigurationColors();
      void setColorDialogWindowText(const QString& colorName = QString());
      void doCancel();
      // Returns true if restart required (style or stylesheet changed).
      bool apply();
      // Ask to close and if so, tell the main window to close the app and return true.
      bool checkClose();
      bool changeTheme();

   private slots:
      void applyClicked();
      void okClicked();
      void cancel();
      void addBackground();
      void removeBackground();
      void clearBackground();
      void colorItemSelectionChanged();
//      void browseStyleSheet();
//      void setDefaultStyleSheet();
      void browseFont(int);
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
      void loadColors();
      void saveColors();
      void chooseColorClicked();
      void colorDialogCurrentChanged(const QColor&);
      void colorDialogFinished(int result);
      void configChangeTimeOut();
      void colorListCustomContextMenuReq(const QPoint&);

   public:
      Appearance(QWidget* parent=0);
      ~Appearance();
      void resetValues();
      static QString& getSetDefaultStyle(const QString *newStyle = NULL);
private slots:
      void on_pbSetFontFamily_clicked();

private slots:
      void on_pbAdjustFontSizes_clicked();
      void on_helpButton_clicked();
};

} // namespace MusEGui

#endif
