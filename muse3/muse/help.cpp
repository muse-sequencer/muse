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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "aboutbox_impl.h"

// Whether to open the pdf or the html
#define MUSE_USE_PDF_HELP_FILE

namespace MusEGui {

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHelpBrowser()
{
    QString museManual = QString("https://github.com/muse-sequencer/muse/wiki/Documentation");
    launchBrowser(museManual);
}

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHomepageBrowser()
      {
      QString museHome = QString("https://muse-sequencer.github.io");
      launchBrowser(museHome);
      }

//---------------------------------------------------------
//   startBugBrowser
//---------------------------------------------------------

void MusE::startBugBrowser()
      {
      QString museBugPage("https://github.com/muse-sequencer/muse/issues");
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

void MusE::launchBrowser(QString &whereTo)
      {
      if (! QDesktopServices::openUrl(QUrl(whereTo)))
            {
            QMessageBox::information(this, tr("Unable to launch browser"),
                                     tr("Error launching default browser"),
                                     QMessageBox::Ok);
            printf("Unable to launch browser\n");
            }
      }

} // namespace MusEGui
