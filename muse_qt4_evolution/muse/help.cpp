//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "config.h"
#include "muse.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "help.h"

//---------------------------------------------------------
//   AboutBoxDialog
//---------------------------------------------------------

AboutBoxDialog::AboutBoxDialog()
      {
      extern int revision;
      setupUi(this);
      versionLabel->setText("Version: " VERSION);
      revisionLabel->setText(QString("Revision: %1").arg(revision));
      }

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHelpBrowser()
      {
      QString lang(getenv("LANG"));
      QFileInfo museHelp(museGlobalShare + QString("/doc/man-") + lang + QString(".pdf"));
      if (!museHelp.isReadable()) {
    	      museHelp.setFile(museGlobalShare + QString("/doc/man-en.pdf"));
            if (!museHelp.isReadable()) {
                  QString info(tr("MusE manual not found at: "));
                  info += museHelp.filePath();
                  QMessageBox::critical(this, tr("MusE: Open Help"), info);
                  return;
                  }
            }
      QString url("file://" + museHelp.filePath());
      QDesktopServices::openUrl(url);
      }

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void MusE::startHomepageBrowser()
      {
      QString museHome = QString("http://lmuse.sourceforge.net");

      QDesktopServices::openUrl(museHome);
      }

//---------------------------------------------------------
//   startBugBrowser
//---------------------------------------------------------

void MusE::startBugBrowser()
      {
      QString museBugPage("http://lmuse.sourceforge.net/bugs.html");
      QDesktopServices::openUrl(museBugPage);
      }

//---------------------------------------------------------
//   about
//---------------------------------------------------------

void MusE::about()
      {
      AboutBoxDialog ab;
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

