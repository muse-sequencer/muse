//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: header.h,v 1.1.1.1 2003/10/27 18:55:03 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HEADER_H__
#define __HEADER_H__

#include <qheader.h>

class Xml;

//---------------------------------------------------------
//   Header
//---------------------------------------------------------

class Header : public QHeader {
      Q_OBJECT

   public:
      Header(QWidget* parent=0, const char* name=0)
         : QHeader(parent, name) {}
      Header(int sections, QWidget* parent=0, const char* name=0)
         : QHeader(sections, parent, name) {}
      void writeStatus(int level, Xml&) const;
      void readStatus(Xml&);
      };
#endif
