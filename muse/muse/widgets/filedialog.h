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

class FileDialogButtons;

//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

class MFileDialog : public QFileDialog {
      bool showButtons;
      QString baseDir;
      Q_OBJECT

   public:
      MFileDialog(const QString& dir, const QString& filter = QString::null,
         QWidget* parent = 0, bool writeFlag = false);
      };

QString getSaveFileName(const QString& startWidth, const QStringList& filter,
         QWidget* parent, const QString& name);
QString getOpenFileName(const QString& startWidth, const QStringList& filter,
         QWidget* parent, const QString& name);
QString getImageFileName(const QString& startWith, const QStringList& filter,
         QWidget* parent, const QString& name);

QFile* fileOpen(QWidget*, QString, const QString&,
   QIODevice::OpenMode, bool = false);


//---------------------------------------------------------
//   MFile
//    "Muse" File
//---------------------------------------------------------

class MFile {
      QFile* f;
      QString path;
      QString ext;

   public:
      MFile(const QString& path, const QString& ext);
      ~MFile();
      QFile* open(QIODevice::OpenMode, const QStringList& pattern,
         QWidget* parent,
         bool warnIfOverwrite, const QString& caption);
      };

