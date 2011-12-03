//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/projectcreateimpl.cpp $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
#include <stdio.h>
#include <QFileDialog>
#include <QDir>
#include <QStringList>

#include "projectcreateimpl.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"
#include "helper.h"
#include "icons.h"

namespace MusEGui {

ProjectCreateImpl::ProjectCreateImpl(QWidget *parent) :
    QDialog(parent)
{
  setupUi(this);

  QStringList filters = localizedStringListFromCharArray(MusEGlobal::project_create_file_save_pattern, "file_patterns");
  projectFileTypeCB->addItems(filters);
  projectFileTypeCB->setCurrentIndex(0);

  projectNameEdit->setText(MusEGui::projectTitleFromFilename(MusEGlobal::muse->projectName())); 
  //projectNameEdit->setText(MusEGui::getUniqueUntitledName(MusEGui::projectTitleFromFilename(MusEGlobal::muse->projectName())));   TODO p4.0.40  // REMOVE Tim.
  
  projDirToolButton->setIcon(*openIcon);
  browseDirButton->setIcon(*openIcon);
  restorePathButton->setIcon(*undoIcon);
  
  createFolderCheckbox->setChecked(MusEGlobal::config.projectStoreInFolder);
  //connect(templateCheckBox,SIGNAL(stateChanged(int)), this, SLOT(templateButtonChanged(int)));
  connect(templateCheckBox,SIGNAL(stateChanged(int)), this, SLOT(updateDirectoryPath()));
  connect(projDirToolButton,SIGNAL(clicked()), this, SLOT(browseProjDir()));
  connect(restorePathButton,SIGNAL(clicked()), this, SLOT(restorePath()));
  connect(browseDirButton,SIGNAL(clicked()), this, SLOT(selectDirectory()));
  //connect(projectNameEdit,SIGNAL(textChanged(QString)), this, SLOT(updateDirectoryPath()));
  connect(projectNameEdit,SIGNAL(textChanged(QString)), this, SLOT(updateProjectName()));
  connect(createFolderCheckbox,SIGNAL(clicked()), this, SLOT(updateDirectoryPath()));
  connect(projectFileTypeCB,SIGNAL(currentIndexChanged(int)), this, SLOT(updateDirectoryPath()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(ok()));
#if QT_VERSION >= 0x040700
  projectNameEdit->setPlaceholderText("<Project Name>");
  // Orcan: Commented out since there is no QPlainTextEdit::setPlaceholderText()
  //        as of Qt-4.7.1
  //commentEdit->setPlaceholderText("<Add information about project here>");
#endif
  directoryPath = MusEGlobal::config.projectBaseFolder;
  //templDirPath = MusEGlobal::configPath + "/templates";
  updateDirectoryPath();
  show();
}

void ProjectCreateImpl::selectDirectory()
{
  QString dpath = templateCheckBox->isChecked() ? 
                   (templDirPath.isEmpty()    ? (MusEGlobal::configPath + QString("/templates")) : templDirPath) :
                   (overrideDirPath.isEmpty() ? directoryPath : overrideDirPath);
  
  QString dir = QFileDialog::getExistingDirectory(this, tr("Select directory"), dpath);
  if(dir.isEmpty())
     return;
  
  (templateCheckBox->isChecked() ? templDirPath : overrideDirPath) = dir;
  updateDirectoryPath();
}

void ProjectCreateImpl::updateProjectName()
{
  QString curExt = projectFileTypeCB->currentText();  
  if(curExt.isEmpty())
    curExt = ".med";
  else
  {  
    curExt = MusEGui::getFilterExtension(curExt);
    // Do we have a valid extension?
    if(curExt.isEmpty())
      curExt = ".med";
  }
  
  QString name = "";
  if (createFolderCheckbox->isChecked()) {
    if (!projectNameEdit->text().isEmpty())
      //name = projectNameEdit->text() + "/" + projectNameEdit->text() + ".med";
      name = projectNameEdit->text() + "/" + projectNameEdit->text() + curExt;
    //storageDirEdit->setText(directoryPath + name );
  }  else {
    if (!projectNameEdit->text().isEmpty())
      //name = projectNameEdit->text() + ".med";
      name = projectNameEdit->text() + curExt;
    //storageDirEdit->setText(directoryPath +"/" + name);
  }
  
  //QString dpath = templateCheckBox->isChecked() ? templDirPath : directoryPath;
  QString dpath = templateCheckBox->isChecked() ? 
                   (templDirPath.isEmpty()    ? (MusEGlobal::configPath + QString("/templates")) : templDirPath) :
                   (overrideDirPath.isEmpty() ? directoryPath : overrideDirPath);
  
  storageDirEdit->blockSignals(true);
  storageDirEdit->setText(dpath + "/" + name );    
  storageDirEdit->blockSignals(false);
}

void ProjectCreateImpl::updateDirectoryPath()
{
  updateProjectName();

  projDirLineEdit->blockSignals(true);
  projDirLineEdit->setText(MusEGlobal::config.projectBaseFolder);
  projDirLineEdit->blockSignals(false);
    
  projDirLineEdit->setEnabled(!templateCheckBox->isChecked() && overrideDirPath.isEmpty());
}

QString ProjectCreateImpl::getProjectPath() const
{
   return storageDirEdit->text();
}

QString ProjectCreateImpl::getSongInfo() const
{
   return commentEdit->toPlainText();
}

void ProjectCreateImpl::ok()
{
  MusEGlobal::config.projectStoreInFolder = createFolderCheckbox->isChecked();
  MusEGlobal::config.projectBaseFolder = directoryPath;
  MusEGlobal::muse->changeConfig(true);
  emit accept();
}

void ProjectCreateImpl::browseProjDir()
{
  QString dir = MusEGui::browseProjectFolder(this);
  if(!dir.isEmpty())
  {  
    directoryPath = dir;
    MusEGlobal::config.projectBaseFolder = dir;
    MusEGlobal::muse->changeConfig(true);  // Save to config file.
    updateDirectoryPath();
  }  
}

/*
void ProjectCreateImpl::templateButtonChanged(int newVal)
{
  //if(newVal == Qt::Checked)
  //  projDirLineEdit->setEnabled(false);
  //else
  //  projDirLineEdit->setEnabled(true);
  updateDirectoryPath();
}
*/

void ProjectCreateImpl::restorePath()  
{
  if(templateCheckBox->isChecked())
    templDirPath.clear();
  else
  {  
    overrideDirPath.clear();
    //projDirLineEdit->setEnabled(true);
  }  
  updateDirectoryPath();
}

/*
bool ProjectCreateImpl::getProjectIsTemplate() const
{
  return templateCheckBox->isChecked();
}

QString ProjectCreateImpl::getTemplatePath() const
{
   return templDirPath;
}
*/

} //namespace MusEGui
