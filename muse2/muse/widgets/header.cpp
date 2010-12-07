//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.cpp,v 1.1.1.1 2003/10/27 18:55:05 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "header.h"
#include "xml.h"

#include <QStringList>
#include <QStandardItemModel>

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
                        QStringList l = QStringList::split(QString(" "), tag);
                        int index = count();
                        for (QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
                              int section = (*it).toInt();
                              moveSection(section, index);
                              --index;
                              }
                        }
                        break;
                  case Xml::TagStart:
                        xml.unknown("Header");
                        break;
                  case Xml::TagEnd:
                        if (tag == name())
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
      xml.nput(level, "<%s> ", Xml::xmlString(name()).toLatin1().constData());
      int n = count() - 1;
      for (int i = n; i >= 0; --i)
            xml.nput("%d ", visualIndex(i));
      //xml.put("</%s>", name());
      xml.put("</%s>", Xml::xmlString(name()).toLatin1().constData());
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
      //setResizeMode(QHeaderView::ResizeToContents);
      setDefaultSectionSize(30);
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

