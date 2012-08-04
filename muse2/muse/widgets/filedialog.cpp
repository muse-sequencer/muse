//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: filedialog.cpp,v 1.3.2.3 2005/06/19 06:32:07 lunar_shuttle Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include <errno.h>

#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QSplitter>
#include <QStringList>
#include <QHeaderView>

#include "icons.h"
#include "filedialog.h"
#include "../globals.h"
#include "gconfig.h"
#include "helper.h"

namespace MusEGui {

MFileDialog::ViewType MFileDialog::lastViewUsed = GLOBAL_VIEW;
//QString MFileDialog::lastUserDir = "";
//QString MFileDialog::lastGlobalDir = "";

//---------------------------------------------------------
//   createDir
//    return true if dir could not created
//---------------------------------------------------------

static bool createDir(const QString& s)
      {
      QString sl("/");
      QStringList l = s.split(sl, QString::SkipEmptyParts);
      QString path(sl);
      QDir dir;
      for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
            dir.setPath(path);
            if (!QDir(path + sl + *it).exists()) {
                  if (!dir.mkdir(*it)) {
                        printf("mkdir failed: %s %s\n",
                           path.toLatin1().constData(), (*it).toLatin1().constData());
                        return true;
                        }
                  }
            path += sl;
            path += *it;
            }
      return false;
      }

//---------------------------------------------------------
//   testDirCreate
//    return true if dir does not exist
//---------------------------------------------------------

static bool testDirCreate(QWidget* parent, const QString& path)
{
      QDir dir(path);
      if (!dir.exists()) 
      {
        if(QMessageBox::information(parent,
            QWidget::tr("MusE: get file name"),
            QWidget::tr("The directory\n%1\ndoes not exist.\nCreate it?").arg(path),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) != QMessageBox::Ok)
          return true;
        
          if (createDir(path)) 
          {
            QMessageBox::critical(parent,
                QWidget::tr("MusE: create directory"),
                QWidget::tr("creating dir failed"));
            return true;
          }
      }
      return false;
}

//---------------------------------------------------------
//   globalToggled
//---------------------------------------------------------

void MFileDialog::globalToggled(bool flag)
      {
      if (flag) {
            buttons.readMidiPortsButton->setChecked(false);
            readMidiPortsSaved = false;
            if (lastGlobalDir.isEmpty())
                  lastGlobalDir = MusEGlobal::museGlobalShare + QString("/") + baseDir; // Initialize if first time
            setDirectory(lastGlobalDir);
            lastViewUsed = GLOBAL_VIEW;
            }
      }

//---------------------------------------------------------
//   userToggled
//---------------------------------------------------------

void MFileDialog::userToggled(bool flag)
      {
      if (flag) {
            buttons.readMidiPortsButton->setChecked(true);
            readMidiPortsSaved = true;
            if (lastUserDir.isEmpty()) {
                  //lastUserDir = MusEGlobal::museUser + QString("/") + baseDir; // Initialize if first time
                  lastUserDir = MusEGlobal::configPath + QString("/") + baseDir; // Initialize if first time    // p4.0.39
                  }

            if (testDirCreate(this, lastUserDir))
                  //setDirectory(MusEGlobal::museUser);
                  setDirectory(MusEGlobal::configPath);  // p4.0.39
            else
                  setDirectory(lastUserDir);

            lastViewUsed = USER_VIEW;
            }
      }

//---------------------------------------------------------
//   projectToggled
//---------------------------------------------------------

void MFileDialog::projectToggled(bool flag)
      {
      if (flag) {
            buttons.readMidiPortsButton->setChecked(true);
            readMidiPortsSaved = true;
            QString s;
            if (MusEGlobal::museProject == MusEGlobal::museProjectInitPath ) {
                  // if project path is uninitialized, meaning it is still set to museProjectInitPath.
                  // then project path is set to current pwd instead.
                  //s = QString(getcwd(0,0)) + QString("/");
                  s = MusEGlobal::config.projectBaseFolder;
                  }
            else
                  s = MusEGlobal::museProject + QString("/"); // + baseDir;

            if (testDirCreate(this, s))
                  setDirectory(MusEGlobal::museProject);
            else
                  setDirectory(s);
            lastViewUsed = PROJECT_VIEW;
            }
      }

void MFileDialog::fileChanged(const QString& path)
{
  bool is_mid = path.endsWith(".mid", Qt::CaseInsensitive) ||
                path.endsWith(".midi", Qt::CaseInsensitive) ||
                path.endsWith(".kar", Qt::CaseInsensitive);
  
  if (is_mid)
  {
    readMidiPortsSaved=buttons.readMidiPortsButton->isChecked();
    buttons.readMidiPortsButton->setEnabled(false);
    buttons.readMidiPortsButton->setChecked(false);
  }
  else
  {
    if (!buttons.readMidiPortsButton->isEnabled())
    {
      buttons.readMidiPortsButton->setEnabled(true);
      buttons.readMidiPortsButton->setChecked(readMidiPortsSaved);
    }
  }
  
}


//---------------------------------------------------------
//   MFileDialog
//---------------------------------------------------------

MFileDialog::MFileDialog(const QString& dir,
   const QString& filter, QWidget* parent, bool writeFlag)
  : QFileDialog(parent, QString(), QString("."), filter)
      {
      readMidiPortsSaved = true;
      showButtons = false;
      lastUserDir = "";
      lastGlobalDir = "";

      if (dir.length() > 0 && dir[0] == QChar('/')) {
            setDirectory(dir);
            }
      else {
            // We replace the original sidebar widget with our 3-button widget
            QLayout* mainlayout = this->layout();
            QSplitter* spl = (QSplitter*)mainlayout->itemAt(2)->widget();
            QWidget* original_sidebarwidget = spl->widget(0);
            original_sidebarwidget->setVisible(false);

            baseDir     = dir;
            showButtons = true;

            spl->insertWidget(0,&buttons);

	    // Qt >= 4.6 allows us to select icons from the theme
#if QT_VERSION >= 0x040600
            buttons.globalButton->setIcon(*globalIcon);
            buttons.userButton->setIcon(*userIcon);
            buttons.projectButton->setIcon(*projectIcon);
#else
            buttons.globalButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
            buttons.userButton->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
            buttons.projectButton->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
#endif	    

            buttons.globalButton->setAutoExclusive(true);
            buttons.userButton->setAutoExclusive(true);
            buttons.projectButton->setAutoExclusive(true);

            connect(buttons.globalButton, SIGNAL(toggled(bool)), this, SLOT(globalToggled(bool)));
            connect(buttons.userButton, SIGNAL(toggled(bool)), this, SLOT(userToggled(bool)));
            connect(buttons.projectButton, SIGNAL(toggled(bool)), this, SLOT(projectToggled(bool)));
            connect(this, SIGNAL(directoryEntered(const QString&)), SLOT(directoryChanged(const QString&)));
            connect(this, SIGNAL(currentChanged(const QString&)), SLOT(fileChanged(const QString&)));

            if (writeFlag) {
                  setAcceptMode(QFileDialog::AcceptSave);
                  buttons.globalButton->setEnabled(false);
                  switch (lastViewUsed) {
                           case GLOBAL_VIEW:
                           case PROJECT_VIEW:
                                 buttons.globalButton->setChecked(true); // Let toggled be called. Don't block these...
                                 break;

                           case USER_VIEW:
                                 buttons.userButton->setChecked(true); 
                                 break;
                        }
                  }
            else {
                  switch (lastViewUsed) {
                        case GLOBAL_VIEW:
                              buttons.globalButton->setChecked(true); 
                              break;

                        case PROJECT_VIEW:
                              buttons.projectButton->setChecked(true); 
                              break;

                        case USER_VIEW:
                              buttons.userButton->setChecked(true); 
                              break;
                        }

	          }
            buttons.readMidiPortsGroup->setVisible(false);
            buttons.writeWinStateGroup->setVisible(false);
            }
      }

//---------------------------------------------------------
//   MFileDialog::directoryChanged
//---------------------------------------------------------
void MFileDialog::directoryChanged(const QString&)
      {
      ViewType currentView = GLOBAL_VIEW;
      QDir ndir = directory();
      ///QString newdir = ndir.absolutePath().toLatin1();
      QString newdir = ndir.absolutePath();
      if (buttons.projectButton->isChecked())
            currentView = PROJECT_VIEW;
      else if (buttons.userButton->isChecked())
            currentView = USER_VIEW;

      switch (currentView) {
            case GLOBAL_VIEW:
                  lastGlobalDir = newdir;
                  break;

            case USER_VIEW:
                  lastUserDir = newdir;
                  break;

            case PROJECT_VIEW: // Do nothing
            default:
                  break;
            }
      }

//---------------------------------------------------------
//   getOpenFileName
//---------------------------------------------------------
QString getOpenFileName(const QString &startWith, const char** filters_chararray,
            QWidget* parent, const QString& name, bool* doReadMidiPorts, MFileDialog::ViewType viewType)
      {
      QStringList filters = localizedStringListFromCharArray(filters_chararray, "file_patterns");
      
      MFileDialog *dlg = new MFileDialog(startWith, QString::null, parent, false);
      dlg->setNameFilters(filters);
      dlg->setWindowTitle(name);
      if (doReadMidiPorts)
            dlg->buttons.readMidiPortsGroup->setVisible(true);
      // Allow overrides. FIXME - some redundancy in MFileDialog ctor. Make this better.
      if (viewType == MFileDialog::GLOBAL_VIEW)
        dlg->buttons.globalButton->setChecked(true); // Let toggled be called. Don't block these...
      else if (viewType == MFileDialog::PROJECT_VIEW)
        dlg->buttons.projectButton->setChecked(true);
      else if (viewType == MFileDialog::USER_VIEW)
        dlg->buttons.userButton->setChecked(true);

      dlg->setFileMode(QFileDialog::ExistingFile);
      QStringList files;
      QString result;
      if (dlg->exec() == QDialog::Accepted) {
          files = dlg->selectedFiles();
          if (!files.isEmpty())
              result = files[0];
          if (doReadMidiPorts)
              *doReadMidiPorts = dlg->buttons.readMidiPortsButton->isChecked();
      }
      delete dlg;
      return result;
}

//---------------------------------------------------------
//   getSaveFileName
//---------------------------------------------------------

QString getSaveFileName(const QString &startWith,
   const char** filters_chararray, QWidget* parent, const QString& name, bool* writeWinState)
      {
      QStringList filters = localizedStringListFromCharArray(filters_chararray, "file_patterns");
      
      MFileDialog *dlg = new MFileDialog(startWith, QString::null, parent, true);
      dlg->setNameFilters(filters);
      dlg->setWindowTitle(name);
      dlg->setFileMode(QFileDialog::AnyFile);
      if (writeWinState)
      {
        dlg->buttons.writeWinStateGroup->setVisible(true);
        dlg->buttons.writeWinStateButton->setChecked(*writeWinState);
      }

      QStringList files;
      QString result;
      if (dlg->exec() == QDialog::Accepted) {
            files = dlg->selectedFiles();
            if (!files.isEmpty())
                  result = files[0];
          if (writeWinState)
              *writeWinState = dlg->buttons.writeWinStateButton->isChecked();
      }
                  
      // Added by T356.
      if(!result.isEmpty())
      {
        QString filt = dlg->selectedNameFilter();
        filt = getFilterExtension(filt);
        // Do we have a valid extension?
        if(!filt.isEmpty())
        {
          // If the rightmost characters of the filename do not already contain
          //  the extension, add the extension to the filename.
          //if(result.right(filt.length()) != filt)
          if(!result.endsWith(filt))
            result += filt;
        }
        else
        {
          // No valid extension, or just * was given. Although it would be nice to allow no extension
          //  or any desired extension by commenting this section out, it's probably not a good idea to do so.
          //
          // NOTE: Most calls to this routine getSaveFileName() are followed by fileOpen(),
          //  which can tack on its own extension, but only if the *complete* extension is blank. 
          // So there is some overlap going on. Enabling this actually stops that action, 
          //  but only if there are no errors in the list of filters. fileOpen() will act as a 'catchall'.
          //
          // Force the filter list to the first one (the preferred one), and then get the filter.
          dlg->selectNameFilter(dlg->nameFilters().at(0));
          filt = dlg->selectedNameFilter();
          filt = getFilterExtension(filt);
              
          // Do we have a valid extension?
          if(!filt.isEmpty())
          {
            // If the rightmost characters of the filename do not already contain
            //  the extension, add the extension to the filename.
            //if(result.right(filt.length()) != filt)
            if(!result.endsWith(filt))
              result += filt;
          }
        }
      }
      
      delete dlg;
      return result;
      }

//---------------------------------------------------------
//   getImageFileName
//---------------------------------------------------------

QString getImageFileName(const QString& startWith,
   const char** filters_chararray, QWidget* parent, const QString& name)
      {
      QStringList filters = localizedStringListFromCharArray(filters_chararray, "file_patterns");
      QString initialSelection;
	QString* workingDirectory = new QString(QDir::currentPath());
      if (!startWith.isEmpty() ) {
            QFileInfo fi(startWith);
            if (fi.exists() && fi.isDir()) {
                  *workingDirectory = startWith;
                  }
            else if (fi.exists() && fi.isFile()) {
                  *workingDirectory = fi.absolutePath();
                  initialSelection = fi.absoluteFilePath();
                  }
            }
      MFileDialog *dlg = new MFileDialog(*workingDirectory, QString::null,
         parent);

      /* ORCAN - disable preview for now. It is not available in qt4. We will
                 need to implement it ourselves.
      dlg->setContentsPreviewEnabled(true);
      ContentsPreview* preview = new ContentsPreview(dlg);
      dlg->setContentsPreview(preview, preview);
      dlg->setPreviewMode(QFileDialog::Contents);
      */
      dlg->setWindowTitle(name);
      dlg->setNameFilters(filters);
      dlg->setFileMode(QFileDialog::ExistingFile);
      QStringList files;
      QString result;
      if (!initialSelection.isEmpty())
           dlg->selectFile( initialSelection);
      if (dlg->exec() == QDialog::Accepted) {
	   files = dlg->selectedFiles();
	   if (!files.isEmpty())
                result = files[0];
      }
      delete dlg;
      return result;
      }

//---------------------------------------------------------
//   fileOpen
//    opens file "name" with extension "ext" in mode "mode"
//    handles "name.ext.bz2" and "name.ext.gz"
//
//    mode = "r" or "w"
//    popenFlag   set to true on return if file was opened
//                with popen() (and therefore must be closed
//                with pclose())
//    noError     show no error if file was not found in "r"
//                mode. Has no effect in "w" mode
//    overwriteWarning
//                warn in "w" mode, if file exists
//---------------------------------------------------------

FILE* fileOpen(QWidget* parent, QString name, const QString& ext,
   const char* mode, bool& popenFlag, bool noError,
   bool overwriteWarning)
      {
      QFileInfo info(name);
      QString zip;

      popenFlag = false;
      if (info.completeSuffix() == "") {
            name += ext;
            info.setFile(name);
            }
      else if (info.suffix() == "gz") {
            popenFlag = true;
            zip = QString("gzip");
            }
      else if (info.suffix() == "bz2") {
            popenFlag = true;
            zip = QString("bzip2");
            }

      if (strcmp(mode,"w") == 0 && overwriteWarning && info.exists()) {
            QString s(QWidget::tr("File\n%1\nexists. Overwrite?").arg(name));
            /*
            int rv = QMessageBox::warning(parent,
               QWidget::tr("MusE: write"),
               s,
               QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save);
            switch(rv) {
                  case 0:  // overwrite
                        break;
                  case 1:  // quit
                        return 0;
                  }
            */      
            if(QMessageBox::warning(parent,
               QWidget::tr("MusE: write"), s,
               QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Save)
               != QMessageBox::Save)
              return 0;
                  
            }
      FILE* fp = 0;
      if (popenFlag) {
            if (strcmp(mode, "r") == 0)
                  //zip += QString(" -d < ");
                  zip += QString(" -d < \"");    // p4.0.40
            else
                  zip += QString(" > \"");
            //zip += name;
            zip = zip + name + QString("\"");    // p4.0.40
            fp  = popen(zip.toLocal8Bit().data(), mode);
            }
      else {
            fp = fopen(name.toLocal8Bit().data(), mode);
            }
      if (fp == 0 && !noError) {
            QString s(QWidget::tr("Open File\n%1\nfailed: %2").arg(name).arg(strerror(errno)));
            QMessageBox::critical(parent, QWidget::tr("MusE: Open File"), s);
            return 0;
            }
      return fp;
      }

//---------------------------------------------------------
//   MFile
//---------------------------------------------------------

MFile::MFile(const QString& _path, const QString& _ext)
   : path(_path), ext(_ext)
      {
      f = 0;
      isPopen = false;
      }

MFile::~MFile()
      {
      if (f) {
            if (isPopen)
                  pclose(f);
            else
                  fclose(f);
            }
      }

//---------------------------------------------------------
//   open
//---------------------------------------------------------

FILE* MFile::open(const char* mode, const char** patterns_chararray,
   QWidget* parent, bool noError, bool warnIfOverwrite, const QString& caption)
      {
      QString name;
      if (strcmp(mode, "r") == 0)
           name = getOpenFileName(path, patterns_chararray, parent, caption, 0);
      else
           name = getSaveFileName(path, patterns_chararray, parent, caption);
      if (name.isEmpty())
            return 0;
      f = fileOpen(parent, name, ext, mode, isPopen, noError,
         warnIfOverwrite);
      return f;
      }

} // namespace MusEGui
