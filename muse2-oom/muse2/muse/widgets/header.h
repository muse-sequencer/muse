//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HEADER_H__
#define __HEADER_H__

#include <QHeaderView>

class QStandardItemModel;

class Xml;

class Header : public QHeaderView {
      Q_OBJECT

      QStandardItemModel *itemModel;

   public:
      Header(QWidget* parent=0, const char* name=0);
      void writeStatus(int level, Xml&) const;
      void readStatus(Xml&);
      void setColumnLabel( const QString & s, int col, int width = -1 );
      void setToolTip(int col, const QString &text);
      void setWhatsThis(int col, const QString &text);
};

#endif

