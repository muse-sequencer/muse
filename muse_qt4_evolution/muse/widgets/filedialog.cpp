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

#include "filedialog.h"
#include "../globals.h"

//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

MFileDialog::MFileDialog(const QString& dir,
   const QString& filter, QWidget* parent, bool /*writeFlag*/)
   : QFileDialog(parent, QString(), dir, filter)
      {
      }

//---------------------------------------------------------
//   getOpenFileName
//---------------------------------------------------------

QString getOpenFileName(const QString &startWith,
   const QStringList& filters, QWidget* parent, const QString& name)
      {
      MFileDialog *dlg = new MFileDialog(startWith, QString::null, parent);
      dlg->setFilters(filters);
      dlg->setWindowTitle(name);
      dlg->setFileMode(QFileDialog::ExistingFile);
      QString result;
      if (dlg->exec() == QDialog::Accepted) {
            QStringList sl = dlg->selectedFiles();
            result = sl.at(0);
            }
      delete dlg;
      return result;
      }

//---------------------------------------------------------
//   getSaveFileName
//---------------------------------------------------------

QString getSaveFileName(const QString &startWith,
   const QStringList& filters, QWidget* parent, const QString& name)
      {
      MFileDialog *dlg = new MFileDialog(startWith, QString::null, parent, true);
      dlg->setFilters(filters);
      dlg->setWindowTitle(name);
      dlg->setFileMode(QFileDialog::AnyFile);
      QString result;
      if (dlg->exec() == QDialog::Accepted) {
            QStringList sl = dlg->selectedFiles();
            result = sl.at(0);
            }
      delete dlg;
      return result;
      }

//---------------------------------------------------------
//   getImageFileName
//---------------------------------------------------------

QString getImageFileName(const QString& startWith,
   const QStringList& filters, QWidget* parent, const QString& name)
      {
      QString initialSelection;
	QString* workingDirectory = new QString(QDir::current().absolutePath());
      if (!startWith.isEmpty() ) {
            QFileInfo fi(startWith);
            if (fi.exists() && fi.isDir()) {
                  *workingDirectory = startWith;
                  }
            else if (fi.exists() && fi.isFile()) {
                  *workingDirectory = fi.absolutePath();
                  initialSelection = fi.absolutePath();
                  }
            }
      MFileDialog *dlg = new MFileDialog(*workingDirectory, QString::null,
         parent);

//TD      dlg->setContentsPreviewEnabled(true);
//      ContentsPreview* preview = new ContentsPreview(dlg);
//      dlg->setContentsPreview(preview, preview);
//      dlg->setPreviewMode(QFileDialog::Contents);

      dlg->setWindowTitle(name);
      dlg->setFilters(filters);
      dlg->setFileMode(QFileDialog::ExistingFile);
      QString result;
      if (!initialSelection.isEmpty())
            dlg->selectFile(initialSelection);
      if (dlg->exec() == QDialog::Accepted) {
            QStringList sl = dlg->selectedFiles();
            result = sl.at(0);
            }
      delete dlg;
      return result;
      }

//---------------------------------------------------------
//   fileOpen
//    opens file "name" with extension "ext" in mode "mode"
//
//    mode = "r" or "w"
//    noError     show no error if file was not found in "r"
//                mode. Has no effect in "w" mode
//    overwriteWarning
//                warn in "w" mode, if file exists
//---------------------------------------------------------

QFile* fileOpen(QWidget* parent, QString name, const QString& ext,
   QIODevice::OpenMode mode, bool overwriteWarning)
      {
      QFileInfo info(name);

      if (info.completeSuffix() == "") {
            name += ext;
            info.setFile(name);
            }
      if (mode == QIODevice::WriteOnly && overwriteWarning && info.exists()) {
            QString s(QWidget::tr("File\n") + name + QWidget::tr("\nexists"));
            int rv = QMessageBox::warning(parent,
               QWidget::tr("MusE: write"),
               s,
               QWidget::tr("Overwrite"),
               QWidget::tr("Quit"), QString::null, 0, 1);
            switch(rv) {
                  case 0:  // overwrite
                        break;
                  case 1:  // quit
                        return 0;
                  }
            }
      QFile* file = new QFile(name);
	if (!file->open(mode)) {
            QString s(QWidget::tr("Open File\n") + name + QWidget::tr("\nfailed: ")
               + QString(strerror(errno)));
            QMessageBox::critical(parent, QWidget::tr("MusE: Open File"), s);
            }
      return file;
      }

//---------------------------------------------------------
//   MFile
//---------------------------------------------------------

MFile::MFile(const QString& _path, const QString& _ext)
   : path(_path), ext(_ext)
      {
      f = 0;
      }

MFile::~MFile()
      {
      if (f) {
		f->close();
            delete f;
            }
      }

//---------------------------------------------------------
//   open
//---------------------------------------------------------

QFile* MFile::open(QIODevice::OpenMode mode, const QStringList& pattern,
   QWidget* parent, bool warnIfOverwrite, const QString& caption)
      {
      QString name;
      if (mode == QIODevice::ReadOnly)
           name = getOpenFileName(path, pattern, parent, caption);
      else
           name = getSaveFileName(path, pattern, parent, caption);
      if (name.isEmpty())
            return 0;
      f = fileOpen(parent, name, ext, mode, warnIfOverwrite);
      return f;
      }

