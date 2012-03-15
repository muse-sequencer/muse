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

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "aboutbox_impl.h"

namespace MusEGui {

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHelpBrowser()
      {
      QString lang(getenv("LANG"));
      QString museHelp = DOCDIR + QString("/html/index_") + lang + QString(".html");
      if (access(museHelp.toLatin1(), R_OK) != 0) {
            museHelp = DOCDIR + QString("/html/index.html");
            if (access(museHelp.toLatin1(), R_OK) != 0) {
                  QString info(tr("no help found at: "));
                  info += museHelp;
                  QMessageBox::critical(this, tr("MusE: Open Help"), info);
                  return;
                  }
            }
      launchBrowser(museHelp);
      }

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHomepageBrowser()
      {
      QString museHome = QString("http://www.muse-sequencer.org");

      launchBrowser(museHome);
      }

//---------------------------------------------------------
//   startBugBrowser
//---------------------------------------------------------

void MusE::startBugBrowser()
      {
      QString museBugPage("http://www.muse-sequencer.org/index.php/Report_a_bug");
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
            QMessageBox::information(this, tr("Unable to launch help"), 
                                     tr("For some reason MusE has to launch the default\n"
                                        "browser on your machine."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            printf("Unable to launch help\n");
            }
      }

} // namespace MusEGui
