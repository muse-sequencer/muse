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

  directoryPath = MusEGlobal::config.projectBaseFolder;

  QStringList filters = localizedStringListFromCharArray(MusEGlobal::project_create_file_save_pattern, "file_patterns");
  projectFileTypeCB->addItems(filters);

  QString proj_title = MusEGlobal::muse->projectTitle();
  QString proj_path  = MusEGlobal::muse->projectPath();
//  QString proj_path  = directoryPath;
  QString proj_ext   = MusEGlobal::muse->projectExtension();
  
  projectNameEdit->setText(proj_title); 

  bool is_template = proj_path.startsWith(MusEGlobal::configPath + "/templates");
  
  templateCheckBox->setChecked(is_template);
  
  projDirPath = proj_path;

  int type_idx = 0;
  if(!proj_ext.isEmpty())
  {  
    // FIXME Imperfect. Trying to avoid adding yet another series of character strings.   p4.0.40
    type_idx = projectFileTypeCB->findText(proj_ext, Qt::MatchContains | Qt::MatchCaseSensitive);
    if(type_idx == -1)
      type_idx = 0;
  }  
  projectFileTypeCB->setCurrentIndex(type_idx);
  
  projDirToolButton->setIcon(*fileopenSVGIcon);
  browseDirButton->setIcon(*fileopenSVGIcon);
  restorePathButton->setIcon(*undoSVGIcon);

  restorePathButton->setEnabled(false);  // Disabled at first.
    
  connect(templateCheckBox,SIGNAL(toggled(bool)), this, SLOT(templateButtonChanged(bool)));
  connect(projDirToolButton,SIGNAL(clicked()), this, SLOT(browseProjDir()));
  connect(restorePathButton,SIGNAL(clicked()), this, SLOT(restorePath()));
  connect(browseDirButton,SIGNAL(clicked()), this, SLOT(selectDirectory()));
  connect(projectNameEdit,SIGNAL(textChanged(QString)), this, SLOT(updateProjectName()));
  connect(createFolderCheckbox,SIGNAL(clicked()), this, SLOT(createProjFolderChanged()));
  connect(projectFileTypeCB,SIGNAL(currentIndexChanged(int)), this, SLOT(updateDirectoryPath()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(ok()));
  projectNameEdit->setPlaceholderText("<Project Name>");
  commentEdit->setPlaceholderText("<Add information about your project here>");
  updateDirectoryPath();
  projectNameEdit->setFocus();
  show();
}

void ProjectCreateImpl::selectDirectory()
{
  QString dpath = templateCheckBox->isChecked() ? 
                   (overrideTemplDirPath.isEmpty()    ? (MusEGlobal::configPath + QString("/templates")) : overrideTemplDirPath) :
                   (overrideDirPath.isEmpty() ? directoryPath : overrideDirPath);
  
  QString dir = QFileDialog::getExistingDirectory(this, tr("Select directory"), dpath);
  if(dir.isEmpty())
     return;
  
  (templateCheckBox->isChecked() ? overrideTemplDirPath : overrideDirPath) = dir;
  restorePathButton->setEnabled(true); 
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
      name = projectNameEdit->text() + "/" + projectNameEdit->text() + curExt;
  }  else {
    if (!projectNameEdit->text().isEmpty())
      name = projectNameEdit->text() + curExt;
  }
  
  bool is_new = (MusEGlobal::museProject == MusEGlobal::museProjectInitPath);  

  QString dpath = templateCheckBox->isChecked() ?
                   (overrideTemplDirPath.isEmpty()    ? (MusEGlobal::configPath + QString("/templates")) : overrideTemplDirPath) :
                   (overrideDirPath.isEmpty() ? (is_new ? directoryPath : projDirPath) : overrideDirPath);
  
  QDir proj_dir(dpath);
  bool is_project = dpath.startsWith(MusEGlobal::config.projectBaseFolder);
                MusEGlobal::config.projectStoreInFolder && 
                (templateCheckBox->isChecked() ? overrideTemplDirPath.isEmpty() : overrideDirPath.isEmpty());  

  dpath = proj_dir.absolutePath();
    
  storageDirEdit->blockSignals(true);
  storageDirEdit->setText(dpath + "/" + name );
  storageDirEdit->blockSignals(false);
  
  projDirLineEdit->setEnabled(!templateCheckBox->isChecked() && is_project);
}

void ProjectCreateImpl::updateDirectoryPath()
{
  updateProjectName();

  projDirLineEdit->blockSignals(true);
  projDirLineEdit->setText(MusEGlobal::config.projectBaseFolder);
  projDirLineEdit->blockSignals(false);
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
  emit accept();
}

void ProjectCreateImpl::createProjFolderChanged()
{
  updateDirectoryPath();
}

void ProjectCreateImpl::browseProjDir()
{
  QString dir = MusEGui::browseProjectFolder(this);
  if(!dir.isEmpty())
  {  
    directoryPath = dir;
    MusEGlobal::config.projectBaseFolder = dir;
    // Save to config file. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
    MusEGlobal::muse->changeConfig(true);
    updateDirectoryPath();
  }  
}

void ProjectCreateImpl::templateButtonChanged(bool v)
{
  restorePathButton->setEnabled(v ? !overrideTemplDirPath.isEmpty() : !overrideDirPath.isEmpty()); 
  winStateCheckbox->setChecked(!v);
  updateDirectoryPath();
}

void ProjectCreateImpl::restorePath()  
{
  if(templateCheckBox->isChecked())
    overrideTemplDirPath.clear();
  else
    overrideDirPath.clear();
  restorePathButton->setEnabled(templateCheckBox->isChecked() ? !overrideTemplDirPath.isEmpty() : !overrideDirPath.isEmpty()); 
  updateDirectoryPath();
}

bool ProjectCreateImpl::getWriteTopwins() const
{
  return winStateCheckbox->isChecked();
}

void ProjectCreateImpl::setWriteTopwins(bool v)
{
  winStateCheckbox->setChecked(v);
}

} //namespace MusEGui
