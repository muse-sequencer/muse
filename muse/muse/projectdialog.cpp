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

#include "projectdialog.h"
#include "gconfig.h"
#include "song.h"

//---------------------------------------------------------
//   ProjectDialog
//---------------------------------------------------------

ProjectDialog::ProjectDialog(QWidget* parent)
  : QDialog(parent)
      {
      setupUi(this);
      projectTree->setSelectionBehavior(QAbstractItemView::SelectRows);
      projectTree->setSelectionMode(QAbstractItemView::SingleSelection);
      QDir pd(QDir::homePath() + "/" + config.projectPath);
      QStringList el = pd.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
      QTreeWidgetItem* current = 0;
      foreach (QString s, el) {
            QFile pf(QDir::homePath() + "/" + config.projectPath + "/" 
               + s + "/" + s + ".med");
            if (pf.exists()) {
                  QTreeWidgetItem* pi = new QTreeWidgetItem;
                  pi->setText(0, s);
                  projectTree->addTopLevelItem(pi);
                  if (s == song->projectName())
                        current = pi;
                  }
            }
      connect(projectTree, 
         SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), 
         SLOT(currentChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
      connect(projectTree, 
         SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
         SLOT(accept()));
      connect(projectName,
         SIGNAL(textEdited(const QString&)),
         SLOT(projectNameEdited(const QString&)));

      if (current)
            projectTree->setCurrentItem(current); 
      else
            currentChanged(0, 0);
      }

//---------------------------------------------------------
//   currentChanged
//---------------------------------------------------------

void ProjectDialog::currentChanged(QTreeWidgetItem* item, QTreeWidgetItem*)
      {
      bool enable = item != 0;
      createdDate->setEnabled(enable);
      modifiedDate->setEnabled(enable);
      comment->setEnabled(enable);
      length->setEnabled(enable);

      if (!enable)
            return;

      projectName->setText(item->text(0));
      QFileInfo pf(QDir::homePath() + "/" + config.projectPath + "/" 
         + item->text(0) + "/" + item->text(0) + ".med");
      createdDate->setDateTime(pf.created());
      modifiedDate->setDateTime(pf.lastModified());

      QTime time(0, 0, 0);

      QFile f(pf.filePath());
      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(&f, false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n    at line: " + ln + " col: " + col;
            printf("error reading med file: %s\n", error.toLatin1().data());
            return;
            }
      for (QDomNode node = doc.documentElement(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "muse") {
                  QString sversion = e.attribute("version", "1.0");
                  int major=0, minor=0;
                  sscanf(sversion.toLatin1().data(), "%d.%d", &major, &minor);
                  int version = major << 8 + minor;
                  if (version >= 0x200) {
                        for (QDomNode n1 = node.firstChild(); !n1.isNull(); n1 = n1.nextSibling()) {
                              QDomElement e = n1.toElement();
                              if (e.tagName() == "song") {
                                    for (QDomNode n2 = n1.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
                                          QDomElement e = n2.toElement();
                                          QString tag(e.tagName());
                                          QString s(e.text());
                                          if (tag == "comment")
                                                comment->setPlainText(s);
                                          else if (tag == "LenInSec") {
                                                int sec = s.toInt();
                                                time = time.addSecs(sec);
                                                }
                                          }
                                    }
                              }
                        }
                  }
            }
      length->setTime(time);
      }

//---------------------------------------------------------
//   projectNameEdited
//---------------------------------------------------------

void ProjectDialog::projectNameEdited(const QString&)
      {
      QTreeWidgetItem* item = projectTree->currentItem();
      if (item)
            projectTree->setItemSelected(item, false);
      projectTree->setCurrentItem(0);      
      }

