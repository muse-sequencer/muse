//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.cpp,v 1.1.1.1 2003/10/27 18:55:05 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include "header.h"
#include "xml.h"
#include "popupmenu.h"

#include <QStringList>
#include <QStandardItemModel>
#include <QMouseEvent>

namespace MusEGui {

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Header::readStatus(MusECore::Xml& xml)
{

    for (;;) {
          MusECore::Xml::Token token = xml.parse();
          const QString& tag = xml.s1();
          switch (token) {
                case MusECore::Xml::Error:
                case MusECore::Xml::End:
                      return;
                case MusECore::Xml::Text:
                      {
                          QStringList l = tag.split(QString(" "), QString::SkipEmptyParts);
                          int index = count() -1;
                          for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                                int logialIdx=abs((*it).toInt());
                                bool isHidden = (*it).toInt() < 0 ? true:false;
                                int section = visualIndex(logialIdx);
                                moveSection(section, index);
                                if (isHidden)
                                  hideSection(logialIdx-1);
                                else
                                  showSection(logialIdx);
                                --index;
                          }

                          // loop again looking for missing indexes
                          for (int i =0; i < count(); i++) {
                              bool foundIt=false;
                              for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                                int id=((*it).toInt());
                                if ( id == i || i ==1-id )
                                    foundIt=true;
                              }
                              if (foundIt == false) {
                                int section = visualIndex(i);
                                moveSection(section, i);
                                //printf("Adding missing i %d index %d section %d!\n", i, index, section);
                              }
                          }
                      }
                      break;
                case MusECore::Xml::TagStart:
                      xml.unknown("Header");
                      break;
                case MusECore::Xml::TagEnd:
                      if (tag ==objectName())
                            return;
                default:
                      break;
                }
          }
}

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Header::writeStatus(int level, MusECore::Xml& xml) const
      {
      //xml.nput(level, "<%s> ", name());
      xml.nput(level, "<%s> ", MusECore::Xml::xmlString(objectName()).toLatin1().constData());
      int n = count();
      for (int i = n; i >= 0; --i) {
            if (isSectionHidden(logicalIndex(i)))
              xml.nput("%d ", -logicalIndex(i)-1); // hidden is stored as negative value starting from -1
            else
              xml.nput("%d ", logicalIndex(i));
          }
      //xml.put("</%s>", name());
      xml.put("</%s>", MusECore::Xml::xmlString(objectName()).toLatin1().constData());
      }

//---------------------------------------------------------
//   Header
//---------------------------------------------------------

Header::Header(QWidget* parent, const char* name)
  : QHeaderView(Qt::Horizontal, parent) 
      {
      setObjectName(name);
      itemModel = new QStandardItemModel;
      setModel(itemModel);
      setDefaultSectionSize(30);

      }

//---------------------------------------------------------
//   setColumnLabel
//---------------------------------------------------------

void Header::setColumnLabel(const QString & text, int col, int width )
      {
      //printf("column set to %s %d %d \n", text.toLatin1().data(), col, width);
      QStandardItem *sitem = new QStandardItem(text );
      itemModel->setHorizontalHeaderItem(col, sitem);
      if (width > -1)
           resizeSection(col, width);
      }

//---------------------------------------------------------
//   setToolTip
//---------------------------------------------------------

void Header::setToolTip(int col, const QString &text)
      {
      QStandardItem *item = itemModel->horizontalHeaderItem(col);
      item->setToolTip(text);
      }

//---------------------------------------------------------
//   setWhatsThis
//---------------------------------------------------------

void Header::setWhatsThis(int col, const QString &text)
      {
      QStandardItem *item = itemModel->horizontalHeaderItem(col);
      item->setWhatsThis(text);
      }

void Header::mousePressEvent ( QMouseEvent * e )
{
  if (e->button() == Qt::RightButton) {

    PopupMenu* p = new PopupMenu();
    p->disconnect();
    p->clear();
    p->setTitle(tr("Track Info Columns"));
    QAction* act = 0;

    for(int i=0; i < count(); i++) {
      act = p->addAction(itemModel->horizontalHeaderItem(logicalIndex(i))->text() +
                         "\t - "+ itemModel->horizontalHeaderItem(logicalIndex(i))->toolTip());

      act->setCheckable(true);
      act->setChecked(!isSectionHidden(logicalIndex(i)));
      int data = logicalIndex(i);
      act->setData(data);
    }
    connect(p, SIGNAL(triggered(QAction*)), SLOT(changeColumns(QAction*)));
    p->exec(QCursor::pos());

    delete p;
    return;
  }

  QHeaderView::mousePressEvent(e);

}
void Header::changeColumns(QAction *a)
{
  int section = a->data().toInt();
  if (isSectionHidden(section))
    showSection(section);
  else
    hideSection(section);
}

} // namespace MusEGui
