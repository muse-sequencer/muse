//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/projectcreateimpl.cpp $
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
#include <stdio.h>
#include <qfiledialog.h>
#include <qdir.h>
#include "projectcreateimpl.h"
#include "gconfig.h"
#include "globals.h"
#include "app.h"

ProjectCreateImpl::ProjectCreateImpl(QWidget *parent) :
    QDialog(parent)
{
  setupUi(this);

  createFolderCheckbox->setChecked(config.projectStoreInFolder);
  connect(browseDirButton,SIGNAL(clicked()), this, SLOT(selectDirectory()));
  connect(projectNameEdit,SIGNAL(textChanged(QString)), this, SLOT(updateDirectoryPath()));
  connect(createFolderCheckbox,SIGNAL(clicked()), this, SLOT(updateDirectoryPath()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(ok()));
#if QT_VERSION >= 0x040700
  projectNameEdit->setPlaceholderText("<Project Name>");
  // Orcan: Commented out since there is no QPlainTextEdit::setPlaceholderText()
  //        as of Qt-4.7.1
  //commentEdit->setPlaceholderText("<Add information about project here>");
#endif
  directoryPath = config.projectBaseFolder;
  updateDirectoryPath();
  show();
}

void ProjectCreateImpl::selectDirectory()
{
  QFileDialog qfd;
  qfd.selectFile(directoryPath);
  qfd.setFileMode(QFileDialog::DirectoryOnly);
  if (qfd.exec() == QDialog::Rejected) {
    return;
  }
  directoryPath=qfd.selectedFiles().first();
  updateDirectoryPath();
}

void ProjectCreateImpl::updateDirectoryPath()
{
  QString name = "";
  if (createFolderCheckbox->isChecked()) {
    if (!projectNameEdit->text().isEmpty())
      name = projectNameEdit->text() + "/" + projectNameEdit->text() + ".med";
    //storageDirEdit->setText(directoryPath + name );
  }  else {
    if (!projectNameEdit->text().isEmpty())
      name = projectNameEdit->text() + ".med";
    //storageDirEdit->setText(directoryPath +"/" + name);
  }
  storageDirEdit->setText(directoryPath +"/" + name );    // Tim
}

QString ProjectCreateImpl::getProjectPath()
{
   return storageDirEdit->text();
}
QString ProjectCreateImpl::getSongInfo()
{
   return commentEdit->toPlainText();
}
void ProjectCreateImpl::ok()
{
  config.projectStoreInFolder = createFolderCheckbox->isChecked();
  config.projectBaseFolder = directoryPath;
  muse->changeConfig(true);
  emit accept();
}
