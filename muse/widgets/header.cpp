//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.cpp,v 1.1.1.1 2003/10/27 18:55:05 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "header.h"
#include "xml.h"
#include "popupmenu.h"

#include <QStringList>
#include <QStandardItemModel>
#include <QMouseEvent>

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Header::readStatus(Xml& xml)
{

    for (;;) {
          Xml::Token token = xml.parse();
          const QString& tag = xml.s1();
          switch (token) {
                case Xml::Error:
                case Xml::End:
                      return;
                case Xml::Text:
                      {
                      //QStringList l = QStringList::split(QString(" "), tag);
                      QStringList l = tag.split(QString(" "), QString::SkipEmptyParts);
                      int index = count() -1;
                      for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                            int logialIdx=abs((*it).toInt());
                            bool isHidden = (*it).toInt() < 0 ? true:false;
                            int section = visualIndex(logialIdx);
                            setMovable(true);
                            moveSection(section, index);
                            if (isHidden)
                              hideSection(logialIdx);
                            else
                              showSection(logialIdx);
                            --index;
                            }
                      }
                      break;
                case Xml::TagStart:
                      xml.unknown("Header");
                      break;
                case Xml::TagEnd:
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

void Header::writeStatus(int level, Xml& xml) const
      {
      //xml.nput(level, "<%s> ", name());
      xml.nput(level, "<%s> ", Xml::xmlString(objectName()).toLatin1().constData());
      int n = count() - 1;
      for (int i = n; i >= 0; --i) {
            if (isSectionHidden(logicalIndex(i)))
              xml.nput("%d ", -logicalIndex(i)); // hidden is stored as negative value
            else
              xml.nput("%d ", logicalIndex(i));
          }
      //xml.put("</%s>", name());
      xml.put("</%s>", Xml::xmlString(objectName()).toLatin1().constData());
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
//      setStretchLastSection(true);

      }

//---------------------------------------------------------
//   setColumnLabel
//---------------------------------------------------------

void Header::setColumnLabel(const QString & text, int col, int width )
      {
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

    for(int i=1; i < count(); i++) {
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

  }
}
void Header::changeColumns(QAction *a)
{
  int section = a->data().toInt();
  if (isSectionHidden(section))
    showSection(section);
  else
    hideSection(section);
}
