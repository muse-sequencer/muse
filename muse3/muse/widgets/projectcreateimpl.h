//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/projectcreateimpl.h $
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
#ifndef PROJECTCREATEIMPL_H
#define PROJECTCREATEIMPL_H

#include <QDialog>
#include "ui_projectcreate.h"

namespace MusEGui {

class ProjectCreateImpl : public QDialog, Ui::ProjectCreate
{
    Q_OBJECT

    QString directoryPath;
    QString overrideDirPath;
    QString overrideTemplDirPath;
    QString projDirPath;
    
public:
    explicit ProjectCreateImpl(QWidget *parent = 0);
    QString getProjectPath() const;
    QString getSongInfo() const;
    bool getWriteTopwins() const;
    void setWriteTopwins(bool);
    //QString getTemplatePath() const;

signals:

protected slots:
    void updateProjectName();
    void updateDirectoryPath();
    void selectDirectory();
    void ok();
    void createProjFolderChanged();
    void browseProjDir();
    void templateButtonChanged(bool);
    void restorePath();
};

} // namespace MusEGui

#endif // PROJECTCREATEIMPL_H
