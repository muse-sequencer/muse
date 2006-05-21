//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2006 by Werner Schweer and others
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

#include "templatedialog.h"
#include "gconfig.h"
#include "song.h"
#include "icons.h"

//
// entry types for templateTree tree widget:
//
enum { DIR_TYPE, LOCAL_TEMPLATE_TYPE, GLOBAL_TEMPLATE_TYPE};

//---------------------------------------------------------
//   processSubdir
//---------------------------------------------------------

void TemplateDialog::processSubdir(QTreeWidgetItem* item, const QString& p, 
   const QString& subdir, int type)
      {
      QDir pd(p + "/" + subdir);
      pd.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
      pd.setNameFilters(QStringList("*.med"));
      QFileInfoList el = pd.entryInfoList();
      foreach (QFileInfo s, el) {
            QTreeWidgetItem* pi;
            if (s.isDir()) {
                  pi = new QTreeWidgetItem(item, DIR_TYPE);
                  itemCollapsed(pi);
                  processSubdir(pi, pd.absolutePath(), s.fileName(), type);
                  }
            else {
                  pi = new QTreeWidgetItem(item, type);
                  pi->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
                  }
            pi->setText(0, s.fileName());
            }
      }

//---------------------------------------------------------
//   TemplateDialog
//---------------------------------------------------------

TemplateDialog::TemplateDialog(QWidget* parent)
  : QDialog(parent)
      {
      setupUi(this);
      templateTree->setSelectionBehavior(QAbstractItemView::SelectRows);
      templateTree->setSelectionMode(QAbstractItemView::SingleSelection);

      //
      //    add global templates to list
      //
      QTreeWidgetItem* pi;
      pi = new QTreeWidgetItem(templateTree, DIR_TYPE);
      templateTree->setItemExpanded(pi, true);
      itemExpanded(pi);
      pi->setText(0, tr("MusE presets"));
      processSubdir(pi, museGlobalShare, "templates", GLOBAL_TEMPLATE_TYPE);

      //
      //    add local templates to list
      //
      pi = new QTreeWidgetItem(templateTree, DIR_TYPE);
      pi->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
      templateTree->setItemExpanded(pi, true);
      itemExpanded(pi);
      pi->setText(0, tr("User presets"));
      processSubdir(pi, QDir::homePath() + "/" + config.templatePath, ".", LOCAL_TEMPLATE_TYPE);

      connect(templateTree, 
         SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), 
         SLOT(currentChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
      connect(templateTree, 
         SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
         SLOT(itemDoubleClicked(QTreeWidgetItem*, int)));
      connect(templateTree,
         SIGNAL(itemCollapsed(QTreeWidgetItem*)),
         SLOT(itemCollapsed(QTreeWidgetItem*)));
      connect(templateTree,
         SIGNAL(itemExpanded(QTreeWidgetItem*)),
         SLOT(itemExpanded(QTreeWidgetItem*)));
        
      currentChanged(0, 0);
      }

//---------------------------------------------------------
//   itemCollapsed
//---------------------------------------------------------

void TemplateDialog::itemCollapsed(QTreeWidgetItem* item)
      {
      item->setIcon(0, style()->standardIcon(QStyle::SP_DirClosedIcon));
      }

//---------------------------------------------------------
//   itemExpanded
//---------------------------------------------------------

void TemplateDialog::itemExpanded(QTreeWidgetItem* item)
      {
      item->setIcon(0, style()->standardIcon(QStyle::SP_DirOpenIcon));
      }

//---------------------------------------------------------
//   itemPath
//---------------------------------------------------------

QString TemplateDialog::itemPath(QTreeWidgetItem* item) const
      {
      QString path;
      QTreeWidgetItem* ti = item;
      QStringList dirComponent;
      do {
            dirComponent.prepend(ti->text(0));
            ti = ti->parent();
            } while (ti);
      size_t n = dirComponent.size();
      for (size_t i = 1; i < n; ++i) {
            if (!path.isEmpty())
                  path += "/";
            path += dirComponent[i];
            }
      return path;
      }

//---------------------------------------------------------
//   currentChanged
//---------------------------------------------------------

void TemplateDialog::currentChanged(QTreeWidgetItem* item, QTreeWidgetItem*)
      {
      bool enable = (item != 0) && 
        (
            (item->type() == LOCAL_TEMPLATE_TYPE)
            ||
            (item->type() == GLOBAL_TEMPLATE_TYPE)
         );
      createdDate->setEnabled(enable);
      modifiedDate->setEnabled(enable);
      comment->setEnabled(enable);
      
      // newFolder->setEnabled(item == 0 || item->type() == DIR_TYPE);
      if (!enable)
            return;

      QString pd;
      if (item->type() == LOCAL_TEMPLATE_TYPE)
            pd = QDir::homePath() + "/" + config.templatePath;
      else
            pd =  museGlobalShare + "/" + "templates";

      pd += "/" + itemPath(item);

      QFileInfo pf(pd);
      modifiedDate->setDateTime(pf.lastModified());

      QTime time(0, 0, 0);
      QDateTime date;

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
                                          else if (tag == "createDate") {
                                                date = QDateTime::fromString(e.text(), Qt::ISODate);
                                                break;
                                                }
                                          }
                                    }
                              }
                        }
                  }
            }
      createdDate->setDateTime(date);
      }

//---------------------------------------------------------
//   templatePath
//---------------------------------------------------------

QString TemplateDialog::templatePath() const
      {
      QTreeWidgetItem* item = templateTree->currentItem();
      QString s;
      if (item) {
            if (item->type() == LOCAL_TEMPLATE_TYPE)
                  s = QDir::homePath() + "/" + config.templatePath;
            else
                  s =  museGlobalShare + "/" + "templates";
            s += "/" + itemPath(item);
            }
      return s;
      }

//---------------------------------------------------------
//   itemDoubleClicked
//---------------------------------------------------------

void TemplateDialog::itemDoubleClicked(QTreeWidgetItem* item, int)
      {
      if ((item->type() == LOCAL_TEMPLATE_TYPE) ||
         (item->type() == GLOBAL_TEMPLATE_TYPE))
            accept();      
      }

