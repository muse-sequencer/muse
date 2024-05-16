//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: help.cpp,v 1.7.2.4 2009/07/05 23:06:21 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

//#include <unistd.h>
//#include <stdlib.h>
#include <stdio.h>

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
//#include <QDebug>

#include "app.h"
#include "globals.h"
//#include "gconfig.h"
//#include "icons.h"
#include "aboutbox_impl.h"
//#include "config.h" // Already in app.h

// Whether to open the pdf or the html
//#define MUSE_USE_PDF_HELP_FILE

namespace MusEGui {

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHelpBrowser()
{
    QWidget* w = QApplication::widgetAt (QCursor::pos());
//    qDebug() << "Debug F1 help: Widget at mouse position:" << w << w->objectName();

    // setting object name for action toolbutton doesn't work, use a workaround
    QToolButton* tb = nullptr;
    if (w && strcmp(w->metaObject()->className(), "QToolButton") == 0) {
        tb = dynamic_cast<QToolButton*>(w);
//        qDebug() << "Debug F1 help: Tool button action:" << tb << tb->defaultAction()->objectName();
    }

    QString museManual;

    if (w && w->objectName() == "PartCanvas")
        museManual = QString(ORGANIZATION_HELP_URL "editoruse");
    else if (w && w->objectName() == "Pianoroll")
        museManual = QString(ORGANIZATION_HELP_URL "editoruse");
    else if (w && w->objectName() == "DrumCanvas")
        museManual = QString(ORGANIZATION_HELP_URL "editoruse");
    else if (w && w->objectName() == "WaveCanvas")
        museManual = QString(ORGANIZATION_HELP_URL "editoruse");
    else if (w && w->objectName() == "TrackList")
        museManual = QString(ORGANIZATION_HELP_URL "tracks#tracks-and-parts");
    else if (w && w->objectName() == "EffectRack")
        museManual = QString(ORGANIZATION_HELP_URL "plugins#the-audio-effects-rack");
    else if (w && w->objectName() == "SoloButton")
        museManual = QString(ORGANIZATION_HELP_URL "trackssolo#track-soloing");
    else if (w && (w->objectName() == "InputRouteButton" || w->objectName() == "OutputRouteButton"))
        museManual = QString(ORGANIZATION_HELP_URL "routes#routes");
    else if (w && (w->objectName() == "AudioAutoType"))
        museManual = QString(ORGANIZATION_HELP_URL "automation#audio-automation");

    else if (w && tb && tb->defaultAction()->objectName() == "PanicButton")
        museManual = QString(ORGANIZATION_HELP_URL "panic");
    else if (w && tb && tb->defaultAction()->objectName() == "MetronomeButton")
        museManual = QString(ORGANIZATION_HELP_URL "metronome");

    else
        museManual = QString(ORGANIZATION_HELP_URL "intro");

    launchBrowser(museManual);
}

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHomepageBrowser()
      {
      QString museHome = QString(ORGANIZATION_URL);
      launchBrowser(museHome);
      }

//---------------------------------------------------------
//   startBugBrowser
//---------------------------------------------------------

void MusE::startBugBrowser()
      {
      QString museBugPage(ORGANIZATION_CODE_REPO_URL "issues");
      launchBrowser(museBugPage);
      }

//---------------------------------------------------------
//   about
//---------------------------------------------------------

void MusE::about()
      {
      MusEGui::AboutBoxImpl ab;
      ab.show();
      ab.exec();
      }

//---------------------------------------------------------
//   aboutQt
//---------------------------------------------------------

void MusE::aboutQt()
      {
      QMessageBox::aboutQt(this, QString("MusE"));
      }

//---------------------------------------------------------
//   launchBrowser
//---------------------------------------------------------

void MusE::launchBrowser(QString &whereTo)
{
    // lib path must be cleared temporarily as some browsers try
    //   to access potentially incompatible libs from the appimage
    QByteArray ldLibPath;
    const QByteArray appDir = qgetenv("APPDIR"); // running in AppImage
    if (!appDir.isEmpty()) {
        ldLibPath = qgetenv("LD_LIBRARY_PATH");
        qputenv("LD_LIBRARY_PATH", "");
    }

    if (! QDesktopServices::openUrl(QUrl(whereTo)))
    {
        QMessageBox::information(this, tr("Unable to launch browser"),
                                 tr("Error launching default browser"),
                                 QMessageBox::Ok);
        printf("Unable to launch browser\n");
    }

    if (!appDir.isEmpty()) {
        qputenv("LD_LIBRARY_PATH", ldLibPath);
    }
}

} // namespace MusEGui
