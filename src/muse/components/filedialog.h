//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: filedialog.h,v 1.2.2.2 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __FILEDIALOG_H__
#define __FILEDIALOG_H__

#include <QFileDialog>
#include <QIODevice>
#include "libs/file/file.h"

#include "ui_fdialogbuttons.h"

namespace MusEGui {

//---------------------------------------------------------
//   FileDialogButtonsWidget
//   Wrapper around Ui::FileDialogButtons
//---------------------------------------------------------

class FileDialogButtonsWidget : public QWidget, public Ui::FileDialogButtons
{
     Q_OBJECT

       public:
  FileDialogButtonsWidget(QWidget *parent = 0)
    : QWidget(parent)
  { setupUi(this); }
};

//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

class MFileDialog : public QFileDialog {
      Q_OBJECT

      QString  lastUserDir, lastGlobalDir;
      bool showButtons;
      QString baseDir;
      
      bool readMidiPortsSaved;

   private slots:
      void directoryChanged(const QString& directory);
      void fileChanged(const QString&);
   public slots:
      void globalToggled(bool);
      void userToggled(bool);
      void projectToggled(bool);
      void homeToggled(bool);

   public:
      enum ViewType { GLOBAL_VIEW, PROJECT_VIEW, USER_VIEW, HOME_VIEW };
      static ViewType lastViewUsed;
      FileDialogButtonsWidget buttons;
      MFileDialog(const QString& dir, const QString& filter = QString(),
         QWidget* parent = 0, bool writeFlag = false);
      };

QString getSaveFileName(const QString& startWith, const char** filters,
         QWidget* parent, const QString& name, bool* writeWinState=nullptr, MFileDialog::ViewType viewType = MFileDialog::PROJECT_VIEW);
QString getOpenFileName(const QString& startWith, const char** filters,
                        QWidget* parent, const QString& name, bool* doReadMidiPorts, MFileDialog::ViewType viewType = MFileDialog::PROJECT_VIEW);
QString getImageFileName(const QString& startWith, const char** filters, 
         QWidget* parent, const QString& name);

MusEFile::File::ErrorCode fileOpen(
  MusEFile::File &file, QIODevice::OpenMode mode, QWidget *parent = nullptr,
  bool noError = false, bool overwriteWarning = false);

//---------------------------------------------------------
//   MFile
//    "Muse" File
//---------------------------------------------------------

class MFile {
      bool isPopen;
      MusEFile::File f;
      QString path;
      QString ext;

   public:
      MFile(const QString& path, const QString& ext);
      ~MFile();
      MusEFile::File::ErrorCode open(MusEFile::File &file, QIODevice::OpenMode mode, const char** patterns_chararray,
         QWidget* parent, bool noError, bool warnIfOverwrite, const QString& caption);
      };

} // namespace MusEGui

#endif // __FILEDIALOG_H__
