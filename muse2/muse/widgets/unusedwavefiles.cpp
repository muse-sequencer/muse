//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/unusedwavefiles.cpp $
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
#include <qdir.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qmessagebox.h>
#include "unusedwavefiles.h"
#include "ui_unusedwavefiles.h"
#include "globals.h"
#include "app.h"

namespace MusEGui {

UnusedWaveFiles::UnusedWaveFiles(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UnusedWaveFiles)
{

    ui->setupUi(this);
    ui->currentProjRadioButton->setChecked(true);
    connect(ui->currentProjRadioButton, SIGNAL(clicked()), SLOT(findWaveFiles()));
    connect(ui->allProjRadioButton, SIGNAL(clicked()), SLOT(findWaveFiles()));
    findWaveFiles();

}

void UnusedWaveFiles::findWaveFiles()
{
    ui->filelistWidget->clear();
    //printf("MusEGlobal::museProject =%s\n", MusEGlobal::museProject.toLatin1().data());
//    QFileInfo proj(MusEGlobal::museProject);
//    QString projPath = proj.absolutePath();
    QDir dir(MusEGlobal::museProject);
    QStringList filter;
    filter.append("*.wav");
    filter.append("*.ogg");
    allWaveFiles= dir.entryList(filter);
    if (!allWaveFiles.count())
        return;
    // get med files
    QStringList medFiles;
    if (ui->currentProjRadioButton->isChecked()) {
        medFiles.append(MusEGlobal::muse->projectName());
    } else {
        //printf("get ALLL *.med files!\n");
        QStringList medFilter("*.med");
        medFiles = dir.entryList(medFilter);
    }
    foreach (QString medFile, medFiles) {
        QString fname = MusEGlobal::museProject+"/"+ medFile;
        //printf("fopen %s\n", fname.toLatin1().data());
        FILE *fp =fopen(fname.toLatin1().data(),"r");
        QTextStream fileContent(fp);
        while (!fileContent.atEnd()) {
            QString line = fileContent.readLine();
            if (line.contains(".wav") || line.contains(".ogg")) { // optimization
                foreach (QString wav, allWaveFiles) {
                    //printf("checking wav [%s]\n", wav.toLatin1().data() );
                    if (line.contains(wav)) {
                        //int beforeSize=allWaveFiles.size();
                        allWaveFiles.removeOne(wav);
                        //printf("removed one from list, %d %d\n", beforeSize, allWaveFiles.size());
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }

    ui->filelistWidget->addItems(allWaveFiles);
    update();
}

UnusedWaveFiles::~UnusedWaveFiles()
{
    delete ui;
}

void UnusedWaveFiles::accept()
{
    int ret = QMessageBox::question(this,"Move files", "Are you sure you want to move away the unused files?",
                                    QMessageBox::Ok, QMessageBox::Cancel);
    if (ret == QMessageBox::Ok) {
        QDir currDir(MusEGlobal::museProject);
        currDir.mkdir("unused");

        foreach(QString file, allWaveFiles) {
            QFile::rename(MusEGlobal::museProject+ "/"+file, MusEGlobal::museProject + "/unused/" +file);
            // move the wca file if it exists
            QFileInfo wf(MusEGlobal::museProject + "/" + file);
            if (QFile::exists(MusEGlobal::museProject + "/" + wf.baseName()+".wca")) {
                QFile::rename(MusEGlobal::museProject + "/" + wf.baseName()+".wca", MusEGlobal::museProject + "/unused/" +wf.baseName()+".wca");

            }
        }
    }
    QDialog::accept();
}

} // namespace MusEGui
