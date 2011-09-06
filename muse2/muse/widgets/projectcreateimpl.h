//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/projectcreateimpl.h $
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
#ifndef PROJECTCREATEIMPL_H
#define PROJECTCREATEIMPL_H

#include <QDialog>
#include "ui_projectcreate.h"

class ProjectCreateImpl : public QDialog, Ui::ProjectCreate
{
    Q_OBJECT

    QString directoryPath;
public:
    explicit ProjectCreateImpl(QWidget *parent = 0);
    QString getProjectPath();
    QString getSongInfo();

signals:

public slots:
    void updateDirectoryPath();
    void selectDirectory();
    void ok();

};

#endif // PROJECTCREATEIMPL_H
