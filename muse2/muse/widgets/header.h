//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HEADER_H__
#define __HEADER_H__

#include <q3header.h>

class QHeaderView;
class QStandardItemModel;

class Xml;

//---------------------------------------------------------
//   Header
//---------------------------------------------------------

class Header : public Q3Header {
      Q_OBJECT

   public:
      Header(QWidget* parent=0, const char* name=0)
         : Q3Header(parent, name) {}
      Header(int sections, QWidget* parent=0, const char* name=0)
         : Q3Header(sections, parent, name) {}
      void writeStatus(int level, Xml&) const;
      void readStatus(Xml&);
      };


// ORCAN - TODO: Once everything is ported to use the new class below, we
// can remove the above Qt3 based class.

class HeaderNew : public QHeaderView {
      Q_OBJECT

      QStandardItemModel *itemModel;
      int columncount;

   public:
      HeaderNew(QWidget* parent=0, const char* name=0);
      void writeStatus(int level, Xml&) const;
      void readStatus(Xml&);
      int addLabel ( const QString & s, int size = -1 );
      void setToolTip(int col, const QString &text);
      void setWhatsThis(int col, const QString &text);
};

#endif

